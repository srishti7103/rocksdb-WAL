/*
 * EXPERIMENT 5: Failure Analysis — What Happens When Data Size Increases?
 *
 * THREE EFFECTS OF DATA GROWTH:
 *   1. RECOVERY TIME grows linearly with WAL size
 *      RecoverLogFiles() at db_impl_open.cc:1132 replays every record
 *   2. WAL ROTATION triggers when wals_total_size_ > max_total_wal_size
 *      PreprocessWrite() at db_impl_write.cc:2097 checks line 2111
 *      SwitchWAL() at db_impl_write.cc:2634 forces a MemTable flush
 *   3. WRITE STALL if flush can't keep up (monitoring/db_impl.cc)
 *
 * THIS EXPERIMENT:
 *   Write increasing volumes of data, measure reopen (recovery) time,
 *   and observe WAL rotation events via DS614 Exp5 instrumentation.
 */
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using Clock = std::chrono::high_resolution_clock;

long long reopen_time_ms(const std::string& path) {
  rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr;
  rocksdb::Options opts;
  opts.create_if_missing = false;
  auto t0 = Clock::now();
  auto s = rocksdb::DB::Open(opts, path, &db_ptr); db = db_ptr.release();
  long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     Clock::now() - t0).count();
  if (s.ok()) delete db;
  return ms;
}

int main() {
  std::cout << "\n=== EXP 5: Data Growth — Recovery Time vs WAL Volume ===\n\n";
  std::cout << std::left << std::setw(12) << "Ops Written"
            << std::setw(15) << "Recovery (ms)" << "Notes\n"
            << std::string(50, '-') << "\n";

  for (int target_ops : {10000, 50000, 100000, 500000}) {
    const std::string path = "../results/exp5_growth";
    fs::remove_all(path);

    rocksdb::Options opts;
    opts.create_if_missing = true;
    opts.disable_auto_compactions = true;
    // Large write buffer so data stays in WAL, not flushed to SST
    opts.write_buffer_size = 1024LL * 1024 * 1024;  // 1 GB — never auto-flush
    opts.max_total_wal_size = 0;  // disable auto rotation for this test
    rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr;
    rocksdb::DB::Open(opts, path, &db_ptr); db = db_ptr.release();

    rocksdb::WriteOptions wo;
    const std::string val(256, 'v');
    for (int i = 0; i < target_ops; i++)
      db->Put(wo, "k" + std::to_string(i), val);

    // Close WITHOUT flushing — all data stays in WAL only
    
    delete db;

    long long ms = reopen_time_ms(path);
    std::cout << std::setw(12) << target_ops
              << std::setw(15) << ms
              << (ms > 500 ? " ← recovery bottleneck" : "") << "\n";
    fs::remove_all(path);
  }

  std::cout << "\nINSIGHT: Recovery time scales with WAL volume.\n"
            << "  RocksDB mitigates this via max_total_wal_size + SwitchWAL().\n\n"
            << "CODE REFS:\n"
            << "  RecoverLogFiles():  db/db_impl/db_impl_open.cc:1132\n"
            << "  PreprocessWrite():  db/db_impl/db_impl_write.cc:2097\n"
            << "  SwitchWAL():        db/db_impl/db_impl_write.cc:2634\n"
            << "  wals_total_size_:   db/db_impl/db_impl_write.cc:2111\n";
  return 0;
}
