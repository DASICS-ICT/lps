#!/bin/sh

get_spec_int() {
    # 这里的换行符会被 shell 视为分隔符
    # 被注释掉的项目直接不写在 echo 中即可
    echo "
    401.bzip2
    429.mcf
    445.gobmk
    456.hmmer
    458.sjeng
    462.libquantum
    464.h264ref
    473.astar
    "
}

# $(get_spec_int) 会展开函数输出的字符串
for name in $(get_spec_int); do
    # 运行脚本
    ./test-wasm.sh "$name"
    
    # 模拟 check=True
    # $? 获取上一个命令的退出状态码，如果不等于 0 则表示出错
    if [ $? -ne 0 ]; then
        echo "Error: Command failed for $name"
    fi
done