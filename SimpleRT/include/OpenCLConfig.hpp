#ifndef _OPENCLCONFIG_HPP_
#define _OPENCLCONFIG_HPP_

#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "Config.hpp"


#include "Camera.hpp"
#include "Sphere.hpp"
#include "SetupGL.hpp"
#include "Scene.hpp"
#include "Utility.hpp"

//void SetUpOpenCL();
//void FreeOpenCLBuffers();
//void DefaultSceneSetup();

// tmp



class OpenCLConfig : public Config {

public:

	OpenCLConfig(int width, int height);

	virtual void updateCamera() override;

	virtual unsigned * getPixels() override;

	virtual ~OpenCLConfig() = default;

protected:


	int forceWorkSize = 0;

	unsigned *pSeeds;
	Vec* pColor;
	unsigned* pPixels;
	Camera* pCamera;
	Sphere* pSpheres;
	unsigned mSphereCount = 0;

	// selection parameters
	const int kPlatformID = 0;


	/* OpenCL Variables */
	cl_context context;
	cl_command_queue commandQueue;
	cl_program program;
	cl_kernel kernel;

	unsigned workGroupSize = 1;
	std::string kernelFileName = "RayTracing_Kernel.cl";


private:

	bool useGPU = true;
};



class OpenCLConfigBuffer : public OpenCLConfig {

public:

	OpenCLConfigBuffer(int width, int height);

	virtual void sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) override;


	~OpenCLConfigBuffer();

private:


	virtual void execute() override;
	virtual void setArguments() override;

	virtual void allocateBuffer() override;
	virtual void freeBuffer() override;

	// Buffers
	cl_mem pixelBuffer;
	cl_mem colorBuffer;
	cl_mem cameraBuffer;
	cl_mem sphereBuffer;
	cl_mem seedBuffer;

};



class OpenCLConfigSVM : public OpenCLConfig {

public:
	OpenCLConfigSVM(int width, int height);

	virtual void sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) override;

	~OpenCLConfigSVM();

private:


	virtual void execute() override;
	virtual void setArguments() override;

	virtual void allocateBuffer() override;
	virtual void freeBuffer() override;

};



#endif
