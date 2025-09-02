#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "utils.h"

#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

void print_ip_header(const ip_header_t* ip_header) {
    printf("    version: %d\n", ip_header->version);
    printf("    ihl: %d\n", ip_header->ihl);
    printf("    type_of_service: %d\n", ip_header->type_of_service);
    printf("    total_length: %d\n", ip_header->total_length);
    printf("    identification: %d\n", ip_header->identification);
    printf("    flags: %02x\n", ip_header->flags);
    printf("    fragment_offset: %d\n", ip_header->fragment_offset);
    printf("    time_to_live: %d\n", ip_header->time_to_live);
    printf("    protocol: %d\n", ip_header->protocol);
    printf("    header_checksum: %04x\n", ip_header->header_checksum);
    ip_addr_t source_ip = from_ip_encoding(ip_header->source_address);
    ip_addr_t destination_ip = from_ip_encoding(ip_header->destination_address);
    printf("    source ip: %d.%d.%d.%d\n", source_ip.a, source_ip.b, source_ip.c, source_ip.d);
    printf("    destination ip: %d.%d.%d.%d\n", destination_ip.a, destination_ip.b, destination_ip.c, destination_ip.d);
}

void print_tcp_packet(const tcp_packet_t* packet) {
    printf("    source_port: %u\n", packet->source_port);
    printf("    destination_port: %u\n", packet->destination_port);
    printf("    sequence_number: %u\n", packet->sequence_number);
    printf("    acknowledgment_number: %u\n", packet->acknowledgment_number);
    printf("    data_offset: %u\n", packet->data_offset);
    printf("    reserved: %u\n", packet->reserved);
    
    // Print flags as readable strings
    printf("    flags: %02x (", packet->flags);
    int first_flag = 1;
    if (packet->flags & TCP_FLAG_FIN) {
        if (!first_flag) printf("|");
        printf("FIN");
        first_flag = 0;
    }
    if (packet->flags & TCP_FLAG_SYN) {
        if (!first_flag) printf("|");
        printf("SYN");
        first_flag = 0;
    }
    if (packet->flags & TCP_FLAG_RST) {
        if (!first_flag) printf("|");
        printf("RST");
        first_flag = 0;
    }
    if (packet->flags & TCP_FLAG_PSH) {
        if (!first_flag) printf("|");
        printf("PSH");
        first_flag = 0;
    }
    if (packet->flags & TCP_FLAG_ACK) {
        if (!first_flag) printf("|");
        printf("ACK");
        first_flag = 0;
    }
    if (packet->flags & TCP_FLAG_URG) {
        if (!first_flag) printf("|");
        printf("URG");
        first_flag = 0;
    }
    if (packet->flags & TCP_FLAG_ECE) {
        if (!first_flag) printf("|");
        printf("ECE");
        first_flag = 0;
    }
    if (packet->flags & TCP_FLAG_CWR) {
        if (!first_flag) printf("|");
        printf("CWR");
        first_flag = 0;
    }
    if (first_flag) printf("NONE");
    printf(")\n");
    
    printf("    window: %u\n", packet->window);
    printf("    checksum: %04x\n", packet->checksum);
    printf("    urgent_pointer: %u\n", packet->urgent_pointer);
    // ... data
    if (packet->data && packet->data_len > 0) {
        printf("    data_len: %ld\n", packet->data_len);
        printf("    data (hex): ");
        for (size_t i = 0; i < packet->data_len; i++) {
            printf("%02x ", packet->data[i]);
            if((i + 1) % 16 == 0) printf("\n                "); // Newline every 16 bytes
        }
        printf("\n");
    } else {
        printf("    data: none\n");
    }
}

// Prints the buf with the ip and tcp parts delimited
void print_raw_buf(const uint8_t* buf, const size_t buf_len) {
    // Assuming the buf starts with the ip header:
    uint8_t ip_header_len = ((uint16_t)buf[0] & 0x0F) * 4;  // ihl repr num of 32 bit (4 byte) words 
    printf("IP header len: %d\n", ip_header_len);
    for (int i = 0; i < buf_len; i++) {
        if (i < ip_header_len) {
            // Use green color
            printf(COLOR_GREEN "%02x " COLOR_RESET, buf[i]);
        } else {
            // use red color
            printf(COLOR_RED "%02x " COLOR_RESET, buf[i]);
        }
    }
    printf("\n");
}

/**
 * Allocates a tun driver
 * https://www.kernel.org/doc/Documentation/networking/tuntap.txt
 */
int tun_alloc(char *dev) {
    /**
     * Interface metadata struct (ifr)
     * https://man7.org/linux/man-pages/man7/netdevice.7.html
     */
    struct ifreq ifr; 

    int fd, err;

    if ( (fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Error opening /dev/net/tun");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr)); 

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; // Use TUN and skip packet info header
    if (*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if ( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}

uint32_t to_ip_encoding_decomposed(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d) {
   return ((a << 24) | (b << 16) | (c << 8) | (d));
}

uint32_t to_ip_encoding(const ip_addr_t* ip_addr) {
   return ((ip_addr->a << 24) | (ip_addr->b << 16) | (ip_addr->c << 8) | (ip_addr->d));
}

ip_addr_t from_ip_encoding(const uint32_t ip){
    ip_addr_t ip_addr = {
        .a = (uint8_t)(ip >> 24),
        .b = (uint8_t)(ip >> 16),
        .c = (uint8_t)(ip >> 8),
        .d = (uint8_t)ip,
    };
    return ip_addr;
}

const char* tcp_state_to_string(TCP_STATE state) {
    switch(state) {
        case LISTEN: return "LISTEN";
        case SYN_SENT: return "SYN_SENT";
        case SYN_RECEIVED: return "SYN_RECEIVED";
        case ESTABLISHED: return "ESTABLISHED";
        case FIN_WAIT_1: return "FIN_WAIT_1";
        case FIN_WAIT_2: return "FIN_WAIT_2";
        case CLOSE_WAIT: return "CLOSE_WAIT";
        case CLOSING: return "CLOSING";
        case LAST_ACK: return "LAST_ACK";
        case TIME_WAIT: return "TIME_WAIT";
        case CLOSED: return "CLOSED";
        default: return "UNKNOWN";
    }
}

void print_conn(const tcp_connection_t* conn, const char* context) {
    printf("%s - Current state: %s\n", context, tcp_state_to_string(conn->state));
}