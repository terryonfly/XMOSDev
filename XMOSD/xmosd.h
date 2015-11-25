#ifndef __XMOSD_H
#define __XMOSD_H

#include <stdio.h>

/*
 * one hubfd pairs zero or many device sockets
 * base on events loop
 */


struct sock_func {
    char *socket;
    int (*func)(int, int);
    int fd;
};

int preprocess(void);
int process_led(int fd, int hubfd);
int process_amp(int fd, int hubfd);
int process_ammeter(int fd, int hubfd);
int process_hub(int, struct sock_func *);

#endif
