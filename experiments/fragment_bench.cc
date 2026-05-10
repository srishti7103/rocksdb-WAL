#include <iostream>
#include <chrono>
#include <atomic>
#include <memory>
#include <fstream>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

namespace ROCKSDB_NAMESPACE {
namespace log {
  extern std::atomic<uint64_t> g_wal_fragment_records;
  extern std::atomic<uint64_t> g_wal_bytes_header;
}
}

using namespace ROCKSDB_NAMESPACE;

int main() {
    std::string kDBPath = "/tmp/rocksdb_fragment_bench";
    std::unique_ptr<DB> db;
    Options options;
    options.create_if_missing = true;
    
    Status s = DB::Open(options, kDBPath, &db);
    if (!s.ok()) return 1;

    WriteOptions write_options;
    // Use 10,000 bytes to show visible fragmentation (approx 0.1%)
    std::string large_value(10000, 'a'); 
    
    int ops = 5000;
    for(int i=0; i<ops; i++) {
        db->Put(write_options, "key" + std::to_string(i), large_value);
    }

    uint64_t header = log::g_wal_bytes_header.load();
    uint64_t payload = (uint64_t)ops * 10000;

    std::ofstream csv("../results/wal_performance_telemetry.csv", std::ios_base::app);
    csv << "WAL_Bytes_Header," << ops << "," << header << "\n";
    csv << "WAL_Bytes_Payload," << ops << "," << payload << "\n";
    csv.close();

    std::cout << " -> Total Operations: " << ops << std::endl;
    std::cout << " -> Header Bytes Wasted: " << header << " bytes" << std::endl;
    std::cout << " -> Payload Bytes Written: " << payload << " bytes" << std::endl;
    std::cout << "Fragment benchmark complete.\n" << std::endl;
    return 0;
}
