#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include <sys/stat.h>
#include <ctype.h>

#define CLI_PATH  "audit_client"

struct audisp_header {
    unsigned int ver;
    unsigned int hlen;
    unsigned int type;
    unsigned int size;
};

// return the socket fd to receive data
int connect_audit(const char *server)
{
    int fd, len;
    struct sockaddr_un un;

    if (server == NULL) {
        return -1;
    }
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("create unix sock");
        return -1;
    }

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, CLI_PATH);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    (void)unlink(CLI_PATH);
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (chmod(CLI_PATH, S_IRWXU) != 0) {
        perror("chmod");
        close(fd);
        return -1;
    }

    // fill server address
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, server);

    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        perror("connect sock");
        close(fd);
        return -1;
    }

    return fd;
}

void display_au_header(struct audisp_header *ah)
{
    if (ah == NULL)
        return;
    printf("ver is %u\n", ah->ver);
    printf("hlen is %u\n", ah->hlen);
    printf("type is %u\n", ah->type);
    printf("size is %u\n", ah->size);
    return;
}


// every message is seperated by the newline '\n'

int main(int argc, char *argv[])
{
    int fd = -1;
    fd = connect_audit("/var/run/audispd_events");
    if (fd < 0) {
        printf("connect audit failed\n");
        return 1;
    }

    int is_dispather_header = 1;
    int header_len = 0;
    struct audisp_header au_head;
    int sum = 0;

    int msg_len = 0;
    while (1) {
        char buf[2] = {0};
        int len = recv(fd, buf, sizeof(buf)-1, 0);
        char c = buf[0];
        unsigned char *p = (unsigned char *)&au_head;
        if (is_dispather_header) {
            // printf("%x", c);
            p[header_len++] = c;
            if (header_len != sizeof(struct audisp_header)) {
                continue;
            } else {
                // printf("\n");
                header_len = 0;
                is_dispather_header = 0;
                display_au_header(&au_head);
                msg_len = au_head.size;
            }
        }
        if (!isprint(c) && c != '\n')
            continue;
        if (sum++ < msg_len) {
            printf("%c", c);
        }

        if (c == '\n') {
            printf("\n");
            sum = 0;
            msg_len = 0;
            is_dispather_header = 1;
            memset(&au_head, 0, sizeof(struct audisp_header));
        }
    }
}
