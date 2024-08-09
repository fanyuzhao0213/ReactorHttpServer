
#CC = gcc: 指定使用 gcc 作为编译器。
#CFLAGS = -Wall -g: 编译器选项，-Wall 启用所有警告，-g 生成调试信息。

# 指定编译器
CC = gcc
# 编译器选项
CFLAGS = -Wall -g

# 链接选项
LDFLAGS = -lpthread

# 可执行文件的名称
TARGET = myprogram

# 获取当前目录下的所有 .c 文件
SRCS = $(wildcard *.c)

# 生成的目标文件列表（将 .c 文件替换为 .o 文件）
OBJS = $(SRCS:.c=.o)

# 默认目标：编译可执行程序
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 生成每个 .o 文件的规则
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理目标文件和可执行文件
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标（防止和同名文件冲突）
.PHONY: clean
