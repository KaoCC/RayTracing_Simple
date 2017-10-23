
#include "CmConfig.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <vector>


#include "Camera.hpp"
#include "Scene.hpp"
#include "SetupGL.hpp"
#include "Utility.hpp"
#include "Sphere.hpp"


#include "CmSVMAllocator.hpp"

extern std::vector<char> ReadKernelSourcesFile(const std::string&);

#include "cm_rt.h"


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

// Cm stuff
static CmDevice* pCmDev;
static CmQueue* pCmQueue;

static const std::string isaFileName = "RayTracing_Cm.isa";

// host arrays
static unsigned* hostSeeds;
static Vec* hostColor;
Camera* hostCamera;
Sphere* hostSpheres;
Sphere* defaultSpheres;
unsigned  defaultSphereCount = 0;
unsigned* hostPixels;

// Cm buffers

CmBuffer* cameraBuffer;
CmBuffer* seedsBuffer;
CmBuffer* colorBuffer;
CmBuffer* spheresBuffer;
CmBuffer* pixelBuffer;

// cm SVM Allocator
std::unique_ptr<CmSVMAllocator> pCmAllocator;


const unsigned kThreadWidth = 20;
const unsigned kThreadheight = 20;
CmThreadSpace* kernelThreadspace;

CmTask* pCmTask;

CmEvent* pCmEvent = nullptr;

static CmKernel* cmKernel = nullptr;


static unsigned currentSampleCount = 0;



static bool useCmSVM = false;


// Sphere: float + Vec * 3 + enum --> 1 + 3 * 3 + 1  --> 11 float 
// padding for alignment: + 1 to 12 float --> 12 * sizeof(float) = 48 (OWORD align)
constexpr const unsigned kSphereFloatCount = 1 + 3 * 3 + 1 + 1;

// Vec orig, target (3 + 3) Vec dir, x, y (3 + 3 + 3)
constexpr const unsigned kCmCamerafloatCount = 3 + 3 + 3 + 3 + 3;

// add padding for alignmen issue
constexpr const unsigned kColorFloatCount = 3 + 1;	// Vec(3) + 1 float padding

// --------------

// tmp
void SetupCmDefaultScene() {

	defaultSpheres = DemoSpheres;
	defaultSphereCount = sizeof(DemoSpheres) / sizeof(Sphere);

	if (useCmSVM) {
		hostSpheres = static_cast<Sphere*>(pCmAllocator->allocate(sizeof(float) * kSphereFloatCount * defaultSphereCount));
	} else {

		hostSpheres = reinterpret_cast<Sphere*>(new float[kSphereFloatCount * defaultSphereCount]);		// leak
		pCmDev->CreateBuffer(sizeof(float) * kSphereFloatCount * defaultSphereCount, spheresBuffer); // Sphere buffer

	}


	// assign Sphere value 
	// ** convert to flaot pointer first

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



void AllocateCmBuffers() {
	const int pixelCount = width * height;


	// SVM allocator

	if (useCmSVM) {

		pCmAllocator = std::make_unique<CmSVMAllocator>(pCmDev);

		// camera
		hostCamera = static_cast<Camera*>(pCmAllocator->allocate(sizeof(float) * (kCmCamerafloatCount)));

		// seed 
		hostSeeds = static_cast<unsigned*>(pCmAllocator->allocate(sizeof(unsigned) * (pixelCount * 2)));

		// color
		hostColor = static_cast<Vec*>(pCmAllocator->allocate(sizeof(float) * kColorFloatCount * pixelCount));

		// pixel
		hostPixels = static_cast<unsigned*>(pCmAllocator->allocate(sizeof(unsigned) * pixelCount));

	} else {
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

		// pixels
		pCmDev->CreateBuffer(sizeof(unsigned int) * pixelCount, pixelBuffer);

		// bad design, for testing only
		// Vec 3


		hostColor = reinterpret_cast<Vec*>(new float[kColorFloatCount * pixelCount]);
		pCmDev->CreateBuffer(sizeof(float) * kColorFloatCount * pixelCount, colorBuffer);


		hostPixels = new unsigned[pixelCount];

	}

	
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

void SetupCM() {

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

	CmProgram* cmProgram = nullptr;
	result = pCmDev->LoadProgram(reinterpret_cast<void*>(pCommonISABuffer.data()), pCommonISABuffer.size(), cmProgram);		// check the size if error !
	if (result != CM_SUCCESS) {
		perror("CM LoadProgram error");
		exit(EXIT_FAILURE);
	}


	// Create Kernel
	// CmKernel* cmKernel = nullptr;
	result = pCmDev->CreateKernel(cmProgram, CM_KERNEL_FUNCTION(RayTracing), cmKernel);
	if (result != CM_SUCCESS) {
		perror("LoadProgram error");
		exit(EXIT_FAILURE);
	}


	// Allocate Cm Buffers
	AllocateCmBuffers();

	// test !
	SetupCmDefaultScene();

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

	// check the values !
	//cmKernel->SetKernelArg(0, sizeof(SurfaceIndex), cameraIndex);

	// TEST !!!
	//hostCamera->orig.x = 100.123;
	//hostCamera->orig.y = 456.789;
	//hostCamera->orig.z = 258.456;

	//hostCamera->dir.x = 100.123;
	//hostCamera->dir.y = 456.789;
	//hostCamera->dir.z = 258.456;

	// --- TEST ----
	//float* tmpCam = (float*)hostCamera;
	//tmpCam[0] = 1; tmpCam[1] = 2; tmpCam[2] = 3; 
	//tmpCam[3] = 4; tmpCam[4] = 5; tmpCam[5] = 6;
	//tmpCam[6] = 7; tmpCam[7] = 8; tmpCam[8] = 9;
	// --- END OF TEST ---

	int kernelArgIndex = 0;
	result = cmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), cameraIndex);
	result = cmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), seedsIndex);
	result = cmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), colorIndex);
	result = cmKernel->SetKernelArg(kernelArgIndex++, sizeof(SurfaceIndex), spheresIndex);
	result = cmKernel->SetKernelArg(kernelArgIndex++, sizeof(unsigned), &defaultSphereCount);

	//	cmKernel->SetKernelArg(2, sizeof(SurfaceIndex), pixelIndex);


	if (result != CM_SUCCESS) {
		std::cerr << "Set Kernel Arg Error ... " << result << std::endl;
	}


	// Create event ??

	// create thread count, thread space 

	// check the value !
	pCmDev->CreateThreadSpace(kThreadWidth, kThreadheight, kernelThreadspace);
	cmKernel->SetThreadCount(kThreadWidth * kThreadheight);


	// create Task
	pCmDev->CreateTask(pCmTask);
	pCmTask->AddKernel(cmKernel);

	// create Queue
	pCmDev->CreateQueue(pCmQueue);


}

void ExecuteCmKernel() {

	//std::cerr << "before Host seed " << 0 << " x value:" << hostSeeds[0] << std::endl;
	//std::cerr << "before Host seed " << 1 << " x value:" << hostSeeds[1] << std::endl;

	cameraBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostCamera), nullptr);
	seedsBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostSeeds), nullptr);
	colorBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostColor), nullptr);
	spheresBuffer->WriteSurface(reinterpret_cast<unsigned char*>(hostSpheres), nullptr);

	// test printf
	pCmDev->InitPrintBuffer();

	int status = pCmQueue->Enqueue(pCmTask, pCmEvent, kernelThreadspace);

	if (status != 0) {
		std::cerr << "Error...: " << status << std::endl;
	}


	pCmEvent->WaitForTaskFinished();

	std::cout << "Cm Done!" << std::endl;
	std::cerr << std::endl;

	pCmDev->FlushPrintBuffer();
	std::cout << std::endl;



	seedsBuffer->ReadSurface(reinterpret_cast<unsigned char*>(hostSeeds), pCmEvent);

	pCmEvent->WaitForTaskFinished();

	colorBuffer->ReadSurface(reinterpret_cast<unsigned char*>(hostColor), pCmEvent);

	pCmEvent->WaitForTaskFinished();

	// tmp color test
	float* tmpColorPtr = reinterpret_cast<float*>(hostColor);
	//std::cerr << "Color 0 1 2 pading: " << tmpColorPtr[0] << " " << tmpColorPtr[1] << " " << tmpColorPtr[2] << " " << tmpColorPtr[3] << " " << std::endl;


	// convert to pixel here ...

	// design trade-off : use Cm to convert and read/write the pixel value or do it locally (with CPU) ?



	const int pixelCount = width * height;
	for (auto i = 0; i < pixelCount; ++i) {
		hostPixels[i] = (toInt(tmpColorPtr[4 * i])) | (toInt(tmpColorPtr[4 * i + 1]) << 8) | (toInt(tmpColorPtr[4 * i + 2]) << 16);

		//if (hostPixels[i] > 0) {
		//	std::cerr << i <<" PIXEL: " << hostPixels[i] << " " << (toInt(tmpColorPtr[4 * i])) << " " << (toInt(tmpColorPtr[4 * i + 1])) << " " << (toInt(tmpColorPtr[4 * i + 2])) << std::endl;
		//}
	}




	// test

	//	std::cerr << "Tmp Test Seed" << std::endl;
	//for (auto i = 0; i < width * height; ++i) {
	//	std::cerr << "Host seed " << i << " x value:" <<hostSeeds[i] << std::endl;
	//}

	//	std::cerr << "Host seed " << 0 << " x value:" << hostSeeds[0] << std::endl;
	//	std::cerr << "Host seed " << 1 << " x value:" << hostSeeds[1] << std::endl;


}

void UpdateRenderingCm() {
	double startTime = WallClockTime();
	int startSampleCount = currentSampleCount;


	// tmp
	cmKernel->SetKernelArg(5, sizeof(unsigned), &currentSampleCount);

	ExecuteCmKernel();
	++currentSampleCount;


	const double elapsedTime = WallClockTime() - startTime;
	const int samples = currentSampleCount - startSampleCount;
	const double sampleSec = samples * height * width / elapsedTime;
	sprintf(captionBuffer, "Rendering time %.3f sec (pass %d)  Sample/sec  %.1fK\n",
		elapsedTime, currentSampleCount, sampleSec / 1000.f);
}

