/*! \file pilot.h
 *  \author Soo-Hyun Yoo
 *  \brief Header for Pilot class.
 *
 *  Details.
 */

#ifndef PILOT_H
#define PILOT_H

#include "globals.h"
#include "motors.h"
#include "pid.h"
#include "triMath.h"

class Pilot {
    byte serRead;
    uint8_t serInput[PACKETSIZE];
    float throttle;
    float throttleLock;   // Follow throttle within some range +/- THROTTLE_LOCK so if throttle suddenly increases past THROTTLE LOCK, throttle does not change.
    int throttleTrim;
    float mapLower, mapUpper;
//  double dir;   // Direction in radians
    bool okayToFly;
    long numGoodComm;
    long numBadComm;

    struct joyInput {
        float axes[6];
        bool buttons[14];
    } joy;

    void update_joystick_input(void);
    void process_joystick_buttons(void);

public:
    Pilot();
    void listen();
    void fly();
    void die();

    bool hasFood;

};

#endif // PILOT_H
