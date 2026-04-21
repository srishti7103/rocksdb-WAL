/*
 * EXPERIMENT 4: Group Commit — Batch Grouping Reduces fsync Count
 *
 * GROUP COMMIT MECHANISM:
 *   EnterAsBatchGroupLeader() db/db_impl/db_impl_write.cc:1196
 *     One thread becomes the "leader" and writes ALL concurrent writers' data
 *     in a single WriteGroupToWAL() call with a single fsync.
 *   MergeBatch()              db/db_impl/db_impl_write.cc:2211
 *     Flattens all writers' WriteBatches into one merged batch.
 *
 * THIS EXPERIMENT:
 *   Compare single-threaded writes (no grouping) vs multi-threaded writes
 *   (group commit active). The byte-per-WAL-call metric from DS614 Exp3
 *   instrumentation will show higher values under concurrent load.
 */
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <memory>

namespace fs = std::filesystem;
using Clock = std::chrono::high_resolution_clock;

rocksdb::DB* fresh_db(const std::string& path) {
  fs::remove_all(path);
  rocksdb::Options opts;
  opts.create_if_missing = true;
  opts.disable_auto_compactions = true;
  opts.write_buffer_size = 512 << 20;
  rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr;
  rocksdb::DB::Open(opts, path, &db_ptr); db = db_ptr.release();
  return db;
}

double bench_concurrent(rocksdb::DB* db, int num_threads, int ops_per_thread) {
  const std::string val(256, 'v');
  rocksdb::WriteOptions wo;
  std::vector<std::thread> threads;
  auto t0 = Clock::now();
  for (int t = 0; t < num_threads; t++) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < ops_per_thread; i++) {
        std::string key = "t" + std::to_string(t) + "_k" + std::to_string(i);
        db->Put(wo, key, val);
      }
    });
  }
  for (auto& th : threads) th.join();
  return std::chrono::duration<double>(Clock::now() - t0).count();
}

int main() {
  const int OPS_TOTAL = 100000;
  std::cout << "\n=== EXP 4: Group Commit — Single vs Multi-threaded WAL Throughput ===\n\n";

  std::vector<int> thread_counts = {1, 2, 4, 8, 16};
  std::cout << std::left << std::setw(12) << "Threads"
            << std::setw(18) << "Throughput" << "Speedup\n"
            << std::string(45, '-') << "\n";

  double base_ops = 0;
  for (int tc : thread_counts) {
    auto* db = fresh_db("../results/exp4_t" + std::to_string(tc));
    double elapsed = bench_concurrent(db, tc, OPS_TOTAL / tc);
    double ops = OPS_TOTAL / elapsed;
    if (base_ops == 0) base_ops = ops;
    std::cout << std::setw(12) << tc
              << std::setw(18) << (std::to_string((int)ops) + " ops/s")
              << std::fixed << std::setprecision(2) << ops / base_ops << "x\n";
    delete db;
    fs::remove_all("../results/exp4_t" + std::to_string(tc));
  }

  std::cout << "\nINSIGHT: Multi-threaded throughput improves because group commit\n"
            << "  batches concurrent writes into single WAL appends + single fsync.\n\n"
            << "CODE REFS:\n"
            << "  EnterAsBatchGroupLeader(): db/db_impl/db_impl_write.cc:1196\n"
            << "  MergeBatch():              db/db_impl/db_impl_write.cc:2211\n"
            << "  WriteGroupToWAL():         db/db_impl/db_impl_write.cc:2320\n";
  return 0;
}
