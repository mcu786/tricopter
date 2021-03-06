EESchema Schematic File Version 2  date Sat 18 Jun 2011 09:49:13 PM PDT
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
EELAYER 43  0
EELAYER END
$Descr A4 11700 8267
Sheet 1 1
Title ""
Date "19 jun 2011"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Wire Line
	2850 1150 2850 2300
Connection ~ 1450 1850
Connection ~ 2050 1850
Connection ~ 1050 1350
Wire Wire Line
	850  1350 2250 1350
Connection ~ 1450 1350
Connection ~ 1650 1350
Wire Wire Line
	1550 1350 1550 1200
Connection ~ 2050 1350
Connection ~ 1850 1350
Connection ~ 1550 1350
Connection ~ 1250 1350
Connection ~ 1850 1850
Connection ~ 1650 1850
Connection ~ 1250 1850
Wire Wire Line
	850  1850 2250 1850
Connection ~ 1050 1850
Wire Wire Line
	1550 1850 1550 2300
Connection ~ 1550 1850
$Comp
L CONN_1 P2
U 1 1 4DFD7C7E
P 2850 2450
F 0 "P2" H 2930 2450 40  0000 L CNN
F 1 "CONN_1" H 2850 2505 30  0001 C CNN
	1    2850 2450
	0    1    1    0   
$EndComp
$Comp
L CONN_1 P1
U 1 1 4DFD7C73
P 1550 2450
F 0 "P1" H 1630 2450 40  0000 L CNN
F 1 "CONN_1" H 1550 2505 30  0001 C CNN
	1    1550 2450
	0    1    1    0   
$EndComp
$Comp
L R R8
U 1 1 4DFD7BF0
P 2250 1600
F 0 "R8" V 2330 1600 50  0000 C CNN
F 1 "R" V 2250 1600 50  0000 C CNN
	1    2250 1600
	1    0    0    -1  
$EndComp
$Comp
L R R7
U 1 1 4DFD7BED
P 2050 1600
F 0 "R7" V 2130 1600 50  0000 C CNN
F 1 "R" V 2050 1600 50  0000 C CNN
	1    2050 1600
	1    0    0    -1  
$EndComp
$Comp
L R R6
U 1 1 4DFD7BEA
P 1850 1600
F 0 "R6" V 1930 1600 50  0000 C CNN
F 1 "R" V 1850 1600 50  0000 C CNN
	1    1850 1600
	1    0    0    -1  
$EndComp
$Comp
L R R5
U 1 1 4DFD7BE7
P 1650 1600
F 0 "R5" V 1730 1600 50  0000 C CNN
F 1 "R" V 1650 1600 50  0000 C CNN
	1    1650 1600
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 4DFD7BE5
P 1450 1600
F 0 "R4" V 1530 1600 50  0000 C CNN
F 1 "R" V 1450 1600 50  0000 C CNN
	1    1450 1600
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 4DFD7BE2
P 1250 1600
F 0 "R3" V 1330 1600 50  0000 C CNN
F 1 "R" V 1250 1600 50  0000 C CNN
	1    1250 1600
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 4DFD7BDD
P 1050 1600
F 0 "R2" V 1130 1600 50  0000 C CNN
F 1 "R" V 1050 1600 50  0000 C CNN
	1    1050 1600
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 4DFD6D4B
P 850 1600
F 0 "R1" V 930 1600 50  0000 C CNN
F 1 "R" V 850 1600 50  0000 C CNN
	1    850  1600
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR01
U 1 1 4DFD6CBB
P 2850 1100
F 0 "#PWR01" H 2850 1100 30  0001 C CNN
F 1 "GND" H 2850 1030 30  0001 C CNN
	1    2850 1100
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR02
U 1 1 4DFD6C63
P 1550 1200
F 0 "#PWR02" H 1550 1290 20  0001 C CNN
F 1 "+5V" H 1550 1290 30  0000 C CNN
	1    1550 1200
	1    0    0    -1  
$EndComp
$EndSCHEMATC
