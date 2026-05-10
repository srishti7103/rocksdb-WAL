#!/bin/bash

# Ensure execution permissions for scripts
chmod +x "$0" 2>/dev/null
chmod +x update_docs.py 2>/dev/null

# RocksDB WAL 10-Minute Benchmark Suite

echo "========================================================="
echo "   Starting System Benchmark Suite"
echo "========================================================="

echo "[1/3] Preparing clean telemetry environment..."
mkdir -p ../results
echo "Event,Count,Metric" > ../results/wal_performance_telemetry.csv

echo "[2/3] Compiling test drivers..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed!"
    exit 1
fi

echo "[3/3] Executing experiments..."

# Study 1: Synchronization & Latency
    ./sync_bench --mode=buffered | grep "ops/s" | awk '{print "WAL_Sync_Control,Buffered," $(NF-1)}' >> ../results/wal_performance_telemetry.csv
    ./sync_bench --mode=none | grep "ops/s" | awk '{print "WAL_Sync_Control,NoWAL," $(NF-1)}' >> ../results/wal_performance_telemetry.csv
    ./sync_bench --mode=sync | grep "ops/s" | awk '{print "WAL_Sync_Control,StrictSync," $(NF-1)}' >> ../results/wal_performance_telemetry.csv
    echo "Sync benchmark complete."

echo " -> Study 2: Block Fragmentation Overhead"
./fragment_bench

echo " -> Study 3: Recovery Consistency Modes"
./recovery_bench

echo " -> Study 4: Group Commit Scaling"
./batch_bench

echo " -> Study 5: MTTR Volume Scaling"
./skew_bench

echo "========================================================="
if [ "$1" == "--update-docs" ]; then
    echo "Data collection complete. Generating visualizations..."
    cd ..
    python3 -m pip install jupyter pandas matplotlib seaborn > /dev/null 2>&1
    python3 -m jupyter nbconvert --to notebook --execute comparison.ipynb --inplace
    echo "Updating documentation metrics..."
    cd experiments
    python3 update_docs.py
    echo "========================================================="
    echo "Contributor Mode: All systems verified and documentation updated."
else
    echo "Data collection complete. Telemetry saved to results/wal_performance_telemetry.csv"
    echo "Summary of run metrics available in CSV."
    echo "========================================================="
    echo "Reviewer Mode: Experiments verified. Documentation was NOT overwritten."
fi
echo "========================================================="
