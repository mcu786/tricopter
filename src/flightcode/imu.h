#ifndef IMU_H
#define IMU_H

#include "itg3200.cpp"
#include "bma180.cpp"
#include "triMath.h"

// Axis numbers
#define AX 0
#define AY 1
#define AZ 2
#define GX 0
#define GY 1
#define GZ 2

#define ACC_WEIGHT 0.01   // Accelerometer data weight relative to gyro's weight of 1
#define ACC_WEIGHT_MAX 0.02   // Maximum accelerometer weight in accelerometer-gyro fusion formula. This value is tuned-up experimentally: if you get too much noise - decrease it. If you get a delayed response of the filtered values - increase it. Starting with a value of  0.01 .. 0.05 will work for most sensors.
#define ACC_ERR_MAX 0.3   // Maximum allowable error (external acceleration) where accWeight becomes 0.
#define MAG_WEIGHT 0.0   // Magnetometer data weight relative to gyro's weight of 1


class IMU {
    BMA180 myAcc;
    ITG3200 myGyr;

    float aVec[3];   // Acceleration vector (uncorrected outputs).
    float gVec[3];   // Gyro vector (uncorrected outputs).
    // float oVec[3];   // Omega vector (calculated rotation rate vector; see p. 13 of DCM draft 2)
    // float oVecP[3];   // Omega vector P
    // float oVecI[3];   // Omega vector I
    // float tmpVec[3];   // Temporary vector for calculations.
    
    // float adcAvg[6];   // ADC outputs of 3-axis accel/gyro. Needed by picquadcontroller imu.h.

    float angle;

    int lastTime;
    float curPos[3]; //   Array of X, Y, and Z coordinates relative to start position
    float curRot[3]; //   Array of X, Y, and Z rotational angles relative to start orientation

    // float DCM[3][3];   // Direction cosine matrix.
    // float dMat[3][3];   // System update matrix.
    // float tmpMat[3][3];   // Temporary matrix.


    float dcmAcc[3][3];                //dcm matrix according to accelerometer
    float dcmGyro[3][3];            //dcm matrix according to gyroscopes
    float dcmEst[3][3];                //estimated dcm matrix by fusion of accelerometer and gyro

    // For accelerometer
    float Kacc[3];
    float wA[3];

    // For magnetometer
    float Imag[3];
    float wM[3];

    // For dcmGyro
    float w[3];   // Gyro rates (angular velocity of a global vector in local coordinates).



public:
    IMU();
    void Init();
    void Update();
    float GetDCM();

    void DCMOrthonormalize(float*);
    void DCMRotate(float*, float*);
    void deadReckoning();
    void reset();
};



#endif

