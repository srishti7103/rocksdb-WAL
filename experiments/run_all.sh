#!/bin/bash
set -e

# Configuration
UPDATE_DOCS=false
if [[ "$1" == "--update-docs" ]]; then
    UPDATE_DOCS=true
fi

prepare_env() {
    echo "[1/3] Preparing clean telemetry environment..."
    rm -f ../results/wal_performance_telemetry.csv
    mkdir -p ../results
    echo "Event,Count,Metric" > ../results/wal_performance_telemetry.csv
}

compile_drivers() {
    echo "[2/3] Compiling test drivers..."
    # Logic to compile benchmarks if needed
    make sync_bench fragment_bench recovery_bench batch_bench skew_bench -j$(nproc)
}

run_benchmarks() {
    echo "[3/3] Executing experiments..."

    echo " -> Study 1: Synchronization & Latency"
    # Capture numeric value (penultimate field before 'ops/s')
    ./sync_bench --mode=buffered | tee /dev/tty | grep "ops/s" | awk '{print "WAL_Sync_Control,Buffered," $(NF-1)}' >> ../results/wal_performance_telemetry.csv
    ./sync_bench --mode=none | tee /dev/tty | grep "ops/s" | awk '{print "WAL_Sync_Control,NoWAL," $(NF-1)}' >> ../results/wal_performance_telemetry.csv
    ./sync_bench --mode=sync | tee /dev/tty | grep "ops/s" | awk '{print "WAL_Sync_Control,StrictSync," $(NF-1)}' >> ../results/wal_performance_telemetry.csv
    echo "Sync benchmark complete."

    echo " -> Study 2: Block Fragmentation Overhead"
    ./fragment_bench | tee /dev/tty
    echo "Fragment benchmark complete."

    echo " -> Study 3: Recovery Consistency Modes"
    ./recovery_bench | tee /dev/tty
    echo "Recovery benchmark complete."

    echo " -> Study 4: Group Commit Scaling"
    ./batch_bench | tee /dev/tty
    echo "Batch benchmark complete."

    echo " -> Study 5: MTTR Volume Scaling"
    ./skew_bench | tee /dev/tty
    echo "Skew benchmark complete."
}

generate_docs() {
    echo "========================================================="
    echo "Data collection complete. Generating visualizations..."
    
    # Run Jupyter to regenerate plots
    jupyter nbconvert --to notebook --execute comparison.ipynb
    
    if [ "$UPDATE_DOCS" = true ]; then
        echo "Updating documentation metrics..."
        python3 update_docs.py
    fi
}

# Main Execution
prepare_env
compile_drivers
run_benchmarks
generate_docs

echo "========================================================="
if [ "$UPDATE_DOCS" = true ]; then
    echo "Contributor Mode: All systems verified and documentation updated."
else
    echo "Reviewer Mode: All systems verified and data collected."
fi
echo "========================================================="
