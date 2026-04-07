#!/bin/bash

# 在终端执行脚本命令: sh conc.sh conc n，批量执行程序
# conc: 启动次数，n: client_concurrent 的 circlenum 参数

# 使用 seq 命令和 for 循环运行 conc 个 client_concurrent 程序
for i in $(seq 1 $1)
do
    ./client_concurrent 192.168.1.128 4819 $2&
done

echo "已启动 $1 个 client_concurrent 程序，circlenum 参数值为 $2"
