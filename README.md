# tappy
A toy tcp library

# Testing
Currently no unit tests. Just grab packets via tcpdump:
```
sudo tcpdump -i tun0 -nn -vvv -X
```
Example output:
```
sudo tcpdump -i tun0 -nn -vvv -X -A -s0 tcp
tcpdump: listening on tun0, link-type RAW (Raw IP), snapshot length 262144 bytes
22:44:17.403871 IP (tos 0x66,ECT(0), ttl 69, id 30583, offset 0, flags [none], proto TCP (6), length 51)
    127.0.0.1.4 > 127.0.0.1.5: Flags [none], cksum 0x2000 (correct), seq 0:11, win 0, length 11
        0x0000:  4566 0033 7777 0000 4506 ffe5 7f00 0001  Ef.3ww..E.......
        0x0010:  7f00 0001 0004 0005 0000 0000 0000 0000  ................
        0x0020:  5000 0000 2000 0000 6865 6c6c 6f20 776f  P.......hello.wo
        0x0030:  726c 64                                  rld
22:44:19.403978 IP (tos 0x66,ECT(0), ttl 69, id 30583, offset 0, flags [none], proto TCP (6), length 51)
    127.0.0.1.4 > 127.0.0.1.5: Flags [none], cksum 0x2000 (correct), seq 0:11, win 0, length 11
        0x0000:  4566 0033 7777 0000 4506 ffe5 7f00 0001  Ef.3ww..E.......
        0x0010:  7f00 0001 0004 0005 0000 0000 0000 0000  ................
        0x0020:  5000 0000 2000 0000 6865 6c6c 6f20 776f  P.......hello.wo
        0x0030:  726c 64                                  rld
^C
2 packets captured
2 packets received by filter
0 packets dropped by kernel
```