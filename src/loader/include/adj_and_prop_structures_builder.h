#pragma once

#include "src/loader/include/thread_pool.h"
#include "src/loader/include/utils.h"
#include "src/storage/include/catalog.h"
#include "src/storage/include/graph.h"

using namespace graphflow::common;
using namespace graphflow::storage;

namespace graphflow {
namespace loader {

// Base class for AdjAndPropertyColumnsBuilder and AdjAndPropertyListsBuilder.
class AdjAndPropertyStructuresBuilder {

    typedef vector<unique_ptr<listSizes_t>> dirLabelNumRels_t;

public:
    void populateNumRelsInfo(
        vector<vector<vector<uint64_t>>>& numRelsPerDirBoundLabelRelLabel, bool forColumns);

    // Sorts data in String overflow pages.
    virtual void sortOverflowStrings() = 0;

    // Saves all structures to disk.
    virtual void saveToFile() = 0;

protected:
    AdjAndPropertyStructuresBuilder(RelLabelDescription& description, ThreadPool& threadPool,
        const Graph& graph, const string outputDirectory)
        : logger{spdlog::get("loader")}, description{description},
          threadPool{threadPool}, graph{graph}, outputDirectory{outputDirectory} {};

protected:
    shared_ptr<spdlog::logger> logger;
    RelLabelDescription& description;
    ThreadPool& threadPool;
    const Graph& graph;
    const string outputDirectory;

    // To count the number of rels per adjLists/adjColumn
    dirLabelNumRels_t dirLabelNumRels{2};
};

} // namespace loader
} // namespace graphflow