#ifndef __ROKID_LED_H
#define __ROKID_LED_H

#define LED_COUNT 8*4

enum rokid_led_error {
	/* Success (no error) */
	ROKID_LED_SUCCESS = 0,

	/* No such device (or maybe have been disconnected) */
	ROKID_LED_ERROR_NO_DEVICE = -201,

	/* Write data failed */
	ROKID_LED_ERROR_WRITE_FAILED = -202,

	/* Read data failed */
	ROKID_LED_ERROR_READ_FAILED = -203,

	/* Frame buffer length dose not match */
	ROKID_LED_ERROR_FRAME_LEN = -204, 

	/* LED device init error */
	ROKID_LED_ERROR_INIT_FAILED = -205,

	/* Can not open device */
	ROKID_LED_ERROR_DEVICE_OPEN_FAILED = -207,
};

struct led_dev;

struct led_dev *led_dev_open();

void led_dev_close(struct led_dev *led_d);

/*
int led_dev_startup(struct led_dev *led_d);
*/

int led_dev_flush_frame(struct led_dev *led_d, unsigned char *data, int data_len);

int led_dev_read(struct led_dev *led_d, unsigned char *data, int data_len, int *actual);

int led_dev_codec_i2c_write(struct led_dev *led_d, unsigned char device_addr, unsigned char *data, int data_len, int send_stop_bit);

int led_dev_codec_i2c_read(struct led_dev *led_d, unsigned char device_addr, unsigned char *data, int data_len, int send_stop_bit, unsigned char *readed_data, int *readed_len);

#endif
