import json

notebook_path = 'comparison.ipynb'

with open(notebook_path, 'r', encoding='utf-8') as f:
    nb = json.load(f)

# Define the updates for the code cells
updates = {
    'plt.title(\'Ingestion Latency per MODE (\\u03bcs)\')': 
    "plt.title('Ingestion Latency per MODE (\\u03bcs)')\\n    plt.xlabel('Write Configuration')\\n    plt.ylabel('Latency (\\u03bcs)')",
    
    'plt.title(\'MTTR per Recovery Mode (ms)\')':
    "plt.title('MTTR per Recovery Mode (ms)')\\n    plt.xlabel('Recovery Configuration')\\n    plt.ylabel('Recovery Time (ms)')",
    
    'plt.title(\'Throughput Scaling (Threads vs Ops/s)\')':
    "plt.title('Throughput Scaling (Threads vs Ops/s)')\\n    plt.xlabel('Concurrent Write Threads')\\n    plt.ylabel('Throughput (Ops/s)')",
    
    'plt.title(\'Recovery Time vs Log Volume\')':
    "plt.title('Recovery Time vs Log Volume')\\n    plt.xlabel('WAL Log Volume (Records)')\\n    plt.ylabel('Replay Duration (ns)')"
}

for cell in nb['cells']:
    if cell['cell_type'] == 'code':
        source = "".join(cell['source'])
        for old, new in updates.items():
            if old in source:
                # Replace the line containing plt.title with the title + labels
                lines = cell['source']
                new_lines = []
                for line in lines:
                    if old.strip() in line.strip():
                        # Preserve indentation if possible, but keep it simple
                        indent = line[:line.find('plt.title')]
                        new_line = f"{indent}{new.replace('\\n', '\\n' + indent)}\n"
                        new_lines.append(new_line)
                    else:
                        new_lines.append(line)
                cell['source'] = new_lines

with open(notebook_path, 'w', encoding='utf-8') as f:
    json.dump(nb, f, indent=1)

print("Notebook axis labels updated successfully.")
