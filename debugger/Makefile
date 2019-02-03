CC = gcc
SRC_DIR = /home/zxh/src/libunwind-1.2.1
CFLAGS = -I ${SRC_DIR}/include
all:
	${CC} -g -o  test test-ptrace.c ${CFLAGS} ${SRC_DIR}/src/.libs/libunwind-ptrace.so ${SRC_DIR}/src/.libs/libunwind-x86_64.so ${SRC_DIR}/src/.libs/libunwind.so -Wl,-rpath=${SRC_DIR}/src/.libs
