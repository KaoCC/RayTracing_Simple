
#include "utility.h"

#define NULL 0

double WallClockTime() 
{
#if defined(__linux__) || defined(__APPLE__)

	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (_WIN32)
	return GetTickCount() / 1000.0;
#else
	Unsupported Platform
#endif
}

