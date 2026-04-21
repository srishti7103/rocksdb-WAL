import pandas as pd
import matplotlib.pyplot as plt
import os

# Ensure image directory exists
img_dir = '../docs/images'
os.makedirs(img_dir, exist_ok=True)

# Load data
df = pd.read_csv('../results/wal_performance_telemetry.csv')

# Set aesthetic style
plt.style.use('bmh')

# Study 1: Latency
batch = df[df['Event'] == 'WAL_Batch'].copy()
plt.figure(figsize=(8, 4))
plt.bar(['Buffered', 'No-WAL', 'Strict Sync'], batch['Metric'].values[:3] / 1000, color=['#3498db', '#e74c3c', '#2c3e50'])
plt.title('Study 1: Ingestion Latency (\u03bcs)')
plt.ylabel('\u03bc-seconds')
plt.tight_layout()
plt.savefig(f'{img_dir}/exp1_throughput.png')
plt.close()

# Study 2: Fragmentation
overhead = df[df['Event'].str.contains('WAL_Bytes')].copy()
plt.figure(figsize=(6, 6))
plt.pie(overhead['Metric'], labels=['Headers', 'Payload'], autopct='%1.1f%%', colors=['#e74c3c', '#2ecc71'], startangle=140)
plt.title('Study 2: WAL Space Utilization')
plt.tight_layout()
plt.savefig(f'{img_dir}/exp2_fragmentation.png')
plt.close()

# Study 3: Recovery Modes
rec_modes = df[df['Event'] == 'WAL_Recovery_Mode'].copy()
plt.figure(figsize=(8, 4))
plt.bar(rec_modes['Count'], rec_modes['Metric'].astype(int), color='#9b59b6')
plt.title('Study 3: MTTR per Recovery Mode (ms)')
plt.ylabel('ms')
plt.tight_layout()
plt.savefig(f'{img_dir}/exp3_recovery_mode.png')
plt.close()

# Study 4: Group Commit
group = df[df['Event'] == 'WAL_Group_Commit'].copy()
plt.figure(figsize=(8, 4))
plt.plot(group['Count'].astype(int), group['Metric'].astype(int), marker='o', color='#16a085', linewidth=2)
plt.title('Study 4: Group Commit Throughput Scaling')
plt.xlabel('Concurrent Threads')
plt.ylabel('Total Ops/s')
plt.tight_layout()
plt.savefig(f'{img_dir}/exp4_group_commit.png')
plt.close()

# Study 5: Recovery Scaling
scaling = df[df['Event'] == 'WAL_Recovery'].copy()
plt.figure(figsize=(8, 4))
plt.plot(scaling['Count'].astype(int), scaling['Metric'].astype(int), marker='s', color='#f39c12', linewidth=2)
plt.title('Study 5: Recovery Scaling (MTTR)')
plt.xlabel('Records Replayed')
plt.ylabel('ms')
plt.tight_layout()
plt.savefig(f'{img_dir}/exp5_scaling.png')
plt.close()

print("All 5 charts generated successfully in docs/images/")
