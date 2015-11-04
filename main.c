#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "rokid_led.h"

struct led_dev *led_dd;

int running = 1;

void test_i2c_read() {
	int r;
	char test_dev_addr = 0x6c;
	int test_send_stop_bit = 1;
	int test_to_read_len = 1;
	int readed_len = 256;
	unsigned char *readed_data = (unsigned char *)malloc(readed_len);
	r = led_dev_codec_i2c_read(led_dd, test_dev_addr, test_to_read_len, test_send_stop_bit, readed_data, &readed_len);
}

void test_i2c_write() {
	int r;
	char test_dev_addr = 0x6c;
	char test_reg_addr = 0x10;
	char test_val = 0x07;
	int test_send_stop_bit = 0;
	int test_i2c_len = 2;
	unsigned char test_i2c_dat[2] = {test_reg_addr, test_val};
	r = led_dev_codec_i2c_write(led_dd, test_dev_addr, test_i2c_dat, test_i2c_len, test_send_stop_bit);
}

void cs(int n)
{
	running = 0;
	led_dev_close(led_dd);
	printf("now dowm!\n");
	exit(0);
}

int main(int argc, char **argv)
{
	unsigned char br_r;
    unsigned char br_g;
    unsigned char br_b;

	br_r = 0xff;
    br_g = 0xff;
    br_b = 0xff;


	printf("now alive!\n");

	signal(SIGINT, cs);  //ctrl+c
	signal(SIGTERM, cs);  //kill

	int r;
	led_dd = led_dev_open(&r);
	printf("dev open success.\n");

	/*	
		r = led_dev_startup(led_dd);
		*/
	int k = 0;
	while (running) {
		unsigned char *one_frame = (unsigned char *)malloc(LED_COUNT * 3);
		int i;
		for (i = 0; i < LED_COUNT * 3; i ++) {
			if (i / 3 == k) {
				if (i % 3 == 0) one_frame[i] = 0xff;
				if (i % 3 == 1) one_frame[i] = 0xff;
				if (i % 3 == 2) one_frame[i] = 0x00;
			} else {
				if (i % 3 == 0) one_frame[i] = 0x00;
                        	if (i % 3 == 1) one_frame[i] = 0x00;
                        	if (i % 3 == 2) one_frame[i] = 0x00;

			}
		//if (i == ind) one_frame[i] = 0xff;
		//else one_frame[i] = 0x00;
		}
		k ++;
		if (k >= LED_COUNT) k = 0;

		r = led_dev_flush_frame(led_dd, one_frame, LED_COUNT * 3);
		//printf("strlen one_frame r = %d, %d, %d\n", r, j, d);
		usleep(17*1000);
		test_i2c_write();
		test_i2c_read();
		usleep(500 * 1000);
	}

	return 0;
}
