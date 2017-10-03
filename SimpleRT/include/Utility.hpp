
#ifndef _UTILITY_H_
#define _UTILITY_H_

#if defined(__linux__) || defined(__APPLE__)
#include <sys/time.h>
#elif defined (_WIN32)
#include <windows.h>
#else
        Unsupported Platform
#endif

double WallClockTime();


#endif