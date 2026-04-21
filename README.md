# 💎 RocksDB WAL: Engineering Analysis & Instrumentation

[![View Code Changes](https://img.shields.io/badge/🛠️-View_Code_Changes_(Diff)-orange?style=for-the-badge)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)

This project reverse-engineers and instruments the **RocksDB Write-Ahead Log (WAL)** to analyze the fundamental tradeoffs between data persistence guarantees and write throughput.

## 📑 Key Project Documents
- 📊 **[System Engineering Report](./report.md)**: Detailed analysis of all 5 experiments, design tradeoffs, and failure scenarios.
- 🔥 **Instrumented Core**: Modified RocksDB source code (visible in the [Diff](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)) featuring atomic telemetry for performance tracking.

## 🏗 Project Folder Structure (Modified Files)
The forked repository contains instrumentation in the following core system files:
- `db/log_writer.cc`: Record fragmentation tracking.
- `db/log_format.h`: Configuration for block-level segmenting.
- `db/write_thread.cc`: Group commit logic instrumentation.
- `db/db_impl/db_impl_write.cc`: Sync-mode performance counters.
- `db/db_impl/db_impl_open.cc`: Recovery telemetry logging.

---
**Team**: Sigma & Spark | **Lead Engineer**: Srishti
**Course**: DS614 - Systems Engineering
