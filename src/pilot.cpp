// ============================================================================
// pilot.cpp
// ============================================================================

#include "pilot.h"

Pilot::Pilot() {
    Serial.begin(BAUDRATE);
    hasFood = false;
    okayToFly = false;
    
    // Assume serial inputs, all axes zeroed.
    serInput[SX] = 126;
    serInput[SY] = 126;
    serInput[ST] = 126;
    /* Keep Z value at some non-zero value (albeit very low so the tricopter 
     * doesn't fly off if something goes awry) so user is forced to adjust 
     * throttle before motors arm. */
    serInput[SZ] = 3;   

    // Zero all PWM update values.
    for (int i=0; i<4; i++) {
        pwmOutUpdate[i] = 0;
    }

    // Zero rotation values.
    for (int i=0; i<3; i++) {
        targetRot[i] = 0;
        pidRot[i] = 0;
    }

    PID[PID_ROT_X].P = 3.0;
    PID[PID_ROT_X].I = 2.0;
    PID[PID_ROT_X].D = -0.43;

    PID[PID_ROT_Y].P = 4.0;
    PID[PID_ROT_Y].I = 0;//2.5;
    PID[PID_ROT_Y].D = -0.76;

    PID[PID_ROT_Z].P = 7.0;
    PID[PID_ROT_Z].I = 0.0;
    PID[PID_ROT_Z].D = 0.0;

    #ifdef DEBUG
    spln("Pilot here!");
    #endif

    numGoodComm = 0;   // Number of good communication packets.
    numBadComm = 0;   // Number of bad communication packets.
}

void Pilot::Listen() {
    if (Serial.available()) {
        serRead = Serial.read();

        // Need delay to prevent dropped bits. 500 microseconds is about as low
        // as it will go.
        delayMicroseconds(500);

        if (serRead == SERHEAD) {   // Receive header.
            hasFood = true;   // Prepare food for watchdog.
            for (int i=0; i<PACKETSIZE; i++) {
                serRead = Serial.read();
                delayMicroseconds(500);   // Delay to prevent dropped bits.

                if (serRead >= INPUT_MIN && serRead <= INPUT_MAX) {
                    serInput[i] = serRead;
                    okayToFly = true;
                    numGoodComm++;
                }
                else {
                    i = 10;
                    okayToFly = false;
                    numBadComm++;
                    sp("Bad!");
                    // Flush remaining buffer to avoid taking in the wrong values.
                    Serial.flush();
                }
            }
        }
        else {
            okayToFly = false;
        }
    }
}

void Pilot::Talk() {
}

void Pilot::Fly() {
    //sp("(");
    //sp(numGoodComm);
    //sp("/");
    //sp(numBadComm);
    //sp(") ");

    // Update axisVal only if okayToFly is true.
    if (okayToFly) {
        // ====================================================================
        // Shift serial input values [1, 251] to correct range for each axis. Z
        // stays positive for ease of calculation.
        // ====================================================================
        axisVal[SX] = (float) serInput[SX] - 126;   // [-125, 125]
        axisVal[SY] = (float) serInput[SY] - 126;   // [-125, 125]
        axisVal[ST] = (float) serInput[ST] - 126;   // [-125, 125]
        axisVal[SZ] = (float) serInput[SZ] - 1;     // [0, 250]

        // ====================================================================
        // Calculate target rotation vector based on joystick input scaled to a
        // maximum rotation of PI/6.
        //
        // TODO: The first two are approximations! Need to figure out how to
        // properly use the DCM.
        // ====================================================================
        targetRot[0] = -axisVal[SY]/125 * PI/10;
        targetRot[1] =  axisVal[SX]/125 * PI/10;
        targetRot[2] += axisVal[ST]/125 / (MASTER_DT * CONTROL_LOOP_INTERVAL);

        // Keep targetRot within [-PI, PI].
        for (int i=0; i<3; i++) {
            if (targetRot[i] > PI) {
                targetRot[i] -= 2*PI;
            }
            else if (targetRot[i] < -PI) {
                targetRot[i] += 2*PI;
            }
        }

        // Calculate Euler angles.
        currentRot[0] = gyroDCM[1][2];
        currentRot[1] = -gyroDCM[0][2];
        currentRot[2] = atan2(gyroDCM[0][1], gyroDCM[0][0]);

        // Calculate the PID outputs that update motor values.
        pidRot[0] = updatePID(targetRot[0], currentRot[0], PID[PID_ROT_X]);
        pidRot[1] = updatePID(targetRot[1], currentRot[1], PID[PID_ROT_Y]);
        pidRot[2] = updatePID(targetRot[2], currentRot[2], PID[PID_ROT_Z]);

        // Keep pidRot[2] within [-PI, PI]. TODO: Rename pidRot, because it is
        // not necessarily an actual rotation angle.
        if (pidRot[2] > PI) {
            pidRot[2] -= PI;
        }
        else if (pidRot[2] < -PI) {
            pidRot[2] += PI;
        }

        //pidRot[2] = updatePID(axisVal[ST]/125 * PI/6,
        //                      0,//atan2(gyroDCM[0][1], gyroDCM[0][0]),
        //                      PID[PID_ROT_Z]);

        //targetRot[0] = -(axisVal[SY]/125 * PI/6 + gyroDCM[1][2]);
        //targetRot[1] =  (axisVal[SX]/125 * PI/6 + gyroDCM[0][2]);
        //targetRot[2] = -(axisVal[ST]/125 * PI/6);

        // ====================================================================
        // Calculate motor/servo values.
        //     MOTOR_X_OFFSET: Offset starting motor values to account for
        //                     chassis imbalance.
        //     MOTOR_X_SCALE: Scale targetRot.
        //     axisVal[SZ]: Throttle.
        //
        // TODO: MOTOR_X_SCALE should be replaced by actual PID gains. Besides,
        // it's incorrect to scale in the negative direction if the
        // corresponding arm is heavier.
        // TODO: The last term for each pwmOut is INACCURATE. Fix this.
        // ====================================================================

        // MOTORVAL SCHEME 5 (pidRot to motorVal conversion)
        //pwmOut[MOTOR_T] = MOTOR_T_OFFSET + (TMIN + axisVal[SZ]*(TMAX-TMIN)/250) + -pidRot[0]*2;
        //pwmOut[MOTOR_R] = MOTOR_R_OFFSET + (TMIN + axisVal[SZ]*(TMAX-TMIN)/250) +  pidRot[0] - pidRot[1]*sqrt(3);
        //pwmOut[MOTOR_L] = MOTOR_L_OFFSET + (TMIN + axisVal[SZ]*(TMAX-TMIN)/250) +  pidRot[0] + pidRot[1]*sqrt(3);
        //pwmOut[SERVO_T] = TAIL_SERVO_DEFAULT_POSITION + pidRot[2];

        // MOTORVAL SCHEME 6
        pwmOut[MOTOR_T] = TMIN + MOTOR_T_OFFSET + axisVal[SZ]*(TMAX-TMIN)/250 + -pidRot[PID_ROT_X];
        pwmOut[MOTOR_R] = TMIN + MOTOR_R_OFFSET + axisVal[SZ]*(TMAX-TMIN)/250 +  pidRot[PID_ROT_X] - pidRot[PID_ROT_Y]*sqrt(3);
        pwmOut[MOTOR_L] = TMIN + MOTOR_L_OFFSET + axisVal[SZ]*(TMAX-TMIN)/250 +  pidRot[PID_ROT_X] + pidRot[PID_ROT_Y]*sqrt(3);
        pwmOut[SERVO_T] = TAIL_SERVO_DEFAULT_POSITION + pidRot[PID_ROT_Z];

        pwmOut[MOTOR_T] = MOTOR_T_SCALE * pwmOut[MOTOR_T];
        pwmOut[MOTOR_R] = MOTOR_R_SCALE * pwmOut[MOTOR_R];
        pwmOut[MOTOR_L] = MOTOR_L_SCALE * pwmOut[MOTOR_L];


        // ====================================================================
        // After finding the maximum and minimum motor values, limit, but NOT
        // fit, motor values to minimum and maximum throttle [TMIN, TMAX]).
        // Doing this incorrectly will result in motor values seemingly stuck
        // mostly at either extremes.
        // ====================================================================
        mapUpper = pwmOut[MOTOR_T] > pwmOut[MOTOR_R] ? pwmOut[MOTOR_T] : pwmOut[MOTOR_R];
        mapUpper = mapUpper > pwmOut[MOTOR_L] ? mapUpper : pwmOut[MOTOR_L];
        mapUpper = mapUpper > TMAX ? mapUpper : TMAX;

        mapLower = pwmOut[MOTOR_T] < pwmOut[MOTOR_R] ? pwmOut[MOTOR_T] : pwmOut[MOTOR_R];
        mapLower = mapLower < pwmOut[MOTOR_L] ? mapLower : pwmOut[MOTOR_L];
        mapLower = mapLower < TMIN ? mapLower : TMIN;

        // We shouldn't have to use these, but uncomment the following two
        // lines if pwmOut goes crazy and makes mapUpper lower than mapLower:
        //mapUpper = mapUpper > TMIN ? mapUpper : TMIN+1;
        //mapLower = mapLower < TMAX ? mapLower : TMAX-1;

        // ====================================================================
        // If map bounds are reasonable, remap range to [mapLower, mapUpper].
        // Otherwise, kill motors. Note that map(), an Arduino function, does
        // integer math and truncates fractions.
        //
        // TODO: pwmOut (and other quantities the Pilot calculates) should be
        // an integer representing the number of milliseconds of PWM duty
        // cycle.
        // ====================================================================
        for (int i=0; i<3; i++) {
            if (mapUpper > mapLower) {
                pwmOut[i] = map(pwmOut[i], mapLower, mapUpper, TMIN, TMAX);
            }
            else {
                pwmOut[i] = TMIN;
            }
        }

        okayToFly = false;
    }
    else {
        // spln("Pilot not okay to fly.");
    }

    #ifdef DEBUG
    spln("Pilot is flying.");
    #endif
}

void Pilot::Abort() {
    // ========================================================================
    // When communication is lost, pilot should set a bunch of stuff to safe
    // values.
    // ========================================================================
    serInput[SX] = 126;
    serInput[SY] = 126;
    serInput[ST] = 126;
    serInput[SZ] = 3;
    okayToFly = false;
    pwmOut[MOTOR_T] = TMIN;
    pwmOut[MOTOR_R] = TMIN;
    pwmOut[MOTOR_L] = TMIN;
    pwmOut[SERVO_T] = 90;
    
    #ifdef DEBUG
    spln("Pilot ejected!");
    #endif
}

