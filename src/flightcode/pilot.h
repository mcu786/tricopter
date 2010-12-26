#ifndef PILOT_H
#define PILOT_H

#include "globals.h"
#include "system.h"

class Pilot {
    char commStr[];
    int serRead;

public:
    Pilot();
    int Send(char[]);
    char* Read(char[]);
    void Listen();
    void Fly(System&);
    
    bool hasFood;
    
    int input[PACKETSIZE];
};

#endif // PILOT_H

