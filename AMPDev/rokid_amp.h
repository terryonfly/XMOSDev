//
// Created by Terry on 15/11/25.
//

#ifndef AMPDEV_ROKID_AMP_H
#define AMPDEV_ROKID_AMP_H

struct amp_dev;

struct amp_dev *amp_dev_open();

void amp_dev_close(struct amp_dev *amp_d);

int amp_dev_set_eq(struct amp_dev *amp_d, unsigned char eq);

#endif //AMPDEV_ROKID_AMP_H
