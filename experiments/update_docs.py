import csv
import re
import os

CSV_PATH = "../results/wal_performance_telemetry.csv"
REPORT_PATH = "../report.md"
README_PATH = "../README.md"

def load_csv_data():
    data = {}
    if not os.path.exists(CSV_PATH):
        print(f"Error: {CSV_PATH} not found.")
        return None
    
    with open(CSV_PATH, 'r') as f:
        reader = csv.reader(f)
        next(reader) # skip header
        for row in reader:
            if len(row) == 3:
                event, count, metric = row
                data[(event, count)] = int(metric)
    return data

def calculate_metrics(data):
    # Sync Tax: WAL_Batch,3 / WAL_Batch,1
    sync_tax = round(data.get(('WAL_Batch', '3'), 1) / data.get(('WAL_Batch', '1'), 1))
    
    # Fragmentation: Header / (Header + Payload)
    header = data.get(('WAL_Bytes_Header', '1000000'), 0)
    payload = data.get(('WAL_Bytes_Payload', '1000000'), 1)
    frag_pct = round((header / (header + payload)) * 100, 1)
    
    # Recovery Speedup: AbsoluteConsistency / TolerateCorrupted
    abs_cons = data.get(('WAL_Recovery_Mode', 'AbsoluteConsistency'), 1)
    tol_corr = data.get(('WAL_Recovery_Mode', 'TolerateCorrupted'), 1)
    recovery_reduction = round(abs_cons / tol_corr)
    
    # Group Commit: Thread 8 / Thread 1
    thread_8 = data.get(('WAL_Group_Commit', '8'), 1)
    thread_1 = data.get(('WAL_Group_Commit', '1'), 1)
    group_commit = round(thread_8 / thread_1, 1)
    
    return {
        'SYNC_TAX': f"{sync_tax}x",
        'FRAG_PCT': f"{frag_pct}%",
        'RECOVERY_REDUCTION': f"{recovery_reduction}x",
        'GROUP_COMMIT': f"{group_commit}x"
    }

def update_file(filepath, metrics):
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found.")
        return
        
    with open(filepath, 'r') as f:
        content = f.read()
        
    for key, value in metrics.items():
        pattern = rf'(<!-- DYNAMIC:{key} -->)\*\*.*?\*\*(<!-- END_DYNAMIC -->)'
        replacement = rf'\g<1>**{value}**\g<2>'
        content = re.sub(pattern, replacement, content)
        
    with open(filepath, 'w') as f:
        f.write(content)
    print(f"Updated {filepath} with new metrics.")

def main():
    print("Reading telemetry data...")
    data = load_csv_data()
    if not data:
        return
        
    metrics = calculate_metrics(data)
    print(f"Calculated Metrics: {metrics}")
    
    update_file(REPORT_PATH, metrics)
    update_file(README_PATH, metrics)
    print("Documentation update complete.")

if __name__ == "__main__":
    main()
