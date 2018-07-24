CFLAGS=-I/home/zxh/gitrepo/cJSON -L/home/zxh/gitrepo/cJSON/build -lcjson -g
LDFLAGS=-Wl,-rpath=/home/zxh/gitrepo/cJSON/build

all:
	gcc -o config_parse config_parse.c ${CFLAGS} ${LDFLAGS} 

clean:
	rm -rf config_parse config_parse.o
