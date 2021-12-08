#pragma once

#include <cstdint>

namespace graphflow {
namespace common {

// Size (in bytes) of the chunks to be read in GraphLoader.
constexpr uint64_t CSV_READING_BLOCK_SIZE = 1 << 23;

// Size of the page which is the unit of read/write to the files.
// For now, this value cannot be changed. But technically it can change from 2^12 to 2^16. 2^12
// lower bound is assuming the OS page size is 4K. 2^16 is because currently we leave 11 fixed
// number of bits for relOffInPage and the maximum number of bytes needed for an edge is 20 bytes
// so 11 + log_2(20) = 15.xxx, so certainly over 2^16-size pages, we cannot utilize the page for
// storing adjacency lists.
constexpr uint64_t PAGE_SIZE_LOG_2 = 12;
constexpr uint64_t PAGE_SIZE = 1 << PAGE_SIZE_LOG_2;

// The default amount of memory pre-allocated to the buffer pool (= 1GB).
constexpr uint64_t DEFAULT_MEMORY_MANAGER_MAX_MEMORY = 1ull << 38;

// By default, memory block size is 256KB
constexpr const uint64_t DEFAULT_MEMORY_BLOCK_SIZE = 1 << 18;
constexpr const double DEFAULT_HT_LOAD_FACTOR = 1.5;

// The number of bytes in a block of memory to store the keys of tuples
constexpr const uint64_t SORT_BLOCK_SIZE = 4096;

struct StorageConfig {
    // The default amount of memory pre-allocated to the buffer pool (= 1GB).
    static constexpr uint64_t DEFAULT_BUFFER_POOL_SIZE = 1ull << 22;
};

// Hash Index Configurations
struct HashIndexConfig {
    static constexpr uint64_t SLOT_CAPACITY = 4;
};

struct LoaderConfig {
    static constexpr char UNSTR_PROPERTY_SEPARATOR[] = ":";
    static constexpr char DEFAULT_METADATA_JSON_FILENAME[] = "metadata.json";

    // mandatory fields in input CSV files
    static constexpr char ID_FIELD[] = "ID";
    static constexpr char START_ID_FIELD[] = "START_ID";
    static constexpr char END_ID_FIELD[] = "END_ID";
    static constexpr char START_ID_LABEL_FIELD[] = "START_ID_LABEL";
    static constexpr char END_ID_LABEL_FIELD[] = "END_ID_LABEL";

    // Default special characters
    static constexpr char DEFAULT_ESCAPE_CHAR = '\\';
    static constexpr char DEFAULT_TOKEN_SEPARATOR = ',';
    static constexpr char DEFAULT_QUOTE_CHAR = '"';
};

} // namespace common
} // namespace graphflow
