#!/bin/bash

# 在终端执行脚本命令: sh concconn.sh，批量执行1000个程序

# 使用 seq 命令和 for 循环运行 n 个 client_concurrent 程序
for i in $(seq 1 20)
do
    ./client_concurrent 192.168.1.128 4819 500&
done

echo "已启动 n 个 client_concurrent 程序"
