#pragma once

#include <string>

#include "src/catalog/include/catalog.h"
#include "src/storage/in_mem_storage_structure/include/in_mem_column.h"
#include "src/storage/in_mem_storage_structure/include/in_mem_lists.h"

using namespace graphflow::catalog;

namespace graphflow {
namespace storage {

class WALReplayerUtils {
public:
    static void createEmptyDBFilesForNewRelTable(Catalog* catalog, label_t labelID,
        const string& directory, const vector<uint64_t>& maxNodeOffsetsPerLabel);

    static void createEmptyDBFilesForNewNodeTable(
        Catalog* catalog, label_t labelID, string directory);

    static void replaceNodeFilesWithVersionFromWALIfExists(
        catalog::NodeLabel* nodeLabel, string directory);

    static void replaceRelPropertyFilesWithVersionFromWALIfExists(
        catalog::RelLabel* relLabel, string directory, const catalog::Catalog* catalog);

private:
    static void initLargeListPageListsAndSaveToFile(InMemLists* inMemLists);

    static void createEmptyDBFilesForRelProperties(RelLabel* relLabel, label_t nodeLabel,
        const string& directory, RelDirection relDireciton, uint32_t numNodes,
        bool isForRelPropertyColumn);

    static void createEmptyDBFilesForColumns(const unordered_set<label_t>& nodeLabels,
        const vector<uint64_t>& maxNodeOffsetsPerLabel, RelDirection relDirection,
        const string& directory, const NodeIDCompressionScheme& directionNodeIDCompressionScheme,
        RelLabel* relLabel);

    static void createEmptyDBFilesForLists(const unordered_set<label_t>& nodeLabels,
        const vector<uint64_t>& maxNodeOffsetsPerLabel, RelDirection relDirection,
        const string& directory, const NodeIDCompressionScheme& directionNodeIDCompressionScheme,
        RelLabel* relLabel);

    static void replaceOriginalColumnFilesWithWALVersionIfExists(string originalColFileName);

    static void replaceOriginalListFilesWithWALVersionIfExists(string originalListFileName);

    static void replaceRelPropertyFilesWithVersionFromWAL(catalog::RelLabel* relLabel,
        label_t nodeLabel, const string& directory, RelDirection relDirection,
        bool isColumnProperty);
};

} // namespace storage
} // namespace graphflow