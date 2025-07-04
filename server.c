#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdlib.h>
#include "server.h"
#include "io.h"
#include "utils.h"
#include "tcp.h"

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
        ssize_t buf_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                            (struct sockaddr*)&sender_addr, &sender_len);
        if (buf_len < 0) {
            perror("recvfrom failed");
            continue; // just keep failing.
        }

        print_raw_buf(buf, buf_len);
        tcp_ip_t* tcp_ip = malloc(sizeof(tcp_ip_t));
        tcp_ip->ip_header = malloc(sizeof(ip_header_t));
        ip_buf_to_packet(buf, 20, tcp_ip->ip_header);

        size_t got_tcp_len = tcp_ip_from_buf(buf, buf_len, tcp_ip);
        if (got_tcp_len > 1) {
            printf("Got TCP packet:\n");
            print_ip_header(tcp_ip->ip_header);
        }
        free(tcp_ip->ip_header);
        free(tcp_ip->tcp_packet);
        free(tcp_ip);
    }
    close(sockfd);

}