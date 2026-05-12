# RocksDB Write-Ahead Log (WAL) Analysis and Instrumentation

[![View Code Changes](https://img.shields.io/badge/View_Code_Changes_(GitHub_Diff)-orange?style=for-the-badge&logo=github)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)

This project provides a deep-dive instrumentation and systems engineering analysis of the RocksDB Write-Ahead Log (WAL). Developed by **Sigma & Spark**, this work explores the fundamental tradeoffs between data durability, ingestion latency, and recovery efficiency in high-performance storage engines.

---

## 1. Technical Foundation & Architecture

### Architectural Overview
RocksDB is optimized for fast storage environments. Our analysis maps the system to five core Big Data Engineering concepts:

1.  **LSM-Tree Storage Core (Storage Architecture):** RocksDB utilizes a Log-Structured Merge-Tree to transform random writes into sequential I/O by buffering updates in volatile memory (MemTable) before persisting them as immutable sorted files (SSTs).
2.  **Write-Optimized Data Ingestion (Ingestion/Streaming):** Throughput is prioritized via append-only logging, deferring expensive compaction tasks to maintain high ingestion rates with minimal stall.
3.  **Append-Only Durability (Reliability):** To ensure zero data loss during system failures, every transaction is recorded in a persistent Write-Ahead Log (WAL) before being applied to memory.
4.  **Atomic Crash Consistency (Fault Tolerance):** Every log record is protected by CRC-32 checksums and fragmented record types to detect and mitigate "Torn Writes," ensuring the engine never recovers into an inconsistent state.
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
The following table contrasts the original implementation with our instrumented logic:

| Instrumented File | Original Implementation | Modified Implementation | Lines Added (+) |
| :--- | :--- | :--- | :--- |
| **db/log_writer.cc** | Sequential record emission without internal size tracking. | Injected `fetch_add` logic into `AddRecord` to track payload vs metadata bytes. | **+19** |
| **db/log_writer.h** | Standard header without telemetry exposure. | Declared external atomic counters for cross-module fragmentation tracking. | **+13** |
| **db/write_thread.cc** | Group Commit queue management without batch-size exposure. | Captured `new_batch_size` within the leader-follower handoff for efficiency analysis. | **+10** |
| **db/write_thread.h** | Standard header for write thread management. | Injected batch-size and throughput tracking metrics into the thread state. | **+19** |
| **db/db_impl/db_impl_write.cc** | Standard write entry point that ignored durability-tier categorization. | Added conditional hooks to count `options.sync` vs `buffered` write events. | **+10** |
| **db/db_impl/db_impl_open.cc** | Replayed WAL files during startup without performance measurement. | Wrapped the `ReplayWAL` loop in high-resolution nanosecond timers to calculate MTTR. | **+13** |
| **db/log_reader.cc** | Validated records without exposing corruption frequency to the engine. | Hooked into the checksum validation path to count and log data integrity failures. | **+5** |
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

## 7. Experimental Evaluation: Hypothesis vs. Reality

### Study 1: The "Safety Tax" (Durability vs. Throughput)
*   **Rationale:** In systems engineering, the "boundary crossing" (User Space to Kernel to Hardware) is the most expensive operation. We conducted this to quantify the exact throughput "cliff" encountered when moving from OS-buffered writes to hardware-synchronized writes.
*   **Instrumentation:** We modified `db/db_impl/db_impl_write.cc` to inject atomic counters that categorize write events. This allowed us to isolate the latency of the `fsync()` system call.
*   **Hypothesis:** Mandatory hardware synchronization will cause an exponential drop in throughput as the bottleneck shifts from RAM to Disk.
*   **Result:** Strict synchronization introduces a <!-- DYNAMIC:SYNC_TAX -->**524x**<!-- END_DYNAMIC --> performance floor.
<div align="center">
  <img src="./docs/images/exp1_throughput.png" width="400" />
</div>

### Study 2: Storage Efficiency (Fragmentation)
*   **Rationale:** High-performance storage engines must align writes to hardware sectors (typically 4KB/32KB). We analyzed this to determine if this alignment causes significant "slack space" (fragmentation) that wastes disk capacity.
*   **Instrumentation:** We modified `db/log_writer.cc` to track `payload_bytes` vs. `header_bytes` across 32KB block boundaries using `std::atomic` fetch-and-add logic.
*   **Hypothesis:** Maintaining fixed 32KB alignment will introduce a metadata overhead proportional to record frequency.
*   **Result:** Fixed-block design introduces exactly <!-- DYNAMIC:FRAG_PCT -->**0.1%**<!-- END_DYNAMIC --> internal fragmentation, proving the design is highly space-efficient.
<div align="center">
  <img src="./docs/images/exp2_fragmentation.png" width="400" />
</div>

### Study 3: Recovery Reduction (MTTR Analysis)
*   **Rationale:** Availability is measured by MTTR (Mean Time To Recovery). We tested different recovery modes to identify the "knob" that allows a system to come back online fastest after a crash.
*   **Instrumentation:** We hooked into `db/db_impl/db_impl_open.cc` with high-resolution nanosecond timers and used `db/log_reader.cc` to track CRC-32 checksum validation overhead.
*   **Hypothesis:** Lenient consistency checks during startup will significantly reduce MTTR without compromising existing data.
*   **Result:** `TolerateCorruptedTailRecords` yields a <!-- DYNAMIC:RECOVERY_REDUCTION -->**1.5x**<!-- END_DYNAMIC --> reduction in MTTR.
<div align="center">
  <img src="./docs/images/exp3_recovery_mode.png" width="400" />
</div>

### Study 4: Group Commit Efficiency (Batching)
*   **Rationale:** To solve the `fsync` bottleneck, RocksDB uses a "Leader-Follower" pattern. We performed this study to measure how effectively the system amortizes I/O costs as concurrent thread pressure increases.
*   **Instrumentation:** We captured the `new_batch_size` within `db/write_thread.cc` during the handoff phase, allowing us to correlate batch density with aggregate throughput.
*   **Hypothesis:** Throughput will scale non-linearly with concurrency as multiple threads are batched into a single physical I/O operation.
*   **Result:** Leader-follower batching delivers a <!-- DYNAMIC:GROUP_COMMIT -->**4.3x**<!-- END_DYNAMIC --> throughput amplification at high concurrency.
<div align="center">
  <img src="./docs/images/exp4_group_commit.png" width="400" />
</div>

### Study 5: Recovery Scaling (Volume Analysis)
*   **Rationale:** Systems must have predictable scaling. We conducted this to verify that recovery time remains linear ($O(N)$) and does not degrade exponentially as the Write-Ahead Log grows.
*   **Instrumentation:** We utilized the timers in `db/db_impl/db_impl_open.cc` to map total WAL replay duration against the raw volume of uncompressed log data.
*   **Hypothesis:** Recovery time will exhibit a linear relationship with the volume of data stored in the WAL.
*   **Result:** Replay duration exhibits strict **Proportional Scaling (O(N))**, confirming predictable recovery windows.
<div align="center">
  <img src="./docs/images/exp5_scaling.png" width="400" />
</div>

---

## 8. Failure Analysis & System Resilience

### 1. What happens when data size increases significantly?
MTTR grows linearly ($O(N)$). Without log rotation, a 10x increase in log volume leads to a 10x increase in recovery time.

### 2. What happens under skew?
Write skew (high thread contention) triggers the **Group Commit** mechanism, making the system *more* efficient as batch sizes grow larger (4.3x gain).

### 3. What happens if a component fails?
Power loss causes **Torn Writes**. RocksDB detects this via **CRC-32 Checksums** in `log_reader.cc` and discards corrupted tail fragments.

### 4. What assumptions does the system rely on?
The system assumes **Storage Honesty** (that `fsync` actually persists data) and **Sequential Advantage** (that logs are faster than random updates).

---

## 9. Conclusion: Engineering the Tradeoff Curve

Our analysis proves that the RocksDB WAL is a carefully balanced engine of tradeoffs. We have quantified the **Durability Barrier** (524x), demonstrated the power of **Amortized I/O** via batching (4.3x gain), and mapped the **Recovery Scaling** (O(N)) required for predictable availability.

---

## Credits
Built by **Sigma & Spark**: where B.Sc. Statistics meets Leveled Sparks 

**Srishti Lamba**: 202518003 | **Nikita Sharma**: 202518038
