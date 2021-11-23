#pragma once

#include "nlohmann/json.hpp"

#include "src/common/include/csv_reader/csv_reader.h"
#include "src/loader/include/adj_and_prop_columns_builder.h"
#include "src/loader/include/adj_and_prop_lists_builder.h"
#include "src/loader/include/dataset_metadata.h"
#include "src/loader/include/thread_pool.h"

namespace graphflow {
namespace loader {

class RelsLoader {
    friend class GraphLoader;

private:
    RelsLoader(ThreadPool& threadPool, Graph& graph, string outputDirectory,
        vector<unique_ptr<NodeIDMap>>& nodeIDMaps,
        const vector<RelFileDescription>& fileDescriptions);

    void load(vector<uint64_t>& numBlocksPerLabel);

    void loadRelsForLabel(RelLabelDescription& relLabelMetadata);

    void constructAdjColumnsAndCountRelsInAdjLists(RelLabelDescription& relLabelMetadata,
        AdjAndPropertyListsBuilder& adjAndPropertyListsBuilder);

    void populateNumRels(AdjAndPropertyColumnsBuilder& adjAndPropertyColumnsBuilder,
        AdjAndPropertyListsBuilder& adjAndPropertyListsBuilder);

    void constructAdjLists(
        RelLabelDescription& description, AdjAndPropertyListsBuilder& adjAndPropertyListsBuilder);

    // Concurrent Tasks

    static void populateAdjColumnsAndCountRelsInAdjListsTask(RelLabelDescription* description,
        uint64_t blockId, AdjAndPropertyListsBuilder* adjAndPropertyListsBuilder,
        AdjAndPropertyColumnsBuilder* adjAndPropertyColumnsBuilder,
        vector<unique_ptr<NodeIDMap>>* nodeIDMaps, const Catalog* catalog,
        shared_ptr<spdlog::logger>& logger);

    static void populateAdjListsTask(RelLabelDescription* description, uint64_t blockId,
        char tokenSeparator, char quoteChar, char escapeChar,
        AdjAndPropertyListsBuilder* adjAndPropertyListsBuilder,
        vector<unique_ptr<NodeIDMap>>* nodeIDMaps, const Catalog* catalog,
        shared_ptr<spdlog::logger>& logger);

    // Task Helpers

    static void inferLabelsAndOffsets(CSVReader& reader, vector<nodeID_t>& nodeIDs,
        vector<unique_ptr<NodeIDMap>>* nodeIDMaps, const Catalog* catalog,
        vector<bool>& requireToReadLabels);

    static void putPropsOfLineIntoInMemPropertyColumns(const vector<PropertyDefinition>& properties,
        CSVReader& reader, AdjAndPropertyColumnsBuilder* adjAndPropertyColumnsBuilder,
        const nodeID_t& nodeID, vector<PageCursor>& stringOverflowPagesCursors);

    static void putPropsOfLineIntoInMemRelPropLists(const vector<PropertyDefinition>& properties,
        CSVReader& reader, const vector<nodeID_t>& nodeIDs, const vector<uint64_t>& pos,
        AdjAndPropertyListsBuilder* adjAndPropertyListsBuilder,
        vector<PageCursor>& stringOverflowPagesCursors);

private:
    shared_ptr<spdlog::logger> logger;
    ThreadPool& threadPool;
    Graph& graph;
    const string outputDirectory;
    vector<unique_ptr<NodeIDMap>>& nodeIDMaps;
    const vector<RelFileDescription>& fileDescriptions;
};

} // namespace loader
} // namespace graphflow
