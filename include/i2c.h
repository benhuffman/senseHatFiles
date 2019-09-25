#ifndef __PI_SENSE_I2C
#define __PI_SENSE_I2C

typedef struct {
    char name[64];
    int fd;
} pi_i2c_t;

pi_i2c_t* geti2cDevice();
void freei2cDevice(pi_i2c_t* device);
int usei2cConnectedDevice(pi_i2c_t* device,unsigned char sensorID);
int getRegister(pi_i2c_t* device, unsigned char reg,unsigned char* value);
int setRegister(pi_i2c_t* device, unsigned char reg,unsigned char value);

#endif
