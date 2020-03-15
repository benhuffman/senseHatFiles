#include <unistd.h>
#include <stdint.h>
#include "hw.h"

#include "i2c.h"
#include "mag.h"

int configureMag(pi_i2c_t* device){
    int res;
    char buf[16];
    usei2cConnectedDevice(device,LSM9DS1_MAG_I2C_ID);
    buf[0]=LSM9DS1_MAG_REG_CTRL_REG1_M;
    buf[1]=0x10; /*00010000 SR=5*/
    res=write(device->fd,buf,2);
    buf[0]=LSM9DS1_MAG_REG_CTRL_REG2_M;
    buf[1]=0x00; /*00000000 FSR=0*/
    res=write(device->fd,buf,2);
    buf[0]=LSM9DS1_MAG_REG_CTRL_REG3_M;
    buf[1]=0x00; /*00000000 must be 0*/
    res=write(device->fd,buf,2);
}

void getMagData(pi_i2c_t* device,coordinate_t* result){
    int res,status,i;
    double scale;
    char buf[2];
    char data[6];
    for (i=0;i<6;i++){
        buf[0]=LSM9DS1_MAG_REG_OUT_X_L_M+i;
        res=write(device->fd,buf,1);
        res=read(device->fd,&data[i],1);
    }
    scale=0.014; //do to FSR of 4
    result->x=(double)((int16_t)(((uint16_t)data[1] << 8) | (uint16_t)data[0])) * scale;
    result->y=(double)((int16_t)(((uint16_t)data[3] << 8) | (uint16_t)data[2])) * scale;
    result->z=(double)((int16_t)(((uint16_t)data[5] << 8) | (uint16_t)data[4])) * scale;
}
