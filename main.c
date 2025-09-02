#include <linux/if.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"
#include "server.h"
#include "client.h"
#include "types.h"

#define CLIENT_PORT 60000
#define SERVER_PORT 60001

int main(int argc, char *argv[]){
    if (argc < 2) {
        printf("Invalid arg len\n");
        return -1;
    }

    int fd;
    if (argc < 4) {
        printf("Usage: %s <client|server> <source_ip> <destination_ip> [--pipe]\n", argv[0]);
        return -1;
    }
    ip_addr_t source_ip, destination_ip;
    sscanf(argv[2], "%hhu.%hhu.%hhu.%hhu", &source_ip.a, &source_ip.b, &source_ip.c, &source_ip.d);
    sscanf(argv[3], "%hhu.%hhu.%hhu.%hhu", &destination_ip.a, &destination_ip.b, &destination_ip.c, &destination_ip.d);

    bool pipe_mode = false;
    if (argc >= 5 && strcmp(argv[4], "--pipe") == 0) {
        pipe_mode = true;
    }

    // printf("Created tun fd:%d\n", fd);
    if (strcmp(argv[1], "client") == 0) {
        client_loop(&source_ip, &destination_ip, CLIENT_PORT, SERVER_PORT, pipe_mode);
    }
    else if (strcmp(argv[1], "server") == 0) {
        server_loop(&source_ip, &destination_ip, SERVER_PORT, CLIENT_PORT);
    }

    close(fd);
}
