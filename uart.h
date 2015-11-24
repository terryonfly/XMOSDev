//
// Created by Terry on 15/11/24.
//

#ifndef XMOSDEV_UART_H
#define XMOSDEV_UART_H

int open_serial(const char *dev_path);

int set_speed(int fd, int speed);

int set_parity(int fd, int databits, int stopbits, int parity);

void close_serial(int fd);

#endif //XMOSDEV_UART_H
