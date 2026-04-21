# DS614 Project Report: RocksDB Write-Ahead Log (WAL) Deep-Dive
**Branch**: `wal-experiments` | **Telemetry Suite**: Enabled

---

## 1. System Overview: The Problem & Solution
**RocksDB** is a high-performance LSM-tree storage engine. The **Write-Ahead Log (WAL)** solves the fundamental problem of **Atomicity and Durability** (the 'A' and 'D' in ACID). Without a WAL, a crash would result in the loss of all data currently in the volatile MemTable.

---

## 2. Execution Path: The "Write-Through" Ingestion
To understand the system, we traced one complete execution path—from a user's `Put()` call to a persistent byte on disk.

### Path Trace: Data Ingestion → WAL Persistence
1. **Entry Point**: `DBImpl::Write` (in [db_impl_write.cc](file:///c:/Users/srrml/Desktop/SEMESTER%202/PROJECT/BDE/Again/rocksdb-WAL/db/db_impl/db_impl_write.cc)) organizes incoming writes into a `WriteThread`.
2. **Concurrency Management**: `WriteThread::JoinBatchGroup()` merges multiple concurrent writers into one "Group Leader".
3. **Internal Handoff**: The leader calls `WriteToWAL()` (line 2258 in [db_impl_write.cc](file:///c:/Users/srrml/Desktop/SEMESTER%202/PROJECT/BDE/Again/rocksdb-WAL/db/db_impl/db_impl_write.cc)).
4. **Physical Formatting**: `Log::Writer::AddRecord()` (in [log_writer.cc](file:///c:/Users/srrml/Desktop/SEMESTER%202/PROJECT/BDE/Again/rocksdb-WAL/db/log_writer.cc)) fragments the user data into physical record types (`kFullType`, `kFirstType`, etc.) to fit within the 32KB WAL blocks.
5. **Disk Commit**: `WritableFileWriter::Append()` writes the formatted record, often followed by an `fsync()` command to ensure the bits are physically on the platter/NAND.

---

## 3. Key Design Decisions
We analyzed three architectural choices that define the RocksDB WAL.

### Decision 1: Append-Only Sequence (LSM Buffer)
- **Implementation**: `Log::Writer::EmitPhysicalRecord()`
- **Problem**: Random I/O is slow.
- **Tradeoff**: By making the WAL append-only, RocksDB achieves sequential I/O performance. The tradeoff is that updates are never "in-place"—the WAL grows until a "Flush" (Memtable → SST) occurs, necessitating **WAL Rotation** and careful **Space Management**.

### Decision 2: The "Group Commit" Algorithm
- **Implementation**: `WriteThread::EnterAsBatchGroupLeader()` in [write_thread.cc](file:///c:/Users/srrml/Desktop/SEMESTER%202/PROJECT/BDE/Again/rocksdb-WAL/db/write_thread.cc)
- **Problem**: If every thread called `fsync()` individually, performance would drop to ~100-200 ops/s.
- **Tradeoff**: RocksDB merges concurrent writes into one I/O operation. This vastly improves **Throughput**, but introduces **Latency Jitter** for individual writers who must wait for the group leader.

### Decision 3: Block-Based Data Segmentation
- **Implementation**: `kBlockSize = 32768` in [log_format.h](file:///c:/Users/srrml/Desktop/SEMESTER%202/PROJECT/BDE/Again/rocksdb-WAL/db/log_format.h)
- **Problem**: Managing arbitrarily large records in a continuous stream.
- **Tradeoff**: RocksDB splits records into 32KB blocks for predictive reading and recovery. This simplifies **Checksum Verification** but causes **Payload Fragmentation** for records larger than 32KB, as tracked in our Experiment 2.

### 3.1 Experiment 1: The Cost of Durability
Comparing **WAL ON (Buffered)** vs **WAL OFF** vs **WAL SYNC**.
- **Observation**:
    - **Buffered (Default)**: ~505,000 ops/s.
    - **WAL Disabled**: ~1,416,000 ops/s (2.8x speedup).
    - **Strict Sync**: ~1,600 ops/s (Massive overhead for safety).
- **Analysis**: Disabling the WAL improves throughput significantly by eliminating the first I/O write path, but leaves the system vulnerable to total data loss in a crash.
- **Trace**: `db_impl_write.cc:2258` (WriteToWAL call).

### 3.3 Experiment 5: Data Growth & Recovery Scaling
- **Observation**: Recovery time scales linearly with WAL volume. We measured **19ms for 10k ops** vs. **540ms for 500k ops**.
- **Risk**: As WAL files grow, the "Mean Time To Recovery" (MTTR) increases, potentially exceeding SLAs.
- **Solution**: RocksDB uses `max_total_wal_size` (`db_impl_write.cc:2111`) to trigger `SwitchWAL()`, ensuring WAL files are purged regularly after data is safely persisted in SST files.

---

## 4. Failure Analysis (Mandatory Segment)
As per the systems engineering rubric, we analyzed the system's behavior under two critical failure conditions.

### Scenario A: What happens when data size increases significantly?
When record sizes exceed the `kBlockSize` (32KB), the system enters a "Fragmentation Loop":
1. The record is split by `Log::Writer::AddRecord()`.
2. Each fragment incurs a new 7-byte header (`kHeaderSize`).
3. **Outcome**: Write throughput drops due to increased syscalls and CPU overhead for checksumming multiple fragments. Our Exp-2 instrumentation identifies exactly how many "First", "Middle", and "Last" fragments were created per GB.

### Scenario B: What assumptions does this system rely on?
The RocksDB WAL relies on the **"Ordered Write Assumption"**:
1. It assumes the underlying filesystem/hardware honors `fsync()` ordering.
2. **Failure Mode**: If the hardware reports a write as persisted but it's still in a volatile disk-cache, a power loss will cause "Log Corruption".
3. **Mitigation**: We instrumented `g_crc_mismatch_count` in [log_reader.cc](file:///c:/Users/srrml/Desktop/SEMESTER%202/PROJECT/BDE/Again/rocksdb-WAL/db/log_reader.cc) to identify exactly when these assumptions fail during our crash-simulation tests (Exp-3).

---

## 5. Concept Mapping (DS614 Syllabus)
The RocksDB WAL directly implements four core class concepts:
1. **LSM-tree Storage**: The WAL serves as the persistent counterpart to the volatile MemTable, allowing for high-performance ingestion.
2. **Crash Recovery (Fault Tolerance)**: Using `kPointInTimeRecovery` to rebuild state from an incomplete log.
3. **Write Amplification (Performance)**: The balance between log data volume and background compaction I/O.
4. **Consistency Guarantees**: Using `Sync` vs. `Non-Sync` modes to define the user-facing durability SLA.

---

## 5. Mandatory Experiment: The "Visibility" Modification
**The Change**: We injected atomic telemetry counters into the "Hot Path" of `log_writer.cc` and `db_impl_write.cc`. 
**The Goal**: Ordinarily, these metrics (fragmentation count, header overhead, recovery latency) are hidden. Our modification allows us to see the **Internal System Behavior** under various stress conditions.

*(Note: Experimental results and visualization graphs are automatically generated in the accompanying `comparison.ipynb` and `results/` folder.)*
