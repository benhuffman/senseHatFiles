#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include "hw.h"
#include "i2c.h"
#include "gyro.h"

#define GYRO_SAMPLE_RATE 119.0
#define GYRO_FULL_SCALE_RANGE 500.0
#define GYRO_IGNORE_COUNT 10
#define GYRO_BIAS_COUNT 100
#define GYRO_SCALE .000175 // 1/GYRO_SAMPLE_RATE*GYRO_FULL_SCALE_RANGE/GYRO_MAX_VALUE
#define GYRO_ACCEL_WEIGHT .1

/*configureAccelGyro
  device: a device structure to hold the configured device
  returns: non-zero on success, 0 on failure
  sets up the accel gyro IMU on the sense hat*/
int configureAccelGyro(pi_i2c_t* device){
    int res;
    char buf[16];
    usei2cConnectedDevice(device,LSM9DS1_ACCEL_I2C_ID);
    setRegister(device,LSM9DS1_ACCEL_REG_CTRL_REG8,0x80); //reboot
    sleep(1);
    /*10001001 SR=119,FSR=.0175rad:500,BW=1*/
    setRegister(device,LSM9DS1_ACCEL_REG_CTRL_REG1_G,0x69);
    /*01000100 HPF=4+turn on filter*/
    setRegister(device,LSM9DS1_ACCEL_REG_CTRL_REG3_G,0x44);
    /*01111011 SR=3,LPF=3,FSR=8*/
    setRegister(device,LSM9DS1_ACCEL_REG_CTRL_REG6_XL,0x7B);
    setRegister(device,LSM9DS1_ACCEL_REG_CTRL_REG7_XL,0);
    return 1;
}

/*getAccelData
    device: a populated i2c device structure
    result: a pointer to a coordinate_t structure to hold the acceleration data
    gets raw acceleration data from the IMU*/
int getAccelData(pi_i2c_t* device,coordinate_t* result){
    int res,i;
    double scale;
    char buf[2];
    char data[6];
    char status;
    getRegister(device,LSM9DS1_ACCEL_REG_STATUS_REG,&status);
    if (status & 0x3 == 0) return 0;
    for (i=0;i<6;i++){
        getRegister(device,LSM9DS1_ACCEL_REG_OUT_X_L_XL+i,&data[i]);
    }
    scale=0.000244; //do to FSR of 8
    result->x=(double)((int16_t)(((uint16_t)data[1] << 8) | (uint16_t)data[0])) * scale;
    result->y=(double)((int16_t)(((uint16_t)data[3] << 8) | (uint16_t)data[2])) * scale;
    result->z=(double)((int16_t)(((uint16_t)data[5] << 8) | (uint16_t)data[4])) * scale;
	return 1;
}

/*getGyroData
    device: a populated i2c device structure
    result: a pointer to a coordinate_t structure to hold the gyro data
    gets raw gyro data from the IMU*/
    int getGyroData(pi_i2c_t* device,coordinate_t* result){
    int res,i;
    double scale;
    char buf[2];
    char data[6];
    char status;
    getRegister(device,LSM9DS1_ACCEL_REG_STATUS_REG,&status);
    if (status & 0x3 == 0) return 0;
    for (i=0;i<6;i++){
        getRegister(device,LSM9DS1_ACCEL_REG_OUT_X_L_G+i,&data[i]);
    }
    scale=0.0175 * (3.1415926535/ 180.0); // do to FSR at 500
    result->x=(double)((int16_t)(((uint16_t)data[1] << 8) | (uint16_t)data[0])) * scale;
    result->y=(double)((int16_t)(((uint16_t)data[3] << 8) | (uint16_t)data[2])) * scale;
    result->z=(double)((int16_t)(((uint16_t)data[5] << 8) | (uint16_t)data[4])) * scale;
    result->z*=-1;
	return 1;
}

/*getRawGyroAccelData
    device: a populated i2c device structure
    gyro: a pointer to a coordinate_t structure to hold the gyro data
    accel: a pointer to a coordinate_t structure to hold the acceleration data
    gets raw gyro and acceleration data from the IMU*/
    static int getRawGyroAccelData(pi_i2c_t* device,double* gyro,double* accel){
    int i;
    unsigned char ch;
    unsigned char gyroBytes[6];
    unsigned char accelBytes[6];
    getRegister(device,LSM9DS1_ACCEL_REG_STATUS_REG,&ch);
    if (ch&0x3 == 0 ) return 0; // return false if no data waiting
    for (i=0;i<6;i++){
        getRegister(device,LSM9DS1_ACCEL_REG_OUT_X_L_G+i,&gyroBytes[i]);
        getRegister(device,LSM9DS1_ACCEL_REG_OUT_X_L_XL+i,&accelBytes[i]);
    }
    for (i=0;i<3;i++){
        gyro[i]=(double)((int16_t)(((uint16_t)gyroBytes[(i*2)+1] << 8) | (uint16_t)gyroBytes[(i*2)]));
        accel[i]=(double)((int16_t)(((uint16_t)accelBytes[(i*2)+1] << 8) | (uint16_t)accelBytes[(i*2)]));
    }
    return 1;
}

/*normalizeVector
    vector: a pointer to an array of length 3 to normalize
    normalizes the vector*/
static void normalizeVector(double* vector){
    int i,max=0;
        for (i=0;i<3;i++){
        if (abs(vector[i])>max) 
            max=abs(vector[i]);
    }
    for (i=0;i<3;i++){
        vector[i]=vector[i]/max;
    }
    for (i=0;i<3;i++){
        vector[i]=(float)((int)(vector[i]*10))/10.0;
    }

}

/*getGyroPosition
    device: a populated i2c device structure
    gyro: a pointer to a coordinate_t structure to hold the gyro data
    gets biased gyro data augmented by accelerometer data.  This function returns
    angles in degrees of rotation about the x, y, and z axes respectively.*/
int getGyroPosition(pi_i2c_t* device, coordinate_t* gyro)
{
    int i;
    double pitchAcc=0,rollAcc=0;
    static double bias[3]={0,0,0};
    static double position[3]={0,0,0};
    static int biasCount=0;
    static int cnt=1;
    double gyroData[3];
    double accelData[3];
    int forceMagnitude=0;
    int res=getRawGyroAccelData(device,gyroData,accelData);
    if (biasCount<GYRO_BIAS_COUNT+GYRO_IGNORE_COUNT+1){
        if (biasCount++>GYRO_IGNORE_COUNT-1 &&biasCount<GYRO_BIAS_COUNT+GYRO_IGNORE_COUNT) //ignore first 10 samples
            for (i=0;i<3;i++) bias[i]+=gyroData[i];
        if (biasCount==GYRO_BIAS_COUNT+GYRO_IGNORE_COUNT)
            for (i=0;i<3;i++) bias[i]=bias[i]/GYRO_BIAS_COUNT;       
        gyro->x=gyro->y=gyro->z=0.0; //return 0 while calculating bias
        return res;
    }
    //Estimate the integral and scale to the FSR (sample range)
    for (i=0;i<3;i++)
        position[i]+=(gyroData[i]-bias[i])*GYRO_SCALE;
    //Get the magnitude of the acceleration
    for (i=0;i<3;i++)
        forceMagnitude+=abs(accelData[i]);
    normalizeVector(accelData);
    //does this look like gravity (total between .5g and 1.5g)
    if (forceMagnitude>2048 && forceMagnitude<6144){
        pitchAcc = atan2f(accelData[1], accelData[2]) * 180 / M_PI;
        rollAcc = atan2f(accelData[0], accelData[2]) * 180 / M_PI;
        position[0]=(1-GYRO_ACCEL_WEIGHT)*position[0]+GYRO_ACCEL_WEIGHT*pitchAcc;
        position[1]=(1-GYRO_ACCEL_WEIGHT)*position[1]+GYRO_ACCEL_WEIGHT*rollAcc;
    }
    gyro->x=position[0];
    gyro->y=position[1];
    gyro->z=position[2];
    return res;
}