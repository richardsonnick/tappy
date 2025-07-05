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