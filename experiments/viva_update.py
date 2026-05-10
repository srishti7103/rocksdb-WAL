import csv
import re
import os

CSV_PATH = "../results/telemetry.csv"
REPORT_PATH = "../report.md"
README_PATH = "../README.md"

def main():
    data = {}
    with open(CSV_PATH, 'r') as f:
        reader = csv.reader(f)
        next(reader)
        for row in reader:
            if len(row) == 3:
                data[(row[0], row[1])] = int(row[2])

    metrics = {}
    
    # Sync Tax
    b = float(data.get(('WAL_Sync_Control', 'Buffered'), 1))
    s = float(data.get(('WAL_Sync_Control', 'StrictSync'), 1))
    metrics['SYNC_TAX'] = f"{round(b/s)}x"
    
    # Frag
    h = float(data.get(('WAL_Bytes_Header', '1000000'), 0))
    p = float(data.get(('WAL_Bytes_Payload', '1000000'), 1))
    metrics['FRAG_PCT'] = f"{round((h/(h+p))*100, 1)}%"
    
    # Recovery
    a = float(data.get(('WAL_Recovery_Mode', 'AbsoluteConsistency'), 1))
    t = float(data.get(('WAL_Recovery_Mode', 'TolerateCorrupted'), 1))
    metrics['RECOVERY_REDUCTION'] = f"{round(a/t, 1)}x"
    
    # Batch
    t1 = float(data.get(('WAL_Group_Commit', '1'), 1))
    t8 = float(data.get(('WAL_Group_Commit', '8'), 1))
    metrics['GROUP_COMMIT'] = f"{round(max(t1/t8, t8/t1), 1)}x"

    print(f"Final Metrics: {metrics}")

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
