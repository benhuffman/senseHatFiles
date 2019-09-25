#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <linux/i2c-dev.h>
#include "../../include/i2c.h"
#define REG_WHO_AM_I 0xF


/*isi2cDevice
  dev: a directory entry for the device file to be checked
  returns: true if device is a i2c device
*/
static int isi2cDevice(const struct dirent * dev){
    return strncmp("i2c",dev->d_name,2)==0;
}

/*geti2cDevice
	returns a device structure for the I2C device interface
*/
pi_i2c_t* geti2cDevice(){
    struct dirent **list;
    pi_i2c_t* result=0;
    int i,ndev,res;
    ndev=scandir("/dev",&list,isi2cDevice,versionsort);
    if(ndev<=0) return 0;
    for (i=0;i<ndev;i++){
        char fname[64];
        snprintf(fname,sizeof(fname),"/dev/%s",list[i]->d_name);
        int fd=open(fname,O_RDWR);
        if (fd>=0){
            result=malloc(sizeof(pi_i2c_t));
            strcpy(result->name,fname);
            result->fd=fd;
            break;
        }
    }
    for (i=0;i<ndev;i++)
        free(list[i]);
    return result;
}

/*freei2cDevice
  device: A device structure to free
  deallocates the device
*/
void freei2cDevice(pi_i2c_t* device){
    close(device->fd);
    free(device);
}

/*getI2cDeviceAddress
  device: A device structure
  id: the expected WHO_AM_I value for the requested device
  returns the address offset of that device
*/
static unsigned char getI2cDeviceAddress(pi_i2c_t* device,unsigned char id){
    int res,i;
    unsigned char data;
    for (i=0;i<0x100;i++){
        res=ioctl(device->fd,I2C_SLAVE,i);
        res=getRegister(device,REG_WHO_AM_I,&data);
        if (res==1){
            if (data==id) return (unsigned char)i;
        }
    }
}

/*usei2cConnectedDevice
  device: the device structure
  sensorID: the expected WHO_AM_I value for the requested device
*/
int usei2cConnectedDevice(pi_i2c_t* device,unsigned char sensorID){
    int res;
    unsigned char sensor=getI2cDeviceAddress(device,sensorID);
    //slave to the sensor
    res=ioctl(device->fd,I2C_SLAVE,sensor);
    return 1;
}

/* setRegister
    device: the device structure
    reg: the register id
    value: the value to be written*/
int setRegister(pi_i2c_t* device, unsigned char reg,unsigned char value){
    int i;
    unsigned char data[2];
    data[0]=reg;
    data[1]=value;
    return write(device->fd,data,2);
}

/*getRegister
    device: the device structure
    reg: the register id
    value: a pointer to a character to hold the value*/
int getRegister(pi_i2c_t* device, unsigned char reg,unsigned char* value){
    int i;
    write(device->fd,&reg,1);
    return read(device->fd,value,1);
}
