/*! \file pilot.cpp
 *  \author Soo-Hyun Yoo
 *  \brief Source for Pilot class.
 *
 *  Details.
 */

#include "pilot.h"

Pilot::Pilot() {
    Serial.begin(BAUDRATE);
    hasFood = false;
    okayToFly = false;

    // Assume serial inputs, all axes zeroed.
    serInput[SX] = 125;
    serInput[SY] = 125;
    serInput[ST] = 125;
    /* Keep Z value at some non-zero value (albeit very low so the tricopter 
     * doesn't fly off if something goes awry) so user is forced to adjust 
     * throttle before motors arm. */
    serInput[SZ] = 3;

    // Zero rotation values.
    for (int i=0; i<3; i++) {
        targetRot[i] = 0;
        currentRot[i] = 0;
        pidRot[i] = 0;
    }

    // Initialize trim to 0.
    throttleTrim = 0;

    // Set PID data ID so the PID function can apply appropriate caps, etc.
    PID[PID_ROT_X].id = PID_ROT_X;
    PID[PID_ROT_Y].id = PID_ROT_Y;
    PID[PID_ROT_Z].id = PID_ROT_Z;

    // Set dt.
    float deltaPIDTime = (float) MASTER_DT * CONTROL_LOOP_INTERVAL / 1000000;   // Time difference in seconds.
    PID[PID_ROT_X].deltaPIDTime = deltaPIDTime;
    PID[PID_ROT_Y].deltaPIDTime = deltaPIDTime;
    PID[PID_ROT_Z].deltaPIDTime = deltaPIDTime;

    // Set initial PID gains.
    PID[PID_ROT_X].P = PID[PID_ROT_Y].P = XY_P_GAIN;
    PID[PID_ROT_X].I = PID[PID_ROT_Y].I = XY_I_GAIN;
    PID[PID_ROT_X].D = PID[PID_ROT_Y].D = XY_D_GAIN;

    PID[PID_ROT_Z].P = Z_P_GAIN;
    PID[PID_ROT_Z].I = Z_I_GAIN;
    PID[PID_ROT_Z].D = Z_D_GAIN;

    numGoodComm = 0;   // Number of good communication packets.
    numBadComm = 0;   // Number of bad communication packets.
}

void Pilot::listen() {
    if (Serial.available() > 0) {
        serRead = Serial.read();

        if (serRead == SERHEAD) {   // Receive header.
            hasFood = true;   // Prepare food for watchdog.
            for (int i=0; i<PACKETSIZE; i++) {
                serRead = Serial.read();

                if (serRead >= INPUT_MIN && serRead <= INPUT_MAX) {
                    serInput[i] = serRead;
                    okayToFly = true;
                    numGoodComm++;
                }
                else {
                    i = 10;
                    okayToFly = false;
                    numBadComm++;
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

void Pilot::fly() {
    //sp("(");
    //sp(numGoodComm);
    //sp("/");
    //sp(numBadComm);
    //spln(") ");

    if (okayToFly) {
        update_joystick_input();

        // ====================================================================
        // Calculate target rotation vector based on joystick input scaled to a
        // maximum rotation of PI/6.
        //
        // TODO: The first two are approximations! Need to figure out how to
        // properly use the DCM.
        // ====================================================================
        targetRot[0] = -joy.axes[SY]/125 * PI/10;
        targetRot[1] =  joy.axes[SX]/125 * PI/10;
        targetRot[2] += joy.axes[ST]/125 * Z_ROT_SPEED / (MASTER_DT * CONTROL_LOOP_INTERVAL);

        process_joystick_buttons();

        // Keep targetRot within [-PI, PI].
        for (int i=0; i<3; i++) {
            if (targetRot[i] > PI) {
                targetRot[i] -= 2*PI;
            }
            else if (targetRot[i] < -PI) {
                targetRot[i] += 2*PI;
            }
        }

        // ====================================================================
        // Calculate current rotation vector (Euler angles) from DCM and make
        // appropriate modifications to make PID calculations work later.
        // ====================================================================
        currentRot[0] = bodyDCM[1][2];
        currentRot[1] = -bodyDCM[0][2];
        currentRot[2] = atan2(bodyDCM[0][1], bodyDCM[0][0]);

        // Keep abs(targetRot[i] - currentRot[i]) within [-PI, PI]. This way,
        // nothing bad happens as we rotate to any angle in [-PI, PI].
        for (int i=0; i<3; i++) {
            if (targetRot[i] - currentRot[i] > PI) {
                currentRot[i] += 2*PI;
            }
            else if (targetRot[i] - currentRot[i] < -PI) {
                currentRot[i] -= 2*PI;
            }
        }

        // ====================================================================
        // Calculate the PID outputs that update motor values.
        //
        // TODO: rename pidRot because it is not necessary a rotation value.
        // ====================================================================
        pidRot[0] = updatePID(targetRot[0], currentRot[0], PID[PID_ROT_X]);
        pidRot[1] = updatePID(targetRot[1], currentRot[1], PID[PID_ROT_Y]);
        pidRot[2] = updatePID(targetRot[2], currentRot[2], PID[PID_ROT_Z]);

        throttle = throttleTrim + joy.axes[SZ] * (TMAX-TMIN) / 250;

        // If throttle has increased past THROTTLE_LOCK_DIFF_UP or decreased
        // past THROTTLE_LOCK_DIFF_DOWN in one step, set throttle to
        // throttleLock and leave throttleLock alone. Otherwise, update
        // throttleLock to throttle.
        //if (throttle - throttleLock > THROTTLE_LOCK_DIFF_UP ||
        //    throttleLock - throttle > THROTTLE_LOCK_DIFF_DOWN) {
        //    throttle = throttleLock;
        //}
        //else {
        //    throttleLock = throttle;
        //}

        calculate_pwm_outputs(throttle, pidRot);

        okayToFly = false;
    }
    else {
        // spln("Pilot not okay to fly.");
    }
}

void Pilot::die() {
    // ========================================================================
    // When communication is lost, pilot should set a bunch of stuff to safe
    // values.
    // ========================================================================
    okayToFly = false;
    pwmOut[MOTOR_T] = 0;
    pwmOut[MOTOR_R] = 0;
    pwmOut[MOTOR_L] = 0;
    pwmOut[SERVO_T] = 1900;
}

void Pilot::update_joystick_input(void) {
    // ========================================================================
    // Shift serial input values [0, 250] to correct range for each axis. Z
    // stays positive for ease of calculation.
    // ========================================================================
    joy.axes[SX] = (float) serInput[SX] - 125;   // [-125, 125]
    joy.axes[SY] = (float) serInput[SY] - 125;   // [-125, 125]
    joy.axes[ST] = (float) serInput[ST] - 125;   // [-125, 125]
    joy.axes[SZ] = (float) serInput[SZ];         // [   0, 250]

    // ========================================================================
    // Set button values. We utilize only the lower 7 bits, since doing
    // otherwise would cause overlaps with serial headers.
    // ========================================================================
    for (int i=0; i<7; i++) {
        joy.buttons[i]   = serInput[SB1] & (1<<i);
        joy.buttons[7+i] = serInput[SB2] & (1<<i);
    }
}

void Pilot::process_joystick_buttons(void) {
    // "Reset" targetRot[2] to currentRot[2] if thumb button is pressed.
    if (joy.buttons[BUTTON_RESET_YAW]) {
        targetRot[2] = currentRot[2];
        targetRot[2] += joy.axes[ST]/125 * Z_ROT_SPEED;
    }

    // Zero integral.
    if (joy.buttons[BUTTON_ZERO_INTEGRAL]) {
        PID[PID_ROT_X].integral = 0;
        PID[PID_ROT_Y].integral = 0;
    }

    // Trim throttle value.
    if (joy.buttons[BUTTON_DECREASE_TRIM] && joy.buttons[BUTTON_INCREASE_TRIM]) {
        throttleTrim = 0;
    }
    else if (joy.buttons[BUTTON_DECREASE_TRIM]) {
        throttleTrim--;
    }
    else if (joy.buttons[BUTTON_INCREASE_TRIM]) {
        throttleTrim++;
    }

    // Adjust gains on-the-fly.
    if (joy.buttons[BUTTON_DECREASE_XY_P_GAIN] && joy.buttons[BUTTON_INCREASE_XY_P_GAIN]) {
        PID[PID_ROT_X].P = XY_P_GAIN;
        PID[PID_ROT_Y].P = XY_P_GAIN;
    }
    else if (joy.buttons[BUTTON_DECREASE_XY_P_GAIN] && PID[PID_ROT_X].P > 0) {
        PID[PID_ROT_X].P -= 1.0;
        PID[PID_ROT_Y].P -= 1.0;
    }
    else if (joy.buttons[BUTTON_INCREASE_XY_P_GAIN]) {
        PID[PID_ROT_X].P += 1.0;
        PID[PID_ROT_Y].P += 1.0;
    }

    if (joy.buttons[BUTTON_DECREASE_XY_I_GAIN] && joy.buttons[BUTTON_INCREASE_XY_I_GAIN]) {
        PID[PID_ROT_X].I = XY_I_GAIN;
        PID[PID_ROT_Y].I = XY_I_GAIN;
    }
    else if (joy.buttons[BUTTON_DECREASE_XY_I_GAIN] && PID[PID_ROT_X].I > 0) {
        PID[PID_ROT_X].I -= 1.0;
        PID[PID_ROT_Y].I -= 1.0;
    }
    else if (joy.buttons[BUTTON_INCREASE_XY_I_GAIN]) {
        PID[PID_ROT_X].I += 1.0;
        PID[PID_ROT_Y].I += 1.0;
    }

    if (joy.buttons[BUTTON_DECREASE_XY_D_GAIN] && joy.buttons[BUTTON_INCREASE_XY_D_GAIN]) {
        PID[PID_ROT_X].D = XY_D_GAIN;
        PID[PID_ROT_Y].D = XY_D_GAIN;
    }
    else if (joy.buttons[BUTTON_DECREASE_XY_D_GAIN] && PID[PID_ROT_X].D < 0) {
        PID[PID_ROT_X].D += 1.0;
        PID[PID_ROT_Y].D += 1.0;
    }
    else if (joy.buttons[BUTTON_INCREASE_XY_D_GAIN]) {
        PID[PID_ROT_X].D -= 1.0;
        PID[PID_ROT_Y].D -= 1.0;
    }
}
