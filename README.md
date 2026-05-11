# RocksDB Write-Ahead Log (WAL) Analysis and Instrumentation

[![View Code Changes](https://img.shields.io/badge/View_Code_Changes_(GitHub_Diff)-orange?style=for-the-badge&logo=github)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)

This project provides a deep-dive instrumentation and systems engineering analysis of the RocksDB Write-Ahead Log (WAL). Developed by **Sigma & Spark**, this work explores the fundamental tradeoffs between data durability, ingestion latency, and recovery efficiency in high-performance storage engines.

---

## 1. System Overview: RocksDB & The WAL

### What is RocksDB? (Concept Mapping)
RocksDB is a high-performance, embeddable key-value store based on the **Log-Structured Merge-Tree (LSM-Tree)** architecture. 
*   **Storage (Concept 1):** Unlike B-Trees, RocksDB uses an LSM-tree to transform random writes into sequential I/O by buffering updates in memory before flushing them to disk as immutable sorted files (SSTs).
*   **Ingestion (Concept 2):** High-speed data ingestion is achieved by prioritizing write throughput.
*   **Reliability (Concept 3):** The **Write-Ahead Log (WAL)** ensures crash consistency by acting as a persistent append-only log of every transaction.
*   **Fault Tolerance (Concept 4):** Every log record is protected by **CRC-32 checksums** to detect and handle torn writes or hardware failure.

### The Role of the WAL
Because data is initially stored in a volatile in-memory structure called the **MemTable**, a system crash would result in total data loss. The WAL ensures that every write is appended to non-volatile storage *before* it is applied to memory, allowing the engine to reconstruct state during recovery.

### Deep Dive: The WAL as a Serial Bottleneck
In a highly concurrent system, the WAL is often the **single serial bottleneck**. While memory can be updated in parallel, the log is inherently sequential. This project analyzes how RocksDB mitigates this bottleneck through techniques like **Group Commit** and **Fixed-Block Alignment**.

---

## 2. Design Decisions: Tradeoff Analysis
We identified three key architectural decisions in the RocksDB WAL. For each, we analyzed the implementation, the problem solved, and the resulting tradeoff.

| Design Decision | Implementation File | Problem Solved | Tradeoff |
| :--- | :--- | :--- | :--- |
| **Strict Sync Policy** | `db_impl_write.cc` | Total Data Loss Prevention | **Durability vs. Latency:** Ensures 0% data loss but introduces a **524x** performance tax. |
| **Group Commit** | `write_thread.cc` | Serial I/O Bottleneck | **Throughput vs. Individual Latency:** Threads wait for a leader to batch writes, amortizing `fsync` costs but adding small wait times. |
| **Fixed-Block Alignment** | `log_writer.cc` | Hardware Page Alignment | **IOPS Efficiency vs. Space:** Aligns writes to 32KB hardware pages to minimize read/write amplification, costing **0.1%** in space. |

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

## 4. Instrumentation Audit: Code-Level Modification
We modified the core execution path of RocksDB to extract high-fidelity telemetry.

| Instrumented File | Line(s) | Summary of Change | Technical Methodology |
| :--- | :--- | :--- | :--- |
| `db/log_writer.cc` | 130, 326 | Fragmentation & payload tracking | Injected `std::atomic` counters into `AddRecord` to track bytes emitted vs payload. |
| `db/write_thread.cc` | 452, 576 | Group commit efficiency logic | Instrumented the Leader-Follower handoff to measure batch amplification ratios. |
| `db/db_impl/db_impl_write.cc` | 2267 | Sync-mode performance counters | Hooked into the sync-policy gate to categorize writes by durability tier. |
| `db/db_impl/db_impl_open.cc` | 1136 | Recovery path timing and telemetry | Wrapped the `ReplayWAL` loop in high-resolution nanosecond timers. |
| `db/log_reader.cc` | 327 | CRC failure detection | Hooked into the checksum validation path to detect and log "Torn Writes." |
| `db/log_format.h` | 54-58 | Configurable block-level macro logic | Modified 32KB block alignment macros to observe fragmentation effects. |

---

## 5. Experimental Evaluations: Hypothesis vs. Reality

### Study 1: The "Safety Tax" (Durability vs. Throughput)

*   **Hypothesis:** Enabling strict `fsync()` for every write will decrease throughput by several orders of magnitude as the system becomes bound by disk I/O latency rather than CPU/RAM speed.
*   **Instrumentation Point (`db_impl_write.cc:2267`):**
    ```cpp
    if (options.sync) rocksdb::WAL_Sync_Control.fetch_add(1, std::memory_order_relaxed);
    ```
*   **Method:** Compared `Buffered` (OS cache) writes vs. `Strict Sync` (hardware flush) writes.
<div align="center">
  <img src="./docs/images/exp1_throughput.png" width="500" />
</div>

*   **Key Insight:** Strict synchronization introduces a <!-- DYNAMIC:SYNC_TAX -->**524x**<!-- END_DYNAMIC --> performance floor.
*   **Theoretical Backing:** Disk IOPS are bounded by mechanical/electronic latency (ms range), whereas RAM access is nanosecond-scale.

### Study 2: Storage Efficiency (Fragmentation)

*   **Hypothesis:** Maintaining fixed 32KB block alignment for the WAL (to optimize hardware page reads) will introduce a constant metadata overhead proportional to the record frequency.
*   **Instrumentation Point (`log_writer.cc:130,326`):**
    ```cpp
    rocksdb::WAL_Bytes_Payload.fetch_add(payload_size, std::memory_order_relaxed);
    ```
*   **Method:** Measured `WAL_Bytes_Header` vs. `WAL_Bytes_Payload`.
<div align="center">
  <img src="./docs/images/exp2_fragmentation.png" width="500" />
</div>

*   **Key Insight:** Fixed-block design introduces exactly <!-- DYNAMIC:FRAG_PCT -->**0.1%**<!-- END_DYNAMIC --> metadata fragmentation.
*   **Theoretical Backing:** Internal fragmentation is an unavoidable byproduct of data alignment requirements for efficient block storage access.

### Study 3: Recovery Reduction (MTTR Analysis)

*   **Hypothesis:** Sacrificing strict consistency checks during startup (`kTolerateCorruptedTailRecords`) will significantly reduce the Mean Time To Recovery (MTTR).
*   **Instrumentation Point (`db_impl_open.cc:1136`):**
    ```cpp
    auto start_t = Env::Default()->NowNanos();
    s = ReplayWAL(options, ...);
    rocksdb::WAL_Recovery_Mode.fetch_add(Env::Default()->NowNanos() - start_t);
    ```
*   **Method:** Measured recovery time across three consistency modes (`Absolute`, `Point-in-Time`, `Tolerate`).
<div align="center">
  <img src="./docs/images/exp3_recovery_mode.png" width="500" />
</div>

*   **Key Insight:** Lenient recovery logic yields a <!-- DYNAMIC:RECOVERY_REDUCTION -->**1.5x**<!-- END_DYNAMIC --> reduction in MTTR.
*   **Theoretical Backing:** Reducing the verification work (checksumming) and IOPS required during log replay directly optimizes the critical path of startup availability.

### Study 4: Group Commit Efficiency (Batching)

*   **Hypothesis:** Under high thread contention, throughput will scale non-linearly as multiple threads are batched into a single "Group Commit" leader.
*   **Instrumentation Point (`write_thread.cc:452,576`):**
    ```cpp
    rocksdb::WAL_Group_Commit.fetch_add(new_batch_size, std::memory_order_relaxed);
    ```
*   **Method:** Scaled concurrency from 1 to 8 threads under synchronous write pressure.
<div align="center">
  <img src="./docs/images/exp4_group_commit.png" width="500" />
</div>

*   **Key Insight:** Leader-follower batching delivers a <!-- DYNAMIC:GROUP_COMMIT -->**4.3x**<!-- END_DYNAMIC --> throughput amplification.
*   **Theoretical Backing:** Amdahl's Law is mitigated here by converting parallel synchronization contention into a single sequential batch operation.

### Study 5: Recovery Scaling (Volume Analysis)

*   **Hypothesis:** Recovery time will exhibit a linear (O(N)) relationship with the volume of data stored in the WAL.
*   **Instrumentation Point (`db_impl_open.cc:1136`):**
    ```cpp
    auto t = ReplayWAL(options, ...); // Replay timing across volumes
    ```
*   **Method:** Measured replay time as the WAL volume scaled from 10k to 500k items.
<div align="center">
  <img src="./docs/images/exp5_scaling.png" width="500" />
</div>

*   **Key Insight:** WAL replay exhibits **Proportional Scaling**.
*   **Theoretical Backing:** Replaying a log is inherently sequential; unless the log is truncated or partitioned, recovery effort grows linearly with log length.

---

## 7. Failure Analysis & Data Integrity

### What happens if a component fails mid-write?
If power is lost during a write operation, a **Torn Write** can occur where only half a sector is persisted to disk. RocksDB's WAL handles this through:
1.  **CRC-32 Checksums:** Every log record is checksummed. If the checksum on disk doesn't match the recalculated checksum during recovery, the record is flagged.
2.  **Fragmented Replay:** Using the `log::Reader` we instrumented, the system can detect the exact point of failure and either halt (Strict mode) or discard the trailing corrupted record (Tolerate mode), ensuring the database never recovers into an "impossible" state.

---

## 8. Conclusion: Engineering the Tradeoff Curve

Our instrumentation and analysis of the RocksDB Write-Ahead Log have demonstrated that data durability is never a "free" operation. The system is a carefully balanced engine of tradeoffs:

1.  **The Durability Barrier:** The **524x** performance gap between buffered and synchronous writes represents the physical limits of current non-volatile storage technologies.
2.  **Concurrency as a Lever:** Through **Group Commit**, RocksDB successfully amortizes the cost of this barrier, turning thread contention into a performance multiplier (yielding **4.3x** amplification).
3.  **Predictable Resilience:** Our recovery analysis proves that while MTTR scales linearly with log volume, it can be tuned via recovery policies to achieve a **1.5x** improvement in availability.

In conclusion, every parameter of the WAL—from block alignment to sync policy—is a precise engineering dial. This project successfully mapped those dials to empirical data, providing a high-fidelity audit of one of the world's most performant storage engines.

---

## Credits
Built by **Sigma & Spark**: where B.Sc. Statistics meets Leveled Sparks 

**Srishti Lamba**: 202518003 | **Nikita Sharma**: 202518038
