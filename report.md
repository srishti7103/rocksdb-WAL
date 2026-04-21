# 💎 RocksDB WAL: Engineering Analysis & Instrumentation

## 1. Executive Summary
This project reverse-engineers the Write-Ahead Log (WAL) within RocksDB to analyze the fundamental tradeoffs between data persistence and ingestion performance. Through custom instrumentation and five targeted experiments, we demonstrate how design decisions in the WAL layer directly impact system latency and reliability under stress.

---

## 2. Execution Path Analysis
To understand the system, we traced the write path from initial request to physical persistence.
- **Entry Point**: `DBImpl::Write()` handles the user request.
- **Path**: `WriteToWAL()` in `db_impl_write.cc:2258` is the critical junction. It determines whether the write is buffered in the OS cache or immediately synchronized to disk.
- **Instrumentation**: We injected atomic counters at these locations to track the ratio of `fsync()` calls to user writes.

---

## 3. Experimental Analysis (The 5 Key Studies)

## 3. Experimental Analysis (The 5 Core Studies)

### 3.1 Study 1: The "Safety Tax" (Durability vs. Throughput)
We analyzed the impact of WAL synchronization modes on write throughput.
- **Objective**: Quantify the performance penalty of `fsync()` on every write.
- **Method**: Execution of a uniform 1MB workload across three modes: `Strict Sync` (sync=true), `Buffered` (sync=false), and `No WAL`.
- **Results**:
    - **Strict Sync**: ~1.6k ops/s. Performance is bottlenecked by disk IOPS (latency of a single physical header seek).
    - **Buffered**: ~505k ops/s. Performance scales with CPU and OS page cache efficiency.
    - **No WAL**: ~1.4M ops/s. Baseline for pure memtable ingestion speed.
- **Engineering Insight**: Strict durability introduces a 300x-900x "Safety Tax". Most production systems use a "Buffered" approach or high-frequency periodic sync to strike a balance.

### 3.2 Study 2: Internal Fragmentation & Header Overhead
We modified `db/log_format.h` to vary the `ROCKSDB_WAL_BLOCK_SIZE` from 4KB to 64KB.
- **Objective**: Analyze the relationship between record size and fixed-block alignment.
- **Observation**: For large 128KB records, a 4KB block size forces 32 fragments per record.
- **Trace Reference**: Instrumented `EmitPhysicalRecord()` in `log_writer.cc` tracked `g_wal_fragment_records`.
- **Engineering Insight**: Smaller blocks increase the proportion of `g_wal_bytes_header` relative to `g_wal_bytes_payload`. Large record sizes benefit significantly from larger blocks to minimize fragmentation overhead.

### 3.3 Study 3: Recovery Mode Tradeoffs (WALRecoveryMode)
RocksDB offers four recovery modes: `kTolerateCorruptedTailRecords`, `kAbsoluteConsistency`, `kPointInTimeRecovery`, and `kSkipAnyCorruptedRecords`.
- **Objective**: Determine the impact of recovery strictness on Mean Time To Recovery (MTTR).
- **Method**: Simulating power loss by killing the process and re-opening with various modes.
- **Result**: `kAbsoluteConsistency` is the safest but slowest, as it performs a full parity/CRC32 check on every block before allowing the DB to open.
- **Instrumentation**: Tracked `g_crc_mismatch_count` in `log_reader.cc` during replay.

### 3.4 Study 4: Group Commit Efficiency (Batching)
We analyzed the `WriteThread` leadership logic to determine how well the WAL handles concurrent load.
- **Observation**: Throughput per thread increases as more threads are added, up to the saturation of the WAL writer.
- **Analysis**: The `EnterAsBatchGroupLeader()` logic in `write_thread.cc` batches concurrent writes into a single physical `AddRecord` call.
- **Metric**: Measured the "Group Size" using our custom atomic counters in `EnterAsBatchGroupLeader()`. Average group size was ~4.2 writers under high contention.

### 3.5 Study 5: Recovery Scaling & MTTR
We analyzed the time required to reconstruct the MemTable from WAL files of varying sizes.
- **Objective**: Measure the scalability of the recovery path.
- **Measurements**: 
    - 10k items (1 WAL): 22ms.
    - 1M items (50 WALs): 1.8 seconds.
- **Engineering Insight**: Recovery time scales linearly with WAL volume. While large WALs increase write throughput by reducing flushes, they significantly increase MTTR, potentially violating availability SLAs.

---

## 4. Source Code Instrumentation Map
The following files were modified to provide the telemetry for these studies:
- [log_writer.cc](file:///db/log_writer.cc): Fragmentation tracking and header overhead metrics.
- [db_impl_write.cc](file:///db/db_impl/db_impl_write.cc): Sync mode frequency and "Safety Tax" analysis.
- [write_thread.cc](file:///db/write_thread.cc): Group commit efficiency and batching metrics.
- [db_impl_open.cc](file:///db/db_impl/db_impl_open.cc): Recovery telemetry and timing.
- [log_reader.cc](file:///db/log_reader.cc): Checksum failure and corruption detection counters.

---

## 5. Credits
- **Team**: Sigma & Spark
- **Engineering Lead**: Srishti
- **Project**: DS614 Systems Engineering - RocksDB WAL Deep-Dive
