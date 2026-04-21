# RocksDB Write-Ahead Log: Engineering Analysis & Instrumentation

## 1. Executive Summary
This report presents a deep-dive analysis into the **Write-Ahead Log (WAL)** subsystem of RocksDB. As part of a Systems Engineering study, we instrumented the core ingestion and recovery paths to quantify the architectural tradeoffs between data durability and system performance. Through five targeted engineering studies, we analyze how synchronization policies, storage fragmentation, and concurrency mechanisms impact the efficiency of a high-performance key-value store.

---

## 2. Theoretical Foundation
The Write-Ahead Log is the primary durability mechanism in LSM-tree based engines. Every write operation is appended to the WAL before hitting the in-memory MemTable.
*   **Durability**: Ensures that in-memory data can be reconstructed following a crash.
*   **Performance Path**: The WAL must be high-throughput to avoid becoming a bottleneck for the MemTable.
*   **Tradeoff**: Synchronizing every WAL write to physical storage (`fsync`) provides safety but introduces significant I/O latency.

---

## 3. Engineering Studies & Findings

### Study 1: The "Safety Tax" (Sync vs. No-WAL)
We analyzed the raw performance cost of data persistence by toggling between memory-only and disk-persisted ingestion.

![Study 1 Chart](./docs/images/exp1_throughput.png)

**Methodology & Findings**:
By comparing a 1GB uniform workload, we found that **Strict Synchronization** (calling `fsync` on every write) reduces throughput by over 300x compared to buffered writes. The performance floor of ~1.6k ops/s corresponds directly to the physical seek latency of the underlying hardware. 
*   **No WAL**: 1.4M ops/s (CPU-bound)
*   **Buffered**: 505k ops/s (OS Page-Cache bound)
*   **Strict Sync**: 1.6k ops/s (I/O-Latency bound)

**Code Instrumentation**:
- **File**: `db/db_impl/db_impl_write.cc`
- **Location**: Inside `DBImpl::WriteToWAL` (Line 2264-2272).
- **Implementation**: We injected atomic counters `s_wal_writes_synced` and `s_wal_writes_notsync` to track the frequency of durable vs. buffered commits.

---

### Study 2: Space Utilization & Fragmentation
This study explored the relationship between block-level alignment and storage overhead.

![Study 2 Chart](./docs/images/exp2_fragmentation.png)

**Methodology & Findings**:
RocksDB partitions the WAL into fixed-size blocks (default 32KB). If a record spans multiple blocks, it is fragmented into "First," "Middle," and "Last" segments. We varied the block size and found that smaller sizes significantly increase the ratio of header-bytes to payload-bytes, decreasing effective storage capacity.

**Code Instrumentation**:
- **File**: `db/log_writer.cc`
- **Location**: `Writer::EmitPhysicalRecord` (Line 320-380).
- **Implementation**: Instrumented the emission path with `g_wal_bytes_header` (Line 359) and `g_wal_bytes_payload` (Line 329) to calculate the real-time efficiency of the WAL.

---

### Study 3: Recovery Mode Performance (MTTR)
We analyzed how different consistency policies impact the Mean Time To Recovery.

![Study 3 Chart](./docs/images/exp3_recovery_mode.png)

**Methodology & Findings**:
Using four modes of `WALRecoveryMode`, we simulated crash recovery. We found that `kAbsoluteConsistency` adds a ~35% overhead to startup time because it performs a full CRC32 validation of every record in the log before allowing the DB to open.

**Code Instrumentation**:
- **File**: `db/log_reader.cc`
- **Location**: `Reader::ReadPhysicalRecord` (Line 326-329).
- **Implementation**: Injected `g_crc_mismatch_count` within the `kBadRecordChecksum` case to track corruption detection efficiency across various recovery strictness levels.

---

### Study 4: Group Commit Scaling
This study explored RocksDB's primary concurrency optimization for the WAL.

![Study 4 Chart](./docs/images/exp4_group_commit.png)

**Methodology & Findings**:
RocksDB uses a leader-follower batching mechanism (`WriteThread`). We found that increasing concurrent writers does not linearly increase latency, because multiple writes are batched into a single WAL sync call. Our data shows throughput amplifies significantly up to 8 threads before the disk I/O saturates.

**Code Instrumentation**:
- **File**: `db/write_thread.cc`
- **Location**: `WriteThread::EnterAsBatchGroupLeader` (Line 440-579).
- **Implementation**: Instrumented the grouping logic with `g_batch_group_writers` (Line 577) to track the average size of atomic commits during heavy contention.

---

### Study 5: Recovery Volume Scaling
We quantified the scalability of the WAL replayer as the total volume of uncompressed data increases.

![Study 5 Chart](./docs/images/exp5_scaling.png)

**Methodology & Findings**:
The MTTR increases linearly ($O(n)$) with the volume of log data. While large WALs improve write performance by reducing "Flush" counts, they create a massive availability risk if the system crashes with a multi-GB log.

**Code Instrumentation**:
- **File**: `db/db_impl/db_impl_open.cc`
- **Location**: `DBImpl::ProcessLogFile` (Line 1237-1373).
- **Implementation**: Integrated telemetry to time the reconstruction process of the MemTable from historical `.log` segments.

---

## 4. Conclusion
The WAL is a delicate balance of safety and speed. Our results demonstrate that while strict durability provides the highest level of data protection, it requires highly optimized batching (Study 4) and careful management of WAL volume (Study 5) to maintain professional-grade throughput and availability in a modern storage engine.

---
**Author**: Srishti
**Project**: DS614 Systems Engineering (RocksDB Deep-Dive)
