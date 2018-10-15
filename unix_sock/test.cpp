#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include <sys/stat.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>

#include <string>
#include <vector>

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

void handle_data_stream(char c)
{
    static bool msg_enable = false;
    static int header_len = 0;
    static struct audisp_header au_head;
    static int sum = 0;
    static std::string log;

    static std::vector<unsigned char> c_vec;

    static int msg_len = 0;
    unsigned char *p = (unsigned char *)&au_head;
    if (!msg_enable) {
        if (c_vec.size() == sizeof(struct audisp_header)) {
            c_vec.erase(c_vec.begin());
        }

        c_vec.push_back(c);
        if (c_vec.size() != sizeof(struct audisp_header)) {
            return;
        } else {
            for (size_t i = 0; i < c_vec.size(); ++i) {
                p[i] = c_vec[i];
            }
            if (au_head.hlen == sizeof(au_head)) {
                msg_enable = true;
                //display_au_header(&au_head);
                msg_len = au_head.size;
                c_vec.clear();
                printf("msg_type %d:", au_head.type);
            }
        }
    } else {
        if (sum++ < msg_len) {
            log.push_back(c);
        } else {
            //printf("read data ok!\n");
            printf("%s\n", log.c_str());
            log.clear();
            sum = 0;
            msg_len = 0;
            msg_enable = false;
        }
    }
}

void write_to_file(char *buf , int len)
{
    FILE *fp = fopen("audit.log", "a+");
    fwrite(buf, 1, len, fp);
    fflush(fp);
    fclose(fp);
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

    nfds_t nfds = 1;
    int timeout = 2000;
    struct pollfd fds;
    memset(&fds, 0, sizeof(fds));
    fds.fd = fd;
    fds.events = POLLIN;

    while (1) {
        int n = poll(&fds, nfds, timeout);
        if (n == -1) {
            printf("poll error. errno %d", errno);
        }
        if (n == 0) {
            continue;
        }

        char buf[2048] = {0};
        int len = 0;
        while (len = recv(fd, buf, sizeof(buf)-1, MSG_DONTWAIT)) {
            if (len < 0)
                break;
            //write_to_file(buf, len);
            int i = 0;
            for ( ; i < len; ++i) {
                handle_data_stream(buf[i]);
            }
        }
    }
}