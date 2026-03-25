import matplotlib.pyplot as plt
import numpy as np

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['Noto Sans CJK JP']
plt.rcParams['axes.unicode_minus'] = False

# 数据
benchmarks = ['401.bzip2', '429.mcf', '445.gobmk', '456.hmmer', 
              '458.sjeng', '462.libquantum', '464.h264ref', '473.astar', 'GEOMEAN']

lps_overhead = [100.90, 102.37, 100.82, 99.92, 100.25, 100.09, 100.47, 101.24, 100.75]

# WASM 原始数据（自动计算几何平均）
wasm_raw = [172.24, 367.62, 276.37, 167.30, 318.61, 207.81, 284.44, 167.00]
wasm_geomean = np.exp(np.mean(np.log(wasm_raw)))
wasm_overhead = wasm_raw + [wasm_geomean]

# 创建图表
fig, ax = plt.subplots(figsize=(14, 5))

# 柱状图位置
x = np.arange(len(benchmarks))
bar_width = 0.35

# 绘制柱状图
bars_lps  = ax.bar(x + bar_width/2, lps_overhead,  width=bar_width,
                   color='#4472C4', alpha=0.85, edgecolor='black', linewidth=0.8, label='LPS')
bars_wasm = ax.bar(x - bar_width/2, wasm_overhead, width=bar_width,
                   color='#ED7D31', alpha=0.85, edgecolor='black', linewidth=0.8, label='WASM')

# 高亮几何平均柱
bars_lps[-1].set_alpha(0.95)
bars_wasm[-1].set_alpha(0.95)

# 坐标轴标签与标题
ax.set_xlabel('测试程序', fontsize=13, fontweight='bold')
ax.set_ylabel('开销 (%)', fontsize=13, fontweight='bold')
ax.set_title('SPEC CPU2006 INT - 相对于原生Linux的运行开销\n（原生归一化为 100%）',
             fontsize=15, fontweight='bold', pad=20)
ax.set_xticks(x)
ax.set_xticklabels(benchmarks, rotation=45, ha='right', fontsize=11)
ax.set_ylim(0, 430)
ax.grid(axis='y', alpha=0.3, linestyle='--')
ax.axhline(y=100, color='red', linestyle='--', linewidth=1.5, label='基准线 (100%)', alpha=0.7)
ax.legend(fontsize=11, loc='upper left')

# 在几何平均柱顶部添加数值标签
for bars in [bars_lps, bars_wasm]:
    bar = bars[-1]
    height = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2., height + 2,
            f'{height:.2f}%',
            ha='center', va='bottom', fontsize=10, fontweight='bold')

plt.tight_layout()

# 保存文件
filename = 'performance_overhead.png'
plt.savefig(filename, dpi=300, bbox_inches='tight')
print(f"图表已保存为: {filename}")
plt.show()