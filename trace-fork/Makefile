CC = gcc
SRC_DIR = /home/zxh/src/libunwind-1.2.1
CFLAGS = -I ${SRC_DIR}/include
all:
	${CC} -g -o  test fork.c -lpthread
	${CC} -g -o  child hello.c -lpthread
