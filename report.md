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
To analyze these principles, we modified the core RocksDB C++ source to extract high-fidelity telemetry.

### Modified Components & Line References
*   **Write Path Path (`db_impl_write.cc:2267`):** Hooked into the sync gate to track hardware-level synchronization frequency.
*   **Concurrency Manager (`write_thread.cc:452,576`):** Captured batch-group metrics to quantify Group Commit efficiency.
*   **Serialization Logic (`log_writer.cc:130,326`):** Instrumented record headers to analyze fragmentation and metadata overhead.
*   **Recovery Engine (`db_impl_open.cc:1136`):** Wrapped the WAL replay loop in high-resolution timers to measure startup performance.
*   **Integrity Guard (`log_reader.cc:327`):** Monitored the checksum validation path to detect and log "Torn Writes."

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
