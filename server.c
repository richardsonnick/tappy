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
#include "server_state_machine.h"

void server_loop(ip_addr_t* source_ip, ip_addr_t* destination_ip, 
    int src_port,
    int dst_port) {
    printf("Running server...\n");
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket creation failed");
    }
    
    uint8_t buf[4096];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    tcp_connection_t* conn = init_tcp_stack(source_ip,
        destination_ip, 
        src_port,
        dst_port, LISTEN);

    while (1) {
        // TODO: Make this read op a callback.
        ssize_t buf_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                            (struct sockaddr*)&sender_addr, &sender_len);
        if (buf_len < 0) {
            perror("recvfrom failed");
            continue; // just keep failing.
        }

        tcp_ip_t* tcp_ip = malloc(sizeof(tcp_ip_t));
        tcp_ip->ip_header = malloc(sizeof(ip_header_t));
        tcp_ip->tcp_packet = malloc(sizeof(tcp_packet_t));
        ip_buf_to_packet(buf, 20, tcp_ip->ip_header);
        size_t tcp_len = tcp_ip->ip_header->total_length - MIN_IP4_HEADER_SIZE;
        tcp_buf_to_packet(buf + 20, tcp_len, tcp_ip->tcp_packet);

        if (tcp_ip->tcp_packet->source_port == src_port && 
            tcp_ip->tcp_packet->destination_port == dst_port) {
            // RECEIVE event
            conn->state = server_handle_event(conn, RECEIVE, tcp_ip);

            printf("Got TCP packet:\n");
            printf("IP Header:\n");
            print_ip_header(tcp_ip->ip_header);
            printf("TCP Packet:\n");
            print_tcp_packet(tcp_ip->tcp_packet);
            printf("Buffer:\n");
            print_raw_buf(buf, buf_len);
        }
        free(tcp_ip->ip_header);
        free(tcp_ip->tcp_packet);
        free(tcp_ip);
    }
    close(sockfd);

}