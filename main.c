#include <linux/if.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "server.h"
#include "client.h"

int main(int argc, char *argv[]){
    if (argc < 2) {
        printf("Invalid arg len\n");
        return -1;
    }

    int fd;

    // printf("Created tun fd:%d\n", fd);
    if (strcmp(argv[1], "client") == 0) {
        char dev_client[IFNAMSIZ] = "tun0";
        if ( (fd = tun_alloc(dev_client)) < 0) {
            printf("Failed to open tun device\n");
            return -1;
        }
        // client_loop(fd);
        send_syn(fd);
    }
    else if (strcmp(argv[1], "server") == 0) {
        char dev_server[IFNAMSIZ] = "tun1";
        if ( (fd = tun_alloc(dev_server)) < 0) {
            printf("Failed to open tun device\n");
            return -1;
        }
        server_loop(8080, 8081);
    }

    close(fd);
}