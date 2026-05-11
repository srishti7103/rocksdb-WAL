# RocksDB Write-Ahead Log: Systems Engineering Audit
**Project Team:** Srishti Lamba (202518003) & Nikita Sharma (202518038)

---

## 1. System Overview: The Problem & Solution
RocksDB is an LSM-tree based storage engine designed for high-performance workloads. The **Write-Ahead Log (WAL)** is the primary mechanism used to solve the **Durability Problem** in ACID transactions. Since the primary write target is the volatile MemTable, the WAL provides a persistent, append-only record of operations that can be replayed after a crash.

### Concept Mapping (Rubric Alignment)
1.  **Storage Architecture:** LSM-tree design (MemTable buffering vs. SST persistence).
2.  **Reliability:** Use of checksums (CRC-32) for fault tolerance against torn writes.
3.  **Data Ingestion:** High-throughput sequential I/O via append-only logging.
4.  **Availability:** Mean Time To Recovery (MTTR) as a function of log replay efficiency.

---

## 2. Key Design Decisions & Tradeoffs
We identified and analyzed three fundamental design decisions within the RocksDB WAL subsystem.

| Design Decision | Code Implementation | Problem Solved | System Tradeoff |
| :--- | :--- | :--- | :--- |
| **Durability Tiers** | `db_impl_write.cc` | Data loss prevention during power failure. | **Durability vs. Throughput:** Syncing every write ensures safety but incurs a **524x** performance penalty. |
| **Group Commit** | `write_thread.cc` | Amortizing I/O costs across threads. | **Throughput vs. Latency:** Batching multiple writes into one sync improves total speed but adds wait-time for individual followers. |
| **Block Alignment** | `log_writer.cc` | Optimizing for hardware page boundaries. | **Hardware Efficiency vs. Space:** Aligning to 32KB blocks reduces write amplification but causes **0.1%** internal fragmentation. |

---

## 3. Instrumentation Strategy (Deep Dive)
To move beyond high-level observation, we modified the RocksDB source code to extract internal telemetry.

### Modified Components & Line References
*   **Write Path (`db_impl_write.cc:2267`):** Instrumented the sync-policy gate to track frequency and timing of hardware flushes.
*   **Batching Logic (`write_thread.cc:452,576`):** Captured batch-group sizes to calculate the efficiency of the Leader-Follower pattern.
*   **Serialization (`log_writer.cc:130,326`):** Tracked bytes emitted per record to measure metadata overhead vs. actual payload.
*   **Recovery Path (`db_impl_open.cc:1136`):** Wrapped the WAL replay loop in high-resolution timers to measure MTTR under stress.
*   **Integrity Guard (`log_reader.cc:327`):** Hooked into the checksum validator to observe torn-write detection.

---

## 4. Experimental Evaluations

### Study 1: The "Safety Tax"
**Hypothesis:** Mandatory `fsync` will cause an exponential drop in throughput as the bottleneck moves from CPU to Disk I/O.
**Result:** Enabling strict sync introduces a **524x** performance floor. 
**Conclusion:** The physical latency of non-volatile storage is the absolute governor of write performance.

### Study 2: Storage Efficiency
**Hypothesis:** Fixed-block alignment will cause measurable internal fragmentation.
**Result:** Observed exactly **0.1%** metadata overhead.
**Conclusion:** Space inefficiency is negligible compared to the performance benefit of hardware-page alignment.

### Study 3: Recovery Reduction
**Hypothesis:** Lenient consistency modes will reduce MTTR by skipping tail validation.
**Result:** Tolerate mode yields a **1.5x** recovery speedup.
**Conclusion:** Reducing work on the startup critical path is a key lever for system availability.

### Study 4: Group Commit Efficiency
**Hypothesis:** Throughput will scale super-linearly with concurrency due to batching.
**Result:** **4.3x** amplification at 8 threads.
**Conclusion:** Leader-follower patterns effectively amortize the serial bottleneck of the WAL.

### Study 5: Recovery Scaling
**Hypothesis:** Recovery time is a linear function (O(N)) of total WAL volume.
**Result:** Proportional linear scaling verified.
**Conclusion:** Unbounded WAL growth poses a direct risk to system availability (SLA).

---

## 5. Failure Analysis
1.  **What happens when data size increases?** MTTR grows linearly (O(N)). Without aggressive WAL rotation, recovery times can exceed SLA targets.
2.  **What happens if a component fails mid-write?** The system experiences a **Torn Write**. Our instrumentation of `log_reader.cc` shows that RocksDB uses CRC-32 checksums to detect this and safely discards corrupted tail fragments, ensuring the database state remains consistent.

---

## 6. Final Insights
The RocksDB Write-Ahead Log is a precision-engineered tradeoff engine. By mapping code-level decisions to empirical performance data, we have demonstrated how sequential I/O, batching, and checksumming work together to solve the fundamental problem of persistent, high-performance data storage.
