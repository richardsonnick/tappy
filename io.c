#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "types.h"
#include "utils.h"
#include "tcp.h"
#include "string.h"
#include "client.h"
#include "utils.h"
#include "io.h"

int create_raw_socket() {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("raw socket creation failed");
        return -1;
    }

    int one = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt IP_HDRINCL failed");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void send_packet_raw(int sockfd, uint8_t* buf, size_t buf_len,
                    const char* dest_ip) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = 0; // I dont think this is used for raw socks

    inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);
    ssize_t sent = sendto(sockfd, buf, buf_len, 0,
                        (struct sockaddr*)&dest_addr, sizeof(dest_addr)
    );

    if(sent < 0) {
        perror("sendto failed");
    }
}

void write_to_dev(int netdev_fd, uint8_t* buf, size_t buf_len) {
    // TODO: gate this with debug
    print_raw_buf(buf, buf_len);
    ssize_t written = write(netdev_fd, buf, buf_len);
    if (written != buf_len) {
        perror("write");
    }
}

void read_from_dev(int netdev_fd, uint8_t* buf, size_t buf_len) {
    ssize_t read_bytes = read(netdev_fd, buf, buf_len);
    if (read_bytes < 0) {
        perror("read");
        return;
    }
    // TODO check if tcp packets
    if (read_bytes >= 20) {
        uint8_t version = (buf[0] >> 4) & 0x0F;
        if (version == 4) {
            uint8_t protocol = buf[9];
            if (protocol == 0x06) { // TCP
                printf("Got TCP packet...\n");
                print_raw_buf(buf, (size_t) read_bytes);
            } else {
                printf("Non-TCP IPv4 protocol: %u\n", protocol);
            }
        } else {
            printf("Got IPv6. Skipping\n");
        }
    }
}