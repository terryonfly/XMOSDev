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

#include "rokid_led.h"

#define SERIAL_DEV "/dev/ttyACM0"
//#define SERIAL_DEV "/dev/tty.usbmodem14235"

static int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1800,
    B1200, B600, B300, B200, B150, B134, B110, B75, B50};

static int name_arr[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1800,
    1200, 600, 300, 200, 150, 134, 110, 75, 50};

// Serial
int open_serial(const char *dev_path)
{
    const char *str = dev_path;
    int fd;
    if (str == NULL)
        str = SERIAL_DEV;

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

#define FRAME_LEN (3 * LED_COUNT)
#define HEADER_LEN 2
#define FOOTER_LEN 2

#define ESCAPE_DATA 0x80
#define ESCAPE_DATA_ZERO 0x83
#define ESCAPE_HEADER 0x81
#define ESCAPE_FOOTER 0x82

#define ORDER_LED_FLUSH_FRAME 0x01
#define ORDER_LED_STARTUP 0x02
#define ORDER_CODEC_I2C_RW 0x03

typedef struct led_dev {
	int fd_xmos;
	unsigned char *frame_data;
	unsigned char *i2c_data;
} led_dev;

struct led_dev *led_dev_open()
{
	int fd_xmos = open_serial(SERIAL_DEV);
	if (fd_xmos == -1) {
		printf("Cannot open device\n");
                return (struct led_dev *)ROKID_LED_ERROR_DEVICE_OPEN_FAILED;
	}
	int ret;
	ret = set_speed(fd_xmos, 115200);
	if (ret == -1) {
		printf("Set speed failed : %d\n", ret);
		return (struct led_dev *)ROKID_LED_ERROR_INIT_FAILED;
	}
	ret = set_parity(fd_xmos, 8, 1, 0);
	if (ret == -1) {
		printf("Set parity failed : %d\n", ret);
		return (struct led_dev *)ROKID_LED_ERROR_INIT_FAILED;
	}

	struct led_dev *led_d = malloc(sizeof(struct led_dev));
	led_d->fd_xmos = fd_xmos;
	int frame_data_len = HEADER_LEN + FRAME_LEN * 2 + FOOTER_LEN;
	led_d->frame_data = (unsigned char *)malloc(frame_data_len);
	led_d->i2c_data = (unsigned char *)malloc(512);
	return led_d;
}

void led_dev_close(struct led_dev *led_d)
{
	close_serial(led_d->fd_xmos);
	led_d->fd_xmos = -1;
	free(led_d->frame_data);
	free(led_d->i2c_data);
	free(led_d);
	led_d = NULL;
}

static int led_dev_write(struct led_dev *led_d, unsigned char *data, int data_len, int *actual)
{
	*actual = write(led_d->fd_xmos, data, data_len);
	int ret;
	ret = (*actual == data_len) ? 0 : ROKID_LED_ERROR_WRITE_FAILED;
        return ret;
}

int led_dev_read(struct led_dev *led_d, unsigned char *data, int data_len, int *actual)
{
	*actual = read(led_d->fd_xmos, data, data_len);
	int ret;
	ret = (*actual == data_len) ? 0 : ROKID_LED_ERROR_READ_FAILED;
	return ret;
}

static int led_dev_encode_data(unsigned char *des, int start, unsigned char *src, int src_len)
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

int led_dev_flush_frame(struct led_dev *led_d,
                        unsigned char *data,
                        int data_len)
{
	if (data_len != FRAME_LEN) return ROKID_LED_ERROR_FRAME_LEN;
	int frame_data_len = HEADER_LEN + data_len * 2 + FOOTER_LEN;
	int index = 0;
	led_d->frame_data[index] = ESCAPE_HEADER;
	index ++;
	led_d->frame_data[index] = ORDER_LED_FLUSH_FRAME;
	index ++;
	led_dev_encode_data(led_d->frame_data, index, data, data_len);
	index += data_len * 2;
	led_d->frame_data[index] = ESCAPE_FOOTER;
	index ++;
	led_d->frame_data[index] = ESCAPE_FOOTER;
	int actual;
	int r;
	r = led_dev_write(led_d, led_d->frame_data, frame_data_len, &actual);
	return r;
}

int led_dev_codec_i2c_write(struct led_dev *led_d,
                            unsigned char device_addr,
                            unsigned char *data,
                            int data_len,
                            int send_stop_bit,
                            int *wrote_len)
{
    if (data_len > 251) return ROKID_LED_ERROR_FRAME_LEN;
    int i2c_data_len = HEADER_LEN + 6 + data_len * 2 + FOOTER_LEN;
    int index = 0;
	led_d->i2c_data[index] = ESCAPE_HEADER;
    index ++;
    led_d->i2c_data[index] = ORDER_CODEC_I2C_RW;
    index ++;
	led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = 0x01;//write
    index ++;
	led_d->i2c_data[index] = ESCAPE_DATA;
	index ++;
	led_d->i2c_data[index] = device_addr;
	index ++;
	led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = (send_stop_bit == 0) ? 0x00 : 0x01;
    index ++;
    led_dev_encode_data(led_d->i2c_data, index, data, data_len);
    index += data_len * 2;
    led_d->i2c_data[index] = ESCAPE_FOOTER;
    index ++;
    led_d->i2c_data[index] = ESCAPE_FOOTER;
    int actual;
    int r;
    r = led_dev_write(led_d, led_d->i2c_data, i2c_data_len, &actual);
    int read_r;
	int read_actual;
    read_r = led_dev_read(led_d, led_d->i2c_data, 512, &read_actual);
	/*
	int k;
	for (k = 0; k < read_actual; k ++) {
		printf("%02x ", led_d->i2c_data[k]);
	}
	printf("\n");
	*/
	int i;
	if (read_actual % 2 != 0) return ROKID_LED_ERROR_WRITE_FAILED;
	if (read_actual < 6) return ROKID_LED_ERROR_WRITE_FAILED;
	if (led_d->i2c_data[0] != 0x81) return ROKID_LED_ERROR_WRITE_FAILED;
	if (led_d->i2c_data[1] != 0x03) return ROKID_LED_ERROR_WRITE_FAILED;
	if (led_d->i2c_data[read_actual - 2] != 0x82) return ROKID_LED_ERROR_WRITE_FAILED;
	if (led_d->i2c_data[read_actual - 1] != 0x82) return ROKID_LED_ERROR_WRITE_FAILED;
	if (led_d->i2c_data[2] != 0x80) return ROKID_LED_ERROR_WRITE_FAILED;
	if (led_d->i2c_data[3] != 0x01) return ROKID_LED_ERROR_WRITE_FAILED;// order
	if (led_d->i2c_data[4] != 0x80) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[5] != 0x01) return ROKID_LED_ERROR_WRITE_FAILED;// ack or nack
	if (led_d->i2c_data[6] != 0x80) return ROKID_LED_ERROR_WRITE_FAILED;
    unsigned char i2c_write_bytes = led_d->i2c_data[7];
    printf("i2c wrote %d bytes\n", i2c_write_bytes);
    *wrote_len = i2c_write_bytes;
    return read_r;
}

int led_dev_codec_i2c_read(struct led_dev *led_d,
                           unsigned char device_addr,
                           unsigned char data_len,
                           int send_stop_bit,
                           unsigned char *readed_data,
                           int *readed_len)
{
    if (data_len > 251) return ROKID_LED_ERROR_FRAME_LEN;
    int i2c_data_len = HEADER_LEN + 6 + 1 * 2 + FOOTER_LEN;
    int index = 0;
    led_d->i2c_data[index] = ESCAPE_HEADER;
    index ++;
    led_d->i2c_data[index] = ORDER_CODEC_I2C_RW;
    index ++;
    led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = 0x02;//read
    index ++;
    led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = device_addr;
    index ++;
    led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = (send_stop_bit == 0) ? 0x00 : 0x01;
    index ++;
    led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = data_len;
    index ++;
    led_d->i2c_data[index] = ESCAPE_FOOTER;
    index ++;
    led_d->i2c_data[index] = ESCAPE_FOOTER;
    int actual;
    int r;
    r = led_dev_write(led_d, led_d->i2c_data, i2c_data_len, &actual);
    int read_r;
    int read_actual;
    read_r = led_dev_read(led_d, led_d->i2c_data, 512, &read_actual);
    /*
    int k;
    for (k = 0; k < read_actual; k ++) {
        printf("%02x ", led_d->i2c_data[k]);
    }
    printf("\n");
    */
    int i;
    if (read_actual % 2 != 0) return ROKID_LED_ERROR_READ_FAILED;
    if (read_actual < 6) return ROKID_LED_ERROR_READ_FAILED;
    if (led_d->i2c_data[0] != 0x81) return ROKID_LED_ERROR_READ_FAILED;
    if (led_d->i2c_data[1] != 0x03) return ROKID_LED_ERROR_READ_FAILED;
    if (led_d->i2c_data[read_actual - 2] != 0x82) return ROKID_LED_ERROR_READ_FAILED;
    if (led_d->i2c_data[read_actual - 1] != 0x82) return ROKID_LED_ERROR_READ_FAILED;
    if (led_d->i2c_data[2] != 0x80) return ROKID_LED_ERROR_READ_FAILED;
    if (led_d->i2c_data[3] != 0x02) return ROKID_LED_ERROR_READ_FAILED;// order
    if (led_d->i2c_data[4] != 0x80) return ROKID_LED_ERROR_READ_FAILED;
    if (led_d->i2c_data[5] != 0x01) return ROKID_LED_ERROR_READ_FAILED;// ack or nack
    int readed_i = 0;
    printf("i2c readed ");
    for (i = 6; i < read_actual - 2; i ++) {
        if (led_d->i2c_data[i] != 0x80) return ROKID_LED_ERROR_READ_FAILED;
        i++;
        readed_data[readed_i] = led_d->i2c_data[i];
        printf("%02x ", readed_data[readed_i]);
        readed_i ++;
    }
    printf("\n");
    *readed_len = readed_i;
    return read_r;
}

int led_dev_codec_i2c_send_stop_bit(struct led_dev *led_d)
{
    int i2c_data_len = HEADER_LEN + 6 + FOOTER_LEN;
    int index = 0;
    led_d->i2c_data[index] = ESCAPE_HEADER;
    index ++;
    led_d->i2c_data[index] = ORDER_CODEC_I2C_RW;
    index ++;
    led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = 0x03;//send_stop_bit
    index ++;
    led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = 0x00;//device_addr; -> No need
    index ++;
    led_d->i2c_data[index] = ESCAPE_DATA;
    index ++;
    led_d->i2c_data[index] = 0x01;//(send_stop_bit == 0) ? 0x00 : 0x01; -> No need
    index ++;
    led_d->i2c_data[index] = ESCAPE_FOOTER;
    index ++;
    led_d->i2c_data[index] = ESCAPE_FOOTER;
    int actual;
    int r;
    r = led_dev_write(led_d, led_d->i2c_data, i2c_data_len, &actual);
    int read_r;
    int read_actual;
    read_r = led_dev_read(led_d, led_d->i2c_data, 512, &read_actual);
    /*
    int k;
    for (k = 0; k < read_actual; k ++) {
        printf("%02x ", led_d->i2c_data[k]);
    }
    printf("\n");
    */
    int i;
    if (read_actual % 2 != 0) return ROKID_LED_ERROR_WRITE_FAILED;
    if (read_actual < 6) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[0] != 0x81) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[1] != 0x03) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[read_actual - 2] != 0x82) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[read_actual - 1] != 0x82) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[2] != 0x80) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[3] != 0x01) return ROKID_LED_ERROR_WRITE_FAILED;// order
    if (led_d->i2c_data[4] != 0x80) return ROKID_LED_ERROR_WRITE_FAILED;
    if (led_d->i2c_data[5] != 0x01) return ROKID_LED_ERROR_WRITE_FAILED;// ack or nack
    printf("stop_bit sent\n");
    return read_r;
}

int led_dev_codec_i2c_write_reg(struct led_dev *led_d,
                                unsigned char device_addr,
                                unsigned char reg_addr,
                                unsigned char data)
{
    int r;
    int send_stop_bit = 0;
    int a_data_len = 2;
    unsigned char a_data[2] = {reg_addr, data};
    int wrote_len;
    r = led_dev_codec_i2c_write(led_d, device_addr, a_data, a_data_len, send_stop_bit, &wrote_len);

    if (wrote_len == 0)
        return ROKID_LED_ERROR_WRITE_FAILED;//I2C_REGOP_DEVICE_NACK;
    if (wrote_len < 2)
        return ROKID_LED_ERROR_WRITE_FAILED;//I2C_REGOP_INCOMPLETE;
    return 0;
}

int led_dev_codec_i2c_read_reg(struct led_dev *led_d,
                                unsigned char device_addr,
                                unsigned char reg_addr,
                                unsigned char *data)
{
    int r;
    int send_stop_bit = 0;
    int a_data_len = 1;
    unsigned char a_data[1] = {reg_addr};
    int wrote_len;
    r = led_dev_codec_i2c_write(led_d, device_addr, a_data, a_data_len, send_stop_bit, &wrote_len);
    if (wrote_len != 1) {
        r = ROKID_LED_ERROR_READ_FAILED;//I2C_REGOP_DEVICE_NACK;
        led_dev_codec_i2c_send_stop_bit(led_d);
        return r;
    }
    send_stop_bit = 1;
    int to_read_len = 1;
    int readed_len = 1;
    r = led_dev_codec_i2c_read(led_d, device_addr, to_read_len, send_stop_bit, data, &readed_len);
    return r;
}

