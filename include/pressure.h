#ifndef __PI_SENSE_PRESSURE
#define __PI_SENSE_PRESSURE
int configurePressureSensor(pi_i2c_t* device);
float getPressure(pi_i2c_t* device);
#endif
