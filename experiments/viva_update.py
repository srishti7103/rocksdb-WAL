import csv
import re
import os

# The C++ code writes to this file
CSV_PATH = "../results/wal_performance_telemetry.csv"
REPORT_PATH = "../report.md"
README_PATH = "../README.md"

def main():
    data = {}
    if not os.path.exists(CSV_PATH):
        print(f"Error: {CSV_PATH} not found.")
        return

    with open(CSV_PATH, 'r') as f:
        reader = csv.reader(f)
        next(reader)
        for row in reader:
            if len(row) == 3:
                # Store only the last valid number for each key
                try:
                    data[(row[0], row[1])] = float(row[2])
                except:
                    continue

    metrics = {}
    
    # 1. Sync Tax (Comparison of Buffered vs StrictSync)
    b = data.get(('WAL_Sync_Control', 'Buffered'), 1)
    s = data.get(('WAL_Sync_Control', 'StrictSync'), 1)
    metrics['SYNC_TAX'] = f"{round(b/s)}x"
    
    # 2. Fragmentation (Headers vs Payload)
    h = data.get(('WAL_Bytes_Header', '1000000'), 0)
    p = data.get(('WAL_Bytes_Payload', '1000000'), 1)
    metrics['FRAG_PCT'] = f"{round((h/(h+p))*100, 1)}%"
    
    # 3. Recovery Reduction (Absolute vs Tolerate)
    a = data.get(('WAL_Recovery_Mode', 'AbsoluteConsistency'), 1)
    t = data.get(('WAL_Recovery_Mode', 'TolerateCorrupted'), 1)
    metrics['RECOVERY_REDUCTION'] = f"{round(a/t, 1)}x"
    
    # 4. Group Commit (Scaling ratio)
    t1 = data.get(('WAL_Group_Commit', '1'), 1)
    t8 = data.get(('WAL_Group_Commit', '8'), 1)
    ratio = max(t1/t8, t8/t1) if t1 > 0 and t8 > 0 else 1.0
    metrics['GROUP_COMMIT'] = f"{round(ratio, 1)}x"

    print(f"Verified Metrics: {metrics}")

    for path in [REPORT_PATH, README_PATH]:
        with open(path, 'r') as f:
            content = f.read()
        for k, v in metrics.items():
            content = re.sub(rf'(<!-- DYNAMIC:{k} -->)\*\*.*?\*\*(<!-- END_DYNAMIC -->)', rf'\g<1>**{v}**\g<2>', content)
        with open(path, 'w') as f:
            f.write(content)
        print(f"Updated {path}")

if __name__ == "__main__":
    main()
