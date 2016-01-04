#ifndef __ROKID_XMOS_H
#define __ROKID_XMOS_H

#define XMOS_LED_COUNT 8*4
#define BACK_LED_COUNT 8*1

enum rokid_xmos_error {
	/* Success (no error) */
	ROKID_XMOS_SUCCESS = 0,

	/* No such device (or maybe have been disconnected) */
	ROKID_XMOS_ERROR_NO_DEVICE = -201,

	/* Write data failed */
	ROKID_XMOS_ERROR_WRITE_FAILED = -202,

	/* Read data failed */
	ROKID_XMOS_ERROR_READ_FAILED = -203,

	/* Frame buffer length dose not match */
	ROKID_XMOS_ERROR_FRAME_LEN = -204, 

	/* LED device init error */
	ROKID_XMOS_ERROR_INIT_FAILED = -205,

	/* Can not open device */
	ROKID_XMOS_ERROR_DEVICE_OPEN_FAILED = -207,
};

int xmos_dev_open();

void xmos_dev_close(int xmos_d);

int xmos_dev_read(int xmos_d,
				  unsigned char *data,
				  int data_len,
				  int *actual);

int xmos_dev_write(int xmos_d,
				   unsigned char order,
				   unsigned char *data,
				   int data_len, int retry_times);

/*
 * LED Transfer
 */

int xmos_dev_led_flush_frame(int xmos_d,
							 unsigned char *data,
							 int data_len);

/*
 * Codec Setting Transfer
 */

int xmos_dev_codec_setting_eq(int xmos_d,
							  unsigned char eq);

/*
 * Electric I2C Transfer
 */

int xmos_dev_electric_i2c_write(int xmos_d,
							unsigned char device_addr,
							unsigned char *data,
							int data_len,
							int send_stop_bit);

int xmos_dev_electric_i2c_read(int xmos_d,
						   unsigned char device_addr,
						   unsigned char data_len,
						   int send_stop_bit);

int xmos_dev_electric_i2c_send_stop_bit(int xmos_d);

unsigned char check_fuel_command_return();

int xmos_dev_electric_gpio_chg_stat(int xmos_d);

int fuel_feedback(int fd, unsigned char *data, int data_len);

#endif
