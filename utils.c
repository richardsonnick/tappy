#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <arpa/inet.h>

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
   return htonl((a << 24) | (b << 16) | (c << 8) | (d));
}

uint32_t to_ip_encoding(const ip_addr_t* ip_addr) {
   return htonl((ip_addr->a << 24) | (ip_addr->b << 16) | (ip_addr->c << 8) | (ip_addr->d));
}

/**
 * "The checksum field is the 16 bit one's complement of the one's
 *    complement sum of all 16 bit words in the header.  For purposes of
 *    computing the checksum, the value of the checksum field is zero."
 *  https://datatracker.ietf.org/doc/html/rfc791#section-3.1
 */
uint16_t compute_checksum(const ip_header_t* packet) {
    uint16_t checksum;
    uint8_t header[20];

    ip_header_t temp_packet = *packet;
    temp_packet.header_checksum = 0; // ensure the checksum is set to zero

    ip_header_to_buf(&temp_packet, header, 20);

    uint32_t sum = 0;

    // Process as 16 bit words
    for (int i = 0; i < 20; i += 2) {
        uint16_t word = (header[i] << 8) | header[i + 1];
        sum += word;
    }

    // carry bits
    // Takes bits added that exceed the 16 bit final width
    // and adds them back to the lower 16 bits.
    // This is necessary for the "one's complement sum"
    // https://datatracker.ietf.org/doc/html/rfc1071#section-1
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

size_t ip_header_to_buf(const ip_header_t* packet, uint8_t* buf, size_t buf_len) {
    if (buf_len < MIN_IP4_HEADER_SIZE) {
        return -1;
    }

    uint8_t i = 0;
    buf[i++] = ((packet->version & 0x0f) << 4) | (packet->ihl & 0x0F);
    buf[i++] = packet->type_of_service;

    uint16_t total_length = htons(packet->total_length);
    buf[i++] = (total_length & 0xFF);
    buf[i++] = (total_length >> 8);

    uint16_t identification = htons(packet->identification);
    buf[i++] = (identification & 0xFF); 
    buf[i++] = (identification >> 8);

    uint16_t flags_fragment = ((packet-> flags & 0x7) << 13) | (packet->fragment_offset & 0x1FFF);
    flags_fragment = htons(flags_fragment);
    buf[i++] = flags_fragment & 0xFF;
    buf[i++] = flags_fragment >> 8;

    buf[i++] = packet->time_to_live;
    buf[i++] = packet->protocol;

    uint16_t header_checksum = htons(packet->header_checksum);
    buf[i++] = (header_checksum & 0xFF); 
    buf[i++] = (header_checksum >> 8);

    // TODO: These look wrong... Why do i add the high bytes first here
    //  but have to add the lower bytes first in header_checksum??
    //   like why does this work... tcpdump shows the correct thing...
    //   this is how i was doing this before, but ran into issues with
    //   the parsing of total_length and identification fields...
    uint32_t source_address = htonl(packet->source_address);
    buf[i++] = (source_address >> 24);
    buf[i++] = (source_address >> 16) & 0xFF; 
    buf[i++] = (source_address >> 8) & 0xFF; 
    buf[i++] = (source_address & 0x00FF); 

    uint32_t destination_address = htonl(packet->destination_address);
    buf[i++] = (destination_address >> 24);
    buf[i++] = (destination_address >> 16) & 0xFF; 
    buf[i++] = (destination_address >> 8) & 0xFF; 
    buf[i++] = (destination_address & 0x00FF); 

    return i;
}

bool ip_buf_to_packet(const uint8_t* buf, size_t len, ip_header_t* out_packet) {
    if (len < MIN_IP4_HEADER_SIZE) {
        return false;
    }
    uint8_t i = 0;
    out_packet->version = (buf[i] >> 4); // version
    out_packet->ihl = (buf[i] & 0x0F); // ihl
    i++;
    out_packet->type_of_service = buf[i++];
    out_packet->total_length = ((uint16_t)buf[i++] << 8); 
    out_packet->total_length = (((uint16_t)buf[i++] & 0x00FF) | out_packet->total_length); 
    out_packet->identification = ((uint16_t)buf[i++] << 8); 
    out_packet->identification = (((uint16_t)buf[i++] & 0x00FF) | out_packet->identification); 

    uint8_t flag_fragment1 = buf[i++];
    uint8_t flag_fragment2 = buf[i++];
    out_packet->flags = (((uint8_t) flag_fragment1 >> 5));
    out_packet->fragment_offset = (((uint16_t) flag_fragment1 & 0x1F) << 8)
                                | ((uint16_t) flag_fragment2);

    out_packet->time_to_live = buf[i++];
    out_packet->protocol = buf[i++];
    out_packet->header_checksum = ((uint16_t)buf[i++] << 8); 
    out_packet->header_checksum = (((uint16_t)buf[i++] & 0x00FF) | out_packet->header_checksum); 
    out_packet->source_address = (((uint32_t) buf[i++]) << 24)
                                | ((uint32_t) buf[i++] << 16)
                                | ((uint32_t) buf[i++] << 8)
                                | ((uint32_t) buf[i++]);
    out_packet->destination_address = (((uint32_t) buf[i++]) << 24)
                                | ((uint32_t) buf[i++] << 16)
                                | ((uint32_t) buf[i++] << 8)
                                | ((uint32_t) buf[i++]);
    return true;
}

size_t tcp_packet_to_buf(const tcp_packet_t* packet, uint8_t* buf, size_t buf_len) {
    if (buf_len < MIN_TCP_PACKET_SIZE) {
        return -1;
    }

    uint8_t i = 0;
    uint16_t source_port = htons(packet->source_port);
    buf[i++] = (source_port & 0xFF); 
    buf[i++] = (source_port >> 8);

    uint16_t destination_port = htons(packet->destination_port);
    buf[i++] = (destination_port & 0xFF); 
    buf[i++] = (destination_port >> 8);

    uint32_t sequence_number = htonl(packet->sequence_number);
    buf[i++] = (sequence_number >> 24);
    buf[i++] = (sequence_number >> 16) & 0xFF; 
    buf[i++] = (sequence_number >> 8) & 0xFF; 
    buf[i++] = (sequence_number & 0x00FF); 

    uint32_t acknowledgment_number = htonl(packet->acknowledgment_number);
    buf[i++] = (acknowledgment_number >> 24);
    buf[i++] = (acknowledgment_number >> 16) & 0xFF; 
    buf[i++] = (acknowledgment_number >> 8) & 0xFF; 
    buf[i++] = (acknowledgment_number & 0x00FF); 

    buf[i++] = ((packet->data_offset & 0x0F) << 4) | (packet->reserved & 0x0F);

    buf[i++] = packet->flags;

    uint16_t window = htons(packet->window);
    buf[i++] = (window & 0xFF); 
    buf[i++] = (window >> 8);

    uint16_t checksum = htons(packet->checksum);
    buf[i++] = (checksum & 0xFF); 
    buf[i++] = (checksum >> 8);

    uint16_t urgent_pointer = htons(packet->urgent_pointer);
    buf[i++] = (urgent_pointer & 0xFF); 
    buf[i++] = (urgent_pointer >> 8);

    //...options

    //...data (variable length)
}