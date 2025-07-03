#ifndef UTILS_H
#define UTILS_H

/**
 * Allocates a tun driver
 * https://www.kernel.org/doc/Documentation/networking/tuntap.txt
 */
int tun_alloc(char *dev);

#endif // UTILS_H