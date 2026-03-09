#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

void mpu6050_reset(void);
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp);

#endif