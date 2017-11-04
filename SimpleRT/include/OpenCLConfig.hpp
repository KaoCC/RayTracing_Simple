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

	// Inherited via OpenCLConfig
	virtual unsigned * getPixels() override;

	virtual ~OpenCLConfig() = default;

protected:


	int forceWorkSize = 0;

	unsigned *pSeeds;

	Vec* color;
	unsigned* pPixels;

	//Camera camera;
	Camera* pCamera;
	Sphere* pSpheres;

	Sphere* spheres_host_ptr;
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

	bool useGPU = false;
};



class OpenCLConfigBuffer : public OpenCLConfig {

public:

	OpenCLConfigBuffer(int width, int height);

	~OpenCLConfigBuffer();

private:


	// Inherited via OpenCLConfig

	virtual void execute() override;
	virtual void setArguments() override;

	virtual void allocateBuffer() override;
	virtual void freeBuffer() override;



	//// Buffers
	cl_mem pixelBuffer;
	cl_mem colorBuffer;
	cl_mem cameraBuffer;
	cl_mem sphereBuffer;
	cl_mem seedBuffer;

	// Inherited via OpenCLConfig
	virtual void sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) override;


};



class OpenCLConfigSVM : public OpenCLConfig {

public:


};



#endif
