#ifndef __PI_GYRO
#define __PI_GYRO
#include "coord.h"

int configureAccelGyro(pi_i2c_t* device);
int getAccelData(pi_i2c_t* device,coordinate_t* result);
int getGyroData(pi_i2c_t* device,coordinate_t* result);
int getGyroPosition(pi_i2c_t* device, coordinate_t* gyro);
#endif
