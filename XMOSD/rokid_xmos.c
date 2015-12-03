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

#define SERIAL_DEV "/dev/ttyACM0"
//#define SERIAL_DEV "/dev/tty.usbmodem14235"

#define HEADER_LEN 2
#define FOOTER_LEN 2

#define ESCAPE_HEADER 0x81
#define ESCAPE_DATA 0x80
#define ESCAPE_DATA_ZERO 0x83
#define ESCAPE_FOOTER 0x82

#define ORDER_LED_FLUSH_FRAME 0x01
#define ORDER_CODEC_SETTING 0x02
#define ORDER_ELECTRIC_I2C_RW 0x03

#define FRAME_LEN (3 * LED_COUNT)

unsigned char xmos_i2c_data[512];
unsigned char xmos_write_data[1024];

unsigned char processing_fuel_command = 0x00;
time_t processing_fuel_time = 0;

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
        if (src[i] == 0x00) {
            des[i * 2 + start] = ESCAPE_DATA_ZERO;
            des[i * 2 + start + 1] = 0x01;
        } else {
            des[i * 2 + start] = ESCAPE_DATA;
            des[i * 2 + start + 1] = src[i];
        }
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
    int actual;
    int ret;
    actual = write(xmos_d, xmos_write_data, xmos_data_len);
    ret = (actual == xmos_data_len) ? 0 : ROKID_XMOS_ERROR_WRITE_FAILED;
    return ret;
}

/*
 * LED Transfer
 */

int xmos_dev_led_flush_frame(int xmos_d,
                             unsigned char *data,
                             int data_len)
{
	if (data_len > FRAME_LEN) return ROKID_XMOS_ERROR_FRAME_LEN;
	return xmos_dev_write(xmos_d, ORDER_LED_FLUSH_FRAME, data, data_len);
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
    processing_fuel_command = 0x01;
    time(&processing_fuel_time);
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
//	int read_actual;
//    ret = xmos_dev_read(xmos_d, xmos_i2c_data, 512, &read_actual);
//	/*
//	int k;
//	for (k = 0; k < read_actual; k ++) {
//		printf("%02x ", xmos_i2c_data[k]);
//	}
//	printf("\n");
//	*/
//	if (read_actual % 2 != 0) return ROKID_XMOS_ERROR_WRITE_FAILED;
//	if (read_actual < 6) return ROKID_XMOS_ERROR_WRITE_FAILED;
//	if (xmos_i2c_data[0] != 0x81) return ROKID_XMOS_ERROR_WRITE_FAILED;
//	if (xmos_i2c_data[1] != 0x03) return ROKID_XMOS_ERROR_WRITE_FAILED;
//	if (xmos_i2c_data[read_actual - 2] != 0x82) return ROKID_XMOS_ERROR_WRITE_FAILED;
//	if (xmos_i2c_data[read_actual - 1] != 0x82) return ROKID_XMOS_ERROR_WRITE_FAILED;
//	if (xmos_i2c_data[2] != 0x80) return ROKID_XMOS_ERROR_WRITE_FAILED;
//	if (xmos_i2c_data[3] != 0x01) return ROKID_XMOS_ERROR_WRITE_FAILED;// order
//	if (xmos_i2c_data[4] != 0x80) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[5] != 0x01) return ROKID_XMOS_ERROR_WRITE_FAILED;// ack or nack
//	if (xmos_i2c_data[6] != 0x80) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    unsigned char i2c_write_bytes = xmos_i2c_data[7];
//    printf("i2c wrote %d bytes\n", i2c_write_bytes);
//    *wrote_len = i2c_write_bytes;
    return ret;
}

int xmos_dev_electric_i2c_read(int xmos_d,
                           unsigned char device_addr,
                           unsigned char data_len,
                           int send_stop_bit)
{
    processing_fuel_command = 0x02;
    time(&processing_fuel_time);
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
//    int read_actual;
//    ret = xmos_dev_read(xmos_d, xmos_i2c_data, 512, &read_actual);
//    /*
//    int k;
//    for (k = 0; k < read_actual; k ++) {
//        printf("%02x ", xmos_i2c_data[k]);
//    }
//    printf("\n");
//    */
//    if (read_actual % 2 != 0) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (read_actual < 6) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (xmos_i2c_data[0] != 0x81) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (xmos_i2c_data[1] != 0x03) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (xmos_i2c_data[read_actual - 2] != 0x82) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (xmos_i2c_data[read_actual - 1] != 0x82) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (xmos_i2c_data[2] != 0x80) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (xmos_i2c_data[3] != 0x02) return ROKID_XMOS_ERROR_READ_FAILED;// order
//    if (xmos_i2c_data[4] != 0x80) return ROKID_XMOS_ERROR_READ_FAILED;
//    if (xmos_i2c_data[5] != 0x01) return ROKID_XMOS_ERROR_READ_FAILED;// ack or nack
//    int readed_i = 0;
//    printf("i2c readed ");
//    for (i = 6; i < read_actual - 2; i ++) {
//        if (xmos_i2c_data[i] != 0x80) return ROKID_XMOS_ERROR_READ_FAILED;
//        i++;
//        readed_data[readed_i] = xmos_i2c_data[i];
//        printf("%02x ", readed_data[readed_i]);
//        readed_i ++;
//    }
//    printf("\n");
//    *readed_len = readed_i;
    return ret;
}

int xmos_dev_electric_i2c_send_stop_bit(int xmos_d)
{
    processing_fuel_command = 0x03;
    time(&processing_fuel_time);
    int i2c_data_len = 3;
    unsigned char i2c_data[i2c_data_len];
    i2c_data[0] = 0x03;// send_stop_bit
    i2c_data[1] = 0x00;// device_addr -> No need
    i2c_data[2] = 0x01;// (send_stop_bit == 0) ? 0x00 : 0x01 -> No need
    int ret;
    ret = xmos_dev_write(xmos_d, ORDER_ELECTRIC_I2C_RW, i2c_data, i2c_data_len);
    if (ret != 0) return ret;
//    int read_actual;
//    ret = xmos_dev_read(xmos_d, xmos_i2c_data, 512, &read_actual);
//    /*
//    int k;
//    for (k = 0; k < read_actual; k ++) {
//        printf("%02x ", xmos_i2c_data[k]);
//    }
//    printf("\n");
//    */
//    int i;
//    if (read_actual % 2 != 0) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (read_actual < 6) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[0] != 0x81) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[1] != 0x03) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[read_actual - 2] != 0x82) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[read_actual - 1] != 0x82) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[2] != 0x80) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[3] != 0x01) return ROKID_XMOS_ERROR_WRITE_FAILED;// order
//    if (xmos_i2c_data[4] != 0x80) return ROKID_XMOS_ERROR_WRITE_FAILED;
//    if (xmos_i2c_data[5] != 0x01) return ROKID_XMOS_ERROR_WRITE_FAILED;// ack or nack
//    printf("stop_bit sent\n");
    return ret;
}

unsigned char check_fuel_command_return()
{
    if (processing_fuel_command != 0x00) {
        time_t now_time;
        time(&now_time);
        if (now_time - processing_fuel_time > 2) {
            unsigned char command = processing_fuel_command;
            processing_fuel_command = 0x00;
            return command;
        }
    }
    return 0x00;
}

int fuel_feedback(int fd, unsigned char *data, int data_len)
{
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
    ret = (actual == xmos_data_len) ? 0 : ROKID_XMOS_ERROR_WRITE_FAILED;
    return ret;
}

