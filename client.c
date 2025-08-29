#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include <stdlib.h>
#include "utils.h"
#include "tcp.h"
#include "string.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include "io.h"
#include "client_state_machine.h"

#include "client.h"

void send_syn(int netdev_fd, ip_addr_t* source_ip, ip_addr_t* destination_ip) {
    printf("Sending syn... ");
    tcp_connection_t* conn = init_tcp_stack(source_ip, destination_ip,
                8080,  //source port
                8081, // destination port
                CLOSED);
    TCP_STATE state = client_handle_event(conn, OPEN, NULL);
}

// TODO make this loop common bw server and client
void client_loop(ip_addr_t* source_ip, ip_addr_t* destination_ip, 
    int src_port,
    int dst_port) {
    printf("Running client...\n");
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
        dst_port, CLOSED);

    conn->state = client_handle_event(conn, OPEN, NULL);

    // LISTENing loop
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

        // The condition here is prob unneccsary i imagine the kernel does this for me.
        if (tcp_ip->tcp_packet->source_port == dst_port && 
            tcp_ip->tcp_packet->destination_port == src_port) {
            // RECEIVE event
            conn->state = client_handle_event(conn, RECEIVE, tcp_ip);

            // SILENCE!
            if (0) {
              printf("Got TCP packet:\n");
              printf("IP Header:\n");
              print_ip_header(tcp_ip->ip_header);
              printf("TCP Packet:\n");
              print_tcp_packet(tcp_ip->tcp_packet);
              printf("Buffer:\n");
              print_raw_buf(buf, buf_len);
            }
        }
        // TODO try to implement without costly frees
        free(tcp_ip->ip_header);
        free(tcp_ip->tcp_packet);
        free(tcp_ip);
    }
    close(sockfd);

}
