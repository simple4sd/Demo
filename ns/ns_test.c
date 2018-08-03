#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <dirent.h>
#include <limits.h>

void scan_dir(char *dirpath)
{
    if (dirpath == NULL)
        return;

    DIR *dirp = opendir(dirpath);
    if (dirp == NULL)
        return;

    struct dirent *ent = NULL;
    while ((ent = readdir(dirp)) != NULL) {
        char ent_path[1024] = {0};
        char real_path[PATH_MAX + 1] = {0};
        sprintf(ent_path, "%s/%s", dirpath, ent->d_name);
        realpath(ent_path, real_path);
        printf("real path is %s\n", real_path);
    }
    closedir(dirp);
}


int thread1(void *arg)
{
    printf("new proc now!\n");
    // create new file
    struct stat st;
    if (stat("new_dir", &st) != 0) {
        if (mkdir("new_dir", 0755) != 0 ) {
            printf("create dir failed\n");
            return -1;
        }
    }

    scan_dir("new_dir");

    if (mount("/", "/", NULL, MS_PRIVATE, NULL) != 0) {
        printf("mount dir failed\n");
        perror("mount");
        return -1;
    }

    if (mount("/tmp/test", "new_dir", NULL, MS_BIND, NULL) != 0) {
        printf("mount dir failed\n");
        perror("mount");
        return -1;
    }
    /*
    if (mount("/tmp/test", "new_dir", NULL, MS_BIND, NULL) != 0) {
        printf("remount dir failed\n");
        perror("remount");
        return -1;
    }
    */

    printf("after mount and chroot");
    chroot("new_dir");
    scan_dir("/");

    while (1) {
        pid_t tid = syscall(SYS_gettid);
        printf("thread %d running now\n", tid);
        sleep(60);
    }
    return 0;
}

int main()
{
    int a = 1, b = 2;

    printf("parent pid is %d\n", getpid());
    sleep(2);
    int stack_size = 1024*1000;
    char *stack = (char *)malloc(stack_size);
    if (stack == NULL)
        return -1;


    pid_t child = clone(thread1, stack+stack_size,
                        CLONE_NEWPID |CLONE_NEWNS | SIGCHLD, 0);

    sleep(1);
    printf("child is %d\n", child);

    int status = 0;
    int ret = waitpid(child, &status, 0);
    printf("wait ret is %d\n", ret);
    if (ret < 0) {
        perror("wait:");
    }

    if (WIFEXITED(status)) {
        printf("exit val is %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("signal val is %d\n", WTERMSIG(status));
    }
    return 0;
}
