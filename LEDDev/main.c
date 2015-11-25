#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#define offsetof(TYPE, MEMBER) ((int)&((TYPE *)0)->MEMBER)

char *sock_addr = "/tmp/xmosd_led";

int main(int argc, char **argv)
{

    char *msg = "Hello, world\n";
    int addrlen, fd;

    struct sockaddr_un addr_un;

    if (argc== 3)
        sock_addr = argv[2];

    addr_un.sun_family = AF_UNIX;
    strcpy(addr_un.sun_path, sock_addr);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    addrlen = offsetof(struct sockaddr_un, sun_path) + strlen(addr_un.sun_path);
    if (connect(fd, (struct sockaddr *)&addr_un,  addrlen) < 0) {
        perror("connect");
        return -1;
    }


    while (1) {
        if (write(fd, argv[1], strlen(argv[1])) < 0)
            perror("write");
        usleep(500*1000);
    }


    close(fd);

    return 0;
}







//#include <unistd.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include <signal.h>
//
//#include "rokid_led.h"
//
//struct led_dev *led_dd;
//
//int running = 1;
//
//void cs(int n)
//{
//    led_dev_close(led_dd);
//    running = 0;
//    printf("now dowm!\n");
//    exit(0);
//}
//
//int main(int argc, char **argv)
//{
//    printf("now alive!\n");
//
//    signal(SIGINT, cs);  //ctrl+c
//    signal(SIGTERM, cs);  //kill
//
//    int r;
//    led_dd = led_dev_open(&r);
//
//    int k = 0;
//    while (running) {
//        unsigned char *one_frame = (unsigned char *)malloc(LED_COUNT * 3);
//        int i;
//        for (i = 0; i < LED_COUNT * 3; i ++) {
//            if (i / 3 == k) {
//                if (i % 3 == 0) one_frame[i] = 0xff;
//                if (i % 3 == 1) one_frame[i] = 0xff;
//                if (i % 3 == 2) one_frame[i] = 0x00;
//            } else {
//                if (i % 3 == 0) one_frame[i] = 0x00;
//                if (i % 3 == 1) one_frame[i] = 0x00;
//                if (i % 3 == 2) one_frame[i] = 0x00;
//            }
//        }
//        k ++;
//        if (k >= LED_COUNT) k = 0;
//
//        int ret;
//        ret = led_dev_flush_frame(led_dd, one_frame, LED_COUNT * 3);
//        usleep(1000 * 1000);
//    }
//
//    return 0;
//}
