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

#include "rokid_fuel.h"

#define HEADER_LEN 2
#define FOOTER_LEN 2

#define ESCAPE_DATA 0x80
#define ESCAPE_HEADER 0x81
#define ESCAPE_FOOTER 0x82

#define ORDER_FUEL_SETTING 0x03

#define offsetof(TYPE, MEMBER) ((int)&((TYPE *)0)->MEMBER)

//char *sock_addr = "/tmp/xmosd_ammeter";
char *sock_addr = "/dev/socket/xmosd_ammeter";

unsigned char fuel_i2c_data[512];

typedef struct fuel_dev {
    int fd_fuel;
    unsigned char *frame_data;
} fuel_dev;

struct fuel_dev *fuel_dev_open()
{
    int addrlen, fd;
    struct sockaddr_un addr_un;

    addr_un.sun_family = AF_UNIX;
    strcpy(addr_un.sun_path, sock_addr);

    if ((fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
        perror("socket");
        return (struct fuel_dev *)-1;
    }

    addrlen = offsetof(struct sockaddr_un, sun_path) + strlen(addr_un.sun_path);
    if (connect(fd, (struct sockaddr *)&addr_un,  addrlen) < 0) {
        perror("connect");
        return (struct fuel_dev *)-1;
    }

    struct fuel_dev *fuel_d = malloc(sizeof(struct fuel_dev));
    fuel_d->fd_fuel = fd;
    int frame_data_len = 1024;
    fuel_d->frame_data = (unsigned char *)malloc(frame_data_len);
    return fuel_d;
}

void fuel_dev_close(struct fuel_dev *fuel_d)
{
    close(fuel_d->fd_fuel);
    fuel_d->fd_fuel = -1;
    free(fuel_d->frame_data);
    free(fuel_d);
    fuel_d = NULL;
}

static int fuel_dev_encode_data(unsigned char *des, int start, unsigned char *src, int src_len)
{
    int i;
    for (i = 0; i < src_len; i ++) {
        des[i * 2 + start] = ESCAPE_DATA;
        des[i * 2 + start + 1] = src[i];
    }
    return 0;
}

int fuel_dev_i2c_write(struct fuel_dev *fuel_d,
                       unsigned char device_addr,
                       unsigned char *data,
                       int data_len,
                       int send_stop_bit,
                       int *wrote_len)
{
    if (data_len > 251) return -2;
    int i;
    int frame_data_len = HEADER_LEN + (data_len + 3) * 2 + FOOTER_LEN;
    int index = 0;
    fuel_d->frame_data[index] = ESCAPE_HEADER;
    index ++;
    fuel_d->frame_data[index] = ORDER_FUEL_SETTING;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x01;// write
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = device_addr;// device_addr
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = (send_stop_bit == 0) ? 0x00 : 0x01;// send_stop_bit
    index ++;
    for (i = 0; i < data_len; i ++) {
        fuel_d->frame_data[index] = ESCAPE_DATA;
        index ++;
        fuel_d->frame_data[index] = data[i];
        index ++;
    }
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    int actual;
    actual = write(fuel_d->fd_fuel, fuel_d->frame_data, frame_data_len);
    if (actual != frame_data_len) return -1;

    int retry_times = 10;
    do {
        actual = read(fuel_d->fd_fuel, fuel_i2c_data, 512);
        if (actual == -1) {
            usleep(200 * 1000);
        }
        retry_times --;
    } while (actual == -1 && retry_times > 0);
    if (actual == -1) return -1;

//    /*
    int k;
    for (k = 0; k < actual; k ++) {
        printf("%02x ", fuel_i2c_data[k]);
    }
    printf("\n");
//    */
    if (actual % 2 != 0) return -1;
    if (actual < 6) return -1;
    if (fuel_i2c_data[0] != 0x81) return -1;
    if (fuel_i2c_data[1] != 0x03) return -1;
    if (fuel_i2c_data[actual - 2] != 0x82) return -1;
    if (fuel_i2c_data[actual - 1] != 0x82) return -1;
    if (fuel_i2c_data[2] != 0x80) return -1;
    if (fuel_i2c_data[3] != 0x01) return -1;// order
    if (fuel_i2c_data[4] != 0x80) return -1;
    if (fuel_i2c_data[5] != 0x01) return -1;// ack or nack
    if (fuel_i2c_data[6] != 0x80) return -1;
    unsigned char i2c_write_bytes = fuel_i2c_data[7];
    printf("i2c wrote %d bytes\n", i2c_write_bytes);
    *wrote_len = i2c_write_bytes;
    return 0;
}

int fuel_dev_i2c_read(struct fuel_dev *fuel_d,
                      unsigned char device_addr,
                      unsigned char data_len,
                      int send_stop_bit,
                      unsigned char *readed_data,
                      int *readed_len)
{
    if (data_len > 251) return -2;
    int i;
    int frame_data_len = HEADER_LEN + (1 + 3) * 2 + FOOTER_LEN;
    int index = 0;
    fuel_d->frame_data[index] = ESCAPE_HEADER;
    index ++;
    fuel_d->frame_data[index] = ORDER_FUEL_SETTING;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x02;// read
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = device_addr;// device_addr
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = (send_stop_bit == 0) ? 0x00 : 0x01;// send_stop_bit
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = data_len;// data_len
    index ++;
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    int actual;
    actual = write(fuel_d->fd_fuel, fuel_d->frame_data, frame_data_len);
    if (actual != frame_data_len) return -1;

    int retry_times = 10;
    do {
        actual = read(fuel_d->fd_fuel, fuel_i2c_data, 512);
        if (actual == -1) {
            usleep(200 * 1000);
        }
        retry_times --;
    } while (actual == -1 && retry_times > 0);
    if (actual == -1) return -1;

    /*
    int k;
    for (k = 0; k < read_actual; k ++) {
        printf("%02x ", fuel_i2c_data[k]);
    }
    printf("\n");
    */
    if (actual % 2 != 0) return -1;
    if (actual < 6) return -1;
    if (fuel_i2c_data[0] != 0x81) return -1;
    if (fuel_i2c_data[1] != 0x03) return -1;
    if (fuel_i2c_data[actual - 2] != 0x82) return -1;
    if (fuel_i2c_data[actual - 1] != 0x82) return -1;
    if (fuel_i2c_data[2] != 0x80) return -1;
    if (fuel_i2c_data[3] != 0x02) return -1;// order
    if (fuel_i2c_data[4] != 0x80) return -1;
    if (fuel_i2c_data[5] != 0x01) return -1;// ack or nack
    int readed_i = 0;
    printf("i2c readed ");
    for (i = 6; i < actual - 2; i ++) {
        if (fuel_i2c_data[i] != 0x80) return -1;
        i++;
        readed_data[readed_i] = fuel_i2c_data[i];
        printf("%02x ", readed_data[readed_i]);
        readed_i ++;
    }
    printf("\n");
    *readed_len = readed_i;
    return 0;
}

int fuel_dev_i2c_send_stop_bit(struct fuel_dev *fuel_d)
{
    int i;
    int frame_data_len = HEADER_LEN + (0 + 3) * 2 + FOOTER_LEN;
    int index = 0;
    fuel_d->frame_data[index] = ESCAPE_HEADER;
    index ++;
    fuel_d->frame_data[index] = ORDER_FUEL_SETTING;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x03;// send_stop_bit command
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x00;// device_addr
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x01;// send_stop_bit
    index ++;
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    int actual;
    actual = write(fuel_d->fd_fuel, fuel_d->frame_data, frame_data_len);
    if (actual != frame_data_len) return -1;

    int retry_times = 10;
    do {
        actual = read(fuel_d->fd_fuel, fuel_i2c_data, 512);
        if (actual == -1) {
            usleep(200 * 1000);
        }
        retry_times --;
    } while (actual == -1 && retry_times > 0);
    if (actual == -1) return -1;

    /*
    int k;
    for (k = 0; k < read_actual; k ++) {
        printf("%02x ", fuel_i2c_data[k]);
    }
    printf("\n");
    */
    if (actual % 2 != 0) return -1;
    if (actual < 6) return -1;
    if (fuel_i2c_data[0] != 0x81) return -1;
    if (fuel_i2c_data[1] != 0x03) return -1;
    if (fuel_i2c_data[actual - 2] != 0x82) return -1;
    if (fuel_i2c_data[actual - 1] != 0x82) return -1;
    if (fuel_i2c_data[2] != 0x80) return -1;
    if (fuel_i2c_data[3] != 0x03) return -1;// order
    if (fuel_i2c_data[4] != 0x80) return -1;
    if (fuel_i2c_data[5] != 0x01) return -1;// ack or nack
    printf("stop_bit sent\n");
    return 0;
}

int fuel_dev_i2c_write_reg(struct fuel_dev *fuel_d,
                           unsigned char device_addr,
                           unsigned char reg_addr,
                           unsigned char data)
{
    int r;
    int send_stop_bit = 0;
    int a_data_len = 2;
    unsigned char a_data[2] = {reg_addr, data};
    int wrote_len;
    r = fuel_dev_i2c_write(fuel_d, device_addr, a_data, a_data_len, send_stop_bit, &wrote_len);

    if (wrote_len == 0)
        return -1;//I2C_REGOP_DEVICE_NACK;
    if (wrote_len < 2)
        return -1;//I2C_REGOP_INCOMPLETE;
    return 0;
}

int fuel_dev_i2c_read_reg(struct fuel_dev *fuel_d,
                          unsigned char device_addr,
                          unsigned char reg_addr,
                          unsigned char *data)
{
    int r;
    int send_stop_bit = 0;
    int a_data_len = 1;
    unsigned char a_data[1] = {reg_addr};
    int wrote_len;
    r = fuel_dev_i2c_write(fuel_d, device_addr, a_data, a_data_len, send_stop_bit, &wrote_len);
    if (wrote_len != 1) {
        r = -1;//I2C_REGOP_DEVICE_NACK;
        fuel_dev_i2c_send_stop_bit(fuel_d);
        return r;
    }
    send_stop_bit = 1;
    int to_read_len = 1;
    int readed_len = 1;
    r = fuel_dev_i2c_read(fuel_d, device_addr, to_read_len, send_stop_bit, data, &readed_len);
    return r;
}

int fuel_dev_gpio_chg_stat(struct fuel_dev *fuel_d, int *gpio_val)
{
    int frame_data_len = HEADER_LEN + (0 + 3) * 2 + FOOTER_LEN;
    int index = 0;
    fuel_d->frame_data[index] = ESCAPE_HEADER;
    index ++;
    fuel_d->frame_data[index] = ORDER_FUEL_SETTING;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x04;// get gpio status chg
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x00;// device_addr
    index ++;
    fuel_d->frame_data[index] = ESCAPE_DATA;
    index ++;
    fuel_d->frame_data[index] = 0x00;// send_stop_bit
    index ++;
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    index ++;
    fuel_d->frame_data[index] = ESCAPE_FOOTER;
    int actual;
    actual = write(fuel_d->fd_fuel, fuel_d->frame_data, frame_data_len);
    if (actual != frame_data_len) return -1;

    int retry_times = 10;
    do {
        actual = read(fuel_d->fd_fuel, fuel_i2c_data, 512);
        if (actual == -1) {
            usleep(200 * 1000);
        }
        retry_times --;
    } while (actual == -1 && retry_times > 0);
    if (actual == -1) return -1;

    /*
    int k;
    for (k = 0; k < read_actual; k ++) {
        printf("%02x ", fuel_i2c_data[k]);
    }
    printf("\n");
    */
    if (actual % 2 != 0) return -1;
    if (actual < 6) return -1;
    if (fuel_i2c_data[0] != 0x81) return -1;
    if (fuel_i2c_data[1] != 0x03) return -1;
    if (fuel_i2c_data[actual - 2] != 0x82) return -1;
    if (fuel_i2c_data[actual - 1] != 0x82) return -1;
    if (fuel_i2c_data[2] != 0x80) return -1;
    if (fuel_i2c_data[3] != 0x04) return -1;// order
    if (fuel_i2c_data[4] != 0x80) return -1;
    if (fuel_i2c_data[5] != 0x01) return -1;// ack or nack
    if (fuel_i2c_data[6] != 0x80) return -1;
    *gpio_val = fuel_i2c_data[7];
    printf("got chg_stat = %d\n", gpio_val);
    return 0;
}
