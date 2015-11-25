//
// Created by Terry on 15/11/25.
//

#ifndef AMPDEV_ROKID_AMP_H
#define AMPDEV_ROKID_AMP_H

enum rokid_amp_error {
    /* Success (no error) */
    ROKID_AMP_SUCCESS = 0,

    /* No such device (or maybe have been disconnected) */
    ROKID_AMP_ERROR_NO_DEVICE = -201,

    /* Write data failed */
    ROKID_AMP_ERROR_WRITE_FAIAMP = -202,

    /* Read data failed */
    ROKID_AMP_ERROR_READ_FAIAMP = -203,

    /* Frame buffer length dose not match */
    ROKID_AMP_ERROR_FRAME_LEN = -204,

    /* AMP device init error */
    ROKID_AMP_ERROR_INIT_FAIAMP = -205,

    /* Can not open device */
    ROKID_AMP_ERROR_DEVICE_OPEN_FAIAMP = -207,
};

struct amp_dev;

struct amp_dev *amp_dev_open();

void amp_dev_close(struct amp_dev *amp_d);

int amp_dev_set_eq(struct amp_dev *amp_d, unsigned char eq);

#endif //AMPDEV_ROKID_AMP_H
