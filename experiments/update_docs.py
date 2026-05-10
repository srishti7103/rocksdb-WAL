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
    metrics = {}
    
    # Study 1: SYNC_TAX (Buffered / StrictSync)
    buffered = float(data.get(('WAL_Sync_Control', 'Buffered'), 1))
    strict = float(data.get(('WAL_Sync_Control', 'StrictSync'), 1))
    sync_tax = int(buffered / strict) if strict > 0 else 0
    metrics['SYNC_TAX'] = f"{sync_tax}x"
    
    # Study 2: FRAG_PCT (Header / Total)
    # The CSV uses a key like ('WAL_Bytes_Header', '1000000')
    header = float(data.get(('WAL_Bytes_Header', '1000000'), 0))
    payload = float(data.get(('WAL_Bytes_Payload', '1000000'), 1))
    frag_pct = (header / (header + payload)) * 100 if (header + payload) > 0 else 0
    metrics['FRAG_PCT'] = f"{frag_pct:.1f}%"
    
    # Study 3: RECOVERY_REDUCTION (Absolute / Tolerate)
    abs_rec = float(data.get(('WAL_Recovery_Mode', 'AbsoluteConsistency'), 1))
    tol_rec = float(data.get(('WAL_Recovery_Mode', 'TolerateCorrupted'), 1))
    reduction = int(abs_rec / tol_rec) if tol_rec > 0 else 1
    metrics['RECOVERY_REDUCTION'] = f"{reduction}x"
    
    # Study 4: GROUP_COMMIT (Max ratio of Scaling/Contention)
    t1 = float(data.get(('WAL_Group_Commit', '1'), 1))
    t8 = float(data.get(('WAL_Group_Commit', '8'), 1))
    if t1 > 0:
        ratio = max(t8/t1, t1/t8)
        metrics['GROUP_COMMIT'] = f"{ratio:.1f}x"
    else:
        metrics['GROUP_COMMIT'] = "1.0x"
    
    return metrics

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
