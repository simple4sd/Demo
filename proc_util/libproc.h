#ifndef _LIBPROC_H
#define _LIBPROC_H

#include <sys/types.h>

typedef struct raw_socket {
	unsigned long inode;
	unsigned long rx_queue;
	unsigned long drops;
} RAW_SOCK;

unsigned long *get_sock_inodes(pid_t pid);
RAW_SOCK get_raw_sock_info(unsigned long inode);
#endif
