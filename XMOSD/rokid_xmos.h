#ifndef __ROKID_XMOS_H
#define __ROKID_XMOS_H

#define LED_COUNT 8*5

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

struct xmos_dev;

struct xmos_dev *xmos_dev_open();

void xmos_dev_close(struct xmos_dev *xmos_d);

int xmos_dev_read(struct xmos_dev *xmos_d,
				  unsigned char *data,
				  int data_len,
				  int *actual);

int xmos_dev_write(struct xmos_dev *xmos_d,
				   unsigned char order,
				   unsigned char *data,
				   int data_len);

/*
 * LED Transfer
 */

int xmos_dev_led_flush_frame(struct xmos_dev *xmos_d,
							 unsigned char *data,
							 int data_len);

/*
 * Codec Setting Transfer
 */

int xmos_dev_codec_setting_eq(struct xmos_dev *xmos_d,
							  unsigned char eq);

/*
 * Electric I2C Transfer
 */

int xmos_dev_electric_i2c_write(struct xmos_dev *xmos_d,
							unsigned char device_addr,
							unsigned char *data,
							int data_len,
							int send_stop_bit,
							int *wrote_len);

int xmos_dev_electric_i2c_read(struct xmos_dev *xmos_d,
						   unsigned char device_addr,
						   unsigned char data_len,
						   int send_stop_bit,
						   unsigned char *readed_data,
						   int *readed_len);

int xmos_dev_electric_i2c_send_stop_bit(struct xmos_dev *xmos_d);

int xmos_dev_electric_i2c_write_reg(struct xmos_dev *xmos_d,
									unsigned char device_addr,
									unsigned char reg,
									unsigned char data);

int xmos_dev_electric_i2c_read_reg(struct xmos_dev *xmos_d,
								   unsigned char device_addr,
								   unsigned char reg_addr,
								   unsigned char *data);

#endif
