#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "rokid_led.h"

struct led_dev *led_dd;

void cs(int n)
{
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
	while (1) {
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
		unsigned char i2c_t_dat[2] = {0x10, 0x07};
		r = led_dev_codec_i2c_write(led_dd, 0x6c, i2c_t_dat, 2, 1);
		//printf("strlen one_frame r = %d, %d, %d\n", r, j, d);
		usleep(500*1000);
	}

	return 0;
}
