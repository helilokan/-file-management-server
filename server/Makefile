#通用Makefile
out:=server#可执行文件名称需要根据需要更改
SRCS:=$(wildcard *.c)#拿到当前目录下所有的.c文件
OBJS:=$(patsubst %.c,%.o,$(SRCS))#把.c文件替换为.o文件
#预定义变量
CC:=gcc
#CFLAGS:=-g
$(out):$(OBJS)
	$(CC) $^ -o $@  -pthread -lcrypt -lmysqlclient -ll8w8jwt -lmbedcrypto -lmbedx509 -g
%.o:%.c
	$(CC) -c $^ -o $@

#.PHONY:clean rebuild
rebuild:clean $(out)
clean:
	rm -rf $(OBJS) $(out)
