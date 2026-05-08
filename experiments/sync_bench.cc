#include <iostream>
#include <chrono>
#include <atomic>
#include <memory>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

// External counters from instrumented RocksDB code
namespace ROCKSDB_NAMESPACE {
  extern std::atomic<uint64_t> s_wal_writes_synced;
  extern std::atomic<uint64_t> s_wal_writes_notsync;
}

using namespace ROCKSDB_NAMESPACE;

int main(int argc, char** argv) {
    std::string kDBPath = "/tmp/rocksdb_sync_bench";
    std::unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;
    
    Status s = DB::Open(options, kDBPath, &db);
    if (!s.ok()) {
        std::cerr << "Unable to open DB: " << s.ToString() << std::endl;
        return 1;
    }

    const int kNumIterations = 10000;
    WriteOptions write_options;
    
    // Experiment 1a: Buffered
    write_options.sync = false;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumIterations; i++) {
        db->Put(write_options, "key" + std::to_string(i), "value" + std::to_string(i));
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Buffered Mode: " << kNumIterations / elapsed.count() << " ops/s" << std::endl;

    // Experiment 1b: Strict Sync
    write_options.sync = true;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumIterations; i++) {
        db->Put(write_options, "sync_key" + std::to_string(i), "sync_value" + std::to_string(i));
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "Strict Sync Mode: " << kNumIterations / elapsed.count() << " ops/s" << std::endl;

    return 0;
}
