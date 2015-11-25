#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "xmosd.h"

struct xmos_dev *xmos_dd;

unsigned char led_pack_buf[1024];
int led_pack_index = 0;

unsigned char amp_pack_buf[1024];
int amp_pack_index = 0;

/* open hubfd */
int preprocess(void)
{
    int fd = 0;
    int r;
    xmos_dd = xmos_dev_open(&r);
    printf("dev open success.\n");
    return fd;
}

int process_ammeter(int fd, int hubfd)
{
    char buf[64*1024];
    int num;

    printf("%s\n", __func__);

    memset(buf, 0, sizeof(buf));

    do {
        num = read(fd, buf, sizeof(buf));
        if (num < 0) {
            return -errno;
        } else if (num == 0) {
            printf("%d closed\n", fd);
            return 0;
        } else {	/* Process data */
            printf("%s %d\n", buf, num);
        }
    } while(0);


    return fd;
}

/* process xmosd_amp connection */

void amp_data_decode(unsigned char *buf, int len)
{
    int i;
    for (i = 0; i < len; i ++) {
        if (buf[i] == 0x81 && (i + 1) < len) {
            i ++;
//            current_order = buf[i];
            continue;
        } else if (buf[i] == 0x82 && (i + 1) < len) {
            i ++;
            if (amp_pack_index == 1)
                xmos_dev_codec_setting_eq(xmos_dd, amp_pack_buf[0]);
            amp_pack_index = 0;
            continue;
        } else if (buf[i] == 0x80 && (i + 1) < len) {
            i ++;
            if (amp_pack_index < 1024)
                amp_pack_buf[amp_pack_index] = buf[i];
            amp_pack_index ++;
            if (amp_pack_index >= 1024)
                amp_pack_index = 0;
            continue;
        }
    }
}

int process_amp(int fd, int hubfd)
{
    char buf[64*1024];
    int num;

    printf("%s\n", __func__);

    memset(buf, 0, sizeof(buf));

    do {
        num = read(fd, buf, sizeof(buf));
        if (num < 0) {
            return -errno;
        } else if (num == 0) {
            printf("%d closed\n", fd);
            return 0;
        } else {	/* Process data */
            amp_data_decode(buf, num);
//            printf("%s %d\n", buf, num);
        }
    } while(0);


    return fd;

}

/* process xmosd_len connection */

void led_data_decode(unsigned char *buf, int len)
{
    int i;
    for (i = 0; i < len; i ++) {
        if (buf[i] == 0x81 && (i + 1) < len) {
            i ++;
//            current_order = buf[i];
            continue;
        } else if (buf[i] == 0x82 && (i + 1) < len) {
            i ++;
            xmos_dev_led_flush_frame(xmos_dd, led_pack_buf, led_pack_index);
            led_pack_index = 0;
            continue;
        } else if (buf[i] == 0x80 && (i + 1) < len) {
            i ++;
            if (led_pack_index < 1024)
                led_pack_buf[led_pack_index] = buf[i];
            led_pack_index ++;
            if (led_pack_index >= 1024)
                led_pack_index = 0;
            continue;
        }
    }
}

int process_led(int fd, int hubfd)
{
    char buf[64*1024];
    int num;

    printf("%s\n", __func__);

    memset(buf, 0, sizeof(buf));

    do {
        num = read(fd, buf, sizeof(buf));
        if (num < 0) {
            return -errno;
        } else if (num == 0) {
            printf("%d closed\n", fd);
            return 0;
        } else {	/* Process data */
            led_data_decode(buf, num);
//            printf("%s %d\n", buf, num);
        }
    } while(0);


    return fd;
}

int process_hub(int hubfd, struct sock_func *sfs)
{
    return hubfd;
}

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

struct hub_func hub_func = {
        .hubfd = 0,
        .func = process_hub,
};
