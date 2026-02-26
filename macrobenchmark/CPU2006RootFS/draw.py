import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# 1. 准备数据
data = {
    'Test Name': [
        '401.bzip2', '429.mcf', '445.gobmk', '456.hmmer', 
        '458.sjeng', '462.libquantum', '464.h264ref', '473.astar', 'geomean'
    ],
    'Raw Overhead (%)': [
        100.90, 102.37, 101.34, 99.92, 
        100.25, 100.09, 100.47, 101.24, 100.82
    ]
}

df = pd.DataFrame(data)

# 2. 数据处理：计算相对于 Native (100%) 的增量
df['Relative Overhead'] = df['Raw Overhead (%)'] - 100

# 3. 设置绘图风格（去掉网格线）
plt.figure(figsize=(12, 6))
plt.style.use('default')  # 使用默认样式，避免网格线

# 4. 绘制柱状图（柱子变细：width=0.3）
# #dd8452
colors = ['#4c72b0' if x != 'geomean' else '#4c72b0' for x in df['Test Name']]
bars = plt.bar(df['Test Name'], df['Relative Overhead'], color=colors, width=0.15)

# 5. 图表装饰
plt.title('Overhead on SPEC 2006 Benchmarks', fontsize=16, pad=20)
plt.xlabel('Benchmark Name', fontsize=12)
plt.ylabel('Overhead Increase (%)', fontsize=12)

# 添加 y=0 的基准线
plt.axhline(y=0, color='black', linewidth=1, linestyle='-')

# 设置 Y 轴范围
plt.ylim(0, 15)
plt.yticks(np.arange(0, 16, 1))

# 去掉网格线
plt.grid(False)

# 去掉边框的上边和右边
# ax = plt.gca()
# ax.spines['top'].set_visible(False)
# ax.spines['right'].set_visible(False)

# 6. 只在 geomean 和最高柱子上添加数值标签
max_idx = df['Relative Overhead'].idxmax()
geomean_idx = df[df['Test Name'] == 'geomean'].index[0]

for i, bar in enumerate(bars):
    if i == geomean_idx:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height + 0.1,
                 f'{height:.2f}%',
                 ha='center', va='bottom', fontsize=10, fontweight='bold')

# 旋转 X 轴标签
plt.xticks(rotation=45, ha='right')

# 自动调整布局
plt.tight_layout()

# 7. 保存到文件
filename = 'performance_overhead.png'
plt.savefig(filename, dpi=300, bbox_inches='tight')
print(f"图表已保存为: {filename}")