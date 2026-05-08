#include <iostream>
#include <chrono>
#include <memory>
#include <fstream>
#include <vector>
#include "rocksdb/db.h"

using namespace ROCKSDB_NAMESPACE;

int main() {
    Options options;
    options.create_if_missing = true;
    std::ofstream csv("../results/wal_performance_telemetry.csv", std::ios_base::app);

    std::vector<int> volumes = {10000, 50000, 100000, 500000};
    
    for(int vol : volumes) {
        std::string kDBPath = "/tmp/rocksdb_skew_bench_" + std::to_string(vol);
        std::unique_ptr<DB> db;
        
        DB::Open(options, kDBPath, &db);
        for (int i = 0; i < vol; i++) {
            db->Put(WriteOptions(), "key" + std::to_string(i), "value");
        }
        db.reset(); 

        auto start = std::chrono::high_resolution_clock::now();
        DB::Open(options, kDBPath, &db);
        auto end = std::chrono::high_resolution_clock::now();
        
        int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        csv << "WAL_Recovery," << vol << "," << ms << "\n";
    }

    csv.close();
    std::cout << "Skew benchmark complete." << std::endl;
    return 0;
}
