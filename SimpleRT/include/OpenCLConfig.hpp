#ifndef _OPENCLCONFIG_HPP_
#define _OPENCLCONFIG_HPP_

#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif





void SetUpOpenCL();
void FreeOpenCLBuffers();
void DefaultSceneSetup();

// tmp
extern int forceWorkSize;
extern int useGPU;
extern std::string kernelFileName;


#endif
