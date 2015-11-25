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

#include "rokid_amp.h"

#define HEADER_LEN 2
#define FOOTER_LEN 2

#define ESCAPE_DATA 0x80
#define ESCAPE_HEADER 0x81
#define ESCAPE_FOOTER 0x82

#define ORDER_CODEC_SETTING 0x02

#define offsetof(TYPE, MEMBER) ((int)&((TYPE *)0)->MEMBER)

char *sock_addr = "/tmp/xmosd_amp";

typedef struct amp_dev {
    int fd_amp;
    unsigned char *frame_data;
} amp_dev;

struct amp_dev *amp_dev_open()
{
    int addrlen, fd;

    struct sockaddr_un addr_un;

    addr_un.sun_family = AF_UNIX;
    strcpy(addr_un.sun_path, sock_addr);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return (struct amp_dev *)-1;
    }

    struct amp_dev *amp_d = malloc(sizeof(struct amp_dev));
    amp_d->fd_amp = fd;
    int frame_data_len = HEADER_LEN + 2 + FOOTER_LEN;
    amp_d->frame_data = (unsigned char *)malloc(frame_data_len);
    return amp_d;
}

void amp_dev_close(struct amp_dev *amp_d)
{
    close(amp_d->fd_amp);
    amp_d->fd_amp = -1;
    free(amp_d->frame_data);
    free(amp_d);
    amp_d = NULL;
}

static int amp_dev_encode_data(unsigned char *des, int start, unsigned char *src, int src_len)
{
    int i;
    for (i = 0; i < src_len; i ++) {
        des[i * 2 + start] = ESCAPE_DATA;
        des[i * 2 + start + 1] = src[i];
    }
    return 0;
}

int amp_dev_set_eq(struct amp_dev *amp_d, unsigned char eq)
{
    // eq = 0x00 -> EFFECT_TYPE_MUSIC
    // eq = 0x01 -> EFFECT_TYPE_TTS
    int frame_data_len = HEADER_LEN + 2 + FOOTER_LEN;
    int index = 0;
    amp_d->frame_data[index] = ESCAPE_HEADER;
    index ++;
    amp_d->frame_data[index] = ORDER_CODEC_SETTING;
    index ++;
    amp_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    amp_d->frame_data[index] = eq;
    index ++;
    amp_d->frame_data[index] = ESCAPE_FOOTER;
    index ++;
    amp_d->frame_data[index] = ESCAPE_FOOTER;
    int ret;
    ret = write(amp_d->fd_amp, amp_d->frame_data, frame_data_len);
    return ret;
}
