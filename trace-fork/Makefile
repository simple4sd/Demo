CC = gcc
SRC_DIR = /home/zxh/src/libunwind-1.2.1

CFLAGS = -I ${SRC_DIR}/include -Wall -fstack-protector-all
LDFLAGS = -lpthread
all:
	${CC} -g -o  test fork.c ${CFLAGS} ${LDFLAGS}
	${CC} -g -o  child hello.c ${CFLAGS} ${LDFLAGS}

clean:
	rm -rf *.o test child
