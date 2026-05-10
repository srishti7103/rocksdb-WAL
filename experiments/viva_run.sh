#!/bin/bash
# VIVA_RUN.SH - CLEAN VERSION

echo "========================================================="
echo "   Starting Clean System Benchmark Suite"
echo "========================================================="

# 1. Prepare Env
rm -f ../results/telemetry.csv
mkdir -p ../results
echo "Event,Count,Metric" > ../results/telemetry.csv

# 2. Compile
make sync_bench fragment_bench recovery_bench batch_bench skew_bench -j$(nproc)

# 3. Run Experiments (Clean & Single)
echo "[1/5] Study 1: Sync..."
B=$(./sync_bench --mode=buffered | grep "ops/s" | awk '{print $(NF-1)}')
N=$(./sync_bench --mode=none | grep "ops/s" | awk '{print $(NF-1)}')
S=$(./sync_bench --mode=sync | grep "ops/s" | awk '{print $(NF-1)}')
echo "WAL_Sync_Control,Buffered,$B" >> ../results/telemetry.csv
echo "WAL_Sync_Control,NoWAL,$N" >> ../results/telemetry.csv
echo "WAL_Sync_Control,StrictSync,$S" >> ../results/telemetry.csv
echo " -> Buffered: $B | No-WAL: $N | Sync: $S"

echo "[2/5] Study 2: Fragmentation..."
./fragment_bench | tee /dev/tty
echo "[3/5] Study 3: Recovery..."
./recovery_bench | tee /dev/tty
echo "[4/5] Study 4: Group Commit..."
./batch_bench | tee /dev/tty
echo "[5/5] Study 5: Skew..."
./skew_bench | tee /dev/tty

echo "========================================================="
echo "Updating documentation..."
jupyter nbconvert --to notebook --execute ../comparison.ipynb --inplace || true
python3 viva_update.py
echo "========================================================="
echo "Project ready for review."
