# Project Plan: Implementing a Full TCP Stack

### Goal 1: Implement Raw Socket I/O
Your library can build packets, but it needs a way to send and receive them over the network.

- [ ] **Tasks:**
    1.  Implement `send_packet()` in `io.c` to send raw IP packets using a raw socket (`AF_INET`, `SOCK_RAW`).
    2.  Implement `recv_packet()` in `io.c` to read incoming IP packets from the raw socket.
    3.  Add function declarations for `send_packet` and `recv_packet` to `io.h`.

### Goal 2: Implement TCP Handshake
With I/O in place, the next step is to establish a connection with the three-way handshake.

- [ ] **Tasks:**
    1.  **Sequence & Acknowledgment Numbers:**
        - [ ] Dynamically manage sequence (`seq`) and acknowledgment (`ack`) numbers in the Transmission Control Block (`tcb_t`).
        - [ ] Update the `tcb_t` after sending and receiving packets.
    2.  **Client-side Handshake Logic:**
        - [ ] In `client_state_machine.c`, implement the logic to:
            - [ ] Send a `SYN` packet.
            - [ ] Wait for a `SYN-ACK` response.
            - [ ] Send an `ACK` to complete the handshake and move to the `ESTABLISHED` state.
    3.  **Server-side Handshake Logic:**
        - [ ] In `server_state_machine.c`, implement the logic to:
            - [ ] Listen for an incoming `SYN` packet.
            - [ ] Respond with a `SYN-ACK`.
            - [ ] Wait for the final `ACK` to move to the `ESTABLISHED` state.

### Goal 3: Implement Data Transfer
Once a connection is established, your stack needs to be able to send and receive data.

- [ ] **Tasks:**
    1.  **Application-level API:**
        - [ ] Create `tcp_send()` and `tcp_recv()` functions that allow an application to send and receive data streams.
    2.  **Packetization and Buffering:**
        - [ ] In `tcp_send()`, break the application data into TCP segments.
        - [ ] Manage the send buffer in the `tcb_t` for data that is waiting to be sent and acknowledged.
    3.  **Receiving and Reassembly:**
        - [ ] In `tcp_recv()`, read data from incoming packets.
        - [ ] Manage the receive buffer in the `tcb_t` to handle incoming data and reassemble the stream for the application.

### Goal 4: Implement Connection Termination
Properly closing a TCP connection is crucial for reliable communication.

- [ ] **Tasks:**
    1.  **Active Close (Client):**
        - [ ] Implement the logic for an application to initiate a close.
        - [ ] Send a `FIN` packet and wait for an `ACK`.
        - [ ] Wait for the server's `FIN` and respond with an `ACK`.
    2.  **Passive Close (Server):**
        - [ ] Implement the logic to handle an incoming `FIN` from the client.
        - [ ] Send an `ACK`, and then send its own `FIN` when the application is ready to close.

### Goal 5: Advanced Features and Reliability
These features will make your TCP implementation robust and efficient.

- [ ] **Tasks:**
    1.  **Retransmission and Timeouts:**
        - [ ] Implement a retransmission timer to handle lost packets. When an acknowledgment isn't received within a certain time, the segment should be re-sent.
    2.  **Flow Control:**
        - [ ] Implement sliding window logic. The receiver should advertise its receive window size, and the sender must respect it to avoid overwhelming the receiver.
    3.  **Congestion Control:**
        - [ ] Implement basic congestion control algorithms like slow start and congestion avoidance to manage network traffic.
