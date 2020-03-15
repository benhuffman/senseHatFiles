#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "i2c.h"
#include "hw.h"


typedef struct {
    __s16 T0_OUT;
    __s16 T1_OUT;
    float T0_degC;
    float T1_degC;
    __s16 H0_T0_OUT;
    __s16 H1_T0_OUT;
    float H0_rH;
    float H1_rH;
} humidity_calibration_t;
humidity_calibration_t h_calib;

static void getHumidCalibrationData(unsigned char* data,humidity_calibration_t* calib){
    __u16 temp;
    __u8 temp2;
    calib->T0_OUT=((__s16*)(data))[6];
    calib->T1_OUT=((__s16*)(data))[7];
    temp=(((__u16)data[5] & 0x3 ) << 8) | (__u16)data[2];
    calib->T0_degC=(float)(temp) / 8.0f;
    temp=(((__u16)data[5] & 0xc ) << 6) | (__u16)data[3];
    calib->T1_degC=(float)(temp) / 8.0f;

    calib->H0_T0_OUT = ((__s16*)(data))[3];
    calib->H1_T0_OUT = ((__s16*)(data))[5];
    temp2 = ((__u8*)(data))[0];
    calib->H0_rH = (float)(temp2) / 2.0f;
    temp2 = ((__u8*)(data))[1];
    calib->H1_rH = (float)(temp2) / 2.0f;
}

int configureHumiditySensor(pi_i2c_t* device){
    int res;
    char buf[16];
    usei2cConnectedDevice(device,HTS221_I2C_ID);
    buf[0]=HTS221_REG_CTRL_REG1;
    buf[1]=0x87; /*10000111*/
    res=write(device->fd,buf,2);
    buf[0]=HTS221_REG_AV_CONF;
    buf[1]=0x1B; /*00011011*/
    res=write(device->fd,buf,2);
    buf[0]=HTS221_REG16_CALIBRATION | HTS221_REG_AUTO;
    res = write( device->fd, buf, 1 );
    res  = read( device->fd, buf, 16 );
    getHumidCalibrationData(buf,&h_calib);
}

float getTemp(pi_i2c_t* device){
    __s16 tempData;
    int res;
    char buf[2];
    usei2cConnectedDevice(device,HTS221_I2C_ID);
    buf[0] = HTS221_REG_TEMP_OUT_L|HTS221_REG_AUTO;
    res=write(device->fd,buf,1);
    res=read(device->fd,buf,2);
    tempData= (((__s16)buf[1])<<8)|(__s16)buf[0];
    return h_calib.T0_degC + (((float)(tempData)-h_calib.T0_OUT)/
        (h_calib.T1_OUT-h_calib.T0_OUT))*(h_calib.T1_degC-h_calib.T0_degC);

}
float getHumidity(pi_i2c_t* device){
    __s16 tempData;
    int res;
    char buf[2];
    usei2cConnectedDevice(device,HTS221_I2C_ID);
    buf[0]=HTS221_REG_HUMIDITY_OUT_L|HTS221_REG_AUTO;
    res = write(device->fd,buf,1);
    res = read(device->fd,buf,2);
    tempData=(((__s16)buf[1])<<8) | (__s16)buf[0];
    return h_calib.H0_rH + (((float)(tempData)-h_calib.H0_T0_OUT)/
        (h_calib.H1_T0_OUT-h_calib.H0_T0_OUT))*(h_calib.H1_rH-h_calib.H0_rH);

}
