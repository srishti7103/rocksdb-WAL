import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

os.makedirs('./docs/images', exist_ok=True)
sns.set_theme(style="whitegrid")
df = pd.read_csv('./results/wal_performance_telemetry.csv')

# Study 1: Ingestion Throughput (Metric is already in ops/s)
batch = df[df['Event'] == 'WAL_Batch']
plt.figure(figsize=(10, 5))
sns.barplot(x=['Buffered', 'No-WAL', 'Strict Sync'], y=batch['Metric'].values[:3])
plt.title('Ingestion Throughput per MODE (Ops/s)')
plt.xlabel('Write Configuration')
plt.ylabel('Throughput (Ops/s)')
plt.savefig('./docs/images/exp1_throughput.png', bbox_inches='tight')
plt.close()

# Study 2: Fragmentation
overhead = df[df['Event'].str.contains('WAL_Bytes')]
plt.figure(figsize=(6, 6))
plt.pie(overhead['Metric'], labels=['Headers', 'Payload'], autopct='%1.1f%%', colors=['#e74c3c', '#2ecc71'])
plt.title('Internal Space Utilization')
plt.savefig('./docs/images/exp2_fragmentation.png', bbox_inches='tight')
plt.close()

# Study 3: Recovery Mode
rec_modes = df[df['Event'] == 'WAL_Recovery_Mode']
plt.figure(figsize=(10, 5))
sns.barplot(x=rec_modes['Count'], y=rec_modes['Metric'].astype(int), palette='magma')
plt.title('MTTR per Recovery Mode (ms)')
plt.xlabel('Recovery Configuration')
plt.ylabel('Recovery Time (ms)')
plt.savefig('./docs/images/exp3_recovery_mode.png', bbox_inches='tight')
plt.close()

# Study 4: Group Commit
group = df[df['Event'] == 'WAL_Group_Commit']
plt.figure(figsize=(10, 5))
sns.lineplot(x=group['Count'].astype(int), y=group['Metric'].astype(int), marker='o')
plt.title('Throughput Scaling (Threads vs Ops/s)')
plt.xlabel('Concurrent Write Threads')
plt.ylabel('Throughput (Ops/s)')
plt.savefig('./docs/images/exp4_group_commit.png', bbox_inches='tight')
plt.close()

# Study 5: Recovery Scaling
scaling = df[df['Event'] == 'WAL_Recovery']
plt.figure(figsize=(10, 5))
sns.lineplot(x=scaling['Count'].astype(int), y=scaling['Metric'].astype(int), marker='s', color='orange')
plt.title('Recovery Time vs Log Volume')
plt.xlabel('WAL Log Volume (Records)')
plt.ylabel('Replay Duration (ns)')
plt.savefig('./docs/images/exp5_scaling.png', bbox_inches='tight')
plt.close()

print("All graphs regenerated. Exp 1 now shows Throughput (Buffered is tallest).")
