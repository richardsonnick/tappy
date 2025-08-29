#include "state_machine.h"
#include "types.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "hash/sha1_c.h" // boogie!

#include "utils.h"

#include "tcp.h"
#include "io.h"

static uint32_t sha1_hash(uint32_t local_ip, uint16_t local_port,
    uint32_t remote_ip, uint16_t remote_port, uint32_t secret_key) {
      // TODO use boogie::sha1;
      uint8_t input[18];

      input[0] = (local_ip >> 24) & 0xFF;
      input[1] = (local_ip >> 16) & 0xFF;
      input[2] = (local_ip >> 8) & 0xFF;
      input[3] = local_ip & 0xFF;

      input[4] = (local_port >> 8) & 0xFF;
      input[5] = local_port & 0xFF;

      input[6] = (remote_ip >> 24) & 0xFF;
      input[7] = (remote_ip >> 16) & 0xFF;
      input[8] = (remote_ip >> 8) & 0xFF;
      input[9] = remote_ip & 0xFF;

      input[10] = (remote_port >> 8) & 0xFF;
      input[11] = remote_port & 0xFF;

      input[12] = (secret_key >> 24) & 0xFF;
      input[13] = (secret_key >> 16) & 0xFF;
      input[14] = (secret_key >> 8) & 0xFF;
      input[15] = secret_key & 0xFF;

      input[16] = 0x00; // Padding
      input[17] = 0x00; // Padding

      char hash_output[41]; // SHA-1 produces a 20-byte hash hex string + null term
      int result = sha1_hash_data(input, sizeof(input), hash_output);
      if (result != 0) {
          fprintf(stderr, "SHA1 hash failed\n");
          return 0;
      }

      char hex_substr[9];
      strncpy(hex_substr, hash_output, 8);
      hex_substr[8] = '\0';
    
      uint32_t hash_value = (uint32_t)strtoul(hex_substr, NULL, 16);

      return hash_value;
}

// From https://www.rfc-editor.org/rfc/rfc6528.txt
uint32_t generate_isn(const tcb_t* tcb) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint32_t M = (tv.tv_sec * 1000000 + tv.tv_usec);
  static uint32_t secret_key = 0x12345678; // In practice, this should be random and secret. But I am lazy.
  uint32_t local_ip = to_ip_encoding(tcb->source_ip);
  uint32_t remote_ip = to_ip_encoding(tcb->destination_ip);
  uint32_t F = sha1_hash(to_ip_encoding(tcb->source_ip), tcb->source_port,
      to_ip_encoding(tcb->destination_ip), tcb->destination_port, secret_key);
  uint32_t isn =  M + F;
  return isn;
};

tcp_connection_t* init_tcp_stack(ip_addr_t* source_ip, ip_addr_t* destination_ip,
                    const uint16_t source_port, const uint16_t destination_port, TCP_STATE init_state) {
    tcp_connection_t* conn = malloc(sizeof(tcp_connection_t));
    tcb_t* tcb = malloc(sizeof(tcb_t));
    if (!tcb) return NULL;

    tcb->recv_buf.data = calloc(TCP_BUF_SIZE, sizeof(uint8_t));
    tcb->recv_buf.length = TCP_BUF_SIZE;
    tcb->recv_buf.read_pos = 0;
    tcb->recv_buf.write_pos = 0;
    tcb->send_buf.data = calloc(TCP_BUF_SIZE, sizeof(uint8_t));
    tcb->send_buf.length = TCP_BUF_SIZE;
    tcb->send_buf.read_pos = 0;
    tcb->send_buf.write_pos = 0;
    tcb->source_ip = source_ip;
    tcb->destination_ip = destination_ip;
    tcb->source_port = source_port;
    tcb->destination_port = destination_port;
    tcb->seq_num = generate_isn(tcb);
    tcb->ack_num = 0;

    conn->tcb = tcb;

    conn->state = init_state;

    return conn;
}

tcp_ip_t* make_packet(const tcb_t* tcb, const uint8_t flags, const uint8_t* data, const size_t data_len) {
        tcp_ip_t* tcp_ip = calloc(1, sizeof(tcp_ip_t));
        tcp_ip->ip_header = calloc(1, sizeof(ip_header_t));
        tcp_ip->tcp_packet = calloc(1, sizeof(tcp_packet_t));
        uint32_t src_ip_encoded = to_ip_encoding(tcb->source_ip);
        uint32_t dst_ip_encoded = to_ip_encoding(tcb->destination_ip);
        size_t total_tcp_len = MIN_TCP_PACKET_SIZE + data_len;

        tcp_ip->ip_header->version = 4;
        tcp_ip->ip_header->ihl = 5; // header length in 32 bit (4 bytes) words i.e. ihl == 5 implies (5 * 4) 20 byte header
        tcp_ip->ip_header->type_of_service = 0x66;
        tcp_ip->ip_header->total_length = (uint16_t)MIN_IP4_HEADER_SIZE + MIN_TCP_PACKET_SIZE;
        tcp_ip->ip_header->identification = 0x7777; // TODO: Fill with actual value
        tcp_ip->ip_header->flags = 0x00;
        tcp_ip->ip_header->fragment_offset = 0x0000;
        tcp_ip->ip_header->time_to_live = 69;
        tcp_ip->ip_header->protocol = 0x06; // TCP
        tcp_ip->ip_header->header_checksum = 0x00;
        tcp_ip->ip_header->source_address = src_ip_encoded;
        tcp_ip->ip_header->destination_address = dst_ip_encoded;
        tcp_ip->ip_header->header_checksum = compute_ip_checksum(tcp_ip->ip_header);
        tcp_ip->ip_header->total_length = (uint16_t)(MIN_IP4_HEADER_SIZE + total_tcp_len);


        tcp_ip->tcp_packet->source_port = tcb->source_port;
        tcp_ip->tcp_packet->destination_port = tcb->destination_port;
        tcp_ip->tcp_packet->sequence_number = tcb->seq_num;
        tcp_ip->tcp_packet->acknowledgment_number = tcb->ack_num;
        tcp_ip->tcp_packet->data_offset = (MIN_TCP_PACKET_SIZE / 4); // data offset in 4 byte words (words == 32 bits)
        tcp_ip->tcp_packet->reserved = 0x00;
        tcp_ip->tcp_packet->flags = flags;
        tcp_ip->tcp_packet->window = 1024;
        tcp_ip->tcp_packet->checksum = 0x00;
        tcp_ip->tcp_packet->urgent_pointer = 0x00;
        tcp_ip->tcp_packet->data_len = 0x00;

        if (data && data_len > 0) {
          tcp_ip->tcp_packet->data = malloc(data_len);
          memcpy(tcp_ip->tcp_packet->data, data, data_len);
          tcp_ip->tcp_packet->data_len = data_len;
        } else {
          tcp_ip->tcp_packet->data = NULL;
          tcp_ip->tcp_packet->data_len = 0;
        }

        tcp_ip->tcp_packet->checksum = compute_tcp_packet_checksum(tcp_ip->tcp_packet, 
            src_ip_encoded, 
            dst_ip_encoded, 
            (uint16_t)total_tcp_len);

        return tcp_ip;
}


/***
 * The checksum field is the 16-bit ones' complement of the ones' complement sum of all 16-bit words in the header and text.
 * The checksum computation needs to ensure the 16-bit alignment of the data being summed.
 * If a segment contains an odd number of header and text octets, alignment can be achieved by padding the last octet with zeros on its right to form a 16-bit word for checksum purposes. 
 * The pad is not transmitted as part of the segment. While computing the checksum, the checksum field itself is replaced with zeros.
 * 
 * The checksum also covers a pseudo-header (Figure 2) conceptually prefixed to the TCP header.
 * The pseudo-header is 96 bits for IPv4 and 320 bits for IPv6.
 * Including the pseudo-header in the checksum gives the TCP connection protection against misrouted segments.
 * This information is carried in IP headers and is transferred across the TCP/network interface in the arguments or results of calls by the TCP implementation on the IP layer.
 * https://datatracker.ietf.org/doc/html/rfc9293#section-3.1
 */
uint16_t compute_tcp_packet_checksum(const tcp_packet_t* packet,
                                    uint32_t src_ip, uint32_t dst_ip,
                                    uint16_t tcp_length) {
    tcp_packet_t temp_packet = *packet; // ensure checksum is set to zero 
    temp_packet.checksum = 0; 

    const uint8_t tcp_header_len = packet->data_offset * 4;
    const uint8_t total_tcp_len = tcp_header_len + packet->data_len;

    size_t checksum_buf_len = total_tcp_len;
    if (checksum_buf_len % 2 != 0) {
        checksum_buf_len++; // ensure checksum buf is even
    }
    uint8_t* checksum_buf = (uint8_t*)calloc(1, checksum_buf_len);
    size_t bytes_written = tcp_packet_to_buf(&temp_packet, checksum_buf, checksum_buf_len);
    if (bytes_written == 0) {
        free(checksum_buf);
        return 0;
    }

    uint32_t sum = 0;

    sum += (src_ip >> 16) & 0xFFFF;
    sum += (src_ip) & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += (dst_ip) & 0xFFFF;
    sum += 6; // TCP protocol
    sum += tcp_length;

    for (int i = 0; i < total_tcp_len; i += 2) {
        uint16_t word;
        if (i + 1 < total_tcp_len) {
            word = (checksum_buf[i] << 8) | checksum_buf[i + 1];
        } else {
            word = checksum_buf[i] << 8;
        }
        sum += word;
    }

    // carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    free(checksum_buf);
    return (uint16_t)(~sum);
}

/**
 * "The checksum field is the 16 bit one's complement of the one's
 *    complement sum of all 16 bit words in the header.  For purposes of
 *    computing the checksum, the value of the checksum field is zero."
 *  https://datatracker.ietf.org/doc/html/rfc791#section-3.1
 */
uint16_t compute_ip_checksum(const ip_header_t* packet) {
    const uint8_t header_len = packet->ihl * 4;
    uint8_t header[header_len];

    ip_header_t temp_packet = *packet;
    temp_packet.header_checksum = 0; // ensure the checksum is set to zero

    ip_header_to_buf(&temp_packet, header, header_len);

    uint32_t sum = 0;

    // Process as 16 bit words
    for (int i = 0; i < header_len; i += 2) {
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

    uint16_t total_length = packet->total_length;
    buf[i++] = (total_length >> 8);
    buf[i++] = (total_length & 0xFF);

    uint16_t identification = packet->identification;
    buf[i++] = (identification >> 8);
    buf[i++] = (identification & 0xFF); 

    uint16_t flags_fragment = ((packet-> flags & 0x7) << 13) | (packet->fragment_offset & 0x1FFF);
    flags_fragment = flags_fragment;
    buf[i++] = flags_fragment >> 8;
    buf[i++] = flags_fragment & 0xFF;

    buf[i++] = packet->time_to_live;
    buf[i++] = packet->protocol;

    uint16_t header_checksum = packet->header_checksum;
    buf[i++] = (header_checksum >> 8);
    buf[i++] = (header_checksum & 0xFF); 

    uint32_t source_address = packet->source_address;
    buf[i++] = (source_address >> 24);
    buf[i++] = (source_address >> 16) & 0xFF; 
    buf[i++] = (source_address >> 8) & 0xFF; 
    buf[i++] = (source_address & 0x00FF); 

    uint32_t destination_address = packet->destination_address;
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

//TODO write tcp_buf_to_packet
bool tcp_buf_to_packet(const uint8_t* buf, size_t len, tcp_packet_t* out_packet) {
    if (len < MIN_TCP_PACKET_SIZE) {
        return false;
    }
    uint8_t i = 0;
    out_packet->source_port = ((uint16_t)buf[i++] << 8); 
    out_packet->source_port = (((uint16_t)buf[i++] & 0x00FF) | out_packet->source_port); 
    out_packet->destination_port = ((uint16_t)buf[i++] << 8); 
    out_packet->destination_port = (((uint16_t)buf[i++] & 0x00FF) | out_packet->destination_port); 
    out_packet->sequence_number = (((uint32_t) buf[i++]) << 24)
                                | ((uint32_t) buf[i++] << 16)
                                | ((uint32_t) buf[i++] << 8)
                                | ((uint32_t) buf[i++]);
    out_packet->acknowledgment_number = (((uint32_t) buf[i++]) << 24)
                                | ((uint32_t) buf[i++] << 16)
                                | ((uint32_t) buf[i++] << 8)
                                | ((uint32_t) buf[i++]);
    out_packet->data_offset = buf[i] >> 4;
    out_packet->reserved = buf[i++] & 0x0F;
    out_packet->flags = buf[i++];
    out_packet->window = ((uint16_t)buf[i++] << 8); 
    out_packet->window = (((uint16_t)buf[i++] & 0x00FF) | out_packet->window); 
    out_packet->checksum = ((uint16_t)buf[i++] << 8); 
    out_packet->checksum = (((uint16_t)buf[i++] & 0x00FF) | out_packet->checksum); 
    out_packet->urgent_pointer = ((uint16_t)buf[i++] << 8); 
    out_packet->urgent_pointer = (((uint16_t)buf[i++] & 0x00FF) | out_packet->urgent_pointer); 

    // Read data
    uint8_t header_len = out_packet->data_offset * 4;

    if (i < len) {
        out_packet->data_len = len - i;
        if (out_packet->data_len > 0) {
            uint8_t* data_copy = malloc(out_packet->data_len);
            if (data_copy) {
                memcpy(data_copy, buf + i, out_packet->data_len);
                out_packet->data = data_copy;
            } else {
                out_packet->data = NULL;
                out_packet->data_len = 0;
            }
        }
    } else {
        // No data payload
        out_packet->data = NULL;
        out_packet->data_len = 0;
    }

    return true;
}

size_t tcp_packet_to_buf(const tcp_packet_t* packet, uint8_t* buf, size_t buf_len) {
    size_t required_len = (packet->data_offset * 4) + packet->data_len;

    if (buf_len < required_len) {
        fprintf(stderr, "Buffer too small for tcp packet");
        return 0;
    }

    uint8_t i = 0;
    uint16_t source_port = packet->source_port;
    buf[i++] = ((source_port >> 8) & 0xFF); 
    buf[i++] = (source_port & 0xFF);

    uint16_t destination_port = packet->destination_port;
    buf[i++] = ((destination_port >> 8) & 0xFF); 
    buf[i++] = (destination_port & 0xFF);

    uint32_t sequence_number = packet->sequence_number;
    buf[i++] = (sequence_number >> 24);
    buf[i++] = (sequence_number >> 16) & 0xFF; 
    buf[i++] = (sequence_number >> 8) & 0xFF; 
    buf[i++] = (sequence_number & 0x00FF); 

    uint32_t acknowledgment_number = packet->acknowledgment_number;
    buf[i++] = (acknowledgment_number >> 24);
    buf[i++] = (acknowledgment_number >> 16) & 0xFF; 
    buf[i++] = (acknowledgment_number >> 8) & 0xFF; 
    buf[i++] = (acknowledgment_number & 0x00FF); 

    buf[i++] = ((packet->data_offset & 0x0F) << 4) | (packet->reserved & 0x0F);

    buf[i++] = packet->flags;

    uint16_t window = packet->window;
    buf[i++] = (window >> 8);
    buf[i++] = (window & 0xFF); 

    uint16_t checksum = packet->checksum;
    buf[i++] = (checksum >> 8);
    buf[i++] = (checksum & 0xFF); 

    uint16_t urgent_pointer = packet->urgent_pointer;
    buf[i++] = (urgent_pointer >> 8);
    buf[i++] = (urgent_pointer & 0xFF); 

    //...options

    if (packet->data && packet->data_len > 0) {
        memcpy(buf + i, packet->data, packet->data_len);
        i += packet->data_len;
    }
    return i;
}

size_t tcp_ip_to_buf(const tcp_ip_t* tcp_ip, uint8_t* buf) {
    size_t bytes_written_ip = ip_header_to_buf(tcp_ip->ip_header, buf, MIN_IP4_HEADER_SIZE);
    size_t bytes_written_tcp = tcp_packet_to_buf(
        tcp_ip->tcp_packet,
        buf + MIN_IP4_HEADER_SIZE,
        tcp_ip->ip_header->total_length - MIN_IP4_HEADER_SIZE // could be issue
    ); 
    return bytes_written_ip + bytes_written_tcp;
}

void update_sequence_number(tcb_t* tcb, tcp_flag_bits_t flags, size_t data_len) {
  if ((flags & TCP_FLAG_SYN) | (flags & TCP_FLAG_FIN)) {
    tcb->seq_num += 1;
  }

  tcb->seq_num += data_len;
}

void process_received_packet(tcb_t* tcb, tcp_packet_t* received_packet) {
  if (received_packet->flags & TCP_FLAG_SYN) {
    tcb->ack_num = received_packet->sequence_number + 1;
  } else {
    tcb->ack_num = received_packet->sequence_number + received_packet->data_len;
  }
}


void simple_send(const tcp_connection_t* conn, tcp_flag_bits_t flags, const uint8_t* data, const size_t data_len) {
    tcp_ip_t* tcp_ip = make_packet(conn->tcb, flags, data, data_len);
    printf("Sending TCP Packet: ");
    print_tcp_packet(tcp_ip->tcp_packet);
    const size_t total_packet_len = MIN_IP4_HEADER_SIZE + MIN_TCP_PACKET_SIZE + data_len;
    send_tcp_ip(tcp_ip, total_packet_len);
    update_sequence_number(conn->tcb, flags, tcp_ip->tcp_packet->data_len);
}

bool store_received_data(tcb_t* tcb, const tcp_packet_t* packet) {
  if (!packet->data || packet->data == 0) {
    return true; // no data so whatevs
  }

  if (tcb->recv_buf.write_pos + packet->data_len >= TCP_BUF_SIZE) {
    printf("Dropped data from packet. Receive buffer full\n");
    return false;
  }

  memcpy(tcb->recv_buf.data + tcb->recv_buf.write_pos, packet->data, packet->data_len);

  tcb->recv_buf.write_pos += packet->data_len;

  printf("Stored %zu bytes in the receive buffer\n", packet->data_len);
  return true;
}
