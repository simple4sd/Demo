CFLAGS=-fPIC -fstack-protector-all -Werror -g -lcap

all:
	gcc -o test net_raw.c ${CFLAGS}
clean:
	rm -rf *.o test
