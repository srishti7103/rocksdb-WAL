# RocksDB Write-Ahead Log (WAL) Analysis and Instrumentation

[![View Code Changes](https://img.shields.io/badge/View_Code_Changes_(GitHub_Diff)-orange?style=for-the-badge&logo=github)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)

This project provides a deep-dive instrumentation and systems engineering analysis of the RocksDB Write-Ahead Log (WAL). Developed by **Sigma & Spark**, this work explores the fundamental tradeoffs between data durability, ingestion latency, and recovery efficiency in high-performance storage engines.

---

## 1. System Overview: RocksDB & The WAL

### What is RocksDB?
RocksDB is a high-performance, embeddable key-value store based on the **Log-Structured Merge-Tree (LSM-Tree)** architecture. Unlike B-Trees, which update data in place, LSM-trees transform random writes into sequential I/O by buffering updates in memory before flushing them to disk as immutable sorted files.

### The Role of the WAL
Because data is initially stored in a volatile in-memory structure called the **MemTable**, a system crash would result in total data loss for all un-flushed writes. The **Write-Ahead Log (WAL)** is the solution to this "Durability" problem in ACID transactions. 

**How it works:** Every write operation is appended to a sequential file on non-volatile storage *before* it is applied to the MemTable. In the event of a crash, RocksDB reconstructs the latest state by replaying the WAL records into a new MemTable.

---

## 2. Project Architecture & Folder Structure

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
│   ├── recovery_scaling_bench.cc   (Study 5: MTTR Scaling)
├── comparison.ipynb                [New: Verification Notebook]
├── report.md                       [Systems Engineering Report]
└── README.md                       [Technical Overview]
```

---

## 3. How to Run & Reproduce (Ubuntu / WSL)

### Step 1: Dependencies & Environment
Clone the repository inside the Linux filesystem (`~/`), **not** on `/mnt/c/`.
```bash
sudo apt-get update && sudo apt-get install -y build-essential libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev libgflags-dev g++
git clone https://github.com/srishti7103/rocksdb-WAL.git && cd rocksdb-WAL
git checkout wal-experiments
```

### Step 2: Build & Execution
```bash
# Build the core library (takes 10-20 mins)
make static_lib -j$(nproc)

# Run the automated benchmark suite
cd experiments && chmod +x viva_run.sh
./viva_run.sh
```

---

## 4. Instrumentation Audit: Deep Dive into the Codebase

We modified the core execution path of RocksDB to extract high-fidelity telemetry. Below is a breakdown of instrumentation changes across the core RocksDB system:

| Instrumented File | Additions (+) | Summary of Change | Original Role |
| :--- | :--- | :--- | :--- |
| `db/log_writer.cc` | 19 | Fragmentation & payload atomic counters | Physical record serialization |
| `db/write_thread.cc` | 10 | Group commit efficiency logic | Concurrency & Leader management |
| `db/db_impl/db_impl_write.cc` | 10 | Sync-mode performance counters | Main write path entry point |
| `db/db_impl/db_impl_open.cc` | 13 | Recovery path timing and telemetry | DB startup and WAL initialization |
| `db/log_reader.cc` | 5 | CRC mismatch and corruption detection | Record validation during replay |
| `db/log_format.h` | 4 | Configurable block-level macro logic | Block-level file structure |

---

## 5. Quick Access: Documentation and Verification
* [README.md](./README.md): Main project landing page.
* [report.md](./report.md): Formal Systems Engineering report.
* [comparison.ipynb](./comparison.ipynb): Data verification and analysis notebook.

### Quick Access: Instrumented Files
* [db/db_impl/db_impl_write.cc](./db/db_impl/db_impl_write.cc): Performance counters for write modes.
* [db/log_writer.cc](./db/log_writer.cc): Fragmentation and payload metrics.
* [db/write_thread.cc](./db/write_thread.cc): Group commit efficiency logic.
* [db/db_impl/db_impl_open.cc](./db/db_impl/db_impl_open.cc): Startup telemetry and recovery path.
* [db/log_reader.cc](./db/log_reader.cc): CRC32 failure and corruption detection.

---

## 6. Experimental Evaluations: Hypothesis vs. Reality

### Study 1: The "Safety Tax" (Durability vs. Throughput)

*   **Hypothesis:** Enabling strict `fsync()` for every write will decrease throughput by several orders of magnitude as the system becomes bound by disk I/O latency rather than CPU/RAM speed.
*   **Instrumentation Point (`db_impl_write.cc`):**
    ```cpp
    // Tracking fsync frequency to measure the 'Safety Tax'
    if (options.sync) rocksdb::WAL_Sync_Control.fetch_add(1);
    ```
*   **Method:** Compared `Buffered` (OS cache) writes vs. `Strict Sync` (hardware flush) writes.
<img src="./docs/images/exp1_throughput.png" width="400" />

*   **Key Insight:** Strict synchronization introduces a <!-- DYNAMIC:SYNC_TAX -->**524x**<!-- END_DYNAMIC --> performance floor.
*   **Theoretical Backing:** Disk IOPS are bounded by mechanical/electronic latency (ms range), whereas RAM access is nanosecond-scale.

### Study 2: Storage Efficiency (Fragmentation)

*   **Hypothesis:** Maintaining fixed 32KB block alignment for the WAL (to optimize hardware page reads) will introduce a constant metadata overhead proportional to the record frequency.
*   **Instrumentation Point (`log_writer.cc`):**
    ```cpp
    // Measuring payload vs header ratio per record
    rocksdb::WAL_Bytes_Payload.fetch_add(payload_size);
    ```
*   **Method:** Measured `WAL_Bytes_Header` vs. `WAL_Bytes_Payload`.
<img src="./docs/images/exp2_fragmentation.png" width="400" />

*   **Key Insight:** Fixed-block design introduces exactly <!-- DYNAMIC:FRAG_PCT -->**0.1%**<!-- END_DYNAMIC --> metadata fragmentation.
*   **Theoretical Backing:** Internal fragmentation is an unavoidable byproduct of data alignment requirements for efficient block storage access.

### Study 3: Recovery Reduction (MTTR Analysis)

*   **Hypothesis:** Sacrificing strict consistency checks during startup (`kTolerateCorruptedTailRecords`) will significantly reduce the Mean Time To Recovery (MTTR).
*   **Instrumentation Point (`db_impl_open.cc`):**
    ```cpp
    // High-resolution timing of the WAL replay loop
    auto start_t = Env::Default()->NowNanos();
    s = ReplayWAL(options, ...);
    rocksdb::WAL_Recovery_Mode.fetch_add(Env::Default()->NowNanos() - start_t);
    ```
*   **Method:** Measured recovery time across three consistency modes (`Absolute`, `Point-in-Time`, `Tolerate`).
<img src="./docs/images/exp3_recovery_mode.png" width="400" />

*   **Key Insight:** Lenient recovery logic yields a <!-- DYNAMIC:RECOVERY_REDUCTION -->**1.5x**<!-- END_DYNAMIC --> reduction in MTTR.
*   **Theoretical Backing:** Reducing the verification work (checksumming) and IOPS required during log replay directly optimizes the critical path of startup availability.

### Study 4: Group Commit Efficiency (Batching)

*   **Hypothesis:** Under high thread contention, throughput will scale non-linearly as multiple threads are batched into a single "Group Commit" leader.
*   **Instrumentation Point (`write_thread.cc`):**
    ```cpp
    // Capturing real-time batch sizes during leader/follower sync
    rocksdb::WAL_Group_Commit.fetch_add(new_batch_size);
    ```
*   **Method:** Scaled concurrency from 1 to 8 threads under synchronous write pressure.
<img src="./docs/images/exp4_group_commit.png" width="400" />

*   **Key Insight:** Leader-follower batching delivers a <!-- DYNAMIC:GROUP_COMMIT -->**4.3x**<!-- END_DYNAMIC --> throughput amplification.
*   **Theoretical Backing:** Amdahl's Law is mitigated here by converting parallel synchronization contention into a single sequential batch operation.

### Study 5: Recovery Scaling (Volume Analysis)

*   **Hypothesis:** Recovery time will exhibit a linear (O(N)) relationship with the volume of data stored in the WAL.
*   **Instrumentation Point (`db_impl_open.cc`):**
    ```cpp
    // Tracking recovery time vs. uncompressed log volume
    auto t = ReplayWAL(options, ...); 
    ```
*   **Method:** Measured replay time as the WAL volume scaled from 10k to 500k items.
<img src="./docs/images/exp5_scaling.png" width="400" />

*   **Key Insight:** WAL replay exhibits **Proportional Scaling**.
*   **Theoretical Backing:** Replaying a log is inherently sequential; unless the log is truncated or partitioned, recovery effort grows linearly with log length.

---

## 7. Failure Analysis & Data Integrity

### What happens if a component fails mid-write?
If power is lost during a write operation, a **Torn Write** can occur where only half a sector is persisted to disk. RocksDB's WAL handles this through:
1.  **CRC-32 Checksums:** Every log record is checksummed. If the checksum on disk doesn't match the recalculated checksum during recovery, the record is flagged.
2.  **Fragmented Replay:** Using the `log::Reader` we instrumented, the system can detect the exact point of failure and either halt (Strict mode) or discard the trailing corrupted record (Tolerate mode), ensuring the database never recovers into an "impossible" state.

---

## Credits
Built by **Sigma & Spark**: where B.Sc. Statistics meets Leveled Sparks 

**Srishti Lamba**: 202518003 | **Nikita Sharma**: 202518038
