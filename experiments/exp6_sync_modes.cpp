/*
 * EXPERIMENT 6: WAL Sync Modes — fsync Cost and Durability Tradeoff
 *
 * SYNC CODE PATH:
 *   WriteGroupToWAL()  db/db_impl/db_impl_write.cc:2320
 *     if (write_options.sync || manual_wal_flush_) →
 *       for (auto& log : logs_) → log.writer->file()->Sync()
 *   This calls fsync() which is measured by DS614 Exp4 instrumentation.
 *
 * THREE MODES TESTED:
 *   sync=false, disableWAL=false → buffered WAL (default, fastest)
 *   sync=true,  disableWAL=false → synchronous WAL (durable, slow)
 *   sync=false, disableWAL=true  → no WAL (fastest, no durability)
 */
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace fs = std::filesystem;
using Clock = std::chrono::high_resolution_clock;

double bench(const std::string& path, bool sync, bool no_wal, int ops) {
  fs::remove_all(path);
  rocksdb::Options opts;
  opts.create_if_missing = true;
  opts.disable_auto_compactions = true;
  opts.write_buffer_size = 512 << 20;
  rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr;
  rocksdb::DB::Open(opts, path, &db_ptr); db = db_ptr.release();

  rocksdb::WriteOptions wo;
  wo.sync = sync;
  wo.disableWAL = no_wal;
  const std::string val(256, 'v');

  auto t0 = Clock::now();
  for (int i = 0; i < ops; i++)
    db->Put(wo, "k" + std::to_string(i), val);
  double elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
  delete db;
  fs::remove_all(path);
  return ops / elapsed;
}

int main() {
  const int OPS = 10000;  // fewer ops for sync mode — fsync is slow
  std::cout << "\n=== EXP 6: WAL Sync Modes — Throughput vs Durability ===\n\n";

  struct Run { std::string label; bool sync; bool no_wal; };
  std::vector<Run> runs = {
    {"WAL buffered (default)", false, false},
    {"WAL sync=true (fsync/op)", true, false},
    {"WAL disabled (no durability)", false, true},
  };

  double base = 0;
  std::cout << std::left << std::setw(35) << "Mode"
            << std::setw(18) << "Throughput" << "Relative\n"
            << std::string(65, '-') << "\n";
  for (auto& r : runs) {
    double ops = bench("../results/exp6_sync", r.sync, r.no_wal, OPS);
    if (base == 0) base = ops;
    std::cout << std::setw(35) << r.label
              << std::setw(18) << (std::to_string((int)ops) + " ops/s")
              << std::fixed << std::setprecision(2) << ops / base << "x\n";
  }

  std::cout << "\nINSIGHT: Each fsync() call blocks until storage confirms persistence.\n"
            << "  Measured ~300-500us per fsync on SSD (DS614 Exp4 instrumentation).\n"
            << "  Group commit amortises this: N writers share 1 fsync.\n\n"
            << "CODE REFS:\n"
            << "  Sync loop: db/db_impl/db_impl_write.cc:2383 (in WriteGroupToWAL)\n"
            << "  sync flag:  include/rocksdb/options.h (WriteOptions::sync)\n";
  return 0;
}
