#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#define DEBUG(...) printf("[CHILD] ");printf("%s:%d ", __func__, __LINE__);printf(__VA_ARGS__);printf("\n");
#define ERROR(...) printf("[CHILD] ");{int err = errno; printf("%s:%d ", __func__, __LINE__);printf("errno = %d ", err);printf(__VA_ARGS__);printf("\n");}

void *do_sleep_thing(void *arg)
{
	DEBUG("create new thread: %ld\n", syscall(SYS_gettid));
	while (1) {
		sleep(1);
	}

	return NULL;
}

void create_thread()
{
	pthread_t tid;
	if (pthread_create(&tid, NULL, do_sleep_thing, NULL) != 0) {
		ERROR("pthread create failed");	
		return;
	}
}
void do_child_thing()
{
//	while (1) {
		DEBUG("fork again! %d\n", getpid());
		pid_t pid = fork();
		sleep(1);
		if (pid > 0) {
			// parent go to endless sleep
			// while(1) sleep(1);

			// parent exit 
			exit(0);
		} else if (pid == 0) {
			DEBUG("create new process: %d\n", getpid());
			create_thread();
		} else {
			ERROR("fork failed\n");
			return;
		}
               
		DEBUG("sleep process: %ld\n", syscall(SYS_gettid));
		sleep(10);
//	}
//	return;
}

int main (int argc, char **argv)
{
    DEBUG("new proc %d: begin fork\n", getpid());
    pid_t pid = fork();
    if (pid == 0)
    	do_child_thing();
    DEBUG("new proc %d: exit\n", getpid());
    return 0;
}

