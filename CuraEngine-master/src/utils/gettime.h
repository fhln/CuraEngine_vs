/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef GETTIME_H
#define GETTIME_H


#include <windows.h>


static inline double getTime()
{

    return double(GetTickCount()) / 1000.0;

}

class TimeKeeper
{
private:
    double startTime;
public:
    TimeKeeper();
    
    double restart();
};

#endif//GETTIME_H
