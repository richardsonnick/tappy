#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "state_machine.h"
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
#include <stdbool.h>
#include "io.h"
#include "client_state_machine.h"

#include "client.h"

// TODO make this loop common bw server and client
void client_loop(ip_addr_t* source_ip, ip_addr_t* destination_ip, 
    int src_port,
    int dst_port, bool pipe_mode) {
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

    bool sent_data = 0;
    char* stdin_buffer = NULL;
    size_t stdin_buffer_size = 0;
    ssize_t stdin_data_len = 0;
    
    if (pipe_mode) {
        stdin_buffer_size = 4096;
        stdin_buffer = malloc(stdin_buffer_size);
        size_t total_read = 0;
        ssize_t bytes_read;
        
        while ((bytes_read = read(STDIN_FILENO, stdin_buffer + total_read, stdin_buffer_size - total_read - 1)) > 0) {
            total_read += bytes_read;
            if (total_read >= stdin_buffer_size - 1) {
                // Resize buffer if needed
                stdin_buffer_size *= 2;
                stdin_buffer = realloc(stdin_buffer, stdin_buffer_size);
            }
        }
        stdin_buffer[total_read] = '\0';  // Null terminate
        stdin_data_len = total_read;
        
        if (stdin_data_len == 0) {
            free(stdin_buffer);
            close(sockfd);
            return;
        }
    }

    // LISTENing loop
    while (1) {

        if (conn->state == ESTABLISHED && !sent_data) {
            if (pipe_mode && stdin_buffer) {
                printf("Sending piped data (%ld bytes)\n", stdin_data_len);
                simple_send(conn, TCP_FLAG_PSH | TCP_FLAG_ACK, (const uint8_t*)stdin_buffer, stdin_data_len);
            } else {
                // Default test data when not in pipe mode
                const char* test_data = "Hey man";
                printf("Sending data: %s\n", test_data);
                simple_send(conn, TCP_FLAG_PSH | TCP_FLAG_SYN ,(const uint8_t*)test_data, strlen(test_data));
            }
          sent_data = 1;
        }

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
        size_t tcp_len = buf_len - MIN_IP4_HEADER_SIZE;
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
    
    // Clean up stdin buffer if it was allocated
    if (pipe_mode && stdin_buffer) {
        free(stdin_buffer);
    }
    
    close(sockfd);

}
