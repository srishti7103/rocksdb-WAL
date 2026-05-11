# RocksDB Write-Ahead Log (WAL) Analysis and Instrumentation

[![View Code Changes](https://img.shields.io/badge/View_Code_Changes_(GitHub_Diff)-orange?style=for-the-badge&logo=github)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)

This project provides a deep-dive instrumentation and systems engineering analysis of the RocksDB Write-Ahead Log (WAL). Developed by **Sigma & Spark**, this work explores the fundamental tradeoffs between data durability, ingestion latency, and recovery efficiency in high-performance storage engines.

---

## 1. Technical Foundation & Architecture

### Architectural Overview
RocksDB is a high-performance, embeddable key-value store optimized for fast storage environments. Our analysis focuses on its core design principles:

*   **LSM-Tree Storage Core:** Unlike traditional B-Trees, RocksDB utilizes a Log-Structured Merge-Tree to transform random writes into sequential I/O by buffering updates in volatile memory (MemTable) before persisting them as immutable sorted files (SSTs).
*   **Write-Optimized Data Ingestion:** Throughput is prioritized by deferring disk-intensive compaction tasks, allowing the engine to handle massive ingestion rates with minimal stall.
*   **Append-Only Durability (WAL):** To ensure zero data loss during system failures, every transaction is recorded in a persistent Write-Ahead Log before being applied to the MemTable.
*   **Atomic Crash Consistency:** The WAL utilizes CRC-32 checksums and fragmented record types to detect and mitigate "Torn Writes," ensuring the database never recovers into an inconsistent state.

### The Role of the WAL
The WAL acts as the system's "Source of Truth" during recovery. Because the MemTable is volatile, a crash results in the loss of all un-flushed data. By replaying the sequential records from the WAL, RocksDB can reconstruct the latest state of the database with high precision.

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

## 3. Design Decisions & System Tradeoffs
We analyzed three fundamental architectural decisions within the RocksDB WAL. Each represents a critical engineering choice balancing performance against safety.

| Design Decision | Implementation File | Problem Solved | System Tradeoff |
| :--- | :--- | :--- | :--- |
| **Strict Sync Policy** | `db_impl_write.cc` | Data Integrity Guarantee | **Durability vs. Latency:** Absolute safety introduces a measurable **524x** performance floor. |
| **Group Commit** | `write_thread.cc` | Sequential I/O Bottleneck | **Throughput vs. Thread Latency:** Batching multiple writes into one I/O cycle amortizes `fsync` costs but adds minor wait-times. |
| **Fixed-Block Alignment** | `log_writer.cc` | Hardware I/O Optimization | **Efficiency vs. Space:** Aligns writes to 32KB hardware pages to minimize write amplification, with a negligible **0.1%** overhead. |

---

## 4. How to Run & Reproduce (Ubuntu / WSL)

### Step 1: Dependencies
Clone the repository inside the Linux filesystem (`~/`) for optimal I/O performance.
```bash
sudo apt-get update && sudo apt-get install -y build-essential libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev libgflags-dev g++
git clone https://github.com/srishti7103/rocksdb-WAL.git && cd rocksdb-WAL
git checkout wal-experiments
```

### Step 2: Build & Execution
```bash
# Compile core library
make static_lib -j$(nproc)

# Execute automated benchmark suite
cd experiments && chmod +x viva_run.sh
./viva_run.sh
```

---

## 5. Instrumentation Audit: Code-Level Modification
We modified the core execution path of RocksDB to extract high-fidelity telemetry. The following table contrasts the original implementation with our instrumented logic:

| Instrumented File | Original Implementation | Modified Implementation | Lines Added (+) |
| :--- | :--- | :--- | :--- |
| **db/log_writer.cc** | Sequential record emission without internal size tracking. | Injected `fetch_add` logic into `AddRecord` to track payload vs metadata bytes. | **+19** |
| **db/write_thread.cc** | Group Commit queue management without batch-size exposure. | Captured `new_batch_size` within the leader-follower handoff for efficiency analysis. | **+10** |
| **db/db_impl/db_impl_write.cc** | Standard write entry point that ignored durability-tier categorization. | Added conditional hooks to count `options.sync` vs `buffered` write events. | **+10** |
| **db/db_impl/db_impl_open.cc** | Replayed WAL files during startup without performance measurement. | Wrapped the `ReplayWAL` loop in high-resolution nanosecond timers to calculate MTTR. | **+13** |
| **db/log_reader.cc** | Validated records without exposing corruption frequency to the engine. | Hooked into the checksum verification path to count and log data integrity failures. | **+5** |
| **db/log_format.h** | Hardcoded 32KB block alignment constant. | Converted the block size into a configurable macro to simulate fragmentation stress. | **+4** |

---

## 6. Quick Access: Documentation and Verification
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

## 7. Experimental Evaluation

### Study 1: The "Safety Tax" (Durability vs. Throughput)

*   **Hypothesis:** Mandatory hardware synchronization will cause an exponential drop in throughput as the bottleneck shifts from RAM to Disk.
*   **Result:** Strict synchronization introduces a <!-- DYNAMIC:SYNC_TAX -->**524x**<!-- END_DYNAMIC --> performance floor.
<div align="center">
  <img src="./docs/images/exp1_throughput.png" width="400" />
</div>

### Study 2: Storage Efficiency (Fragmentation)

*   **Hypothesis:** Maintaining fixed 32KB alignment will introduce a metadata overhead proportional to record frequency.
*   **Result:** Fixed-block design introduces exactly <!-- DYNAMIC:FRAG_PCT -->**0.1%**<!-- END_DYNAMIC --> internal fragmentation.
<div align="center">
  <img src="./docs/images/exp2_fragmentation.png" width="400" />
</div>

### Study 3: Recovery Reduction (MTTR Analysis)

*   **Hypothesis:** Lenient consistency checks during startup will significantly reduce the Mean Time To Recovery (MTTR).
*   **Result:** Lenient recovery yields a <!-- DYNAMIC:RECOVERY_REDUCTION -->**1.5x**<!-- END_DYNAMIC --> reduction in MTTR.
<div align="center">
  <img src="./docs/images/exp3_recovery_mode.png" width="400" />
</div>

### Study 4: Group Commit Efficiency (Batching)

*   **Hypothesis:** Throughput will scale non-linearly with concurrency as multiple threads are batched into a single I/O operation.
*   **Result:** Leader-follower batching delivers a <!-- DYNAMIC:GROUP_COMMIT -->**4.3x**<!-- END_DYNAMIC --> throughput amplification.
<div align="center">
  <img src="./docs/images/exp4_group_commit.png" width="400" />
</div>

### Study 5: Recovery Scaling (Volume Analysis)

*   **Hypothesis:** Recovery time will exhibit a linear relationship with the volume of data stored in the WAL.
*   **Result:** Replay duration exhibits **Proportional Scaling (O(N))**.
<div align="center">
  <img src="./docs/images/exp5_scaling.png" width="400" />
</div>

---

## 8. Failure Analysis & Data Integrity

If a system failure occurs during a write, a **Torn Write** may result. RocksDB mitigates this via:
1.  **CRC-32 Checksums:** Recalculated during recovery to detect bit-level corruption.
2.  **Fragmented Replay:** The `log::Reader` identifies partial records and safely discards corrupted tail fragments, ensuring the engine remains consistent.

---

## 9. Conclusion: Engineering the Tradeoff Curve

Our analysis proves that the RocksDB WAL is a carefully balanced engine of tradeoffs. We have quantified the **Durability Barrier** (524x), demonstrated the power of **Amortized I/O** via batching (4.3x gain), and mapped the **Recovery Scaling** (O(N)) required for predictable availability.

---

## Credits
Built by **Sigma & Spark**: where B.Sc. Statistics meets Leveled Sparks 

**Srishti Lamba**: 202518003 | **Nikita Sharma**: 202518038
