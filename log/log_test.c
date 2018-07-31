#include <stdio.h>
#include <pthread.h>

#include "log.h"


void *thread1(void *arg)
{
	DEBUGLOG("thread 1 now!\n");
	return NULL;
}

int main()
{
	int a = 1, b = 2;
	DEBUGLOG("a\n");
	DEBUGLOG("a = %d\n", a);

	pthread_t tid;
	pthread_create(&tid, NULL, thread1, NULL);
	pthread_join(tid, NULL);
	return 0;
}
