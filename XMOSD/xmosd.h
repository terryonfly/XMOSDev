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

struct hub_func {
    int hubfd;
    int (*func)(int, struct sock_func*);
};

int preprocess(void);

void check_fuel_err_cmd();

#endif
