CFLAGS=-fPIC -fstack-protector-all -Werror -g -lpthread

all:
	gcc -o test log_test.c log.c ${CFLAGS}
clean:
	rm -rf *.o test
