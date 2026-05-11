# RocksDB Write-Ahead Log (WAL) Analysis and Instrumentation

[![View Code Changes](https://img.shields.io/badge/View_Code_Changes_(GitHub_Diff)-orange?style=for-the-badge&logo=github)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)

This project provides a deep-dive instrumentation and systems engineering analysis of the RocksDB Write-Ahead Log (WAL). Developed by **Sigma & Spark**, this work explores the fundamental tradeoffs between data durability, ingestion latency, and recovery efficiency in high-performance storage engines.

---

## 1. Technical Foundation & Architecture

### Architectural Overview (Concept Mapping)
RocksDB is optimized for fast storage environments. Our analysis maps the system to five core Big Data Engineering concepts:

1.  **LSM-Tree Storage Core (Storage):** RocksDB utilizes a Log-Structured Merge-Tree to transform random writes into sequential I/O by buffering updates in memory before persisting them as SSTs.
2.  **Write-Optimized Data Ingestion (Ingestion):** Throughput is prioritized via append-only logging, deferring expensive compaction tasks to maintain high ingestion rates.
3.  **Append-Only Durability (Reliability):** The WAL serves as the "Source of Truth," ensuring that every transaction is persistent before it is acknowledged to the user.
4.  **Atomic Crash Consistency (Fault Tolerance):** Every log record is protected by CRC-32 checksums to detect "Torn Writes," ensuring the database never recovers into an inconsistent state.
5.  **MTTR Optimization (Availability):** By measuring and tuning WAL replay duration, the system balances the time required to get back online (Availability) against the strictness of data validation.

---

## 2. Project Architecture & Folder Structure

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

## 3. Design Decisions & System Tradeoffs
| Design Decision | Code Implementation | Problem Solved | System Tradeoff |
| :--- | :--- | :--- | :--- |
| **Strict Sync Policy** | `db_impl_write.cc` | Data Integrity Guarantee | **Durability vs. Latency:** Absolute safety introduces a **524x** performance floor. |
| **Group Commit** | `write_thread.cc` | Sequential I/O Bottleneck | **Throughput vs. Thread Latency:** Batching multiple writes into one I/O cycle amortizes `fsync` costs but adds minor wait-times. |
| **Fixed-Block Alignment** | `log_writer.cc` | Hardware I/O Optimization | **Efficiency vs. Space:** Aligns writes to 32KB hardware pages to minimize write amplification, with a negligible **0.1%** overhead. |

---

## 4. Environment Setup & Reproduction

### Step 1: Dependencies
```bash
sudo apt-get update && sudo apt-get install -y build-essential libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev libgflags-dev gdb
git clone https://github.com/srishti7103/rocksdb-WAL.git && cd rocksdb-WAL
git checkout wal-experiments
```

### Step 2: Build & Execution
```bash
make static_lib -j$(nproc)
cd experiments && chmod +x viva_run.sh && ./viva_run.sh
```

---

## 5. Instrumentation Audit: Code-Level Modification
| Instrumented File | Original Implementation | Modified Implementation | Lines Added (+) |
| :--- | :--- | :--- | :--- |
| **db/log_writer.cc** | Sequential record emission without internal size tracking. | Injected `fetch_add` logic into `AddRecord` to track payload vs metadata bytes. | **+19** |
| **db/write_thread.cc** | Group Commit queue management without batch-size exposure. | Captured `new_batch_size` within the leader-follower handoff for efficiency analysis. | **+10** |
| **db/db_impl/db_impl_write.cc** | Standard write entry point that ignored durability-tier categorization. | Added conditional hooks to count `options.sync` vs `buffered` write events. | **+10** |
| **db/db_impl/db_impl_open.cc** | Replayed WAL files during startup without performance measurement. | Wrapped the `ReplayWAL` loop in high-resolution nanosecond timers to calculate MTTR. | **+13** |
| **db/log_reader.cc** | Validated records without exposing corruption frequency to the engine. | Hooked into the checksum verification path to count and log data integrity failures. | **+5** |

---

## 6. Experimental Evaluation: Highlights

*   **Study 1 (Sync Tax):** Enabling strict hardware sync introduces a **524x** performance penalty.
*   **Study 2 (Fragmentation):** Block alignment costs only **0.1%** in wasted space.
*   **Study 3 (MTTR):** Lenient recovery reduces startup downtime by **1.5x**.
*   **Study 4 (Batching):** Group Commit provides **4.3x** throughput amplification under load.
*   **Study 5 (Scaling):** Recovery time scales linearly **O(N)** with WAL volume.

---

## 7. Failure Analysis & System Resilience

### 1. What happens when data size increases significantly?
MTTR grows linearly ($O(N)$). Without log rotation, a 10x increase in log volume leads to a 10x increase in recovery time.

### 2. What happens under skew?
Write skew (high thread contention) triggers the **Group Commit** mechanism, making the system *more* efficient as batch sizes grow larger.

### 3. What happens if a component fails?
Power loss causes **Torn Writes**. RocksDB detects this via **CRC-32 Checksums** in `log_reader.cc` and discards corrupted tail fragments.

### 4. What assumptions does the system rely on?
The system assumes **Storage Honesty** (that `fsync` actually persists data) and **Sequential Advantage** (that logs are faster than random updates).

---

## 8. Conclusion
The RocksDB WAL is a masterclass in systems engineering tradeoffs. By mapping code-level instrumentation to empirical results, we have demonstrated how sequential I/O, batching, and checksumming work together to build a high-fidelity storage foundation.

---

## Credits
**Srishti Lamba**: 202518003 | **Nikita Sharma**: 202518038
**Sigma & Spark**
