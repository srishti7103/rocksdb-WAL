#include <iostream>
#include <chrono>
#include <memory>
#include <fstream>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

using namespace ROCKSDB_NAMESPACE;

int main(int argc, char** argv) {
    std::string kDBPath = "/tmp/rocksdb_sync_bench";
    std::unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;
    
    Status s = DB::Open(options, kDBPath, &db);
    if (!s.ok()) return 1;

    const int kNumIterations = 100000;
    WriteOptions write_options;
    
    // Experiment 1a: Buffered
    write_options.sync = false;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumIterations; i++) {
        db->Put(write_options, "key" + std::to_string(i), "value" + std::to_string(i));
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    int buffered_ops = kNumIterations / elapsed.count();

    // Experiment 1b: No-WAL
    write_options.disableWAL = true;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNumIterations; i++) {
        db->Put(write_options, "nowal_key" + std::to_string(i), "nowal_value" + std::to_string(i));
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    int nowal_ops = kNumIterations / elapsed.count();

    // Experiment 1c: Strict Sync
    write_options.disableWAL = false;
    write_options.sync = true;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) { // Less iterations because it's slow
        db->Put(write_options, "sync_key" + std::to_string(i), "sync_value" + std::to_string(i));
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    int sync_ops = 1000 / elapsed.count();

    // Append to CSV
    std::ofstream csv("../results/wal_performance_telemetry.csv", std::ios_base::app);
    csv << "WAL_Batch,1," << buffered_ops << "\n";
    csv << "WAL_Batch,2," << nowal_ops << "\n";
    csv << "WAL_Batch,3," << sync_ops << "\n";
    csv.close();

    std::cout << "Sync benchmark complete." << std::endl;
    return 0;
}
