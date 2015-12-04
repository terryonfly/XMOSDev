#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "rokid_fuel.h"

struct fuel_dev *fuel_dd;

int running = 1;

// DEMO
void test_i2c_read_reg(struct fuel_dev *fuel_d) {
    int r;
    char test_dev_addr = 0x55;
    char test_reg_addr = 0x10;
    unsigned char test_val;
    r = fuel_dev_i2c_read_reg(fuel_d, test_dev_addr, test_reg_addr, &test_val);
}

void test_i2c_write_reg(struct fuel_dev *fuel_d) {
    int r;
    char test_dev_addr = 0x55;
    char test_reg_addr = 0x10;
    char test_val = 0x07;
    r = fuel_dev_i2c_write_reg(fuel_d, test_dev_addr, test_reg_addr, test_val);
}

void test_i2c_read(struct fuel_dev *fuel_d) {
    int r;
    char test_dev_addr = 0x55;
    int test_send_stop_bit = 1;
    int test_to_read_len = 1;
    int readed_len = 256;
    unsigned char *readed_data = (unsigned char *)malloc(readed_len);
    r = fuel_dev_i2c_read(fuel_d, test_dev_addr, test_to_read_len, test_send_stop_bit, readed_data, &readed_len);
}

void test_i2c_write(struct fuel_dev *fuel_d) {
    int r;
    char test_dev_addr = 0x55;
    char test_reg_addr = 0x10;
    char test_val = 0x07;
    int test_send_stop_bit = 1;
    int test_i2c_len = 2;
    unsigned char test_i2c_dat[2] = {test_reg_addr, test_val};
    int test_wrote_len;
    r = fuel_dev_i2c_write(fuel_d, test_dev_addr, test_i2c_dat, test_i2c_len, test_send_stop_bit, &test_wrote_len);
}

void cs(int n)
{
    fuel_dev_close(fuel_dd);
    running = 0;
    printf("now dowm!\n");
    exit(0);
}

int main(int argc, char **argv)
{
    printf("now alive!\n");

    signal(SIGINT, cs);  //ctrl+c
    signal(SIGTERM, cs);  //kill

    fuel_dd = fuel_dev_open();

    int k = 0;
    while (running) {
        switch (k) {
            case 0:
            {
                printf("func -> test_i2c_read_reg\n");
                test_i2c_read_reg(fuel_dd);
            }
            case 1:
            {
                printf("func -> test_i2c_write_reg\n");
                test_i2c_write_reg(fuel_dd);
            }
            case 2:
            {
                printf("func -> test_i2c_read\n");
                test_i2c_read(fuel_dd);
            }
            case 3:
            {
                printf("func -> test_i2c_write\n");
                test_i2c_write(fuel_dd);
            }
        }
        usleep(1000 * 1000);

        k ++;
        if (k >= 4) k = 0;
    }

    return 0;
}