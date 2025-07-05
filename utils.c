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