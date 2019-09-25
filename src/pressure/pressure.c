#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <linux/i2c-dev.h>
#include "hw.h"
#include "i2c.h"

int configurePressureSensor(pi_i2c_t* device){
    int res;
    char buf[3];
    usei2cConnectedDevice(device,LPS25H_I2C_ID);
    buf[0]=LPS25H_REG_CTRL_REG1;
    buf[1]=0xB4; /*10110100*/
    res=write(device->fd,buf,2);
    buf[0]=LPS25H_REG_RES_CONF;
    buf[1]=0x3; /*00000011*/
    res=write(device->fd,buf,2);
    buf[0] = LPS25H_REG_FIFO_CTRL;
    buf[1]=0xC1; /*11000001*/
    res=write(device->fd,buf,2);
}

float getPressure(pi_i2c_t* device){
    __s32 tempData;
    int res;
    char buf[3];
    usei2cConnectedDevice(device,LPS25H_I2C_ID);
    buf[0]=LPS25H_REG_PRESS_OUT_XL|LPS25H_REG_AUTO;
    res=write(device->fd,buf,1);
    res=read(device->fd,buf,3);
    tempData= (((__s32)(buf[2])) << 16) | (((__s32)(buf[1])) << 8) | (((__s32)(buf[0])));
    return (float)(tempData) / 4096.0f;
}
