#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include "libproc.h"

#define MAX_PATH_LEN 1024
#define MAX_LINE_LEN 1024
#define INIT_SIZE    16

extern int errno;

typedef struct long_list {
    unsigned long *head;
    int size;
} LONG_LIST;

typedef struct str_list {
    char **head;
    int size;
} STR_LIST;

typedef struct char_list {
    char *head;
    int size;
} CHAR_LIST;

int append_to_long_list(LONG_LIST *long_list, unsigned long node)
{
    if (long_list == NULL || node == 0)
        return -1;

    int ent_size = sizeof(unsigned long);

    unsigned long *p = long_list->head;
    if (p == NULL) {
        p = calloc(INIT_SIZE, ent_size);
        if (p == NULL)
            return -1;
        long_list->head = p;
        long_list->size = INIT_SIZE;
    }

    int i = 0;
    // must assure the last pos is always empty
    while (i < long_list->size - 1 && long_list->head[i] != 0) {
        if (long_list->head[i] == node)
            return 0;
        i++;
    }
    if (i < long_list->size - 1) {
        long_list->head[i] = node;
    } else {
        unsigned long *new_head = calloc(long_list->size * 2, ent_size);
        if (new_head == NULL)
            return -1;
        memcpy(new_head, long_list->head,
               long_list->size * ent_size);
        free(long_list->head);
        long_list->head = new_head;
        long_list->size *= 2;
        long_list->head[i] = node;
    }
    return 0;
}

int append_to_str_list(STR_LIST *str_list, char *node)
{
    if (str_list == NULL || node == 0)
        return -1;

    int ent_size = sizeof(char *);

    char **p = str_list->head;
    if (p == NULL) {
        p = (char **)calloc(INIT_SIZE, ent_size);
        if (p == NULL)
            return -1;
        str_list->head = p;
        str_list->size = INIT_SIZE;
    }

    int i = 0;
    // must assure the last pos is always empty
    while (i < str_list->size - 1 && str_list->head[i] != NULL) {
        if (strcmp(str_list->head[i], node) == 0)
            return 0;
        i++;
    }
    if (i < str_list->size - 1) {
        str_list->head[i] = strdup(node);
    } else {
        char **new_head = calloc(str_list->size * 2, ent_size);
        if (new_head == NULL)
            return -1;
        memcpy(new_head, str_list->head,
               str_list->size * ent_size);
        free(str_list->head);
        str_list->head = new_head;
        str_list->size *= 2;
        str_list->head[i] = strdup(node);
    }
    return 0;
}

int append_to_char_list(CHAR_LIST *my_list, char node)
{
    if (my_list == NULL || node == 0)
        return -1;

    int ent_size = sizeof(char);

    char *p = my_list->head;
    if (p == NULL) {
        p = (char *)calloc(INIT_SIZE, ent_size);
        if (p == NULL)
            return -1;
        my_list->head = p;
        my_list->size = INIT_SIZE;
    }

    int i = 0;
    // must assure the last pos is always empty
    while (i < my_list->size - 1 && my_list->head[i] != 0) {
        i++;
    }

    if (i < my_list->size - 1) {
        my_list->head[i] = node;
    } else {
        char *new_head = calloc(my_list->size * 2, ent_size);
        if (new_head == NULL)
            return -1;
        memcpy(new_head, my_list->head,
               my_list->size * ent_size);
        free(my_list->head);
        my_list->head = new_head;
        my_list->size *= 2;
        my_list->head[i] = node;
    }
    return 0;
}

unsigned long *get_sock_inodes(pid_t pid)
{
    char fd_dirpath[MAX_PATH_LEN] = {0};
    DIR *fd_dirp = NULL;
    struct dirent *ent = NULL;

    LONG_LIST long_list;
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
            append_to_long_list(&long_list, st.st_ino);
        }
    }
    closedir(fd_dirp);
    return long_list.head;
}

RAW_SOCK get_raw_sock_info(unsigned long sock_inode)
{
    RAW_SOCK rs;
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

        sscanf(buf, "%*s%*s%*s%*s%*[^:]:%lx%*s%*s%*s%*s%lu%*s%*s%lu",
               &(rs.rx_queue), &(rs.inode), &(rs.drops));
        if (rs.inode == sock_inode)
            break;
        memset(&rs, 0, sizeof(rs));
    } while (1);

    fclose(fp);
    return rs;
}

TCP_SOCK get_tcp_sock_info(unsigned long sock_inode)
{
    TCP_SOCK ts;
    memset(&ts, 0, sizeof(ts));

    FILE *fp = fopen("/proc/net/tcp", "r");
    if (fp == NULL)
        return ts;

    int read_head = 0;

    do {
        char buf[MAX_PATH_LEN] = {0};
        if (fgets(buf, sizeof(buf), fp) == NULL)
            break;
        if (!read_head) {
            read_head = 1;
            continue;
        }

        sscanf(buf, "%*s%lx:%x%lx:%x%x%*s%*s%*s%*s%*s%lu",
               &(ts.local_ip), &(ts.local_port),
               &(ts.remote_ip), &(ts.remote_port),
               &(ts.state), &(ts.inode));
        if (ts.inode == sock_inode)
            break;
        memset(&ts, 0, sizeof(ts));
    } while (1);

    fclose(fp);
    return ts;
}


char **read_proc_file_to_list(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

    STR_LIST str_list;
    str_list.size = 0;
    str_list.head = NULL;

    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
        return NULL;

    char cmd_part[MAX_LINE_LEN] = {0};

    int j = 0;

    do {
        ssize_t len = 0;
        char buf[MAX_LINE_LEN] = {0};
        if ((len = read(fd, buf, MAX_LINE_LEN)) <= 0)
            break;
        int i = 0;
        for ( ; i < len; ++i) {
            if (buf[i] == 0) {
                if (strlen(cmd_part) != 0) {
                    append_to_str_list(&str_list, cmd_part);
                    memset(cmd_part, 0, sizeof(cmd_part));
                    j = 0;
                }
            } else {
                if ( j < MAX_LINE_LEN )
                    cmd_part[j++] = buf[i];
            }
        }
    } while (1);
    close(fd);
    return str_list.head;
}

char *read_proc_file(char *file_path)
{
    if (file_path == NULL)
        return NULL;

    CHAR_LIST char_list;
    char_list.size = 0;
    char_list.head = NULL;

    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
        return NULL;

    do {
        ssize_t len = 0;
        char buf[MAX_LINE_LEN] = {0};
        if ((len = read(fd, buf, MAX_LINE_LEN)) <= 0)
            break;
        int i = 0;
        for ( ; i < len; ++i) {
            if (buf[i] == 0) {
                append_to_char_list(&char_list, ' ');
            } else {
                append_to_char_list(&char_list, buf[i]);
            }
        }
    } while (1);
    close(fd);
    return char_list.head;
}

char *parse_from_file(const char *file_path,
                      const char *item, const char *format)
{
    if (file_path == NULL || item == NULL || format == NULL)
        return NULL;

    FILE *fp = fopen(file_path, "r");
    if (fp == NULL)
        return NULL;

    char *match_str = NULL;
    do {
        char line[MAX_LINE_LEN] = {0};
        char buf[MAX_LINE_LEN] = {0};
        if (fgets(line, sizeof(line), fp) == NULL)
            break;
        if (strncmp(line, item, strlen(item)) != 0)
            continue;
        sscanf(line, format, buf);
        match_str = strdup(buf);
        break;
    } while (1);

    return match_str;
}


char *parse_from_status(pid_t pid, char *item)
{
    if (item == NULL)
        return NULL;

    char status_path[MAX_PATH_LEN] = {0};
    sprintf(status_path, "/proc/%d/status", pid);
    return parse_from_file(status_path, item,"%*s%s");
}

char *get_real_path(char *file_path)
{
    if (file_path == NULL)
        return NULL;

    char real_path[PATH_MAX + 1] = {0};
    (void)realpath(file_path, real_path);
    if (errno == 0 || errno == ENOENT)
        return strdup(real_path);
    return NULL;
}

char *get_proc_name(pid_t pid)
{
    return parse_from_status(pid, "Name:");
}

pid_t get_proc_ppid(pid_t pid)
{
    char *ppid_str = parse_from_status(pid, "PPid:");
    if (ppid_str == NULL)
        return 0;
    pid_t ppid = atoi(ppid_str);
    free(ppid_str);
    ppid_str = NULL;
    return ppid;
}

char *get_proc_exe(pid_t pid)
{
    char exe_path[MAX_PATH_LEN] = {0};
    sprintf(exe_path, "/proc/%d/exe", pid);
    return get_real_path(exe_path);
}

char *get_proc_cmdline(pid_t pid)
{
    char cmdline_path[MAX_PATH_LEN] = {0};
    sprintf(cmdline_path, "/proc/%d/cmdline", pid);
    return read_proc_file(cmdline_path);
}

char **get_proc_cmdline_list(pid_t pid)
{
    char cmdline_path[MAX_PATH_LEN] = {0};
    sprintf(cmdline_path, "/proc/%d/cmdline", pid);
    return read_proc_file_to_list(cmdline_path);
}