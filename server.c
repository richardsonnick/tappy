#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include "server.h"
#include "io.h"

void server_loop(int port) {
    printf("Running server...\n");
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket creation failed");
    }
    
    uint8_t buf[4096];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (1) {
        ssize_t packet_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                            (struct sockaddr*)&sender_addr, &sender_len);
        if (packet_len < 0) {
            perror("recvfrom failed");
            continue; // just keep failing.
        }

        struct iphdr* ip_header = (struct iphdr*)buf;
        if (ip_header->protocol != IPPROTO_TCP) continue;
        
        struct tcphdr* tcp_header = (struct tcphdr*)(buf + ip_header->ihl * 4);

        if (ntohs(tcp_header->dest) == port) {
            printf("Got TCP packet for port %d from %s:%d\n",
                port,
                inet_ntoa(sender_addr.sin_addr),
                ntohs(tcp_header->source));
            if (tcp_header->syn) {
                printf("  -> SYN packet");
            }
        }

    }
    close(sockfd);

}