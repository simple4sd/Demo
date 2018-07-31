#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdarg.h>

#define MAX_LINE_LEN 1024

void write_log(int log_levl, const char *code_path,
               int line, const char *fmt, ...)
{
    time_t t;
    struct tm tm;
    pid_t tid = 0;
    char time_str[MAX_LINE_LEN] = {0};
    char fmt_output[MAX_LINE_LEN] = {0};
    char *level = "DEBUG";

    t = time(NULL);
    if (localtime_r(&t, &tm) == NULL)
        return;

    if (strftime(time_str, sizeof(time_str), "%F %T", &tm) == 0)
        return;

    tid = syscall(SYS_gettid);

    va_list ap;
    va_start(ap, fmt);
    vsprintf(fmt_output,fmt,ap);
    va_end(ap);

    fprintf(stdout, "%s %s [%s:%d] %d %s",
            time_str, level, code_path,
            line, tid, fmt_output);
    return;
}