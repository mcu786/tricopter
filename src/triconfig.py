#!/usr/bin/env python
# -*- coding: utf-8 -*-

# =============================================================================
# Communication
# =============================================================================

# General
#serialPort = "/dev/ttyUSB0"   # Uncomment to specify serial port. Otherwise, will connect to first available port.
baudRate = 38400
debug = False
loopPeriod = 0.005   # 5 ms interval = 200 Hz.

# TX
txInterval = 6   # 200 Hz / 6 = 33 Hz.
serHeader = '\xff'
#dogBone = '\xfe'

# RX
rxInterval = 1   # 200 Hz / 1 = 200 Hz.
newlineSerTag  = '\xde\xad\xbe\xef'
fieldSerTag    = '\xff\xff'
dcmSerTag      = '\xfb'
rotationSerTag = '\xfc'
motorSerTag    = '\xfd'
pidSerTag      = '\xfe'


# Joystick axis sign flips.
axisSigns = [-1, 1, 1, 1, 1, 1, -1]

# Joystick axis indices.
axisX  = 0
axisY  = 1
axisZ  = 2
axisT0 = 3
axisT1 = 4
axisH0 = 5
axisH1 = 6

