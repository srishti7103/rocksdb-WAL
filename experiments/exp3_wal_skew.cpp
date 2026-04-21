/*
 * EXPERIMENT 3: WAL Behavior Under Skew
 */
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::high_resolution_clock;

rocksdb::DB* fresh_db(const std::string& path) {
  fs::remove_all(path);
  rocksdb::Options opts;
  opts.create_if_missing = true;
  opts.disable_auto_compactions = true;
  rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr;
  rocksdb::DB::Open(opts, path, &db_ptr); db = db_ptr.release();
  return db;
}

double bench_skew(const std::string& path, int num_ops, int vsize, bool hot_key) {
  auto* db = fresh_db(path);
  rocksdb::WriteOptions wo;
  const std::string val(vsize, 'v');
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, num_ops - 1);

  auto t0 = Clock::now();
  for (int i = 0; i < num_ops; i++) {
    std::string key = hot_key ? "key_0" : "key_" + std::to_string(dist(rng));
    db->Put(wo, key, val);
  }
  double elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
  delete db;
  fs::remove_all(path);
  return num_ops / elapsed;
}

int main() {
  std::cout << "\n=== EXP 3: WAL Skew Analysis ===\n\n";
  const int OPS = 10000;
  double small = bench_skew("../results/exp3_small", OPS, 16, false);
  double large = bench_skew("../results/exp3_large", OPS, 65536, false);
  std::cout << "Small (16B):  " << (int)small << " ops/s\n";
  std::cout << "Large (64KB): " << (int)large << " ops/s\n";
  return 0;
}
