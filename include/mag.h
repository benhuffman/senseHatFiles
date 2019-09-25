#ifndef __PI_MAG
#define __PI_MAG
#include "coord.h"

int configureMag(pi_i2c_t* device);
void getMagData(pi_i2c_t* device,coordinate_t* result);
#endif
