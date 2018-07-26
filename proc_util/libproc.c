#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "libproc.h"

#define MAX_PATH_LEN 1024
#define INIT_SIZE    16

typedef struct list_head {
    unsigned long *head;
    int size;
} LIST_HEAD;

int append_to_list(LIST_HEAD *head, unsigned long node)
{
    if (head == NULL || node == 0)
        return -1;

    unsigned long *p = head->head;
    if (p == NULL) {
        p = calloc(INIT_SIZE, sizeof(unsigned long));
        if (p == NULL)
            return -1;
        head->head = p;
        head->size = INIT_SIZE;
    }

    int i = 0;
    // must assure the last pos is always empty
    while (i < head->size - 1 && head->head[i] != 0) {
        if (head->head[i] == node)
            return 0;
        i++;
    }
    if (i < head->size - 1) {
        head->head[i] = node;
    } else {
        unsigned long *new_head = calloc(head->size * 2, sizeof(unsigned long));
        if (new_head == NULL)
            return -1;
        memcpy(new_head, head, head->size * sizeof(unsigned long));
        head->head = new_head;
        head->size *= 2;
        head->head[i] = node;
    }
    return 0;
}

unsigned long *get_sock_inodes(pid_t pid)
{
    char fd_dirpath[MAX_PATH_LEN] = {0};
    DIR *fd_dirp = NULL;
    struct dirent *ent = NULL;

    LIST_HEAD long_list;
    long_list.size = 0;
    long_list.head = NULL;

    sprintf(fd_dirpath, "/proc/%d/fd", pid);

    fd_dirp = opendir(fd_dirpath);
    if (fd_dirp == NULL)
        return NULL;

    int i = 0;
    while (ent = readdir(fd_dirp)) {
        char fd_path[MAX_PATH_LEN] = {0};
        struct stat st;

        sprintf(fd_path, "%s/%s", fd_dirpath, ent->d_name);
        if (stat(fd_path, &st) != 0)
            continue;
        if (S_ISSOCK(st.st_mode)) {
            printf("%s inode %ld\n", fd_path, st.st_ino);
            append_to_list(&long_list, st.st_ino);
        }
    }
    closedir(fd_dirp);
    return long_list.head;
}

RAW_SOCK get_raw_sock_info(unsigned long sock_inode)
{
    RAW_SOCK rs;
    unsigned long rx_queue = 0;
    unsigned long inode = 0;
    unsigned long drops = 0;

    memset(&rs, 0, sizeof(rs));

    FILE *fp = fopen("/proc/net/raw", "rb");
    if (fp == NULL)
        return rs;

    int read_head = 0;
    do {
        char buf[MAX_PATH_LEN] = {0};
        if (fgets(buf, sizeof(buf), fp) == NULL)
            break;
        if (!read_head) {
            read_head = 1;
            continue;
        }

        sscanf(buf, "%*s%*s%*s%*s%*[^:]:%lx%*s%*s%*s%*s%lu%*s%*s%lu", &rx_queue, &inode, &drops);
        if (inode == sock_inode) {
            rs.rx_queue = rx_queue;
            rs.inode = inode;
            rs.drops = drops;
            break;
        }
    } while (1);

    fclose(fp);
    return rs;
}