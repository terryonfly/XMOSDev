#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "xmosd.h"
#include "rokid_xmos.h"

unsigned char led_pack_buf[1024];
int led_pack_index = 0;

unsigned char amp_pack_buf[1024];
int amp_pack_index = 0;

unsigned char ammeter_pack_buf[1024];
int ammeter_pack_index = 0;

unsigned char hub_pack_buf[1024];
int hub_pack_index = 0;
unsigned char hub_current_order;

int fuel_fd = -1;
int amp_fd = -1;
int led_fd = -1;

/* open hubfd */
int preprocess(void)
{
    int fd = 0;
    int r;
    fd = xmos_dev_open(&r);
    printf("dev open success.\n");
    return fd;
}

/* process xmosd_ammeter connection */

void ammeter_command_decode(int hubfd, unsigned char *buf, int len)
{
    printf("one command\n");
    if (len < 3) return;
    char dev_addr = buf[1];
    int send_stop_bit;
    unsigned char i2c_dat[512];
    switch(buf[0]) {
        case 0x01://write
            send_stop_bit = (buf[2] == 0x01) ? 1 : 0;
            int num_bytes_sent;
            int dat_i = 3;
            while(dat_i < len) {
                i2c_dat[dat_i - 3] = buf[dat_i];
                dat_i ++;
            }
            xmos_dev_electric_i2c_write(hubfd, dev_addr, i2c_dat, dat_i - 3, send_stop_bit);
            break;
        case 0x02://read
            send_stop_bit= (buf[2] == 0x01) ? 1 : 0;
            if (len < 4) return;
            unsigned char size_to_read = buf[3];
            xmos_dev_electric_i2c_read(hubfd, dev_addr, size_to_read, send_stop_bit);
            break;
        case 0x03://send_stop_bit
            xmos_dev_electric_i2c_send_stop_bit(hubfd);
            break;
    }
}

void ammeter_data_decode(int hubfd, unsigned char *buf, int len)
{
    int i;
    for (i = 0; i < len; i ++) {
        if (buf[i] == 0x81 && (i + 1) < len) {
            i ++;
//            current_order = buf[i];
            continue;
        } else if (buf[i] == 0x82 && (i + 1) < len) {
            i ++;
            ammeter_command_decode(hubfd, ammeter_pack_buf, ammeter_pack_index);
            ammeter_pack_index = 0;
            continue;
        } else if (buf[i] == 0x80 && (i + 1) < len) {
            i ++;
            if (ammeter_pack_index < 1024)
                ammeter_pack_buf[ammeter_pack_index] = buf[i];
            ammeter_pack_index ++;
            if (ammeter_pack_index >= 1024)
                ammeter_pack_index = 0;
            continue;
        }
    }
}

int process_ammeter(int fd, int hubfd)
{
    fuel_fd = fd;
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
            ammeter_data_decode(hubfd, buf, num);
//            printf("%s %d\n", buf, num);
        }
    } while(0);


    return fd;
}

/* process xmosd_amp connection */

void amp_data_decode(int hubfd, unsigned char *buf, int len)
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
                xmos_dev_codec_setting_eq(hubfd, amp_pack_buf[0]);
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
    amp_fd = fd;
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
            amp_data_decode(hubfd, buf, num);
//            printf("%s %d\n", buf, num);
        }
    } while(0);

    return fd;

}

/* process xmosd_len connection */

void led_data_decode(int hubfd, unsigned char *buf, int len)
{
    int i;
    for (i = 0; i < len; i ++) {
        if (buf[i] == 0x81 && (i + 1) < len) {
            i ++;
//            current_order = buf[i];
            continue;
        } else if (buf[i] == 0x82 && (i + 1) < len) {
            i ++;
            xmos_dev_led_flush_frame(hubfd, led_pack_buf, led_pack_index);
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
    led_fd = fd;
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
            led_data_decode(hubfd, buf, num);
//            printf("%s %d\n", buf, num);
        }
    } while(0);


    return fd;
}

void hub_data_decode(struct sock_func *sfs, unsigned char *buf, int len)
{
    struct sock_func *sf;
    int i;
    for (i = 0; i < len; i ++) {
        if (buf[i] == 0x81 && (i + 1) < len) {
            i ++;
            hub_current_order = buf[i];
            continue;
        } else if (buf[i] == 0x82 && (i + 1) < len) {
            i ++;

            if (hub_current_order == 0x03) {
//                sf = sfs;
//                while (sf->fd > 0) {
//                    if (sf->func == process_ammeter)
//                        break;
//                    sf++;
//                }
                fuel_feedback(fuel_fd, hub_pack_buf, hub_pack_index);
            }

            hub_pack_index = 0;
            continue;
        } else if (buf[i] == 0x80 && (i + 1) < len) {
            i ++;
            if (hub_pack_index < 1024)
                hub_pack_buf[hub_pack_index] = buf[i];
            hub_pack_index ++;
            if (hub_pack_index >= 1024)
                hub_pack_index = 0;
            continue;
        }
    }
}

int process_hub(int hubfd, struct sock_func *sfs)
{
    char buf[64*1024];
    int num;

    printf("%s\n", __func__);

    memset(buf, 0, sizeof(buf));

    do {
        num = read(hubfd, buf, sizeof(buf));
        if (num < 0) {
            return -errno;
        } else if (num == 0) {
            printf("%d closed\n", hubfd);
            return 0;
        } else {	/* Process data */
            hub_data_decode(sfs, buf, num);
//            printf("%s %d\n", buf, num);
        }
    } while(0);

    return hubfd;
}

void check_fuel_err_cmd(struct sock_func *sfs)
{
//    struct sock_func *sf = sfs;
//    while (sf->fd > 0) {
//        if (sf->func == process_ammeter)
//            break;
//        sf++;
//    }

    unsigned char err_cmd = check_fuel_command_return();
    if (err_cmd == 0x00) return;
    unsigned char err_data[64];
    size_t err_data_len = 6;
    err_data[0] = 0x81;
    err_data[1] = 0x03;
    err_data[2] = 0x80;
    err_data[3] = err_cmd;
    err_data[4] = 0x82;
    err_data[5] = 0x82;
    write(fuel_fd, err_data, err_data_len);
    printf("**********err***********\n");
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
