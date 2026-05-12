import json

notebook_path = 'comparison.ipynb'

with open(notebook_path, 'r', encoding='utf-8') as f:
    nb = json.load(f)

# Update the specific cell for Experiment 1
exp1_target = "sns.barplot(x=['Buffered', 'No-WAL', 'Strict Sync'], y=batch['Metric'].values[:3] / 1000)"
exp1_replacement = "latencies = (1 / batch['Metric'].values[:3]) * 1000000\\n    sns.barplot(x=['Buffered', 'No-WAL', 'Strict Sync'], y=latencies)"

for cell in nb['cells']:
    if cell['cell_type'] == 'code':
        source = "".join(cell['source'])
        if exp1_target in source:
            lines = cell['source']
            new_lines = []
            for line in lines:
                if exp1_target in line:
                    indent = line[:line.find('sns.barplot')]
                    new_line = f"{indent}{exp1_replacement.replace('\\n', '\\n' + indent)}\n"
                    new_lines.append(new_line)
                else:
                    new_lines.append(line)
            cell['source'] = new_lines

with open(notebook_path, 'w', encoding='utf-8') as f:
    json.dump(nb, f, indent=1)

print("Notebook code for Experiment 1 updated to show Latency.")
