#include <iostream>
#include <chrono>
#include <memory>
#include <fstream>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

using namespace ROCKSDB_NAMESPACE;

int main() {
    std::string kDBPath = "/tmp/rocksdb_recovery_bench";
    std::unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;
    
    DB::Open(options, kDBPath, &db);
    for (int i = 0; i < 10000; i++) {
        db->Put(WriteOptions(), "rec_key" + std::to_string(i), "val");
    }
    db.reset(); 

    std::ofstream csv("../results/wal_performance_telemetry.csv", std::ios_base::app);

    // Absolute
    options.wal_recovery_mode = WALRecoveryMode::kAbsoluteConsistency;
    auto start = std::chrono::high_resolution_clock::now();
    DB::Open(options, kDBPath, &db);
    auto end = std::chrono::high_resolution_clock::now();
    int abs_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    csv << "WAL_Recovery_Mode,AbsoluteConsistency," << abs_ms << "\n";
    db.reset();

    // Point in time
    options.wal_recovery_mode = WALRecoveryMode::kPointInTimeRecovery;
    start = std::chrono::high_resolution_clock::now();
    DB::Open(options, kDBPath, &db);
    end = std::chrono::high_resolution_clock::now();
    int pit_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    csv << "WAL_Recovery_Mode,PointInTime," << pit_ms << "\n";
    db.reset();

    // Tolerate
    options.wal_recovery_mode = WALRecoveryMode::kTolerateCorruptedTailRecords;
    start = std::chrono::high_resolution_clock::now();
    DB::Open(options, kDBPath, &db);
    end = std::chrono::high_resolution_clock::now();
    int tol_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    csv << "WAL_Recovery_Mode,TolerateCorrupted," << tol_ms << "\n";
    
    csv.close();
    
    std::cout << " -> Absolute Consistency: " << abs_ms << " ms" << std::endl;
    std::cout << " -> Point In Time:        " << pit_ms << " ms" << std::endl;
    std::cout << " -> Tolerate Corrupted:   " << tol_ms << " ms" << std::endl;
    std::cout << "Recovery benchmark complete.\n" << std::endl;
    return 0;
}
