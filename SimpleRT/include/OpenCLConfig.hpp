#ifndef _OPENCLCONFIG_HPP_
#define _OPENCLCONFIG_HPP_

#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "Config.hpp"


void SetUpOpenCL();
void FreeOpenCLBuffers();
void DefaultSceneSetup();

// tmp



#endif
