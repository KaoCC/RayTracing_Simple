#ifndef _OPENCLCONFIG_HPP_
#define _OPENCLCONFIG_HPP_

#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "Config.hpp"


//void SetUpOpenCL();
//void FreeOpenCLBuffers();
void DefaultSceneSetup();

// tmp



class OpenCLConfig : public Config {

public:

	OpenCLConfig();

	// Inherited via Config

	virtual void execute() override;

	virtual ~OpenCLConfig();


private:

	bool useGPU = false;
	bool useSVM = false;


};







#endif
