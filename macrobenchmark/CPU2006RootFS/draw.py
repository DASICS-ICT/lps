import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# 设置中文字体，兼容 Windows (SimHei) 和 Mac (Arial Unicode MS)
plt.rcParams['font.sans-serif'] = ['SimHei', 'Arial Unicode MS', 'sans-serif']
plt.rcParams['axes.unicode_minus'] = False  # 用来正常显示负号

# 1. 准备数据
test_names = [
    '401.bzip2', '429.mcf', '445.gobmk', '456.hmmer', 
    '458.sjeng', '462.libquantum', '464.h264ref', '473.astar'
]

# 原始数据
raw_overhead = [100.90, 102.37, 100.82, 99.92, 100.25, 100.09, 100.47, 101.24]
wasm_overhead = [172.24, 367.62, 276.37, 167.30, 318.61, 207.81, 284.44, 167.00]

# 计算几何平均值 (geomean)
raw_geomean = np.exp(np.mean(np.log(raw_overhead)))
wasm_geomean = np.exp(np.mean(np.log(wasm_overhead)))

# 将 geomean 追加到列表中
test_names.append('几何平均 (geomean)')
raw_overhead.append(raw_geomean)
wasm_overhead.append(wasm_geomean)

data = {
    '测试名称': test_names,
    '原方案开销 (%)': raw_overhead,
    'WASM开销 (%)': wasm_overhead
}

df = pd.DataFrame(data)

# 2. 数据处理：计算相对于 Native (100%) 的增量
df['原方案相对开销'] = df['原方案开销 (%)'] - 100
df['WASM相对开销'] = df['WASM开销 (%)'] - 100

# 3. 设置绘图风格
plt.style.use('default')
# 重新应用字体设置（因为 style.use 可能会重置字体）
plt.rcParams['font.sans-serif'] = ['SimHei', 'Arial Unicode MS', 'sans-serif']
plt.rcParams['axes.unicode_minus'] = False

fig, ax = plt.subplots(figsize=(12, 6))

# 4. 绘制分组柱状图
x = np.arange(len(df['测试名称']))  # 标签位置
width = 0.35  # 柱子的宽度

# 绘制两组柱子
bars1 = ax.bar(x - width/2, df['原方案相对开销'], width, label='原方案', color='#4c72b0')
bars2 = ax.bar(x + width/2, df['WASM相对开销'], width, label='WASM', color='#dd8452')

# 5. 图表装饰
ax.set_title('SPEC 2006 基准测试性能开销', fontsize=16, pad=20)
ax.set_xlabel('基准测试名称', fontsize=12)
ax.set_ylabel('开销增加 (%)', fontsize=12)

# 设置 X 轴刻度和标签
ax.set_xticks(x)
ax.set_xticklabels(df['测试名称'], rotation=45, ha='right')

# 添加图例
ax.legend()

# 添加 y=0 的基准线
ax.axhline(y=0, color='black', linewidth=1, linestyle='-')

# 去掉网格线
ax.grid(False)

# 去掉边框的上边和右边
ax.spines['top'].set_visible(False)
ax.spines['right'].set_visible(False)

# 6. 只在 geomean 柱子上添加数值标签
geomean_idx = len(df) - 1  # 最后一项是 geomean

# 为原方案的 geomean 添加标签
h1 = bars1[geomean_idx].get_height()
ax.text(bars1[geomean_idx].get_x() + bars1[geomean_idx].get_width()/2., h1 + 2,
        f'{h1:.2f}%', ha='center', va='bottom', fontsize=10, fontweight='bold', color='#4c72b0')

# 为 WASM 的 geomean 添加标签
h2 = bars2[geomean_idx].get_height()
ax.text(bars2[geomean_idx].get_x() + bars2[geomean_idx].get_width()/2., h2 + 2,
        f'{h2:.2f}%', ha='center', va='bottom', fontsize=10, fontweight='bold', color='#dd8452')

# 自动调整布局
plt.tight_layout()

# 7. 保存到文件
filename = 'performance_overhead_cn.png'
plt.savefig(filename, dpi=300, bbox_inches='tight')
print(f"图表已保存为: {filename}")
