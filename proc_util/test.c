#include <stdio.h>
#include <stdlib.h>

#include "libproc.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("wrong parameter number %d\n", argc);
        return -1;
    }

    pid_t pid = 0;
    sscanf(argv[1], "%d", &pid);
    printf("pid = %d\n", pid);

    char *name = get_proc_name(pid);
    if (name != NULL) {
        printf("[%d] %s\n", pid, name);
        free(name);
    }
    char *exe = get_proc_exe(pid);
    if (exe != NULL) {
        printf("[%d] %s\n", pid, exe);
        free(exe);
    }
    printf("[%d] ppid %d\n", pid, get_proc_ppid(pid));

    unsigned long *inodes_ptr = get_sock_inodes(pid);
    if (inodes_ptr == NULL)
        return -1;

    int i = 0;
    while (inodes_ptr[i] != 0) {
        RAW_SOCK rs = get_raw_sock_info(inodes_ptr[i++]);
        if (rs.inode != 0) {
            printf("inode %lu rx_queue %lu drops %lu\n",
                   rs.inode, rs.rx_queue, rs.drops);
            break;
        }
    }

    free(inodes_ptr);

    char **str_arr = get_proc_cmdline_list(pid);

    printf("cmdline list is :\n");
    while (str_arr != NULL && *str_arr != NULL) {
        printf("%s\n", *str_arr++);
    }

    printf("cmdline content is :\n");
    char *cmdline = get_proc_cmdline(pid);
    if (cmdline != NULL) {
        printf("%s\n", cmdline);
    }

    return 0;
}
