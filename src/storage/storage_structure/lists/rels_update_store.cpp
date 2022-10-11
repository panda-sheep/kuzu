#include "src/storage/storage_structure/include/lists/rels_update_store.h"

#include "src/storage/storage_structure/include/lists/lists.h"

namespace graphflow {
namespace storage {

RelsUpdateStore::RelsUpdateStore(MemoryManager& memoryManager, RelTableSchema& relTableSchema)
    : relTableSchema{relTableSchema} {
    auto factorizedTableSchema = make_unique<FactorizedTableSchema>();
    // The first two columns of factorizedTable are for srcNodeID and dstNodeID.
    factorizedTableSchema->appendColumn(
        make_unique<ColumnSchema>(false /* isUnflat */, 0 /* dataChunkPos */, sizeof(nodeID_t)));
    factorizedTableSchema->appendColumn(
        make_unique<ColumnSchema>(false /* isUnflat */, 0 /* dataChunkPos */, sizeof(nodeID_t)));
    for (auto& relProperty : relTableSchema.properties) {
        propertyIDToColIdxMap.emplace(
            relProperty.propertyID, factorizedTableSchema->getNumColumns());
        factorizedTableSchema->appendColumn(make_unique<ColumnSchema>(false /* isUnflat */,
            0 /* dataChunkPos */, Types::getDataTypeSize(relProperty.dataType)));
    }
    nodeDataChunk = make_shared<DataChunk>(2);
    nodeDataChunk->state->currIdx = 0;
    srcNodeVector = make_shared<ValueVector>(NODE_ID, &memoryManager);
    nodeDataChunk->insert(0 /* pos */, srcNodeVector);
    dstNodeVector = make_shared<ValueVector>(NODE_ID, &memoryManager);
    nodeDataChunk->insert(1 /* pos */, dstNodeVector);
    factorizedTable = make_unique<FactorizedTable>(&memoryManager, move(factorizedTableSchema));
    initInsertedRelsPerTableIDPerDirection();
}

void RelsUpdateStore::readToListAndUpdateOverflowIfNecessary(ListFileID& listFileID,
    vector<uint64_t> tupleIdxes, InMemList& inMemList, uint64_t numElementsInPersistentStore,
    DiskOverflowFile* diskOverflowFile, DataType dataType,
    NodeIDCompressionScheme* nodeIDCompressionScheme) {
    factorizedTable->copyToInMemList(getColIdxInFT(listFileID), tupleIdxes, inMemList.getListData(),
        inMemList.nullMask.get(), numElementsInPersistentStore, diskOverflowFile, dataType,
        nodeIDCompressionScheme);
}

void RelsUpdateStore::addRel(vector<shared_ptr<ValueVector>>& srcDstNodeIDAndRelProperties) {
    validateSrcDstNodeIDAndRelProperties(srcDstNodeIDAndRelProperties);
    factorizedTable->append(srcDstNodeIDAndRelProperties);
    auto srcNodeVector = srcDstNodeIDAndRelProperties[0];
    auto dstNodeVector = srcDstNodeIDAndRelProperties[1];
    auto pos = srcNodeVector->state->selVector
                   ->selectedPositions[srcNodeVector->state->getPositionOfCurrIdx()];
    auto srcNodeID = ((nodeID_t*)srcNodeVector->values)[pos];
    auto dstNodeID = ((nodeID_t*)dstNodeVector->values)[pos];
    for (auto direction : REL_DIRECTIONS) {
        auto nodeID = direction == RelDirection::FWD ? srcNodeID : dstNodeID;
        auto chunkIdx = StorageUtils::getListChunkIdx(nodeID.offset);
        insertedRelsPerTableIDPerDirection[direction][nodeID.tableID][chunkIdx][nodeID.offset]
            .push_back(factorizedTable->getNumTuples() - 1);
    }
}

uint64_t RelsUpdateStore::getNumInsertedRelsForNodeOffset(
    ListFileID& listFileID, node_offset_t nodeOffset) const {
    auto chunkIdx = StorageUtils::getListChunkIdx(nodeOffset);
    auto relNodeTableAndDir = getRelNodeTableAndDirFromListFileID(listFileID);
    auto insertedRelsPerTableID = insertedRelsPerTableIDPerDirection[relNodeTableAndDir.dir].at(
        relNodeTableAndDir.srcNodeTableID);
    if (!insertedRelsPerTableID.contains(chunkIdx) ||
        !insertedRelsPerTableID[chunkIdx].contains(nodeOffset)) {
        return 0;
    }
    return insertedRelsPerTableID.at(chunkIdx).at(nodeOffset).size();
}

void RelsUpdateStore::readValues(ListFileID& listFileID, ListSyncState& listSyncState,
    shared_ptr<ValueVector> valueVector) const {
    auto numTuplesToRead = listSyncState.getNumValuesToRead();
    auto nodeOffset = listSyncState.getBoundNodeOffset();
    if (numTuplesToRead == 0) {
        valueVector->state->initOriginalAndSelectedSize(0);
        return;
    }
    auto vectorsToRead = vector<shared_ptr<ValueVector>>{valueVector};
    auto columnsToRead = vector<uint32_t>{getColIdxInFT(listFileID)};
    auto relNodeTableAndDir = getRelNodeTableAndDirFromListFileID(listFileID);
    auto tupleIdxesToRead = insertedRelsPerTableIDPerDirection[relNodeTableAndDir.dir]
                                .at(relNodeTableAndDir.srcNodeTableID)
                                .at(StorageUtils::getListChunkIdx(nodeOffset))
                                .at(nodeOffset);
    factorizedTable->lookup(vectorsToRead, columnsToRead, tupleIdxesToRead,
        listSyncState.getStartElemOffset(), numTuplesToRead);
    valueVector->state->originalSize = numTuplesToRead;
}

uint32_t RelsUpdateStore::getColIdxInFT(ListFileID& listFileID) const {
    if (listFileID.listType == ADJ_LISTS) {
        return listFileID.adjListsID.relNodeTableAndDir.dir == FWD ? 1 : 0;
    } else {
        return propertyIDToColIdxMap.at(listFileID.relPropertyListID.propertyID);
    }
}

// TODO(Ziyi): This function is designed to help implementing the front-end of insertRels. Once the
// front-end of insertRels has been implemented, we should delete this function.
void RelsUpdateStore::validateSrcDstNodeIDAndRelProperties(
    vector<shared_ptr<ValueVector>> srcDstNodeIDAndRelProperties) const {
    // Checks whether the number of vectors inside srcDstNodeIDAndRelProperties matches the number
    // of columns in factorizedTable.
    if (factorizedTable->getTableSchema()->getNumColumns() != srcDstNodeIDAndRelProperties.size()) {
        throw InternalException(
            StringUtils::string_format("Expected number of valueVectors: %d. Given: %d.",
                relTableSchema.properties.size() + 2, srcDstNodeIDAndRelProperties.size()));
    }
    // Checks whether the dataType of each given vector matches the one defined in tableSchema.
    for (auto i = 0u; i < srcDstNodeIDAndRelProperties.size(); i++) {
        // Note that: we store srcNodeID and dstNodeID in the first two columns of factorizedTable.
        // So, we only need to compare the columns whose colIdx > 2 with tableSchema.
        if (i >= 2 && srcDstNodeIDAndRelProperties[i]->dataType !=
                          relTableSchema.properties[i - 2].dataType) {
            throw InternalException(StringUtils::string_format(
                "Expected vector with type %s, Given: %s.",
                Types::dataTypeToString(relTableSchema.properties[i - 2].dataType.typeID).c_str(),
                Types::dataTypeToString(srcDstNodeIDAndRelProperties[i]->dataType.typeID).c_str()));
        } else if (i < 2 &&
                   srcDstNodeIDAndRelProperties[i]->dataType.typeID != DataTypeID(NODE_ID)) {
            throw InternalException("The first two vectors of srcDstNodeIDAndRelProperties should "
                                    "be src/dstNodeVector.");
        }
    }
    // Checks whether the srcTableID and dstTableID is a valid src/dst table ID defined in
    // tableSchema.
    auto srcNodeVector = srcDstNodeIDAndRelProperties[0];
    auto dstNodeVector = srcDstNodeIDAndRelProperties[1];
    auto pos = srcNodeVector->state->selVector
                   ->selectedPositions[srcNodeVector->state->getPositionOfCurrIdx()];
    auto srcNodeID = ((nodeID_t*)srcNodeVector->values)[pos];
    auto dstNodeID = ((nodeID_t*)dstNodeVector->values)[pos];
    if (!insertedRelsPerTableIDPerDirection[0].contains(srcNodeID.tableID)) {
        getErrorMsgForInvalidTableID(
            srcNodeID.tableID, true /* isSrcTableID */, relTableSchema.tableName);
    } else if (!insertedRelsPerTableIDPerDirection[1].contains(dstNodeID.tableID)) {
        getErrorMsgForInvalidTableID(
            dstNodeID.tableID, false /* isSrcTableID */, relTableSchema.tableName);
    }
}

void RelsUpdateStore::initInsertedRelsPerTableIDPerDirection() {
    insertedRelsPerTableIDPerDirection.clear();
    auto srcDstTableIDs = relTableSchema.getSrcDstTableIDs();
    for (auto direction : REL_DIRECTIONS) {
        insertedRelsPerTableIDPerDirection.push_back(map<table_id_t, InsertedRelsPerChunk>{});
        auto tableIDs = direction == RelDirection::FWD ? srcDstTableIDs.srcTableIDs :
                                                         srcDstTableIDs.dstTableIDs;
        for (auto tableID : tableIDs) {
            insertedRelsPerTableIDPerDirection[direction].emplace(tableID, InsertedRelsPerChunk{});
        }
    }
}

void RelsUpdateStore::getErrorMsgForInvalidTableID(
    uint64_t tableID, bool isSrcTableID, string tableName) {
    throw InternalException(
        StringUtils::string_format("TableID: %d is not a valid %s tableID in rel %s.", tableID,
            isSrcTableID ? "src" : "dst", tableName.c_str()));
}

} // namespace storage
} // namespace graphflow
