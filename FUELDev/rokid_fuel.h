//
// Created by Terry on 15/11/25.
//

#ifndef AMPDEV_ROKID_AMP_H
#define AMPDEV_ROKID_AMP_H

struct fuel_dev;

struct fuel_dev *fuel_dev_open();

void fuel_dev_close(struct fuel_dev *fuel_d);

int fuel_dev_i2c_write(struct fuel_dev *fuel_d,
                       unsigned char device_addr,
                       unsigned char *data,
                       int data_len,
                       int send_stop_bit,
                       int *wrote_len);

int fuel_dev_i2c_read(struct fuel_dev *fuel_d,
                      unsigned char device_addr,
                      unsigned char data_len,
                      int send_stop_bit,
                      unsigned char *readed_data,
                      int *readed_len);

int fuel_dev_i2c_send_stop_bit(struct fuel_dev *fuel_d);

int fuel_dev_i2c_write_reg(struct fuel_dev *fuel_d,
                           unsigned char device_addr,
                           unsigned char reg_addr,
                           unsigned char data);

int fuel_dev_i2c_read_reg(struct fuel_dev *fuel_d,
                          unsigned char device_addr,
                          unsigned char reg_addr,
                          unsigned char *data);

int fuel_dev_gpio_chg_stat(struct fuel_dev *fuel_d,
                           int *gpio_val);

#endif //AMPDEV_ROKID_AMP_H
