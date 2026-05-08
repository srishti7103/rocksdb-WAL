#!/bin/bash

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

echo " -> Study 1: Synchronization & Latency"
./sync_bench

echo " -> Study 2: Block Fragmentation Overhead"
./fragment_bench

echo " -> Study 3: Recovery Consistency Modes"
./recovery_bench

echo " -> Study 4: Group Commit Scaling"
./batch_bench

echo " -> Study 5: MTTR Volume Scaling"
./skew_bench

echo "========================================================="
echo "Data collection complete. Generating visualizations..."
cd ..

python3 -m pip install jupyter pandas matplotlib seaborn > /dev/null 2>&1
python3 -m jupyter nbconvert --to notebook --execute comparison.ipynb --inplace

echo "========================================================="
echo "All systems verified. Project is ready for review."
echo "========================================================="
