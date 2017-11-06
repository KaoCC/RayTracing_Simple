#ifndef _CMCONFIG_HPP_
#define _CMCONFIG_HPP_


#include "Config.hpp"

#include "cm_rt.h"


#include "Camera.hpp"
#include "Scene.hpp"
#include "SetupGL.hpp"
#include "Utility.hpp"
#include "Sphere.hpp"

#include "CmSVMAllocator.hpp"

// tmp
//extern unsigned int* hostPixels;






//---------------


class CmConfig : public Config {

public:

	CmConfig(int width, int height);

	virtual void updateCamera() override;
	virtual unsigned * getPixels() override;

	virtual ~CmConfig() = default;

protected:

	// Cm stuff
	CmDevice* pCmDev;
	CmQueue* pCmQueue;



	CmThreadSpace* kernelThreadspace;

	CmTask* pCmTask = nullptr;
	CmEvent* pCmEvent = nullptr;
	CmKernel* pCmKernel = nullptr;



	// tmp ..
	bool useCmSVM = false;		// change this ...
	const std::string isaFileName = useCmSVM ? "RayTracing_Cm_SVM.isa" : "RayTracing_Cm.isa";

	// host arrays
	unsigned* hostSeeds;
	Vec* hostColor;
	Camera* hostCamera;
	Sphere* hostSpheres;
	const Sphere* defaultSpheres;
	unsigned defaultSphereCount = 0;
	unsigned* hostPixels;


	void setSceneArguemnts();

private:

};



class CmConfigBuffer : public CmConfig {

public:

	CmConfigBuffer(int width, int height);

	virtual void sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) override;

	virtual ~CmConfigBuffer();

private:

	virtual void setArguments() override;
	virtual void execute() override;
	virtual void allocateBuffer() override;
	virtual void freeBuffer() override;



	// Cm buffers
	CmBuffer* cameraBuffer;
	CmBuffer* seedsBuffer;
	CmBuffer* colorBuffer;
	CmBuffer* spheresBuffer;
	CmBuffer* pixelBuffer;

};


class CmConfigSVM : public CmConfig {

public:

	CmConfigSVM(int width, int height);

	virtual void sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) override;

	virtual ~CmConfigSVM();

private:

	virtual void setArguments() override;
	virtual void execute() override;
	virtual void allocateBuffer() override;
	virtual void freeBuffer() override;

	// cm SVM Allocator
	std::unique_ptr<CmSVMAllocator> pCmAllocator;

};




#endif
