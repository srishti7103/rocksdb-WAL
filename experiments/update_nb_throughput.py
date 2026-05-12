import json

notebook_path = 'comparison.ipynb'

with open(notebook_path, 'r', encoding='utf-8') as f:
    nb = json.load(f)

# Define the updates for the code cells to show Throughput
updates = {
    'plt.title(\'Ingestion Latency per MODE (\\u03bcs)\')': 
    "plt.title('Ingestion Throughput per MODE (Ops/s)')\\n    plt.xlabel('Write Configuration')\\n    plt.ylabel('Throughput (Ops/s)')",
    
    'plt.title(\'MTTR per Recovery Mode (ms)\')':
    "plt.title('MTTR per Recovery Mode (ms)')\\n    plt.xlabel('Recovery Configuration')\\n    plt.ylabel('Recovery Time (ms)')",
    
    'plt.title(\'Throughput Scaling (Threads vs Ops/s)\')':
    "plt.title('Throughput Scaling (Threads vs Ops/s)')\\n    plt.xlabel('Concurrent Write Threads')\\n    plt.ylabel('Throughput (Ops/s)')",
    
    'plt.title(\'Recovery Time vs Log Volume\')':
    "plt.title('Recovery Time vs Log Volume')\\n    plt.xlabel('WAL Log Volume (Records)')\\n    plt.ylabel('Replay Duration (ns)')"
}

# Also need to make sure the Y values aren't inverted in Study 1
exp1_invert_code = "latencies = (1 / batch['Metric'].values[:3]) * 1000000"

for cell in nb['cells']:
    if cell['cell_type'] == 'code':
        source = "".join(cell['source'])
        # Revert inversion if present
        if exp1_invert_code in source:
             cell['source'] = [line for line in cell['source'] if exp1_invert_code not in line]
             # Update the sns.barplot line to use raw metric
             cell['source'] = [line.replace('y=latencies', "y=batch['Metric'].values[:3]") for line in cell['source']]
        
        # Apply title and label updates
        for old, new in updates.items():
            if old.strip() in source:
                lines = cell['source']
                new_lines = []
                for line in lines:
                    if old.strip() in line.strip():
                        indent = line[:line.find('plt.title')]
                        new_line = f"{indent}{new.replace('\\n', '\\n' + indent)}\n"
                        new_lines.append(new_line)
                    else:
                        new_lines.append(line)
                cell['source'] = new_lines

with open(notebook_path, 'w', encoding='utf-8') as f:
    json.dump(nb, f, indent=1)

print("Notebook updated to show Throughput for Experiment 1.")
