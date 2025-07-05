#include <stdint.h>
#include <stddef.h>

#define TCP_BUF_SIZE 8192

// https://datatracker.ietf.org/doc/html/rfc9293#section-3.3.2
typedef enum {
    LISTEN,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    LAST_ACK,
    TIME_WAIT,
    CLOSED // This is fictional, it represents the state with no TCB (thus no connection).
} TCP_STATE;

typedef enum {
    OPEN,
    SEND,
    RECEIVE,
    CLOSE,
    ABORT,
    STATUS
} TCP_EVENT;

typedef struct {
    uint8_t* data;
    size_t length;
} tcp_buffer_t;

typedef struct {
    uint32_t seq_num;
    tcp_buffer_t send_buf;
    tcp_buffer_t recv_buf;
} tcb_t;

// Tracks tcp connection state.
typedef struct {
    TCP_STATE state;
    tcb_t* tcb;
} tcp_connection_t;
