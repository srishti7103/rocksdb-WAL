# RocksDB Write-Ahead Log: Systems Engineering Audit
**Project Team:** Srishti Lamba (202518003) & Nikita Sharma (202518038)

---

## 1. Technical Foundation & Architecture
RocksDB is a high-performance, embeddable key-value store optimized for high-throughput environments. The **Write-Ahead Log (WAL)** is the primary mechanism for ensuring data persistence in a write-buffered architecture.

### Core Systems Principles
*   **LSM-Tree Storage Architecture:** By utilizing a Log-Structured Merge-Tree, the system transforms random application writes into sequential disk operations, significantly reducing write amplification.
*   **Atomic Crash Consistency:** The engine ensures that every record is protected by hardware-aligned headers and CRC-32 checksums, enabling reliable recovery even after catastrophic power failure.
*   **Sequential Data Ingestion:** High-speed throughput is maintained by an append-only logging strategy, allowing for O(1) write performance at the log level.
*   **Durable Persistence:** The WAL serves as the persistent source of truth, allowing volatile in-memory MemTables to be reconstructed during the recovery phase.

---

## 2. Architectural Design Decisions & Tradeoffs
We identified three fundamental design decisions that define the RocksDB WAL subsystem.

| Design Decision | Code Implementation | Problem Solved | System Tradeoff |
| :--- | :--- | :--- | :--- |
| **Durability Tiers** | `db_impl_write.cc` | Data loss prevention. | **Safety vs. Throughput:** Syncing every write ensures durability but incurs a **524x** performance floor. |
| **Group Commit** | `write_thread.cc` | Amortizing I/O latency. | **Throughput vs. Wait Time:** Batching improves total efficiency but introduces minor latency for concurrent threads. |
| **Block Alignment** | `log_writer.cc` | Hardware page optimization. | **Efficiency vs. Space:** Aligning to 32KB hardware pages optimizes I/O at the cost of a minor **0.1%** overhead. |

---

## 3. Instrumentation Methodology
To move beyond high-level observation, we modified the RocksDB source code to extract internal telemetry. The table below outlines the specific modifications made to the codebase:

| Instrumented File | Original Implementation | Modified Implementation | Lines Added (+) |
| :--- | :--- | :--- | :--- |
| **db/log_writer.cc** | Sequential record emission without internal size tracking. | Injected `fetch_add` logic into `AddRecord` to track payload vs metadata bytes. | **+19** |
| **db/write_thread.cc** | Group Commit queue management without batch-size exposure. | Captured `new_batch_size` within the leader-follower handoff for efficiency analysis. | **+10** |
| **db/db_impl/db_impl_write.cc** | Standard write entry point that ignored durability-tier categorization. | Added conditional hooks to count `options.sync` vs `buffered` write events. | **+10** |
| **db/db_impl/db_impl_open.cc** | Replayed WAL files during startup without performance measurement. | Wrapped the `ReplayWAL` loop in high-resolution nanosecond timers to calculate MTTR. | **+13** |
| **db/log_reader.cc** | Validated records without exposing corruption frequency to the engine. | Hooked into the checksum verification path to count and log data integrity failures. | **+5** |
| **db/log_format.h** | Hardcoded 32KB block alignment constant. | Converted the block size into a configurable macro to simulate fragmentation stress. | **+4** |

---

## 4. Experimental Evaluations

### Study 1: The "Safety Tax"
**Finding:** Mandatory `fsync` creates a **524x** performance floor compared to buffered writes.
**Conclusion:** Hardware latency is the primary bottleneck in strictly durable systems.

### Study 2: Storage Efficiency
**Finding:** Observed exactly **0.1%** metadata overhead from fixed-block alignment.
**Conclusion:** The space penalty for hardware-aligned I/O is negligible.

### Study 3: Recovery Reduction
**Finding:** Lenient recovery policies yield a **1.5x** reduction in MTTR.
**Conclusion:** Availability can be optimized by tuning consistency checks on the recovery critical path.

### Study 4: Group Commit Efficiency
**Finding:** Leader-follower batching delivers a **4.3x** throughput amplification at 8 threads.
**Conclusion:** Shared I/O costs (amortization) effectively overcome the sequential bottleneck of the log.

### Study 5: Recovery Scaling
**Finding:** Replay duration exhibits linear (O(N)) scaling with log volume.
**Conclusion:** Predictable MTTR requires aggressive log rotation to prevent unbounded recovery times.

---

## 5. Failure Analysis
1.  **Volume Scaling:** As log size increases, the startup critical path lengthens linearly, posing a risk to system availability SLAs.
2.  **Crash Integrity:** In the event of a power failure mid-write, the system detects "Torn Writes" via CRC-32 checksums in `log_reader.cc` and safely ignores corrupted tail fragments.

---

## 6. Synthesis
Our analysis of the RocksDB Write-Ahead Log has demonstrated that high-performance storage requires a precise balance of durability, concurrency, and alignment. By mapping code-level decisions to empirical data, we have quantified the engineering tradeoffs that allow RocksDB to serve as a high-fidelity foundation for modern data systems.
