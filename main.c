#include <linux/if.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "server.h"
#include "client.h"
#include "types.h"

int main(int argc, char *argv[]){
    if (argc < 2) {
        printf("Invalid arg len\n");
        return -1;
    }

    int fd;
    if (argc < 4) {
        printf("Usage: %s <client|server> <source_ip> <destination_ip>\n", argv[0]);
        return -1;
    }
    ip_addr_t source_ip, destination_ip;
    sscanf(argv[2], "%hhu.%hhu.%hhu.%hhu", &source_ip.a, &source_ip.b, &source_ip.c, &source_ip.d);
    sscanf(argv[3], "%hhu.%hhu.%hhu.%hhu", &destination_ip.a, &destination_ip.b, &destination_ip.c, &destination_ip.d);

    // printf("Created tun fd:%d\n", fd);
    if (strcmp(argv[1], "client") == 0) {
        client_loop(&source_ip, &destination_ip, 8081, 8080);
    }
    else if (strcmp(argv[1], "server") == 0) {
        server_loop(&source_ip, &destination_ip, 8080, 8081);
    }

    close(fd);
}
