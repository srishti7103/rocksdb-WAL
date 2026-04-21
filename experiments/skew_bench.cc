#include <iostream>
#include <chrono>
#include "rocksdb/db.h"

using namespace ROCKSDB_NAMESPACE;

int main() {
    std::string kDBPath = "/tmp/rocksdb_skew_bench";
    DB* db;
    Options options;
    options.create_if_missing = true;
    
    // 1. Heavy Volume Load
    std::cout << "Loading 100k items into WAL..." << std::endl;
    DB::Open(options, kDBPath, &db);
    for (int i = 0; i < 100000; i++) {
        db->Put(WriteOptions(), "key" + std::to_string(i), "value");
    }
    delete db; // Shutdown without flush

    // 2. Measure Recovery Time
    std::cout << "Measuring MTTR (Mean Time To Recovery)..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    DB::Open(options, kDBPath, &db);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "MTTR for 100k items: " << elapsed.count() << " ms" << std::endl;

    delete db;
    return 0;
}
