#include <config.h>
#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <libunwind-ptrace.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/wait.h>

extern char **environ;

static const int nerrors_max = 100;

int nerrors;
int verbose;
int print_names = 1;
static struct UPT_info *ui_child;


int trace_fork(pid_t pid);

enum {
    INSTRUCTION,
    SYSCALL,
    TRIGGER
}
trace_mode = SYSCALL;

#define panic(args...)						\
	do { printf("%-20s:%d  ", __func__, __LINE__);fprintf (stdout, args); ++nerrors; } while (0)

static unw_addr_space_t as;

void do_backtrace (struct UPT_info *ui)
{
    unw_word_t ip, sp, start_ip = 0, off;
    int n = 0, ret;
    unw_proc_info_t pi;
    unw_cursor_t c;
    char buf[512];
    size_t len;

    ret = unw_init_remote (&c, as, ui);
    if (ret < 0)
        panic ("unw_init_remote() failed: ret=%d\n", ret);

    do {
        if ((ret = unw_get_reg (&c, UNW_REG_IP, &ip)) < 0
            || (ret = unw_get_reg (&c, UNW_REG_SP, &sp)) < 0)
            panic ("unw_get_reg/unw_get_proc_name() failed: ret=%d\n", ret);

        if (n == 0)
            start_ip = ip;

        buf[0] = '\0';
        if (print_names)
            unw_get_proc_name (&c, buf, sizeof (buf), &off);

        if (off) {
            len = strlen (buf);
            if (len >= sizeof (buf) - 32)
                len = sizeof (buf) - 32;
            sprintf (buf + len, "+0x%lx", (unsigned long) off);
        }
        printf ("%016lx %-32s (sp=%016lx)\n", (long) ip, buf, (long) sp);

        if ((ret = unw_get_proc_info (&c, &pi)) < 0)
            panic ("unw_get_proc_info(ip=0x%lx) failed: ret=%d\n", (long) ip, ret);
        else if (verbose)
            printf ("\tproc=%016lx-%016lx\n\thandler=%lx lsda=%lx",
                    (long) pi.start_ip, (long) pi.end_ip,
                    (long) pi.handler, (long) pi.lsda);

        if (verbose)
            printf ("\n");

        ret = unw_step (&c);
        if (ret < 0) {
            unw_get_reg (&c, UNW_REG_IP, &ip);
            panic ("FAILURE: unw_step() returned %d for ip=%lx (start ip=%lx)\n",
                   ret, (long) ip, (long) start_ip);
        }

        if (++n > 64) {
            /* guard against bad unwind info in old libraries... */
            panic ("too deeply nested---assuming bogus unwind (start ip=%lx)\n",
                   (long) start_ip);
            break;
        }
        if (nerrors > nerrors_max) {
            panic ("Too many errors (%d)!\n", nerrors);
            break;
        }
    } while (ret > 0);

    if (ret < 0)
        panic ("unwind failed with ret=%d\n", ret);

    if (verbose)
        printf ("================\n\n");
}

static pid_t target_pid;
static void target_pid_kill (void)
{
    kill (target_pid, SIGKILL);
}

void trace_continue(pid_t pid, int sig)
{
    panic("continue proc %d with sig %d\n", pid, sig);
    if (ptrace(PTRACE_CONT, pid, NULL, (void *)sig) != 0)
        panic("cont failed\n");
}

int wait_pid(int *pid, int *pending_sig)
{
    int status = 0;
    *pid = waitpid (-1, &status, 0);
    if (*pid == -1) {
        if (errno == EINTR)
            return -1;
        panic ("waitpid failed (errno=%d)\n", errno);
    }

    if (WIFEXITED (status)) {
        panic ("child %d exit status %d\n", *pid, WEXITSTATUS (status));
        return -1;
    }

    if  (WIFSIGNALED (status)) {
        *pending_sig = WTERMSIG(status);
        panic ("child %d terminated by signal %d\n", *pid, WTERMSIG (status));
        return 0;
    }

    if (WIFSTOPPED (status)) {
        *pending_sig = WSTOPSIG(status);
        panic ("child %d, stopped by signal %d\n", *pid, WSTOPSIG (status));
        return 0;
    }
    return -1;
}

// ret 0 will continue, ret 1 should handle
int wait_pid_deliver_sig(int *pid, int *pending_sig)
{
    pid_t wpid;
    int sig;
    if (wait_pid(&wpid, &sig) != 0) {
        panic("wait failed\n");
        return -1;
    }

    *pid = wpid;
    *pending_sig = sig;

    if (sig == SIGSTOP || sig == SIGTERM || sig == SIGTRAP ||
        sig == SIGSEGV || sig == SIGBUS) {
        panic("stop %d, sig %d\n", wpid, sig);
        return 1;
    }

    trace_continue(wpid, sig);
    return 0;

}


int attach_pid(pid_t pid)
{
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) != 0) {
        panic("attach failed, errno = %d\n", errno);
        return -1;
    }

    int wpid, sig;
    if (wait_pid(&wpid, &sig) != 0) {
        panic("wait failed\n");
        return -1;
    }

    if (trace_fork(wpid) != 0) {
        panic("trace fork failed\n");
        return -1;
    }

    panic("attach pid %d. stop %d, sig %d\n", pid, wpid, sig);
    if (sig == SIGSTOP) {
        trace_continue(target_pid, 0);
    } else {
        return -1;
    }
}

void crash1()
{
    char *p = NULL;
    *p = 'a';
}

void crash()
{
    crash1();
}

int wait_child_crash(pid_t *crash_pid)
{
    int wpid, sig;
    while (1) {
        int i = 0;
        panic("waiting now! %d\n", i++);
        int ret = wait_pid_deliver_sig(&wpid, &sig);
        if (ret < 0) {
            panic("wait failed\n");
            return -1;
        } else if (ret > 0) {
            if (sig == SIGTRAP) {
                trace_continue(wpid, 0);
                continue;
            }

            if (sig == SIGSTOP) {
                pid_t pid = wpid;
                panic("child pid is %d\n", pid);
                if (trace_fork(pid) != 0) {
                    panic("trace fork %d\n", pid);
                    return -1;
                }
                panic("create UPT info\n");
                ui_child = _UPT_create (pid);
                if (ui_child == NULL) {

                    panic("create UPT info failed\n");
                    return -1;
                }
                trace_continue(pid, 0);
                continue;
            }
        }
        break;
    }

    *crash_pid = wpid;
    panic("stop %d, sig %d\n", wpid, sig);
    return 0;
}

int trace_fork(pid_t pid)
{
    if (ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACEFORK |
               PTRACE_O_TRACEVFORK|
               PTRACE_O_TRACECLONE) != 0) {
        panic("set ptrace clone failed\n");
        return -1;
    }

    panic("set %s ok\n", __func__);
    return 0;
}

void child_thing()
{
    panic("p1 %d\n", getpid());
    sleep(1);
    if (fork() > 0) {
        //parent
        sleep(10);
    } else {
        panic("p2 %d\n", getpid());
        sleep(1);
        crash();
    }
    exit(1);
}

int main (int argc, char **argv)
{
    int status, optind = 1, state = 1;

    verbose = 0;
    print_names = 1;
    as = unw_create_addr_space (&_UPT_accessors, 0);
    if (!as)
        panic ("unw_create_addr_space() failed");

    target_pid = fork ();
    if (!target_pid) {
        /* child */
        //ptrace (PTRACE_TRACEME, 0, 0, 0);
        //execve (argv[optind], argv + optind, environ);
        child_thing();
    }

    struct UPT_info *ui = _UPT_create (target_pid);

    if (ui == NULL) {
        panic("create ui failed\n");
        return 0;
    }

    if (attach_pid(target_pid) != 0)
        return -1;


    pid_t crash_pid;
    if (wait_child_crash(&crash_pid) != 0) {
        return -1;
    }

    if (crash_pid == target_pid)  {
        do_backtrace(ui);
    } else {
        do_backtrace(ui_child);
    }

    exit(1);
    atexit (target_pid_kill);

    _UPT_destroy (ui);
    unw_destroy_addr_space (as);
    return 0;
}

