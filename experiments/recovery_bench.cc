#include <iostream>
#include <chrono>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

using namespace ROCKSDB_NAMESPACE;

int main() {
    std::string kDBPath = "/tmp/rocksdb_recovery_bench";
    DB* db;
    Options options;
    options.create_if_missing = true;
    
    // 1. Initial Load
    DB::Open(options, kDBPath, &db);
    for (int i = 0; i < 1000; i++) {
        db->Put(WriteOptions(), "rec_key" + std::to_string(i), "val");
    }
    delete db; // Simulate "unclean" shutdown by just closing without flush

    // 2. Test Recovery Mode: Absolute Consistency
    options.wal_recovery_mode = WALRecoveryMode::kAbsoluteConsistency;
    auto start = std::chrono::high_resolution_clock::now();
    DB::Open(options, kDBPath, &db);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "Recovery (kAbsoluteConsistency): " << elapsed.count() << " ms" << std::endl;
    delete db;

    // 3. Test Recovery Mode: Point In Time
    options.wal_recovery_mode = WALRecoveryMode::kPointInTimeRecovery;
    start = std::chrono::high_resolution_clock::now();
    DB::Open(options, kDBPath, &db);
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "Recovery (kPointInTimeRecovery): " << elapsed.count() << " ms" << std::endl;
    
    delete db;
    return 0;
}
