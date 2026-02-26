import matplotlib.pyplot as plt
import numpy as np

# 设置中文字体支持
# plt.rcParams['font.sans-serif'] = ['SimHei', 'Arial Unicode MS', 'DejaVu Sans']
# plt.rcParams['axes.unicode_minus'] = False

# 数据准备
test_items = ['pipe', 'syscall', 'yield']

# 每个测试项的数据（None表示空值）
linux_data = [27919, 952, None]
lps_data = [4176, 406, 2150]
lps_bound_data = [2916, 408, 1059]

test_items.reverse()
linux_data.reverse()
lps_bound_data.reverse()
lps_data.reverse()


# 设置图形
fig, ax = plt.subplots(figsize=(12, 6))

# 设置柱子的位置
y_pos = np.arange(len(test_items))
bar_height = 0.25  # 每个柱子的高度

# 绘制柱状图
bars1 = ax.barh([y - bar_height for y in y_pos], 
                 [v if v is not None else 0 for v in lps_bound_data],
                 bar_height, 
                 label='lps(single bound)',
                 color='#2ecc71', ##2ecc71
                 alpha=0.8)

bars2 = ax.barh(y_pos, 
                 [v if v is not None else 0 for v in lps_data],
                 bar_height, 
                 label='lps',
                 color='#3498db',
                 alpha=0.8)

bars3 = ax.barh([y + bar_height for y in y_pos], 
                 [v if v is not None else 0 for v in linux_data],
                 bar_height, 
                 label='linux',
                 color='#e74c3c',
                 alpha=0.8)

# 在柱子上添加数值标签
def add_labels(bars, data):
    for i, (bar, value) in enumerate(zip(bars, data)):
        if value is not None and value > 0:
            width = bar.get_width()
            ax.text(width, bar.get_y() + bar.get_height()/2,
                   f'{value:,}',
                   ha='left', va='center',
                   fontsize=9,
                   color='black')

add_labels(bars3, linux_data)
add_labels(bars2, lps_data)
add_labels(bars1, lps_bound_data)

# 设置坐标轴
ax.set_yticks(y_pos)
ax.set_yticklabels(test_items)
ax.set_xlabel('cycles', fontsize=12)
ax.set_ylabel('name', fontsize=12)
ax.set_title('Microbenchmark', fontsize=14, fontweight='bold')

# 添加图例 - 翻转顺序
handles, labels = ax.get_legend_handles_labels()
ax.legend(reversed(handles), reversed(labels), loc='best', fontsize=10)

# 添加网格线
# ax.grid(axis='x', alpha=0.3, linestyle='--')

# 调整布局
plt.tight_layout()

filename = 'micro.png'
plt.savefig(filename, dpi=300, bbox_inches='tight')
print(f"图表已保存为: {filename}")