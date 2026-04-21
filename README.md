# 💎 RocksDB WAL: Engineering Analysis & Instrumentation

[![View Code Changes](https://img.shields.io/badge/🛠️-View_Code_Changes_(Diff)-orange?style=for-the-badge)](https://github.com/srishti7103/rocksdb-WAL/compare/main...wal-experiments)
[![View Comparison Analysis](https://img.shields.io/badge/🔍-View_Comparison_Notebook-blue?style=for-the-badge)](./analytics/comparison.ipynb)
[![View Full Report](https://img.shields.io/badge/📄-View_Detailed_Report-green?style=for-the-badge)](./report.md)

This project reverse-engineers and instruments the **RocksDB Write-Ahead Log (WAL)** to analyze the fundamental tradeoffs between persistence guarantees and write throughput. Developed as part of the DS614 Systems Engineering curriculum.

---

## 🚦 Quick Access Links
- 🧪 **[Experiment Suite](./experiments/)**: C++ drivers for performance benchmarking.
- 📈 **[Analytics Dashboard](./analytics/comparison.ipynb)**: Data visualization and telemetry analysis.
- 📚 **[Metric Glossary](./stats.md)**: Definition of custom instrumentation points.
- 📑 **[System Report](./report.md)**: Academic write-up and failure analysis.

---

## 🏗 Project Folder Structure
Highlighted files (🔥) contain manual source code instrumentation.

```text
rocksdb-WAL/
├── 🔥 db/
│   ├── log_writer.cc          # Instrumentation: Record fragmentation (Exp 2 & 5)
│   ├── log_format.h           # Instrumentation: Block Size Config (Exp 2)
│   ├── write_thread.cc        # Instrumentation: Group Commit (Exp 4)
│   ├── db_impl/
│   │   ├── db_impl_write.cc   # Instrumentation: Sync modes & Batching (Exp 1 & 4)
│   │   └── db_impl_open.cc    # Instrumentation: Recovery Telemetry (Exp 3)
├── experiments/               # Standalone C++ benchmark drivers
│   ├── exp1_wal_throughput.cpp
│   ├── exp2_crash_recovery.cpp
│   └── ... (Exp 3-6)
├── analytics/                 # Python/Jupyter Data Analysis
│   ├── comparison.ipynb       # Deep-dive visualization
│   └── requirements.txt       # Python dependencies
├── results/                   # Raw telemetry data (.csv)
└── docs/                      # Scientific reports and images
```

---

## ⚙️ How to Run the Pipeline

### 1. Environment Setup
The build requires a Linux environment (Ubuntu/WSL recommended):
```bash
# Install dependencies
sudo apt-get install build-essential libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev
```

### 2. Building the Instrumented Core
```bash
# Build the static library
wsl make static_lib -j8
```

### 3. Running Experiments
```bash
cd experiments
# Compiles all drivers and generates wal_performance_telemetry.csv
wsl make run_all ROCKSDB_ROOT=..
```

---

## 🔍 Performance Insights & Experimental Results

### Exp 1: The Cost of Durability
Comparing standard ingestion vs. strict durability and No-WAL modes.
![Throughput Comparison](./docs/images/exp1_throughput.png)
> **Insight**: Disabling the WAL provides a ~2.8x speedup but risks total data loss in a crash. Strict `fsync()` per write drops performance by 300x.

### Exp 2: Recovery Scaling
How system recovery time (MTTR) scales with the volume of data in the WAL.
![Recovery Scaling](./docs/images/exp2_recovery.png)
> **Insight**: Recovery time is linear with WAL volume. We measured a jump from 19ms to 540ms as WAL size grew toward 1GB.

### Exp 5: Header Overhead Ratio
Analyzing the "Tax" of fixed-block logging.
![Efficiency Ratio](./docs/images/exp5_efficiency.png)
> **Insight**: Header overhead remains low (~2-5%) for standard payloads, but spikes when value sizes cross the 32KB block boundary.

---

## 📝 Code-Level Changes (Reference)

| Feature | Source File | Line | Implementation Detail |
|---|---|---|---|
| **Sync Tracking** | [db_impl_write.cc](./db/db_impl/db_impl_write.cc) | `2281` | Instrumented `s_wal_writes_synced` atomic counter. |
| **Fragmentation** | [log_writer.cc](./db/log_writer.cc) | `325` | Tracked `g_wal_fragment_records` in `EmitPhysicalRecord()`. |
| **Batch Grouping**| [write_thread.cc](./db/write_thread.cc) | `463` | Added `g_batch_group_count` in `EnterAsBatchGroupLeader()`. |
| **Recovery Telemetry**| [db_impl_open.cc](./db/db_impl/db_impl_open.cc) | `1132` | Added log reporting in `RecoverLogFiles()`. |

---

## 👥 Credits & Team
**Group**: `Sigma` & `Spark`
- **Lead Engineer**: Srishti
- **Research Team**: Sigma-Spark Group Members

---
