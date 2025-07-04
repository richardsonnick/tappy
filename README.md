# tappy
A toy tcp library

# Setup
Currently this is in a testing phase using `/dev/tun`. So this requires setting up a tun device:
```
# Create the TUN interface
sudo ip tuntap add dev tun0 mode tun

# Bring it up
sudo ip link set tun0 up

# Assign an IP address (ip address is just an example)
sudo ip addr add 10.0.0.1/24 dev tun0
```

# Testing
Currently no unit tests. Just grab packets via tcpdump:
```
sudo tcpdump -i tun0 -nn -vvv -X
```
Example output:
```
╰─ sudo tcpdump -i tun0 -nn -vvv -X
tcpdump: listening on tun0, link-type RAW (Raw IP), snapshot length 262144 bytes
12:54:41.320062 IP (tos 0x66,ECT(0), ttl 255, id 30583, offset 0, flags [none], proto TCP (6), length 20)
    127.0.0.1 > 127.0.0.1: [|tcp]
        0x0000:  4566 0014 7777 0000 ff06 4604 7f00 0001  Ef..ww....F.....
        0x0010:  7f00 0001                                ....
12:54:43.320153 IP (tos 0x66,ECT(0), ttl 255, id 30583, offset 0, flags [none], proto TCP (6), length 20)
    127.0.0.1 > 127.0.0.1: [|tcp]
        0x0000:  4566 0014 7777 0000 ff06 4604 7f00 0001  Ef..ww....F.....
        0x0010:  7f00 0001                                ....
12:54:45.320235 IP (tos 0x66,ECT(0), ttl 255, id 30583, offset 0, flags [none], proto TCP (6), length 20)
    127.0.0.1 > 127.0.0.1: [|tcp]
        0x0000:  4566 0014 7777 0000 ff06 4604 7f00 0001  Ef..ww....F.....
        0x0010:  7f00 0001                                ....
12:54:47.320312 IP (tos 0x66,ECT(0), ttl 255, id 30583, offset 0, flags [none], proto TCP (6), length 20)
    127.0.0.1 > 127.0.0.1: [|tcp]
        0x0000:  4566 0014 7777 0000 ff06 4604 7f00 0001  Ef..ww....F.....
        0x0010:  7f00 0001                                ....
^C
4 packets captured
4 packets received by filter
0 packets dropped by kernel
```