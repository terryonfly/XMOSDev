//
// Created by Terry on 15/11/24.
//
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "uart.h"

static int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1800,
                           B1200, B600, B300, B200, B150, B134, B110, B75, B50};

static int name_arr[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1800,
                         1200, 600, 300, 200, 150, 134, 110, 75, 50};

int open_serial(const char *dev_path)
{
    const char *str = dev_path;
    int fd;
    if (str == NULL)
        return -1;

    fd = open(str, O_RDWR);
    if (-1 == fd) {
        printf("open %s failed\n", str);
        return -1;
    }

    return fd;
}

int set_speed(int fd, int speed)
{
    int i;
    int status;
    int find = 0;
    int ret;
    struct termios opt;

    if (fd <= 0) {
        perror("fd invalid\n");
        return -1;
    }

    ret = tcgetattr (fd, &opt);
    if (ret != 0) {
        perror("tcgetattr failed\n");
        return -1;
    }

    for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
        if (speed == name_arr[i]) {
            find = 1;
            ret = tcflush(fd, TCIOFLUSH);
            if (ret != 0) {
                perror("tcflush failed\n");
                return -1;
            }

            ret = cfsetispeed(&opt, speed_arr[i]);
            if (ret != 0 ) {
                perror("cfsetispeed failed\n");
                return -1;
            }

            ret = cfsetospeed(&opt, speed_arr[i]);
            if (ret != 0) {
                perror("cfsetospeed failed\n");
                return -1;
            }

            status = tcsetattr(fd, TCSANOW, &opt);
            if (status != 0) {
                perror("tcsetattr failed\n");
                return -1;
            }

            ret = tcflush(fd, TCIOFLUSH);
            if (ret != 0) {
                perror("tcflush failed\n");
                return -1;
            }
        }
    }

    if (!find) {
        printf("bad speed arg\n");
        return -1;
    }

    return 0;
}

int set_parity(int fd, int databits, int stopbits, int parity)
{
    struct termios opt;
    int ret = 0;
    if (fd <= 0) {
        printf("fd invalid \n");
        return -1;
    }

    if (tcgetattr(fd, &opt) != 0) {
        perror("tcgetattr failed\n");
        return -1;
    }

    opt.c_cflag &= ~CSIZE;
    switch(databits) {
        case 7:
            opt.c_cflag |= CS7;
            break;
        case 8:
            opt.c_cflag |= CS8;
            break;

        default :
            printf("unsupported data size\n");
            return -1;
    }

    switch(parity) {
        case 0://n
            opt.c_cflag &= ~PARENB;
            break;
        case 1://o
            opt.c_cflag |= (PARODD | PARENB);
            break;
        case 2://e
            opt.c_cflag |= PARENB;
            opt.c_cflag &= ~PARODD;
            break;
        case 3://s
            opt.c_cflag &= ~PARENB;
            opt.c_cflag &= ~CSTOPB;
            break;

        default:
            printf ("unsupported parity\n");
            return -1;
    }

    switch(stopbits) {
        case 1:
            opt.c_cflag &= ~CSTOPB;
            break;
        case 2:
            opt.c_cflag |= CSTOPB;
            break;

        default:
            printf("unsupported stop bits\n");
            return -1;
    }

    if (parity != 0 && parity != 0)
        opt.c_iflag |= INPCK;
    else
        opt.c_iflag &= ~INPCK;

    opt.c_lflag &= ~ECHO;
    opt.c_lflag &= ~ISIG;
    opt.c_lflag &= ~ICANON;
    opt.c_iflag &= ~ICRNL;
    ret = tcflush(fd, TCIFLUSH);
    if (ret != 0) {
        perror("tcflush failed\n");
        return -1;
    }

    opt.c_cc[VTIME] = 0;
    opt.c_cc[VMIN] = 1;

    cfmakeraw(&opt);

    if (tcsetattr(fd, TCSANOW, &opt) != 0) {
        perror("setup serial opt failed\n");
        return -1;
    }

    return 0;
}

void close_serial(int fd)
{
    if (fd <= 0) {
        printf ("fd %d invalid\n", fd);
        return;
    }
    close (fd);
}
