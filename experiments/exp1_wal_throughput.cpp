/*
 * EXPERIMENT 1: WAL Write Throughput — WAL ON vs WAL OFF vs SYNC ON
 */
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
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
  auto s = rocksdb::DB::Open(opts, path, &db_ptr); db = db_ptr.release();
  if (!s.ok()) { std::cerr << s.ToString() << "\n"; exit(1); }
  return db;
}

struct Result { std::string label; double ops_per_sec; double us_per_op; };

Result bench(const std::string& label, bool wal_off, bool sync, int batch_size) {
  const int TOTAL = 100000;
  const std::string val(256, 'v');
  const std::string path = "../results/exp1_" + label.substr(0, 8);
  auto db = fresh_db(path);

  rocksdb::WriteOptions wo;
  wo.disableWAL = wal_off;
  wo.sync = sync;

  auto t0 = Clock::now();
  for (int i = 0; i < TOTAL; i += batch_size) {
    rocksdb::WriteBatch wb;
    for (int j = i; j < std::min(i + batch_size, TOTAL); j++)
      wb.Put("k" + std::to_string(j), val);
    db->Write(wo, &wb);
  }
  double elapsed = std::chrono::duration<double>(Clock::now() - t0).count();

  delete db;
  fs::remove_all(path);
  return { label, TOTAL / elapsed, elapsed * 1e6 / TOTAL };
}

int main() {
  std::cout << "\n=== EXP 1: WAL Write Throughput (100k writes, 256-byte values) ===\n\n";
  std::vector<Result> results = {
    bench("WAL=ON  sync=OFF batch=1",   false, false, 1),
    bench("WAL=OFF sync=OFF batch=1",   true,  false, 1),
    bench("WAL=ON  sync=ON  batch=1",   false, true,  1),
    bench("WAL=ON  sync=OFF batch=100", false, false, 100),
  };

  std::cout << std::left << std::setw(30) << "Configuration"
            << std::setw(15) << "Throughput" << "Latency\n"
            << std::string(60, '-') << "\n";
  for (auto& r : results)
    std::cout << std::setw(30) << r.label
              << std::setw(15) << (std::to_string((int)r.ops_per_sec) + " ops/s")
              << std::to_string((int)r.us_per_op) + " us/op\n";

  return 0;
}
