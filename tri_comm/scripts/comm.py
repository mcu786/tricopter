#!/usr/bin/env python

import serial
import string
from time import sleep
import threading
from threading import Timer, Thread
from signal import signal, SIGINT
from math import pi

# ROS stuff
import roslib; roslib.load_manifest("tricopter")
import rospy
from tricopter.msg import Telemetry, Inputs

import triconfig as cfg   # Import config.

# =============================================================================
# TX data
# =============================================================================

armed = False   # System arm status. Set to True once throttle is set to zero. Communication will not start until this is True.
axisValues = [0, 0, 0, 0]
buttonValues = 0

# =============================================================================
# RX data
# =============================================================================

# Initial DCM values.
dcm = [[1.0, 0.0, 0.0],
       [0.0, 1.0, 0.0],
       [0.0, 0.0, 1.0]]

# Target rotation values
targetRot = [0.0, 0.0, 0.0]

# Motor/servo values (MT, MR, ML, ST)
motorVal = [0.0, 0.0, 0.0, 0.0]

# PID data
pidData = [0.0, 0.0, 0.0]

# Loop time
loopTime = 0

# Telemetry
serBuffer = ''
serLines = ['']


# =============================================================================
# TX
# =============================================================================
def inputsCallback (inputs):
    axisValues = inputs.axisValues
    buttonValues = inputs.buttonValues

# Serial write.
def serWrite(myStr):
    try:
        for i in range(len(myStr)):
            ser.write(myStr[i])
            #rospy.sleep(0.00005)   # Sometimes, a delay seems to help. Maybe?
    except:
        rospy.logerr("[Comm] Unable to send data. Check connection.")
        # TODO: Comm should do something to ensure safety when it loses connection.

def transmit():
    global armed
    if armed:
        serWrite(cfg.serHeader +
                 chr(axisValues[cfg.axisX]) +
                 chr(axisValues[cfg.axisY]) +
                 chr(axisValues[cfg.axisT]) +
                 chr(axisValues[cfg.axisZ]) +
                 chr(buttonValues & 0b01111111) +
                 chr(buttonValues >> 7))
        #rospy.loginfo(str(axisValues[cfg.axisX]) + " " +
        #              str(axisValues[cfg.axisY]) + " " +
        #              str(axisValues[cfg.axisT]) + " " +
        #              str(axisValues[cfg.axisZ]) + " " +
        #              str(buttonValues))
    elif axisValues[cfg.axisZ] == 0:
        armed = True
        rospy.loginfo("[Comm] Joystick throttle at minimum. Initiating communication!")
    else:
        rospy.loginfo("[Comm] Joystick throttle not at minimum! Current value: " + str(axisValues[cfg.axisZ]))


# =============================================================================
# RX
# =============================================================================
def telemetry():
    global dcm, targetRot, motorVal, pidData, loopTime, serBuffer, serLines
    dcmDataIndex = 0       # Do I see IMU data?
    rotationDataIndex = 0       # Do I see rotation data?
    motorDataIndex = 0     # Do I see motor data?
    pidDataIndex = 0     # Do I see PID data?

    try:
        dataIsGood = True
        if ser.inWaiting() > 0:
            # =========================================================
            # Update buffer, adding onto incomplete line if necessary.
            # =========================================================
            serBuffer = ser.read(ser.inWaiting())
            serLines = serLines[:-1] + (serLines[-1] + serBuffer).split(cfg.newlineSerTag)
            # =========================================================
            # If there are two or more entries in serLines (i.e., at
            # least one entry is complete), pop off the earliest entry.
            # =========================================================
            if len(serLines) > 1:
                # Parse fields separated by 0xf0f0.
                fields = serLines[0].split(cfg.fieldSerTag)

                # Discard one entry.
                serLines = serLines[1:]

            # =========================================================
            # Scan for data field headers.
            # TODO: Move this someplace else so it's run only once.
            # =========================================================
            for i in range(1, len(fields)):
                if not dcmDataIndex and fields[i][0] == cfg.dcmSerTag:
                    dcmDataIndex = i
                elif not rotationDataIndex and fields[i][0] == cfg.rotationSerTag:
                    rotationDataIndex = i
                elif not motorDataIndex and fields[i][0] == cfg.motorSerTag:
                    motorDataIndex = i
                elif not pidDataIndex and fields[i][0] == cfg.pidSerTag:
                    pidDataIndex = i

            # =========================================================
            # Check if we're receiving DCM data.
            # =========================================================
            if dcmDataIndex:
                # Structure of DCM block:
                #     'DCMxxxxxxxxx', where x represents a single byte.
                #     The 9 floats of the DCM are mapped to one-byte
                #     integers before being pushed to serial in the
                #     same scheme in which we encode joystick values in
                #     comm.py.
                # TODO: Make a function to make reading from serial
                # easier.
                try:
                    for i in range(3):
                        for j in range(3):
                            dcm[i][j] = float(int(fields[dcmDataIndex][i*3+j+1:i*3+j+2].encode('hex'), 16)-1)/250*2-1
                            #dcm[i][j] = struct.unpack('f', fields[dcmDataIndex][3+(i*3+j)*4:3+(i*3+j)*4+4])[0]
                            #dcmT[j][i] = dcm[i][j]
                except Exception, e:
                    dataIsGood = False
                    if cfg.debug:
                        print "DCM:", str(e)

            # =========================================================
            # Check if we're receiving target rotation data.
            # =========================================================
            if rotationDataIndex:
                try:
                    for i in range(3):
                        targetRot[i] = float(int(fields[rotationDataIndex][i+1:i+2].encode('hex'), 16))/250*2*pi-pi
                        #targetRot[i] = struct.unpack('f', fields[rotationDataIndex][3+i*4:3+i*4+4])[0]
                except Exception, e:
                    dataIsGood = False
                    if cfg.debug:
                        print "ROT:", str(e)

            # =========================================================
            # Check if we're receiving motor/servo output data.
            # =========================================================
            if motorDataIndex:
                try:
                    for i in range(4):
                        motorVal[i] = int(fields[motorDataIndex][i+1:i+2].encode('hex'), 16) * 376 / 250
                        #motorVal[i] = struct.unpack('f', fields[motorDataIndex][3+i*4:3+(i+1)*4])[0]
                except Exception, e:
                    dataIsGood = False
                    if cfg.debug:
                        print "MTR:", str(e)

            # =========================================================
            # Check if we're receiving PID gains and values.
            # =========================================================
            if pidDataIndex:
                try:
                    for i in range(3):
                        pidData[i] = int(fields[pidDataIndex][i+1:i+2].encode('hex'), 16)
                except Exception, e:
                    dataIsGood = False
                    if cfg.debug:
                        print "PID:", str(e)

            # Record loop time.
            loopTime = int(fields[-1])

            # =========================================================
            # Printout
            # =========================================================
            if dataIsGood:
                print "Arm:", int(fields[0].encode('hex'), 16)
                print "Rot:", targetRot
                print "Mot:", motorVal
                print "PID:", pidData
                print "Loop:", loopTime

                pub.publish(Telemetry(dcm[0][0], dcm[0][1], dcm[0][2],
                                      dcm[1][0], dcm[1][1], dcm[1][2],
                                      dcm[2][0], dcm[2][1], dcm[2][2],
                                      targetRot[0], targetRot[1], targetRot[2],
                                      motorVal[0], motorVal[1], motorVal[2], motorVal[3],
                                      pidData[0], pidData[1], pidData[2],
                                      loopTime))
            else:
                if cfg.debug:
                    print "[Comm] Bad data!"

            print "\n--\n"

    except:
        pass


###############################################################################

if __name__ == "__main__":
    # Initialize ROS node.
    rospy.init_node("tricopter_comm", anonymous=False)
    pub = rospy.Publisher("tricopter_telemetry", Telemetry)
    sub = rospy.Subscriber("tricopter_inputs", Inputs, inputsCallback, queue_size=100)

    # =========================================================================
    # Try to initialize a serial connection. If serialPort is defined, try
    # opening that. If it is not defined, loop through a range of integers
    # starting from 0 and try to connect to /dev/ttyUSBX where X is the
    # integer. In either case, process dies if serial port cannot be opened.
    #
    # TODO: This needs to be made more concise.
    # =========================================================================
    try:
        ser = serial.Serial(cfg.serialPort, cfg.baudRate, timeout=0)
    except serial.SerialException:
        rospy.logerr("[Comm] Unable to open specified serial port! Exiting...")
        exit(1)
    except AttributeError:
        for i in range(4):
            try:
                ser = serial.Serial("/dev/ttyUSB"+str(i), cfg.baudRate, timeout=0)
                rospy.loginfo("[Comm] Opened serial port at /dev/ttyUSB%d.", i)
                break
            except serial.SerialException:
                rospy.logerr("[Comm] No serial at /dev/ttyUSB%d.", i)
                if i == 3:
                    rospy.logerr("[Comm] No serial found. Giving up!")
                    exit(1)

    loopCount = 0
    while not rospy.is_shutdown():
        if loopCount % cfg.rxInterval == 0:
            telemetry()
        if loopCount % cfg.txInterval == 0:
            transmit()

        loopCount = (loopCount+1) % 1000

        rospy.sleep(cfg.loopPeriod)
