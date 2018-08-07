#ifndef _LIBPROC_H
#define _LIBPROC_H

#include <sys/types.h>

typedef struct raw_socket {
    unsigned long inode;
    unsigned long rx_queue;
    unsigned long drops;
} RAW_SOCK;

typedef struct tcp_socket {
    unsigned long inode;
    unsigned long local_ip;
    unsigned long remote_ip;
    int local_port;
    int remote_port;
    int state;
} TCP_SOCK;

unsigned long *get_sock_inodes(pid_t pid);
RAW_SOCK get_raw_sock_info(unsigned long inode);
TCP_SOCK get_tcp_sock_info(unsigned long inode);

char *get_proc_name(pid_t pid);
char *get_proc_exe(pid_t pid);
pid_t get_proc_ppid(pid_t pid);
char *get_proc_cmdline(pid_t pid);
char **get_proc_cmdline_list(pid_t pid);

#endif
