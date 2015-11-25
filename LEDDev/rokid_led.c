//
// Created by Terry on 15/11/25.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rokid_led.h"

#define FRAME_LEN (3 * LED_COUNT)
#define HEADER_LEN 2
#define FOOTER_LEN 2

#define ESCAPE_DATA 0x80
#define ESCAPE_HEADER 0x81
#define ESCAPE_FOOTER 0x82

#define ORDER_FLUSH_FRAME 0x01

#define offsetof(TYPE, MEMBER) ((int)&((TYPE *)0)->MEMBER)

char *sock_addr = "/tmp/xmosd_led";

typedef struct led_dev {
    int fd_led;
    unsigned char *frame_data;
} led_dev;

struct led_dev *led_dev_open()
{
    int addrlen, fd;

    struct sockaddr_un addr_un;

    addr_un.sun_family = AF_UNIX;
    strcpy(addr_un.sun_path, sock_addr);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return (struct led_dev *)-1;
    }

    struct led_dev *led_d = malloc(sizeof(struct led_dev));
    led_d->fd_led = fd;
    int frame_data_len = HEADER_LEN + FRAME_LEN * 2 + FOOTER_LEN;
    led_d->frame_data = (unsigned char *)malloc(frame_data_len);
    return led_d;
}

void led_dev_close(struct led_dev *led_d)
{
    close(led_d->fd_led);
    led_d->fd_led = -1;
    free(led_d->frame_data);
    free(led_d);
    led_d = NULL;
}

static int led_dev_encode_data(unsigned char *des, int start, unsigned char *src, int src_len)
{
    int i;
    for (i = 0; i < src_len; i ++) {
        des[i * 2 + start] = ESCAPE_DATA;
        des[i * 2 + start + 1] = src[i];
    }
    return 0;
}

int led_dev_flush_frame(struct led_dev *led_d, unsigned char *data, int data_len)
{
    if (data_len != FRAME_LEN) return ROKID_LED_ERROR_FRAME_LEN;
    int frame_data_len = HEADER_LEN + data_len * 2 + FOOTER_LEN;
    int index = 0;
    led_d->frame_data[index] = ESCAPE_HEADER;
    index ++;
    led_d->frame_data[index] = ORDER_FLUSH_FRAME;
    index ++;
    led_dev_encode_data(led_d->frame_data, index, data, data_len);
    index += data_len * 2;
    led_d->frame_data[index] = ESCAPE_FOOTER;
    index ++;
    led_d->frame_data[index] = ESCAPE_FOOTER;
    int ret;
    ret = write(led_d->fd_led, led_d->frame_data, frame_data_len);
    return ret;
}
