#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>
#include <fstream>
#include "rocksdb/db.h"

using namespace ROCKSDB_NAMESPACE;

void WriterThread(DB* db, int id, int num_ops) {
    WriteOptions wo;
    for (int i = 0; i < num_ops; i++) {
        db->Put(wo, "thread" + std::to_string(id) + "_key" + std::to_string(i), "val");
    }
}

int main() {
    std::string kDBPath = "/tmp/rocksdb_batch_bench";
    std::unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;
    DB::Open(options, kDBPath, &db);

    const int kOpsPerThread = 5000;
    std::vector<int> thread_counts = {1, 4, 8};
    std::ofstream csv("../results/wal_performance_telemetry.csv", std::ios_base::app);

    for (int kThreads : thread_counts) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        for (int i = 0; i < kThreads; i++) {
            threads.emplace_back(WriterThread, db.get(), i, kOpsPerThread);
        }
        for (auto& t : threads) t.join();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        int tput = (kThreads * kOpsPerThread) / elapsed.count();
        csv << "WAL_Group_Commit," << kThreads << "," << tput << "\n";
    }

    csv.close();
    std::cout << "Batch benchmark complete." << std::endl;
    return 0;
}
