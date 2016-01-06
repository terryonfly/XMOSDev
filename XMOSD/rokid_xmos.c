#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include <string.h>
#include <signal.h>

#include <time.h>

#include "uart.h"
#include "rokid_xmos.h"

#ifdef ANDROID
#define SERIAL_DEV "/dev/tty.usbmodem14235"
#else
#define SERIAL_DEV "/dev/ttyACM0"
#endif

#define HEADER_LEN 2
#define FOOTER_LEN 2

#define ESCAPE_HEADER 0x81
#define ESCAPE_DATA 0x80
#define ESCAPE_FOOTER 0x82

#define ORDER_LED_FLUSH_FRAME 0x01
#define ORDER_CODEC_SETTING 0x02
#define ORDER_ELECTRIC_I2C_RW 0x03

#define FRAME_LEN (3 * (XMOS_LED_COUNT + BACK_LED_COUNT))
#define BACK_FRAME_LEN (3 * BACK_LED_COUNT)

unsigned char xmos_i2c_data[512];
unsigned char xmos_write_data[512];

unsigned char xmos_write_data_group[512];
int xmos_write_data_group_index = 0;
int xmos_d_for_group = -1;

int back_led_fd = -1;

int xmos_dev_open()
{
	int fd_xmos = open_serial(SERIAL_DEV);
	if (fd_xmos == -1) {
		printf("Cannot open device\n");
        return ROKID_XMOS_ERROR_DEVICE_OPEN_FAILED;
	}
	int ret;
	ret = set_speed(fd_xmos, 115200);
	if (ret == -1) {
		printf("Set speed failed : %d\n", ret);
		return ROKID_XMOS_ERROR_INIT_FAILED;
	}
	ret = set_parity(fd_xmos, 8, 1, 0);
	if (ret == -1) {
		printf("Set parity failed : %d\n", ret);
		return ROKID_XMOS_ERROR_INIT_FAILED;
	}

	back_led_fd = open("/dev/dm163", O_RDWR | O_NONBLOCK);
	if (back_led_fd < 0) {
		perror("open dm163 error");
	}

	return fd_xmos;
}

void xmos_dev_close(int xmos_d)
{
	close_serial(xmos_d);
}

int xmos_dev_encode_data(unsigned char *des, int start, unsigned char *src, int src_len)
{
    int i;
    for (i = 0; i < src_len; i ++) {
        //if (src[i] == 0x00) {
        //    des[i * 2 + start] = ESCAPE_DATA_ZERO;
        //    des[i * 2 + start + 1] = 0x01;
        //} else {
            des[i * 2 + start] = ESCAPE_DATA;
            des[i * 2 + start + 1] = src[i];
        //}
    }
    return 0;
}

int xmos_dev_read(int xmos_d, unsigned char *data, int data_len, int *actual)
{
    *actual = read(xmos_d, data, data_len);
    return 0;
}

int xmos_dev_write(int xmos_d, unsigned char order, unsigned char *data, int data_len)
{
    int xmos_data_len = HEADER_LEN + data_len * 2 + FOOTER_LEN;
    int index = 0;
    xmos_write_data[index] = ESCAPE_HEADER;
    index ++;
    xmos_write_data[index] = order;
    index ++;
    xmos_dev_encode_data(xmos_write_data, index, data, data_len);
    index += data_len * 2;
    xmos_write_data[index] = ESCAPE_FOOTER;
    index ++;
    xmos_write_data[index] = ESCAPE_FOOTER;
    int actual = xmos_data_len;
    int ret;
    xmos_dev_write_group(xmos_d, xmos_write_data, xmos_data_len);
    ret = (actual == xmos_data_len) ? 0 : ROKID_XMOS_ERROR_WRITE_FAILED;
    return ret;
}

void xmos_dev_write_group(int xmos_d, unsigned char *data, int data_len)
{
//    // timer stop
//    ualarm(0, 0);
    xmos_d_for_group = xmos_d;
    int i;
    for (i = 0; i < data_len; i ++) {
        xmos_write_data_group[xmos_write_data_group_index ++] = data[i];
        if (xmos_write_data_group_index >= 512) {
            xmos_dev_write_current_data(10);
            xmos_write_data_group_index = 0;
        }
    }
//    if (xmos_write_data_group_index > 0) {
//        // timer start
//        ualarm(5000, 0);
//        signal(SIGALRM, xmos_dev_write_group_no_wait);
//    }
}

void xmos_dev_write_group_no_wait()
{
    xmos_dev_write_current_data(10);
    xmos_write_data_group_index = 0;
}

void xmos_dev_write_current_data(int retry_times)
{
    int actual;
    do {
        actual = write(xmos_d_for_group, xmos_write_data_group, xmos_write_data_group_index);
        if (actual == -1) usleep(50 * 1000);
        retry_times --;
    } while (actual == -1 && errno == EAGAIN && retry_times >= 0);
}

/*
 * LED Transfer
 */

int xmos_dev_led_flush_frame(int xmos_d,
                             unsigned char *data,
                             int data_len)
{
	if (data_len > FRAME_LEN) return ROKID_XMOS_ERROR_FRAME_LEN;
	int ret;
	if (data_len == FRAME_LEN) {
		ret = xmos_dev_write(xmos_d, ORDER_LED_FLUSH_FRAME, data, (FRAME_LEN - BACK_FRAME_LEN));
		data += (FRAME_LEN - BACK_FRAME_LEN);
		unsigned char back_tmp_data[BACK_FRAME_LEN + 18];
		int i;
		for (i = 0; i < BACK_FRAME_LEN + 18; i ++) {
		    if (i < 18) back_tmp_data[i] = 0xff;
		    else {
		        if (i % 3 == 0) back_tmp_data[i] = data[i - 18 + 2];
		        if (i % 3 == 1) back_tmp_data[i] = data[i - 18];
		        if (i % 3 == 2) back_tmp_data[i] = data[i - 18 - 2];
		    }
		}
#ifdef ANDROID
		ret = write(back_led_fd, back_tmp_data, BACK_FRAME_LEN + 18);
#endif
		if (ret < 0) {
			perror("back led write error");
		}
	} else {
		ret = xmos_dev_write(xmos_d, ORDER_LED_FLUSH_FRAME, data, data_len);
	}
	return ret;
}

/*
 * Codec Transfer
 */

int xmos_dev_codec_setting_eq(int xmos_d,
                              unsigned char eq)
{
    // eq = 0x00 -> EFFECT_TYPE_MUSIC
    // eq = 0x01 -> EFFECT_TYPE_TTS
    int data_len = 2;
    unsigned char data[2] = {0x01, eq};
    return xmos_dev_write(xmos_d, ORDER_CODEC_SETTING, data, data_len);
}

/*
 * Electric I2C Transfer
 */

int xmos_dev_electric_i2c_write(int xmos_d,
                            unsigned char device_addr,
                            unsigned char *data,
                            int data_len,
                            int send_stop_bit)
{
    if (data_len > 251) return ROKID_XMOS_ERROR_FRAME_LEN;
    int i;
    int i2c_data_len = data_len + 3;
    unsigned char i2c_data[i2c_data_len];
    i2c_data[0] = 0x01;// write
    i2c_data[1] = device_addr;
    i2c_data[2] = (send_stop_bit == 0) ? 0x00 : 0x01;
    for (i = 0; i < data_len; i ++) {
        i2c_data[i + 3] = data[i];
    }
    int ret;
    ret = xmos_dev_write(xmos_d, ORDER_ELECTRIC_I2C_RW, i2c_data, i2c_data_len);
    if (ret != 0) return ret;
    return ret;
}

int xmos_dev_electric_i2c_read(int xmos_d,
                           unsigned char device_addr,
                           unsigned char data_len,
                           int send_stop_bit)
{
    if (data_len > 251) return ROKID_XMOS_ERROR_FRAME_LEN;
    int i;
    int i2c_data_len = 4;
    unsigned char i2c_data[i2c_data_len];
    i2c_data[0] = 0x02;// read
    i2c_data[1] = device_addr;
    i2c_data[2] = (send_stop_bit == 0) ? 0x00 : 0x01;
    i2c_data[3] = data_len;
    int ret;
    ret = xmos_dev_write(xmos_d, ORDER_ELECTRIC_I2C_RW, i2c_data, i2c_data_len);
    if (ret != 0) return ret;
    return ret;
}

int xmos_dev_electric_i2c_send_stop_bit(int xmos_d)
{
    int i2c_data_len = 3;
    unsigned char i2c_data[i2c_data_len];
    i2c_data[0] = 0x03;// send_stop_bit
    i2c_data[1] = 0x00;// device_addr -> No need
    i2c_data[2] = 0x01;// (send_stop_bit == 0) ? 0x00 : 0x01 -> No need
    int ret;
    ret = xmos_dev_write(xmos_d, ORDER_ELECTRIC_I2C_RW, i2c_data, i2c_data_len);
    if (ret != 0) return ret;
    return ret;
}

int xmos_dev_electric_gpio_chg_stat(int xmos_d)
{
    int i2c_data_len = 3;
    unsigned char i2c_data[i2c_data_len];
    i2c_data[0] = 0x04;// send_stop_bit
    i2c_data[1] = 0x00;// device_addr -> No need
    i2c_data[2] = 0x00;// (send_stop_bit == 0) ? 0x00 : 0x01 -> No need
    int ret;
    ret = xmos_dev_write(xmos_d, ORDER_ELECTRIC_I2C_RW, i2c_data, i2c_data_len);
    if (ret != 0) return ret;
    return ret;
}

int fuel_feedback(int fd, unsigned char *data, int data_len)
{
    printf("kb = %d\n", data_len);
    int xmos_data_len = HEADER_LEN + data_len * 2 + FOOTER_LEN;
    int index = 0;
    xmos_write_data[index] = ESCAPE_HEADER;
    index ++;
    xmos_write_data[index] = ORDER_ELECTRIC_I2C_RW;
    index ++;
    xmos_dev_encode_data(xmos_write_data, index, data, data_len);
    index += data_len * 2;
    xmos_write_data[index] = ESCAPE_FOOTER;
    index ++;
    xmos_write_data[index] = ESCAPE_FOOTER;
    int actual;
    int ret;
    actual = write(fd, xmos_write_data, xmos_data_len);
    printf("kb back = %d\n", actual);
    ret = (actual == xmos_data_len) ? 0 : ROKID_XMOS_ERROR_WRITE_FAILED;
    return ret;
}

