//
// Created by Terry on 15/11/24.
//
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "rokid_xmos.h"

void test_i2c_read_reg(struct xmos_dev *xmos_d) {
    int r;
    char test_dev_addr = 0x6c;
    char test_reg_addr = 0x10;
    unsigned char test_val;
    r = xmos_dev_electric_i2c_read_reg(xmos_d, test_dev_addr, test_reg_addr, &test_val);
}

void test_i2c_write_reg(struct xmos_dev *xmos_d) {
    int r;
    char test_dev_addr = 0x6c;
    char test_reg_addr = 0x10;
    char test_val = 0x07;
    r = xmos_dev_electric_i2c_write_reg(xmos_d, test_dev_addr, test_reg_addr, test_val);
}

void test_i2c_read(struct xmos_dev *xmos_d) {
    int r;
    char test_dev_addr = 0x6c;
    int test_send_stop_bit = 1;
    int test_to_read_len = 1;
    int readed_len = 256;
    unsigned char *readed_data = (unsigned char *)malloc(readed_len);
    r = xmos_dev_electric_i2c_read(xmos_d, test_dev_addr, test_to_read_len, test_send_stop_bit, readed_data, &readed_len);
}

void test_i2c_write(struct xmos_dev *xmos_d) {
    int r;
    char test_dev_addr = 0x6c;
    char test_reg_addr = 0x10;
    char test_val = 0x07;
    int test_send_stop_bit = 1;
    int test_i2c_len = 2;
    unsigned char test_i2c_dat[2] = {test_reg_addr, test_val};
    int test_wrote_len;
    r = xmos_dev_electric_i2c_write(xmos_d, test_dev_addr, test_i2c_dat, test_i2c_len, test_send_stop_bit, &test_wrote_len);
}
