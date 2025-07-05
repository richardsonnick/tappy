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