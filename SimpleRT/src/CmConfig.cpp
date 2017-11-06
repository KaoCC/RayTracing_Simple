
#include "CmConfig.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <vector>







extern std::vector<char> ReadKernelSourcesFile(const std::string&);




// simple impl.
float clamp(float x, float low, float high) {
	if (x < low) {
		return low;
	} else if (x > high) {
		return high;
	} else {
		return x;
	}
}



// TEST !
#define toInt(x) ((int)(pow(clamp(x, 0.f, 1.f), 1.f / 2.2f) * 255.f + .5f))

// ------------------------





const unsigned kThreadWidth = 100;
const unsigned kThreadHeight = 100;



// Sphere: float + Vec * 3 + enum --> 1 + 3 * 3 + 1  --> 11 float 
// padding for alignment: + 1 to 12 float --> 12 * sizeof(float) = 48 (OWORD align)
constexpr const unsigned kSphereFloatCount = 1 + 3 * 3 + 1 + 1;

// Vec orig, target (3 + 3) Vec dir, x, y (3 + 3 + 3)
constexpr const unsigned kCmCamerafloatCount = 3 + 3 + 3 + 3 + 3;

// add padding for alignmen issue
constexpr const unsigned kColorFloatCount = 3 + 1;	// Vec(3) + 1 float padding

// --------------




//void UpdateRenderingCm() {
//	double startTime = WallClockTime();
//	int startSampleCount = currentSampleCount;
//
//
//	// tmp
//	cmKernel->SetKernelArg(6, sizeof(unsigned), &currentSampleCount);
//
//	ExecuteCmKernel();
//	++currentSampleCount;
//
//
//	const double elapsedTime = WallClockTime() - startTime;
//	const int samples = currentSampleCount - startSampleCount;
//	const double sampleSec = samples * height * width / elapsedTime;
//	sprintf(captionBuffer, "Rendering time %.3f sec (pass %d)  Sample/sec  %.1fK\n",
//		elapsedTime, currentSampleCount, sampleSec / 1000.f);
//}

//-----------




CmConfig::CmConfig(int width, int height) : Config(width, height) {

	int result = 0;

	pCmDev = nullptr;
	UINT version = 0;

	result = CreateCmDevice(pCmDev, version);
	if (result != CM_SUCCESS) {
		printf("CmDevice creation error");
		exit(EXIT_FAILURE);
	}

	if (version < CM_1_0) {
		printf(" The runtime API version is later than runtime DLL version");
		exit(EXIT_FAILURE);
	}

	FILE* pISA = fopen(isaFileName.c_str(), "rb");
	if (pISA == NULL) {
		perror("isa file");
		exit(EXIT_FAILURE);
	}

	std::vector<char> pCommonISABuffer = ReadKernelSourcesFile(isaFileName);

	CmProgram* pCmProgram = nullptr;
	result = pCmDev->LoadProgram(reinterpret_cast<void*>(pCommonISABuffer.data()), pCommonISABuffer.size(), pCmProgram);		// check the size if error !
	if (result != CM_SUCCESS) {
		perror("CM LoadProgram error");
		exit(EXIT_FAILURE);
	}


	// Create Kernel
	// CmKernel* cmKernel = nullptr;
	result = pCmDev->CreateKernel(pCmProgram, CM_KERNEL_FUNCTION(RayTracing), pCmKernel);
	if (result != CM_SUCCESS) {
		perror("LoadProgram error");
		exit(EXIT_FAILURE);
	}



	pCmDev->CreateThreadSpace(kThreadWidth, kThreadHeight, kernelThreadspace);
	pCmKernel->SetThreadCount(kThreadWidth * kThreadHeight);


	// create Task
	pCmDev->CreateTask(pCmTask);
	pCmTask->AddKernel(pCmKernel);

	// create Queue
	pCmDev->CreateQueue(pCmQueue);

}

void CmConfig::updateCamera() {
}

unsigned * CmConfig::getPixels() {
	return hostPixels;
}

void CmConfig::setSceneArguments() {


	float* tmpSphereBuf = reinterpret_cast<float*>(hostSpheres);

	unsigned tmpSphereIndex = 0;

	for (unsigned i = 0; i < defaultSphereCount; ++i) {

		// rad
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].rad;

		// Vec p
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].p.x;
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].p.y;
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].p.z;

		// Vec e
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].e.x;
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].e.y;
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].e.z;

		// Vec c
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].c.x;
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].c.y;
		tmpSphereBuf[tmpSphereIndex++] = defaultSpheres[i].c.z;

		// Refl
		tmpSphereBuf[tmpSphereIndex++] = static_cast<float>(defaultSpheres[i].refl);

		// padding
		tmpSphereBuf[tmpSphereIndex++] = -1;
	}


	// assign camera value
	// **  convert to float pointer first

	float* tmpCameraBuf = reinterpret_cast<float*>(hostCamera);

	unsigned tmpCameraIndex = 0;

	// orig
	tmpCameraBuf[tmpCameraIndex++] = 20.f;
	tmpCameraBuf[tmpCameraIndex++] = 100.f;
	tmpCameraBuf[tmpCameraIndex++] = 120.f;

	// target
	tmpCameraBuf[tmpCameraIndex++] = 0.f;
	tmpCameraBuf[tmpCameraIndex++] = 25.f;
	tmpCameraBuf[tmpCameraIndex++] = 0.f;

}



CmConfigBuffer::CmConfigBuffer(int width, int height) : CmConfig(width, height) {
	allocateBuffer();
}

void CmConfigBuffer::sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) {

	defaultSpheres = spheres.data();
	defaultSphereCount = spheres.size();

	hostSpheres = reinterpret_cast<Sphere*>(new float[kSphereFloatCount * defaultSphereCount]);		// leak
	pCmDev->CreateBuffer(sizeof(float) * kSphereFloatCount * defaultSphereCount, spheresBuffer); // Sphere buffer

	setSceneArguments();
}

CmConfigBuffer::~CmConfigBuffer() {
	// yet to be done
}

void CmConfigBuffer::setArguments() {

	// get index 
	// 
	SurfaceIndex* cameraIndex;
	SurfaceIndex* seedsIndex;
	SurfaceIndex* colorIndex;
	SurfaceIndex* spheresIndex;
	SurfaceIndex* pixelIndex;

	cameraBuffer->GetIndex(cameraIndex);
	seedsBuffer->GetIndex(seedsIndex);
	colorBuffer->GetIndex(colorIndex);
	spheresBuffer->GetIndex(spheresIndex);
	pixelBuffer->GetIndex(pixelIndex);


	// Set Kernel Args (tmp)

	int kernelArgIndex = 0;

	int result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), cameraIndex);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), seedsIndex);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), colorIndex);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), spheresIndex);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), pixelIndex);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(unsigned), &defaultSphereCount);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(unsigned), &currentSampleCount);

}

void CmConfigBuffer::execute() {

	cameraBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostCamera), nullptr);
	seedsBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostSeeds), nullptr);
	colorBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostColor), nullptr);
	spheresBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostSpheres), nullptr);

	int status = pCmQueue->Enqueue(pCmTask, pCmEvent, kernelThreadspace);

	if (status != 0) {
		std::cerr << "Error...: " << status << std::endl;
	}


	pCmEvent->WaitForTaskFinished();

	std::cout << "Cm Done!" << std::endl;

	seedsBuffer->ReadSurface(reinterpret_cast<unsigned char*>(hostSeeds), pCmEvent);
	pCmEvent->WaitForTaskFinished();

	colorBuffer->ReadSurface(reinterpret_cast<unsigned char*>(hostColor), pCmEvent);
	pCmEvent->WaitForTaskFinished();

	pixelBuffer->ReadSurface(reinterpret_cast<unsigned char*>(hostPixels), pCmEvent);
	pCmEvent->WaitForTaskFinished();
}

void CmConfigBuffer::allocateBuffer() {


	const int pixelCount = mWidth * mHeight;

	// should change to smart pointer later.


	// bad design, for testing only   	// Vec orig, target (3 + 3) Vec dir, x, y (3 + 3 + 3)
	hostCamera = reinterpret_cast<Camera*>(new float[kCmCamerafloatCount]);
	pCmDev->CreateBuffer(sizeof(float) * (kCmCamerafloatCount), cameraBuffer);


	// TEST, use SVM
	//hostCamera = static_cast<Camera*>(pCmAllocator->allocate(sizeof(Camera)));

	// seed
	hostSeeds = new unsigned[pixelCount * 2];


	// test

	//std::cerr << "SEED TEST\n";
	//for (int i = 0; i < 8; ++i) {
	//	std::cerr << hostSeeds[i] << std::endl;
	//}

	//std::cerr << "hostSeed 160: " << hostSeeds[160] << std::endl;

	//std::cerr << "END SEED TEST\n";

	pCmDev->CreateBuffer(sizeof(unsigned int) * pixelCount * 2, seedsBuffer);


	// bad design, for testing only
	// Vec 3

	hostColor = reinterpret_cast<Vec*>(new float[kColorFloatCount * pixelCount]);
	pCmDev->CreateBuffer(sizeof(float) * kColorFloatCount * pixelCount, colorBuffer);


	// pixels
	hostPixels = new unsigned[pixelCount];
	pCmDev->CreateBuffer(sizeof(unsigned) * pixelCount, pixelBuffer);		// alignment ?  current setting : 4 * 800 * 600 


	//  ---- init part ---- 

	// seeds
	for (int i = 0; i < pixelCount * 2; ++i) {
		//hostSeeds[i] = std::rand();

		// TEST!
		hostSeeds[i] = std::rand();

		if (hostSeeds[i] < 2)
			hostSeeds[i] = 2;
	}

	// color
	float* tmpColor = (float*)hostColor;
	for (unsigned i = 0; i < kColorFloatCount * pixelCount; ++i) {
		tmpColor[i] = 0;
	}

	// pixel
	for (int i = 0; i < pixelCount; ++i) {
		hostPixels[i] = 0;
	}


}

void CmConfigBuffer::freeBuffer() {
	// yet to be done 
}



// -----------------------------


CmConfigSVM::CmConfigSVM(int width, int height) : CmConfig(width, height) {
	allocateBuffer();
}

void CmConfigSVM::sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) {
	defaultSpheres = spheres.data();
	defaultSphereCount = spheres.size();


	hostSpheres = static_cast<Sphere*>(pCmAllocator->allocate(sizeof(float) * kSphereFloatCount * defaultSphereCount));

	setSceneArguments();
}

CmConfigSVM::~CmConfigSVM() {
	// yet to be done
}

void CmConfigSVM::setArguments() {


	int kernelArgIndex = 0;

	std::cerr << "sizeof ptr:" << sizeof(void*) << std::endl;

	int result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(void*), &hostCamera);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(void*), &hostSeeds);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(void*), &hostColor);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(void*), &hostSpheres);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(void*), &hostPixels);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(unsigned), &defaultSphereCount);
	result = pCmKernel->SetKernelArg(kernelArgIndex++, sizeof(unsigned), &currentSampleCount);

}

void CmConfigSVM::execute() {

	int status = pCmQueue->Enqueue(pCmTask, pCmEvent, kernelThreadspace);

	if (status != 0) {
		std::cerr << "Error...: " << status << std::endl;
	}

	pCmEvent->WaitForTaskFinished();
	std::cout << "Cm Done!" << std::endl;
}

void CmConfigSVM::allocateBuffer() {


	const int pixelCount = mWidth * mHeight;

	pCmAllocator = std::make_unique<CmSVMAllocator>(pCmDev);

	// camera
	hostCamera = static_cast<Camera*>(pCmAllocator->allocate(sizeof(float) * (kCmCamerafloatCount)));

	// seed 
	hostSeeds = static_cast<unsigned*>(pCmAllocator->allocate(sizeof(unsigned) * (pixelCount * 2)));

	// color
	hostColor = static_cast<Vec*>(pCmAllocator->allocate(sizeof(float) * kColorFloatCount * pixelCount));

	// pixel
	hostPixels = static_cast<unsigned*>(pCmAllocator->allocate(sizeof(unsigned) * pixelCount));

	//  ---- init part ---- 

	// seeds
	for (int i = 0; i < pixelCount * 2; ++i) {
		//hostSeeds[i] = std::rand();

		// TEST!
		hostSeeds[i] = std::rand();

		if (hostSeeds[i] < 2)
			hostSeeds[i] = 2;
	}

	// color
	float* tmpColor = (float*)hostColor;
	for (unsigned i = 0; i < kColorFloatCount * pixelCount; ++i) {
		tmpColor[i] = 0;
	}

	// pixel
	for (int i = 0; i < pixelCount; ++i) {
		hostPixels[i] = 0;
	}


}

void CmConfigSVM::freeBuffer() {
	// yet to be done
}
