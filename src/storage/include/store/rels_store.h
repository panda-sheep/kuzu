#pragma once

#include "src/common/include/types.h"
#include "src/storage/include/catalog.h"
#include "src/storage/include/data_structure/column.h"
#include "src/storage/include/data_structure/lists/lists.h"

using namespace graphflow::common;
using namespace std;

namespace graphflow {
namespace storage {

// RelsStore stores adjacent rels of a node as well as the properties of rels in the system.
class RelsStore {

public:
    RelsStore(const Catalog& catalog, const vector<uint64_t>& numNodesPerLabel,
        const string& directory, BufferManager& bufferManager);

    inline BaseColumn* getRelPropertyColumn(
        const label_t& relLabel, const label_t& nodeLabel, const uint64_t& propertyIdx) const {
        return propertyColumns[nodeLabel][relLabel][propertyIdx].get();
    }

    inline BaseLists* getRelPropertyLists(const Direction& direction, const label_t& nodeLabel,
        const label_t& relLabel, const uint64_t& propertyIdx) const {
        return propertyLists[direction][nodeLabel][relLabel][propertyIdx].get();
    }

    inline AdjColumn* getAdjColumn(
        const Direction& direction, const label_t& nodeLabel, const label_t& relLabel) const {
        return adjColumns[direction][nodeLabel][relLabel].get();
    }

    inline AdjLists* getAdjLists(
        const Direction& direction, const label_t& nodeLabel, const label_t& relLabel) const {
        return adjLists[direction][nodeLabel][relLabel].get();
    }

    inline static string getAdjColumnFname(const string& directory, const label_t& relLabel,
        const label_t& nodeLabel, const Direction& direction) {
        return directory + "/r-" + to_string(relLabel) + "-" + to_string(nodeLabel) + "-" +
               to_string(direction) + ".col";
    }

    inline static string getAdjListsFname(const string& directory, const label_t& relLabel,
        const label_t& nodeLabel, const Direction& direction) {
        return directory + "/r-" + to_string(relLabel) + "-" + to_string(nodeLabel) + "-" +
               to_string(direction) + ".lists";
    }

    inline static string getRelPropertyColumnFname(const string& directory, const label_t& relLabel,
        const label_t& nodeLabel, const string& propertyName) {
        return directory + "/r-" + to_string(relLabel) + "-" + to_string(nodeLabel) + "-" +
               propertyName + ".col";
    }

    inline static string getRelPropertyListsFname(const string& directory, const label_t& relLabel,
        const label_t& nodeLabel, const Direction& dir, const string& propertyName) {
        return directory + "/r-" + to_string(relLabel) + "-" + to_string(nodeLabel) + "-" +
               to_string(dir) + "-" + propertyName + ".lists";
    }

private:
    void initAdjColumns(const Catalog& catalog, const vector<uint64_t>& numNodesPerLabel,
        const string& directory, BufferManager& bufferManager);

    void initAdjLists(const Catalog& catalog, const vector<uint64_t>& numNodesPerLabel,
        const string& directory, BufferManager& bufferManager);

    void initPropertyListsAndColumns(const Catalog& catalog,
        const vector<uint64_t>& numNodesPerLabel, const string& directory,
        BufferManager& bufferManager);

    void initPropertyColumnsForRelLabel(const Catalog& catalog,
        const vector<uint64_t>& numNodesPerLabel, const string& directory,
        BufferManager& bufferManager, const label_t& relLabel, const Direction& dir);

    void initPropertyListsForRelLabel(const Catalog& catalog,
        const vector<uint64_t>& numNodesPerLabel, const string& directory,
        BufferManager& bufferManager, const label_t& relLabel);

private:
    shared_ptr<spdlog::logger> logger;
    // propertyColumns are organized in 2-dimensional vectors wherein the first dimension gives
    // nodeLabel and the second dimension is the relLabel.
    vector<vector<vector<unique_ptr<BaseColumn>>>> propertyColumns;
    // propertyLists, adjColumns and adjLists are organized in 3-dimensional vectors wherein the
    // first dimension gives direction, second dimension gives nodeLabel and the third dimension is
    // the relLabel.
    vector<vector<vector<vector<unique_ptr<BaseLists>>>>> propertyLists{2};
    vector<vector<vector<unique_ptr<AdjColumn>>>> adjColumns{2};
    vector<vector<vector<unique_ptr<AdjLists>>>> adjLists{2};
};

} // namespace storage
} // namespace graphflow