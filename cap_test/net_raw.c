#include<stdio.h>
#include <unistd.h>
#include<string.h> //memset
#include<sys/socket.h>
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include<netinet/tcp.h>	//Provides declarations for tcp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/prctl.h>
#include<sys/capability.h>

void drop_capability()
{
    cap_t caps;
    cap_value_t cap_list[1];

    caps = cap_get_proc();
    if (caps == NULL) {
        printf("get proc error\n");
        return;
    }

    cap_list[0] = CAP_NET_RAW;
    if (cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_list, CAP_CLEAR) == -1) {
        printf("set flag error!\n");
        return;
    }

    if (cap_set_proc(caps) == -1) {
        printf("set proc error!\n");
        return;
    }

    if (cap_free(caps) == -1) {
        printf("free caps error!\n");
        return;
    }
}


int main (int argc, char **argv)
{
    if (argc == 2)
        drop_capability();
    //Create a raw socket
    int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);

    if ( s > 0) {
        printf("create sock succeeds! fd = %d\n", s);
    } else {
        printf("create sock failed! fd = %d\n", s);
    }
    while (1) {
        sleep(1);
    }
    return 0;
}
