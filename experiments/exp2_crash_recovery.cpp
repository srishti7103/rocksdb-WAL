/*
 * EXPERIMENT 2: WAL Crash Recovery — Durability Guarantee
 */
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <memory>

namespace fs = std::filesystem;
const int NUM_KEYS = 5000;
const int VSIZE    = 128;

void write_and_flush(const std::string& path, bool wal_off) {
  fs::remove_all(path);
  rocksdb::Options opts;
  opts.create_if_missing = true;
  opts.disable_auto_compactions = true;
  rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr;
  rocksdb::DB::Open(opts, path, &db_ptr); db = db_ptr.release();
  rocksdb::WriteOptions wo;
  wo.disableWAL = wal_off;
  const std::string val(VSIZE, 'x');
  for (int i = 0; i < NUM_KEYS; i++)
    db->Put(wo, "key" + std::to_string(i), val);
  db->Flush(rocksdb::FlushOptions());
  delete db;
}

void delete_sst(const std::string& path) {
  for (auto& e : fs::directory_iterator(path)) {
    auto ext = e.path().extension().string();
    if (ext == ".sst" || ext == ".ldb") fs::remove(e.path());
  }
}

int count_keys(rocksdb::DB* db) {
  int count = 0;
  auto* it = db->NewIterator(rocksdb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) count++;
  delete it;
  return count;
}

int main() {
  std::cout << "\n=== EXP 2: WAL Crash Recovery ===\n\n";

  const std::string path_a = "../results/exp2_normal";
  write_and_flush(path_a, false);
  {
    rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr; rocksdb::Options opts; opts.create_if_missing = false;
    rocksdb::DB::Open(opts, path_a, &db_ptr); db = db_ptr.release();
    std::cout << "[A] Normal open (SST intact):      " << count_keys(db) << "/" << NUM_KEYS << "\n";
    delete db;
  }
  fs::remove_all(path_a);

  const std::string path_b = "../results/exp2_wal_on";
  write_and_flush(path_b, false);
  delete_sst(path_b);
  {
    rocksdb::DB* db; std::unique_ptr<rocksdb::DB> db_ptr; rocksdb::Options opts; opts.create_if_missing = false;
    opts.wal_recovery_mode = rocksdb::WALRecoveryMode::kPointInTimeRecovery;
    auto s = rocksdb::DB::Open(opts, path_b, &db_ptr); db = db_ptr.release();
    if (s.ok()) {
      std::cout << "[B] WAL=ON, crash:                " << count_keys(db) << "/" << NUM_KEYS << "\n";
      delete db;
    }
  }
  fs::remove_all(path_b);

  return 0;
}
