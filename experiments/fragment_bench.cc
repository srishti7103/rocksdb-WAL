#include <iostream>
#include <chrono>
#include <atomic>
#include <memory>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

// External counters from instrumented log_writer.cc
namespace ROCKSDB_NAMESPACE {
namespace log {
  extern std::atomic<uint64_t> g_wal_fragment_records;
  extern std::atomic<uint64_t> g_wal_bytes_header;
}
}

using namespace ROCKSDB_NAMESPACE;

int main() {
    std::string kDBPath = "/tmp/rocksdb_fragment_bench";
    std::unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;
    
    Status s = DB::Open(options, kDBPath, &db);
    if (!s.ok()) return 1;

    WriteOptions write_options;
    // Insert a very large record (128KB) to force fragmentation
    // Default block size is 32KB, so this will be split into >= 4 fragments
    std::string large_value(128 * 1024, 'a');
    
    std::cout << "Starting Fragmentation Benchmark..." << std::endl;
    db->Put(write_options, "large_key", large_value);

    std::cout << "Fragmented Records Tracked: " << log::g_wal_fragment_records.load() << std::endl;
    std::cout << "Total Header Overhead (bytes): " << log::g_wal_bytes_header.load() << std::endl;

    return 0;
}
