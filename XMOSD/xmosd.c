#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "xmosd.h"

struct sock_func sock_funcs[] = {
        {
                .socket = "xmosd_led",
                .func = process_led,
        },
        {
                .socket = "xmosd_amp",
                .func = process_amp,
        },
        {
                .socket = "xmosd_ammeter",
                .func = process_ammeter,
        },
        {NULL, NULL, 0},
};

/* open hubfd */
int preprocess(void)
{
    int fd;

    return fd;
}


int process_ammeter(int fd, int hubfd)
{
    return fd;
}

/* process xmosd_amp connection */
int process_amp(int fd, int hubfd)
{
    char buf[64*1024];
    int num;

    memset(buf, 0, sizeof(buf));

    do {
        num = read(fd, buf, sizeof(buf));
        if (num < 0) {
            if (errno != EAGAIN) {
                close(fd);
                perror("read");
                return -1;
            }
            break;
        } else if (num == 0) {
            printf("%d closed", fd);
            close(fd);
        } else {	/* Process data */
            printf("amp: %02x %d\n", buf, num);
        }
    } while(0);


    return fd;

}

/* process xmosd_len connection */
int process_led(int fd, int hubfd)
{
    char buf[64*1024];
    int num;

    memset(buf, 0, sizeof(buf));

    do {
        num = read(fd, buf, sizeof(buf));
        if (num < 0) {
            if (errno != EAGAIN) {
                fd = 0;
                close(fd);
                perror("read");
                return -1;
            }
            break;
        } else if (num == 0) {
            printf("%d closed", fd);
            fd = 0;
            close(fd);
        } else {	/* Process data */
            printf("%s %d\n", buf, num);
        }
    } while(0);


    return fd;
}

int process_hub(int hubfd, struct sock_func *sfs)
{
    return hubfd;
}

