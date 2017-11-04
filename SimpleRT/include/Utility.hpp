
#ifndef _UTILITY_H_
#define _UTILITY_H_

#if defined(__linux__) || defined(__APPLE__)
#include <sys/time.h>
#elif defined (_WIN32)
#include <windows.h>
#else
        Unsupported Platform
#endif

#include <vector>
#include <string>

#include "Camera.hpp"

double WallClockTime();

std::vector<char> ReadKernelSourcesFile(const std::string& fileName);

void computeCameraVariables(Camera* cameraPtr, int width, int height);

#endif

