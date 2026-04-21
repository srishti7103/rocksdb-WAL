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

### 3.1 Experiment 1: The Cost of Durability (Sync vs. Buffered)
We compared **Buffered WAL** (default), **No WAL**, and **Strict Sync**.
- **Results**:
    - **Strict Sync**: 1,647 ops/s (1 fsync per write).
    - **Buffered**: 505,603 ops/s (OS manages persistence).
    - **No WAL**: 1,416,893 ops/s (Maximum speed, zero durability).
- **Insight**: Strict durability introduces a 300x performance penalty. Group commit is essential for production systems to amortize this cost.

### 3.2 Experiment 2: Crash Recovery Integrity
We simulated a catastrophic failure by deleting all SST (Persistent Table) files while leaving the WAL intact.
- **Observation**: RocksDB successfully recovered 100% of the data (5,000/5,000 keys) by replaying the WAL via `RecoverLogFiles()` (`db_impl_open.cc:1132`).
- **Significance**: This validates the WAL as the authoritative source of truth for the "Last Mile" of data persistence before compaction.

### 3.3 Experiment 3: Failure Under Skew (Fragmentation)
Analyzing how large value sizes impact WAL efficiency.
- **Observation**: Latency increased by 15x when value sizes exceeded the 32KB block boundary defined in `log_format.h:54`.
- **Reasoning**: Records are fragmented across multiple blocks, requiring multiple headers and increasing the checksum verification overhead during recovery.

### 3.4 Experiment 4: Group Commit Efficiency
Comparing 1 writer vs. 16 concurrent writers.
- **Results**: Overall throughput improved as the number of threads increased.
- **Analysis**: The `EnterAsBatchGroupLeader()` logic in `write_thread.cc:1196` allows a single thread to "group" multiple concurrent writes into one physical WAL append, reducing the total number of I/O operations per record.

### 3.5 Experiment 5: Recovery Scaling (MTTR)
We measured the time to recover the database as the WAL volume increased.
- **Measurements**:
    - 10k ops: 19ms recovery.
    - 500k ops: 540ms recovery.
- **Tradeoff**: Larger WAL files provide better write performance by deferring compactions, but increase the Mean Time To Recovery (MTTR), violating availability SLAs after a crash.

---

## 4. Design Decisions & Tradeoffs
1. **Append-only Structure**: Simplifies recovery and ensures high sequential write speed, but leads to storage growth until compaction.
2. **Fixed 32KB Blocking**: Simplifies checksumming and read-ahead in `log_reader.cc`, but causes internal fragmentation for large values.
3. **Group Commit Leader**: Reduces the "Sync Tax" by batching concurrent writes, though it introduces a small latency floor for individual writers.

---

## 5. Credits
**Team**: Sigma & Spark
**Lead Engineer**: Srishti
**Course**: DS614 - Systems Engineering
