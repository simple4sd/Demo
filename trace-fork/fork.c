#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/ptrace.h>
#include <sys/wait.h>

#define DEBUG(...) printf("%s:%d ", __func__, __LINE__);printf(__VA_ARGS__);printf("\n");fflush(stdout);
#define ERROR(...) {int err = errno; printf("%s:%d ", __func__, __LINE__);printf("errno = %d ", err);printf(__VA_ARGS__);printf("\n");fflush(stdout);}


void trace_continue(pid_t pid, int sig)
{
    DEBUG("continue proc %d with sig %d", pid, sig);
    if (ptrace(PTRACE_CONT, pid, NULL, (void *)sig) == -1)
        ERROR("cont failed");
}

// wait any child proces
// err happen return -1

int wait_pid(int *pid, int *pending_sig, int *err)
{
    int status = 0;
    *pid = waitpid (-1, &status, 0);
    if (*pid == -1) {
	*err = errno;
        ERROR("waitpid failed\n");
	return -1;
    }

    if (WIFEXITED (status)) {
        DEBUG("child %d exit status %d\n", *pid, WEXITSTATUS (status));
	*pending_sig = 0;
        return 0;
    }

    if (WIFSIGNALED (status)) {
        *pending_sig = WTERMSIG(status);
        DEBUG("child %d terminated by signal %d\n", *pid, WTERMSIG (status));
        return 0;
    }

    if (WIFSTOPPED (status)) {
        *pending_sig = WSTOPSIG(status);
        DEBUG("child %d, stopped by signal %d\n", *pid, WSTOPSIG (status));
        return 0;
    }
    return -1;
}

// ret 0  means every thing is ok, no need to care
// ret 1  tracer should handle the signal
int wait_pid_deliver_sig(int *pid, int *pending_sig)
{
    pid_t wpid;
    int sig = 0;
    int err = 0;
    if (wait_pid(&wpid, &sig, &err) != 0) {
	if (err == ECHILD) {
        	DEBUG("wait failed. no children now, err: %d\n", err);
        	return -1;
	} else {
		DEBUG("maybe still have children. err is %d", err);	
		return 0;
	}
    }

    *pid = wpid;
    *pending_sig = sig;

    if (sig == SIGSTOP || sig == SIGTERM || sig == SIGTRAP ||
        sig == SIGSEGV || sig == SIGBUS) {
        DEBUG("stop %d, sig %d\n", wpid, sig);
        return 1;
    }

    trace_continue(wpid, sig);
    return 0;

}

int trace_fork(pid_t pid)
{
    if (ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACEFORK |
               PTRACE_O_TRACEVFORK|
               PTRACE_O_TRACECLONE) != 0) {
        ERROR("set ptrace clone failed");
        return -1;
    }

    DEBUG("set trace fork %d ok", pid);
    return 0;
}




int wait_child()
{
    int wpid, sig;
    while (1) {
        int i = 0;
        DEBUG("waiting now! %d\n", i++);
        int ret = wait_pid_deliver_sig(&wpid, &sig);
        if (ret < 0) {
            DEBUG("wait failed\n");
            return -1;
        } else if (ret > 0) {
            if (sig == SIGTRAP) {
                DEBUG("SIGTRAP trace fork %d\n", wpid);
                if (trace_fork(wpid) != 0) {
                    DEBUG("trace fork %d failed\n", wpid);
                    return -1;
                }
		
                trace_continue(wpid, 0);
                continue;
            }

            if (sig == SIGSTOP) {
                pid_t pid = wpid;
                DEBUG("child pid is %d\n", pid);
                DEBUG("SIGSTOP trace fork %d", pid);
                if (trace_fork(pid) != 0) {
                    DEBUG("SIGSTOP trace fork %d failed", pid);
                    return -1;
                }

                trace_continue(pid, 0);
                continue;
            }
        }
	continue;
    }

    DEBUG("stop %d, sig %d\n", wpid, sig);
    return 0;
}

void do_child_thing()
{
	if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) != 0) {
		ERROR("set TRACEME failed");
	}
	execl("./child", "xxx", NULL);
}
int main (int argc, char **argv)
{
    DEBUG("Hello world!");
   
    pid_t pid = fork(); 
    if (pid == 0) {
	do_child_thing();
    }
    sleep(1);
    wait_child();
    return 0;
}

