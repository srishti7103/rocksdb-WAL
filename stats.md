# 📈 RocksDB WAL Telemetry: Metric Glossary

This document defines the custom instrumentation points added to the RocksDB core for the DS614 WAL Analysis experiments.

## 1. Physical Logging Metrics
These metrics track the physical layout of the WAL on storage.

### `g_wal_full_records`
- **Location**: `db/log_writer.cc:23`
- **Description**: Count of records that fit completely within a single 32KB WAL block.
- **Role**: Measures "ideal" write efficiency. High counts indicate small value sizes relative to block size.

### `g_wal_fragment_records`
- **Location**: `db/log_writer.cc:24`
- **Description**: Count of fragmented records (`kFirst`, `kMiddle`, `kLast`).
- **Role**: Tracks I/O amplification. High counts indicate records crossing block boundaries.

### `g_wal_bytes_payload`
- **Location**: `db/log_writer.cc:25`
- **Description**: Accumulator for raw user data bytes (Keys + Values).
- **Role**: Foundation for calculating **Goodput vs Throughput**.

### `g_wal_bytes_header`
- **Location**: `db/log_writer.cc:26`
- **Description**: Accumulator for WAL header overhead (7 bytes per fragment).
- **Role**: Quantifies the "tax" of fixed-block logging.

---

## 2. Ingestion Path Metrics
These metrics track the logic in the write path.

### `s_wal_writes_synced`
- **Location**: `db/db_impl/db_impl_write.cc:2281`
- **Description**: Count of total writes where `WriteOptions.sync = true`.
- **Role**: Identifies "Strict Durability" operations.

### `s_wal_writes_notsync`
- **Location**: `db/db_impl/db_impl_write.cc:2282`
- **Description**: Count of total writes using buffered (non-sync) mode.
- **Role**: Measures "Relaxed Durability" operations.

---

## 3. Concurrency (Group Commit) Metrics
Tracked during the batch grouping phase of the write thread.

### `g_batch_group_count`
- **Location**: `db/write_thread.cc:463`
- **Description**: Total number of merged batches formed.
- **Role**: Measures the effectiveness of the Group Commit algorithm.

### `g_batch_group_total_size`
- **Location**: `db/write_thread.cc:464`
- **Description**: Total bytes processed across all merged groups.
- **Role**: Used to calculate average "Batch Density".

---

## 4. Recovery & Integrity Metrics
Used during system startup and log replay.

### `g_crc_mismatch_count`
- **Location**: `db/log_reader.cc:349`
- **Description**: Total checksum failures encountered during recovery.
- **Role**: Critical indicator of "Lost Writes" or partial persistence failures.
