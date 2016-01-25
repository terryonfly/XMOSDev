#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "rokid_led.h"

struct led_dev *led_dd;

int running = 1;

void cs(int n)
{
    led_dev_close(led_dd);
    running = 0;
    printf("now dowm!\n");
    exit(0);
}

int main(int argc, char **argv)
{
    printf("now alive!\n");

    signal(SIGINT, cs);  //ctrl+c
    signal(SIGTERM, cs);  //kill

    led_dd = led_dev_open();

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
        }
        k ++;
        if (k >= LED_COUNT) k = 0;

        int ret;
        ret = led_dev_flush_frame(led_dd, one_frame, LED_COUNT * 3);
	usleep(15 * 1000);
    }

    return 0;
}
