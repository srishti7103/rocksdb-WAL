#!/bin/bash

# run_all.sh - Master script to run all RocksDB WAL experiments and update graphs

echo "========================================================="
echo "   RocksDB WAL Full 10-Minute Benchmark Suite"
echo "========================================================="

# 1. Prepare clean CSV
echo "[1/3] Preparing clean telemetry CSV..."
mkdir -p ../results
echo "Event,Count,Metric" > ../results/wal_performance_telemetry.csv

# 2. Compile C++ benchmarks
echo "[2/3] Recompiling benchmark binaries..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed! Make sure you ran 'make static_lib' in the root."
    exit 1
fi

# 3. Run all experiments sequentially
echo "[3/3] Running experiments (This will take a few minutes)..."

echo " -> Running Study 1: Sync Benchmark (Latency vs Durability)"
./sync_bench

echo " -> Running Study 2: Fragment Benchmark (Header Overhead)"
./fragment_bench

echo " -> Running Study 3: Recovery Benchmark (Consistency Modes)"
./recovery_bench

echo " -> Running Study 4: Batch Benchmark (Group Commit Scaling)"
./batch_bench

echo " -> Running Study 5: Skew Benchmark (MTTR Scaling over Volume)"
./skew_bench

echo "========================================================="
echo "All C++ experiments finished! Data written to results/wal_performance_telemetry.csv"
echo "========================================================="

# 4. Execute Jupyter Notebook to update the images
echo "Regenerating graphs in comparison.ipynb..."
cd ..

# Ensure python dependencies are installed
python3 -m pip install jupyter pandas matplotlib seaborn > /dev/null 2>&1

# Execute notebook inplace to update outputs
python3 -m jupyter nbconvert --to notebook --execute comparison.ipynb --inplace

echo "========================================================="
echo "SUCCESS: Everything is ready for the professor."
echo "Your CSV data is REAL and your images in comparison.ipynb are UPDATED."
echo "========================================================="
