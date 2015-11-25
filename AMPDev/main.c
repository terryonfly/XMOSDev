#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "rokid_amp.h"

struct amp_dev *amp_dd;

int running = 1;

void cs(int n)
{
    amp_dev_close(amp_dd);
    running = 0;
    printf("now dowm!\n");
    exit(0);
}

int main(int argc, char **argv)
{
    printf("now alive!\n");

    signal(SIGINT, cs);  //ctrl+c
    signal(SIGTERM, cs);  //kill

    int r;
    amp_dd = amp_dev_open(&r);

    int k = 0;
    while (running) {
        k = !k;
        int ret;
        ret = amp_dev_set_eq(amp_dd, k);
        usleep(1000 * 1000);
    }

    return 0;
}