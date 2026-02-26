import matplotlib.pyplot as plt
import numpy as np

# Data
benchmarks = ['401.bzip2', '429.mcf', '445.gobmk', '456.hmmer', 
              '458.sjeng', '462.libquantum', '464.h264ref', '473.astar', 'geomean']

overhead = [100.90, 102.37, 100.82, 99.92, 100.25, 100.09, 100.47, 101.24, 100.75]

# Create figure
fig, ax = plt.subplots(figsize=(12, 5))

# Create bar chart with thinner bars
x = np.arange(len(benchmarks))
bars = ax.bar(x, overhead, width=0.2, color='#4472C4', alpha=0.85, edgecolor='black', linewidth=0.8)

# Highlight geomean bar
# bars[-1].set_color('#ED7D31')
bars[-1].set_alpha(0.9)

ax.set_xlabel('Benchmark', fontsize=13, fontweight='bold')
ax.set_ylabel('Overhead (%)', fontsize=13, fontweight='bold')
ax.set_title('SPEC CPU2006 INT - LPS Overhead Relative to Linux\n(Linux normalized to 100%)', 
             fontsize=15, fontweight='bold', pad=20)
ax.set_xticks(x)
ax.set_xticklabels(benchmarks, rotation=45, ha='right', fontsize=11)
ax.set_ylim(0, 120)
ax.grid(axis='y', alpha=0.3, linestyle='--')
ax.axhline(y=100, color='red', linestyle='--', linewidth=1.5, label='Baseline (100%)', alpha=0.7)
# ax.legend(fontsize=11, loc='upper left')

# Add value labels on bars
# for i, bar in enumerate(bars):
#     height = bar.get_height()
#     ax.text(bar.get_x() + bar.get_width()/2., height + 0.5,
#             f'{height:.2f}%',
#             ha='center', va='bottom', fontsize=10, fontweight='bold')
bar = bars[-1]
height = bar.get_height()
ax.text(bar.get_x() + bar.get_width()/2., height + 0.5,
            f'{height:.2f}%',
            ha='center', va='bottom', fontsize=10, fontweight='bold')

plt.tight_layout()

# 7. 保存到文件
filename = 'performance_overhead.png'
plt.savefig(filename, dpi=300, bbox_inches='tight')
print(f"图表已保存为: {filename}")
