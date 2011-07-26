#ifndef GYRO_H
#define GYRO_H

#include "i2c.h"
#include "globals.h"

#define READ_SIZE 6   // Number of bytes to read each time
#define REGADDR 0x1D

// Calibration values
#define GXOFFSET 0 // 151 // 312
#define GYOFFSET 0 // -42 // -90
#define GZOFFSET 0 // 85 // 180

// ITG-3200 address defines
#define GYRADDR 0x69 // gyro address, binary = 11101001
#define SMPLRT_DIV 0x15
#define DLPF_FS 0x16
#define INT_CFG 0x17
#define TEMP_OUT 0x1B   // 0x1B is high bit, 0x1C is low bit.
#define PWR_MGM 0x3E


/* WIRING GUIDE
 * 10 kOhm pull-ups on I2C lines.
 * SCL -> SCL
 * SDA -> SDA
 * CLK -> GND
 * INT -- not connected
 * GND -> GND
 * VIO -> 3.3V
 * VDD -> 3.3V
 */


class ITG3200 {
    void Calibrate();

    byte gBuffer[10];   // Buffer for general use. Increase size as needed.
    char gStr[512];

    int calIndex;   // Calibration index

    uint16_t gRaw[3];   // Raw bits received from ITG-3200
    float tempData[3];   // Temporary storage of calibration data
    float gZero[3];   // Zero values
    float gVal[3];   // Bits mapped to [-1,1]
    float temp;


    int rkIndex;
    float rkVal[3][4];   // Four Runge-Kutta integrator values for each of three axes
    float angle[3];   // Calculated angles of rotation

public:
    ITG3200(uint8_t);
    void Poll();   // Get bits from ITG-3200 and update gVal[].
    float* GetRate();
    float GetRate(int);
    float* GetAngle();
    float GetAngle(int);
};

#endif // GYRO_H

