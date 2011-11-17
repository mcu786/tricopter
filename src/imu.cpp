// ============================================================================
// imu.cpp
// ============================================================================
// I, J, K unit vectors of global coordinate system
//     I - east
//     J - north
//     K - zenith
// i, j, k unit vectors of body coordinate system
//     i - right
//     j - forward
//     k - up
// ============================================================================
// We keep track of the body frame in relation to the global coordinate frame.
// That is, orientation is described in the global coordinate system.
//
//           [i.I i.J i.K]
//     DCM = [j.I j.J j.K]
//           [k.I k.J k.K]
//
// ============================================================================

#include "imu.h"

IMU::IMU() : myAcc(4, 2),   // range, bandwidth: DS p. 27
             myGyr(3)   // 0, 1, 2, 3 are Reserved, Reserved, Reserved, 2000 deg/s
{}

void IMU::Init() {
    IMU::Reset();
    spln("IMU here!");

    // Calibrate sensors. TODO: use accelerometer to find initial tricopter
    // orientation.
    myGyr.Calibrate(500);

    // Set initial DCM as the identity matrix.
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            gyroDCM[i][j] = (i==j) ? 1.0 : 0.0;
}

void IMU::Update() {
    // ========================================================================
    // Acelerometer
    //     Frame of reference: BODY
    //     Units: G (gravitational acceleration)
    //     Purpose: Measure the acceleration vector aVec with components
    //              codirectional with the i, j, and k vectors. Note that the
    //              gravitational vector is the negative of the K vector.
    // ========================================================================
    myAcc.Poll();
    aVec[0] = myAcc.Get(0);
    aVec[1] = myAcc.Get(1);
    aVec[2] = myAcc.Get(2);
    vNorm(aVec);

    // Express K unity vector in BODY frame as KB for use in drift correction
    // (we need K to be described in the BODY frame because gravity is measured
    // by the accelerometer in the BODY frame). Technically we could just
    // create a transpose of gyroDCM, but since we don't (yet) have a
    // magnetometer, we don't need the first two rows of the transpose. This
    // saves a few clock cycles.
    KB[0] = gyroDCM[0][2];
    KB[1] = gyroDCM[1][2];
    KB[2] = gyroDCM[2][2];

    // Calculate gyro drift correction rotation vector wA, which will be used
    // later to bring KB closer to the gravity vector (i.e., the negative of
    // the K vector). Although we do not explicitly negate the gravity vector,
    // the cross product below produces a rotation vector that we can later add
    // to the angular displacement vector to correct for gyro drift in the X
    // and Y axes.
    vCrossP(KB, aVec, wA);

    // Uncomment to debug K and gravity vectors.
    //if (loopCount % TELEMETRY_LOOP_INTERVAL == 0) {
    //    sp("(");
    //    sp(KB[0]); sp(" "); sp(aVec[0]); sp(" | ");
    //    sp(KB[1]); sp(" "); sp(aVec[1]); sp(" | ");
    //    sp(KB[2]); sp(" "); sp(aVec[2]);
    //    sp(") ");
    //}

    // ========================================================================
    // Gyroscope
    //     Frame of reference: BODY
    //     Units: rad/s
    //     Purpose: Measure the rotation rate of the body about the body's i,
    //              j, and k axes.
    // ========================================================================
    myGyr.Poll();
    gVec[0] = myGyr.GetRate(0);
    gVec[1] = myGyr.GetRate(1);
    gVec[2] = myGyr.GetRate(2);

    // Scale gVec by elapsed time (in seconds) to get angle w*dt in radians,
    // then compute weighted average with the accelerometer correction vector.
    for (int i=0; i<3; i++) {
        wdt[i] = gVec[i] * MASTER_DT/1000;
        wdt[i] = (wdt[i] + ACC_WEIGHT*wA[i]) / (1.0 + ACC_WEIGHT);
    }

    // ========================================================================
    // Direction Cosine Matrix
    //     Frame of reference: GLOBAL
    //     Units: None (unit vectors)
    //     Purpose: Calculate the components of the body's i, j, and k unit
    //              vectors in the global frame of reference.
    // ========================================================================
    // Skew the rotation vector and sum appropriate components by combining the
    // skew symmetric matrix with the identity matrix. The math can be
    // summarized as follows:
    //
    // All of this is calculated in the BODY frame. If w is the angular
    // velocity vector, let wdt (w*dt) be the angular displacement vector of
    // the DCM over a time interval dt. Let wdt_i, wdt_j, and wdt_k be the
    // components of wdt codirectional with the i, j, and k unit vectors,
    // respectively. Also, let dr be the linear displacement vector of the DCM
    // and dr_i, dr_j, and dr_k once again be the i, j, and k components,
    // respectively.
    //
    // In very small dt, certain vectors approach orthogonality, so we can
    // assume that (draw this out for yourself!):
    //
    //     dr_x = <    0,  dw_k, -dw_j>,
    //     dr_y = <-dw_k,     0,  dw_i>, and
    //     dr_z = < dw_j, -dw_i,     0>,
    //
    // which can be expressed as the rotation matrix:
    //
    //          [     0  dw_k -dw_j ]
    //     dr = [ -dw_k     0  dw_i ]
    //          [  dw_j -dw_i     0 ].
    //
    // This can then be multiplied by the current DCM and added to the current
    // DCM to update the DCM. To minimize the number of calculations performed
    // by the processor, however, we can combine the last two steps by
    // combining dr with the identity matrix to produce:
    //
    //              [     1  dw_k -dw_j ]
    //     dr + I = [ -dw_k     1  dw_i ]
    //              [  dw_j -dw_i     1 ],
    //
    // which we multiply with the current DCM to produce the updated DCM
    // directly.
    //
    // It may be helpful to read the Wikipedia pages on cross products and
    // rotation representation.
    // ========================================================================
    dDCM[0][0] =       1;
    dDCM[0][1] =  wdt[2];
    dDCM[0][2] = -wdt[1];
    dDCM[1][0] = -wdt[2];
    dDCM[1][1] =       1;
    dDCM[1][2] =  wdt[0];
    dDCM[2][0] =  wdt[1];
    dDCM[2][1] = -wdt[0];
    dDCM[2][2] =       1;

    // Multiply the current DCM with the change in DCM and update.
    mProduct(dDCM, gyroDCM, gyroDCM);

    // Orthogonalize the i and j unit vectors (DCMDraft2 Eqn. 19).
    errDCM = vDotP(gyroDCM[0], gyroDCM[1]);
    vScale(gyroDCM[1], -errDCM/2, dDCM[0]);   // i vector correction
    vScale(gyroDCM[0], -errDCM/2, dDCM[1]);   // j vector correction
    vAdd(gyroDCM[0], dDCM[0], gyroDCM[0]);
    vAdd(gyroDCM[1], dDCM[1], gyroDCM[1]);

    // k = i x j
    vCrossP(gyroDCM[0], gyroDCM[1], gyroDCM[2]);

    // Normalize all three vectors.
    vNorm(gyroDCM[0]);
    vNorm(gyroDCM[1]);
    vNorm(gyroDCM[2]);
}

void IMU::Reset() {
}
