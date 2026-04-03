# 定义默认目标：当执行 make 时，默认编译 demo11 和 demo12 两个可执行文件
all: demo11 demo12

# 生成 demo11 可执行文件：依赖 demo11.cpp
# 使用 g++ 编译，-g 表示生成调试信息，-o 指定输出文件名
demo11: demo11.cpp
	g++ -g demo11.cpp -o demo11

# 生成 demo12 可执行文件：依赖 demo12.cpp
demo12: demo12.cpp
	g++ -g demo12.cpp -o demo12

# 清理编译产物：执行 make clean 时，删除 demo11 和 demo12 两个可执行文件
# -f 选项表示强制删除，即使文件不存在也不会报错
clean:
	rm -f demo11 demo12