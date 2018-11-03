# 可执行文件
TARGET = sc16x2l
# 依赖目标
OBJS = sc16x2l.o

# 指令编译器和选项
CC=gcc
CFLAGS=-Wall -std=gnu99

$(TARGET):$(OBJS)
# @echo TARGET:$@
# @echo OBJECTS:$^
	$(CC) -o $@ $^

clean:
	rm -rf $(TARGET) $(OBJS)
