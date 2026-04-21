# RocksDB Write-Ahead Log (WAL) Analysis and Instrumentation

[![View Code Changes](https://img.shields.io/badge/View_Code_Changes_(GitHub_Diff)-orange?style=for-the-badge&logo=github)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)
[![View Data Verification](https://img.shields.io/badge/View_Data_Verification_(Notebook)-blue?style=for-the-badge&logo=jupyter)](./comparison.ipynb)

This project involves the deep instrumentation and technical analysis of the RocksDB Write-Ahead Log (WAL), focusing on the critical tradeoff between persistence durability and write performance. It was developed to explore how sequential I/O, record fragmentation, and group commit logic impact the overall ingestion efficiency of a high-performance key-value store.

---

## Technical Overview: What is RocksDB WAL?
The Write-Ahead Log (WAL) is the fundamental durability component of RocksDB. Every write operation (Put, Merge, Delete) is appended to the WAL before being inserted into the MemTable. This ensures that in the event of a crash, the database can reconstruct the in-memory state by replaying the log.

Our instrumentation targets the core execution path to capture:
* Ratio of fsync() calls to logical writes (Performance Tax).
* Record splitting and header overhead (Fragmentation).
* Batch grouping efficiency during heavy contention.

---

## Folder Structure
Focused overview of edited and newly added components:

```
.
├── db/                             [Core Source Code]
│   ├── log_writer.cc               (Modified: Record telemetry)
│   ├── log_reader.cc               (Modified: Recovery telemetry)
│   ├── write_thread.cc             (Modified: Batching telemetry)
│   ├── log_format.h                (Modified: Configurable block logic)
│   └── db_impl/
│       ├── db_impl_write.cc        (Modified: Sync-mode tracking)
│       └── db_impl_open.cc         (Modified: Recovery timing)
├── experiments/                    [New: Benchmark Suite]
│   ├── Makefile                    (Automated build system)
│   ├── sync_bench.cc               (Study 1: Performance vs Durability)
│   ├── fragment_bench.cc           (Study 2: Overhead Analysis)
│   ├── recovery_bench.cc           (Study 3: Consistency Modes)
│   ├── batch_bench.cc              (Study 4: Concurrent Writing)
│   └── skew_bench.cc               (Study 5: MTTR Scaling)
├── comparison.ipynb                [New: Verification Notebook]
├── report.md                       [Systems Engineering Report]
└── README.md                       [Technical Overview]
```

---

## Quick Access: Documentation and Verification
* [README.md](./README.md): Main project landing page.
* [report.md](./report.md): Formal Systems Engineering report.
* [comparison.ipynb](./comparison.ipynb): Data verification and analysis notebook.

## Quick Access: Instrumented Files
* [db/db_impl/db_impl_write.cc](./db/db_impl/db_impl_write.cc): Performance counters for write modes.
* [db/log_writer.cc](./db/log_writer.cc): Fragmentation and payload metrics.
* [db/write_thread.cc](./db/write_thread.cc): Group commit efficiency logic.
* [db/db_impl/db_impl_open.cc](./db/db_impl/db_impl_open.cc): Startup telemetry and recovery path.
* [db/log_reader.cc](./db/log_reader.cc): CRC32 failure and corruption detection.

---

## How to Run in WSL/Ubuntu

1. Install build dependencies:
```bash
sudo apt-get update
sudo apt-get install build-essential libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev
```

2. Compile the static library:
```bash
make static_lib -j$(nproc)
```

3. Enter benchmark suite and compile:
```bash
cd experiments
make
```

4. Execute studies:
```bash
./sync_bench
./fragment_bench
./recovery_bench
./batch_bench
./skew_bench
```

---

## Experimental Analysis
Individual insights derived from custom instrumentation telemetry.

### Study 1: Performance Tax of Durability
<img src="./docs/images/exp1_throughput.png" width="400" />

Insight: Strict synchronization (fsync) introduces a significant performance floor limited by disk IOPS.

### Study 2: Header Overhead and Fragmentation
<img src="./docs/images/exp2_fragmentation.png" width="400" />

Insight: Smaller block sizes increase fragmentation, leading to a higher ratio of header bytes to payload bytes.

### Study 3: Recovery Mode Comparison
<img src="./docs/images/exp3_recovery_mode.png" width="400" />

Insight: Recovery time scales with the strictness of consistency checks performed during the WAL replay.

### Study 4: Group Commit Efficiency
<img src="./docs/images/exp4_group_commit.png" width="400" />

Insight: Increased concurrency leverages the leader-follower batching mechanism to amortize synchronization costs.

### Study 5: Recovery Scaling and MTTR
<img src="./docs/images/exp5_scaling.png" width="400" />

Insight: Mean Time To Recovery (MTTR) increases linearly with the volume of uncompressed WAL data.

---

## Code Modification Statistics
Quantitative breakdown of instrumentation changes across the core RocksDB system files:

| Instrumented File | Additions (+) | Deletions (-) | Summary of Change |
| :--- | :--- | :--- | :--- |
| `db/log_writer.cc` | 18 | 0 | Fragmentation and payload atomic counters |
| `db/log_writer.h` | 13 | 0 | External counter declarations |
| `db/write_thread.h` | 19 | 0 | Batching and group commit metrics |
| `db/write_thread.cc` | 10 | 0 | Group commit efficiency logic |
| `db/db_impl/db_impl_write.cc` | 10 | 0 | Sync-mode performance counters |
| `db/db_impl/db_impl_open.cc` | 10 | 0 | Recovery path timing and telemetry |
| `db/log_reader.cc` | 5 | 0 | CRC mismatch and corruption detection |
| `db/log_format.h` | 4 | 0 | Configurable block-level macro logic |

---
