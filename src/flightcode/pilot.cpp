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

    #ifdef DEBUG
    Serial.println("Pilot here!");
    #endif
}

// int Pilot::Send(char sStr[]) {
//     zeroStr(commStr);
//     for (int i=0; i<128; i++) {
//         commStr[i] = sStr[i];
//     }
//    
//     Serial.println(commStr);
// 
//     return 0;
// }
// 
// char* Pilot::Read(char sStr[]) {
//     zeroStr(commStr);
//     int i = 0;
//     while (Serial.available() > 0) {
//         commStr[i] = sStr[i];
//         i++;
//     }
// 
//     return commStr;
// }

void Pilot::Listen() {
    if (Serial.available()) {
        serRead = Serial.read();

        // Need delay to prevent dropped bits. 500 microseconds is about as low
        // as it will go.
        delayMicroseconds(500);
        // Serial.println(serRead);

        // if (serRead == DOGBONE) {   // Receive dogbone.
        //     hasFood = true;
        // }
        if (serRead == SERHEAD) {   // Receive header.
            hasFood = true;
            for (int i=0; i<PACKETSIZE; i++) {
                serRead = Serial.read();
                // Serial.print(int(serRead));
                // Serial.print("   ");
                delayMicroseconds(500);   // Delay to prevent dropped bits.

                // if (serRead == SERHEAD) {   // Dropped byte?
                //     // i = -1;   // Discard and start over.
                //     // Serial.println("Pilot detected packet drop.");
                //     Serial.flush();
                //     okayToFly = false;
                // }
                // else if (serRead == -1) {   // This happens when serial is empty.
                //     // Serial.println("Pilot detected malformed packet.");
                //     okayToFly = false;
                // }
                if (serRead >= INPUT_MIN && serRead <= INPUT_MAX) {
                    serInput[i] = serRead;
                    // Serial.println("Pilot determined motor value.");
                    okayToFly = true;
                }
                else {
                    i = 10;
                    okayToFly = false;
                    Serial.print("Bad!");
                    // Flush remaining buffer to avoid taking in the wrong values.
                    Serial.flush();
                }
            }
            // Serial.println("");
        }
        else {
            // #ifdef DEBUG
            // Serial.print("Weird header!   ");   // Warn if something weird happens.
            // Serial.print(int(serRead));
            // Serial.println("");
            // #endif
            okayToFly = false;
        }
        // Serial.flush();
    }
}

void Pilot::Fly() {
    if (okayToFly) {   // Update axisVal only if okayToFly is true.
        /* Shift serial input values [1, 251] to correct range for each axis. Z 
         * stays positive for ease of calculation. */
        axisVal[SX] = serInput[SX] - 126;   // [-125, 125]
        axisVal[SY] = serInput[SY] - 126;   // [-125, 125]
        axisVal[ST] = serInput[ST] - 126;   // [-125, 125]
        axisVal[SZ] = serInput[SZ] - 1;     // [0, 250]
    
        // for (int i=0; i<4; i++) {
        //     Serial.print(serInput[i]);
        //     Serial.print("   ");
        // }
        // Serial.println("");
    
        okayToFly = false;

    }
    else {
        // Serial.println("Pilot not okay to fly.");
    }

    // mySystem.UpdateHoverPos(axisVal); TODO: Implement this later.

    // dir = atan2(axisVal[SY], axisVal[SX]);   // May need this eventually for IMU.

    // // Intermediate calculations
    // motorVal[MT] = axisVal[SZ] + 0.6667*axisVal[SY];   // Watch out for floats vs. ints
    // motorVal[MR] = axisVal[SZ] - 0.3333*axisVal[SY] - axisVal[SX]/sqrt(3);
    // motorVal[ML] = axisVal[SZ] - 0.3333*axisVal[SY] + axisVal[SX]/sqrt(3);

    // // Find max/min motor values
    // mapUpper = motorVal[MT] > motorVal[MR] ? motorVal[MT] : motorVal[MR];
    // mapUpper = mapUpper > motorVal[ML] ? mapUpper : motorVal[ML];
    // mapLower = motorVal[MT] < motorVal[MR] ? motorVal[MT] : motorVal[MR];
    // mapLower = mapLower < motorVal[ML] ? mapLower : motorVal[ML];

    // // Find map boundaries (need to limit, but NOT fit, to [0, 250])
    // mapUpper = mapUpper > 250 ? mapUpper : 250;
    // mapLower = mapLower < 0 ? mapLower : 0;

    // // Final calculations
    // for (int i=0; i<3; i++) {
    //     motorVal[i] = map(motorVal[i], mapLower, mapUpper, TMIN, TMAX);
    // }
    // tailServoVal = 70 - 0.6*axisVal[ST];

    // // Serial.print(millis());
    // // Serial.print(": ");

    // // Write to motors
    // for (int i=0; i<3; i++) {
    //     mySystem.SetMotor(i, motorVal[i]);
    //     // Serial.print(motorVal[i]);
    //     // Serial.print("   ");
    // }
    // mySystem.SetServo(tailServoVal);
    // // Serial.print(tailServoVal);
    // // Serial.println("");

    #ifdef DEBUG
    Serial.println("Pilot is flying.");
    #endif
}

void Pilot::Abort() {
    /* When communication is lost, pilot should set all motorVal[] to TMIN and 
     * tell system to set all motors to TMIN. Pilot should also tell system to 
     * set armed status to false so that if communication resumes, throttle 
     * must be reduced to zero before control is restored. */
    // for (int i=0; i<3; i++) {
    //     motorVal[i] = 0;
    //     mySystem.SetMotor(i, 0);
    //     mySystem.armed = false;
    // }
    // mySystem.SetServo(90);
    #ifdef DEBUG
    Serial.println("Pilot ejected!");
    #endif
}

