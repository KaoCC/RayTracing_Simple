

#include <cstdio>
#include <cstdlib>
#include <ctime>
//#include <cstring>

#include <string>
#include <vector>
#include <iostream>

#include <memory>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "Camera.hpp"
#include "Scene.hpp"
#include "SetupGL.hpp"
#include "Utility.hpp"
#include "Sphere.hpp"

#include "CmSVMAllocator.hpp"

// Cm Related

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

#ifdef CMRT_EMU

// function prototype

#endif

//OpenCL pixel buffer;
extern unsigned int *pixels;

/* Options Flags*/
static int useGPU = 0;
static int forceWorkSize = 0;

/* OpenCL Variables */
static cl_context context;


static cl_command_queue commandQueue;
static cl_program program;
static cl_kernel kernel;
static unsigned int workGroupSize = 1;
static std::string kernelFileName = "RayTracing_Kernel.cl";

// ------------------------

// Cm stuff
static CmDevice* pCmDev;
static CmQueue* pCmQueue;

static const std::string isaFileName = "RayTracing_Cm.isa";

// host arrays
static unsigned int* hostSeeds;
static Vec* hostColor;
Camera* hostCamera;
Sphere* hostSpheres;
Sphere* defaultSpheres;
unsigned  defaultSphereCount = 0;
static unsigned int* hostPixels;

// Cm buffers

CmBuffer* cameraBuffer;
CmBuffer* seedsBuffer;
CmBuffer* colorBuffer;
CmBuffer* spheresBuffer;
CmBuffer* pixelBuffer;

// cm SVM Allocator
std::unique_ptr<CmSVMAllocator> pCmAllocator;


const unsigned int kThreadWidth = 10;
const unsigned int kThreadheight = 10;
CmThreadSpace* kernelThreadspace;

CmTask* pCmTask;

CmEvent* pCmEvent = nullptr;


// --------------

static unsigned int *seeds;
static Vec* color;

//Camera camera;
Camera* cameraPtr;
static int currentSample = 0;
Sphere *spheres;
Sphere *spheres_host_ptr;
unsigned int sphereCount;
static unsigned int* clPixels;

// selection parameters
static const int kPlatformID = 0;

static void SetupOpenCLDefaultScene()
{
	spheres_host_ptr = DemoSpheres;
	sphereCount = sizeof(DemoSpheres) / sizeof(Sphere);

	spheres = (Sphere *)clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(Sphere) * sphereCount, 0);

	for (unsigned i = 0; i < sphereCount; ++i) {
		spheres[i] = spheres_host_ptr[i];
	}

	//vinit(&cameraPtr->orig, 20.f, 100.f, 120.f);
	cameraPtr->orig = { 20.f, 100.f, 120.f };

	//vinit(&cameraPtr->target, 0.f, 25.f, 0.f);
	cameraPtr->target = { 0.f, 25.f, 0.f };
}


// tmp
static void SetupCmDefaultScene() {

	defaultSpheres = DemoSpheres;
	defaultSphereCount = sizeof(DemoSpheres) / sizeof(Sphere);

	
	// Sphere: float + Vec * 3 + enum --> 1 + 3 * 3 + 1  --> 11 float 
	// padding for alignment: + 1 to 12 float --> 12 * sizeof(float) = 48 (OWORD align)

	const unsigned kSphereFloatCount = 1 + 3 * 3 + 1 + 1;

	hostSpheres = reinterpret_cast<Sphere*>(new float[kSphereFloatCount * defaultSphereCount]);		// leak

	// Sphere buffer
	pCmDev->CreateBuffer(sizeof(float) * kSphereFloatCount * defaultSphereCount, spheresBuffer);


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

static void FreeOpenCLBuffers() {

	// openCL Buffer: color, pixel, seed

	clSVMFree(context, seeds);
	clSVMFree(context, clPixels);
	clSVMFree(context, color);
	clSVMFree(context, spheres);
}

static void AllocateOpenCLBuffers() {
	const int pixelCount = width * height;
	cameraPtr = (Camera*)clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(Camera), 0);

	seeds = (unsigned int *)clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(unsigned int) * pixelCount * 2, 0);
	for (int i = 0; i < pixelCount * 2; ++i) {
		seeds[i] = std::rand();
		if (seeds[i] < 2)
			seeds[i] = 2;
	}

	clPixels = (unsigned int *)clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(unsigned int) * pixelCount, 0);

	color = (Vec*)clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(Vec) * pixelCount, 0);


}



static void AllocateCmBuffers() {
	const int pixelCount = width * height;



	// SVM allocator
	//pCmAllocator = std::make_unique<CmSVMAllocator>(pCmDev);


	// should change to smart pointer later.

	// Camera
	//hostCamera = new Camera();
	//pCmDev->CreateBuffer(sizeof(Camera), cameraBuffer);

	// bad design, for testing only   	// Vec orig, target (3 + 3) Vec dir, x, y (3 + 3 + 3)
	hostCamera = reinterpret_cast<Camera*>(new float[3 + 3 + 3 + 3 + 3]);		
	pCmDev->CreateBuffer(sizeof(float) * (3 + 3 + 3 + 3 + 3), cameraBuffer);


	// TEST, use SVM
	//hostCamera = static_cast<Camera*>(pCmAllocator->allocate(sizeof(Camera)));


	// seed
	hostSeeds = new unsigned int[pixelCount * 2];
	for (int i = 0; i < pixelCount * 2; ++i) {
		//hostSeeds[i] = std::rand();

		// TEST!
		hostSeeds[i] = seeds[i];

		if (hostSeeds[i] < 2)
			hostSeeds[i] = 2;
	}
	
	pCmDev->CreateBuffer(sizeof(unsigned int) * pixelCount * 2 , seedsBuffer);
	


	// pixels 
#pragma message ( "Check the pixel host buffer ! (AllocateCmBuffers)" )


	pCmDev->CreateBuffer(sizeof(unsigned int) * pixelCount, pixelBuffer);


	// color
	//hostColor = new Vec[pixelCount];
	//pCmDev->CreateBuffer(sizeof(Vec) * pixelCount, colorBuffer);


	// bad design, for testing only
	// Vec 3

	// add padding for alignmen issue
	const unsigned kColorFloatCount = 3 + 1;	// Vec(3) + 1 float padding

	hostColor = reinterpret_cast<Vec*>(new float[kColorFloatCount * pixelCount]);
	pCmDev->CreateBuffer(sizeof(float) * kColorFloatCount * pixelCount, colorBuffer);

	// tmp init to zero, should be changed later 
	float* tmpColor = (float*)hostColor;
	for (int i = 0; i < kColorFloatCount * pixelCount; ++i) {
		tmpColor[i] = 0;
	}

	hostPixels = new unsigned[pixelCount];

	// tmp disable 
	//pCmDev->CreateBuffer(sizeof(unsigned) * pixelCount , pixelBuffer);

	for (int i = 0; i < pixelCount; ++i) {
		hostPixels[i] = 0;
	}


}


static std::vector<char> ReadKernelSourcesFile(const std::string& fileName) {

	FILE *file = fopen(fileName.c_str(), "rb");
	if (!file) {
		fprintf(stderr, "Failed to open file '%s'\n", fileName.c_str());
		exit(-1);
	}

	if (fseek(file, 0, SEEK_END)) {
		fprintf(stderr, "Failed to seek file '%s'\n", fileName.c_str());
		exit(-1);
	}

	long size = ftell(file);
	if (size == 0) {
		fprintf(stderr, "Failed to check position on file '%s'\n", fileName.c_str());
		exit(-1);
	}

	rewind(file);

	//char *src = (char *)malloc(sizeof(char) * size + 1);
	//if (!src) {
	//	fprintf(stderr, "Failed to allocate memory for file '%s'\n", fileName.c_str());
	//	exit(-1);
	//}

	std::vector<char> src(size + 1);

	fprintf(stderr, "Reading file '%s' (size %ld bytes)\n", fileName.c_str(), size);
	size_t res = fread(src.data(), sizeof(char), sizeof(char) * size, file);
	if (res != sizeof(char) * size) {
		fprintf(stderr, "Failed to read file '%s' (read %ld)\n", fileName.c_str(), res);
		exit(-1);
	}
	src[size] = '\0'; /* NULL terminated */

	fclose(file);


	return src;

}

static void SetUpOpenCLKernelArguments()
{
		/* Set kernel arguments */
	cl_int status = clSetKernelArgSVMPointer(
			kernel,
			0,
			color);

	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #1: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArgSVMPointer(
			kernel,
			1,
			seeds);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #2: %d\n", status);
		exit(-1);
	}

	// wait
	status = clSetKernelArgSVMPointer(
			kernel,
			2,
			spheres);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #3: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArgSVMPointer(
			kernel,
			3,
			cameraPtr);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #4: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
			kernel,
			4,
			sizeof(unsigned int),
			(void *)&sphereCount);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #5: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
			kernel,
			5,
			sizeof(int),
			(void *)&width);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #6: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
			kernel,
			6,
			sizeof(int),
			(void *)&height);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #7: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
			kernel,
			7,
			sizeof(int),
			(void *)&currentSample);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #8: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArgSVMPointer(
			kernel,
			8,
			clPixels);

	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #9: %d\n", status);
		exit(-1);
	}
}

static void SetUpOpenCL() {
	cl_device_type dType = useGPU ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU;

	// Select the platform

    cl_uint numPlatforms;
	cl_platform_id platform = NULL;
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to get OpenCL platforms\n");
		exit(-1);
	}

	if (numPlatforms > 0) {
		cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * numPlatforms);
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
			if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL platform IDs\n");
			exit(-1);
		}

		
		for (unsigned i = 0; i < numPlatforms; ++i) {
			char pbuf[100];
			status = clGetPlatformInfo(platforms[i],
				CL_PLATFORM_NAME,
					sizeof(pbuf),
					pbuf,
					NULL);

			status = clGetPlatformIDs(numPlatforms, platforms, NULL);
			if (status != CL_SUCCESS) {
				fprintf(stderr, "Failed to get OpenCL platform IDs\n");
				exit(-1);
			}

			fprintf(stderr, "OpenCL Platform %d: %s\n", i, pbuf);
		}

		platform = platforms[kPlatformID];
		fprintf(stderr, "[Selected] OpenCL Platform %d\n", kPlatformID);



		free(platforms);
	}

	// Select the device
	cl_uint deviceCount;
	cl_device_id devices[32];
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 32, devices, &deviceCount);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to get OpenCL device IDs\n");
		exit(-1);
	}

	int deviceFound = 0;
	cl_device_id selectedDevice;

	for (unsigned i = 0; i < deviceCount; ++i) {
		cl_device_type type = 0;
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_TYPE,
				sizeof(cl_device_type),
				&type,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		char *stype;
		switch (type) {
			case CL_DEVICE_TYPE_ALL:
				stype = "TYPE_ALL";
				break;
			case CL_DEVICE_TYPE_DEFAULT:
				stype = "TYPE_DEFAULT";
				break;
			case CL_DEVICE_TYPE_CPU:
				stype = "TYPE_CPU";
				if (!useGPU && !deviceFound) {
					selectedDevice = devices[i];
					deviceFound = 1;
				}
				break;
			case CL_DEVICE_TYPE_GPU:
				stype = "TYPE_GPU";
				if (useGPU && !deviceFound) {
					selectedDevice = devices[i];	//choose th first one we got
					deviceFound = 1;
				}
				break;
			default:
				stype = "TYPE_UNKNOWN";
				break;
		}
		fprintf(stderr, "OpenCL Device %d: Type = %s\n", i, stype);

		char buf[256];
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_NAME,
				sizeof(char[256]),
				&buf,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		fprintf(stderr, "OpenCL Device %d: Name = %s\n", i, buf);

		cl_uint units = 0;
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_MAX_COMPUTE_UNITS,
				sizeof(cl_uint),
				&units,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		fprintf(stderr, "OpenCL Device %d: Compute units = %u\n", i, units);

		size_t gsize = 0;
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_MAX_WORK_GROUP_SIZE,
				sizeof(size_t),
				&gsize,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		fprintf(stderr, "OpenCL Device %d: Max. work group size = %d\n", i, (unsigned int)gsize);
	}

	//tmp
//	selectedDevice = devices[1];

	if (!deviceFound) {
		fprintf(stderr, "Unable to select an appropriate device\n");
		exit(-1);
	}

	// Create the context

	cl_context_properties cps[] = {
		CL_CONTEXT_PLATFORM,
		(cl_context_properties) platform,
		0
	};

	cl_context_properties *cprops = (NULL == platform) ? NULL : cps;
	context = clCreateContext(
			cprops,
			1,
			&selectedDevice,
			NULL,
			NULL,
			&status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to open OpenCL context\n");
		exit(-1);
	}


    /* Get the device list data */
	size_t deviceListSize;
    status = clGetContextInfo(
            context,
            CL_CONTEXT_DEVICES,
            32,
            devices,
            &deviceListSize);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to get OpenCL context info: %d\n", status);
		exit(-1);
    }

	/* Print devices list */
	for (unsigned i = 0; i < deviceListSize / sizeof(cl_device_id); ++i) {
		cl_device_type type = 0;
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_TYPE,
				sizeof(cl_device_type),
				&type,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		char *stype;
		switch (type) {
			case CL_DEVICE_TYPE_ALL:
				stype = "TYPE_ALL";
				break;
			case CL_DEVICE_TYPE_DEFAULT:
				stype = "TYPE_DEFAULT";
				break;
			case CL_DEVICE_TYPE_CPU:
				stype = "TYPE_CPU";
				break;
			case CL_DEVICE_TYPE_GPU:
				stype = "TYPE_GPU";
				break;
			default:
				stype = "TYPE_UNKNOWN";
				break;
		}
		fprintf(stderr, "[Selected] OpenCL Device %d: Type = %s\n", i, stype);

		char buf[256];
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_NAME,
				sizeof(char[256]),
				&buf,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		fprintf(stderr, "[Selected] OpenCL Device %d: Name = %s\n", i, buf);

		cl_uint units = 0;
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_MAX_COMPUTE_UNITS,
				sizeof(cl_uint),
				&units,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		fprintf(stderr, "[Selected] OpenCL Device %d: Compute units = %u\n", i, units);

		size_t gsize = 0;
		status = clGetDeviceInfo(devices[i],
				CL_DEVICE_MAX_WORK_GROUP_SIZE,
				sizeof(size_t),
				&gsize,
				NULL);
		if (status != CL_SUCCESS) {
			fprintf(stderr, "Failed to get OpenCL device info: %d\n", status);
			exit(-1);
		}

		fprintf(stderr, "[Selected] OpenCL Device %d: Max. work group size = %d\n", i, (unsigned int)gsize);
	}

	cl_command_queue_properties prop = 0;
	commandQueue = clCreateCommandQueue(
			context,
			selectedDevice,
			prop,
			&status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to create OpenCL command queue: %d\n", status);
		exit(-1);
    }


	/* Create the kernel program */
	const std::vector<char>& buffer = ReadKernelSourcesFile(kernelFileName);
	const char *sources = buffer.data();
	program = clCreateProgramWithSource(
        context,
        1,
        &sources,
        NULL,
        &status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to open OpenCL kernel sources: %d\n", status);
		exit(-1);
    }


	status = clBuildProgram(program, 0, 0, "-I. -w -cl-std=CL2.0", NULL, NULL);

	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to build OpenCL kernel: %d\n", status);

        size_t retValSize;
		status = clGetProgramBuildInfo(
				program,
			selectedDevice,
				CL_PROGRAM_BUILD_LOG,
				0,
				NULL,
				&retValSize);
        if (status != CL_SUCCESS) {
            fprintf(stderr, "Failed to get OpenCL kernel info size: %d\n", status);
			exit(-1);
		}

        char *buildLog = (char *)malloc(retValSize + 1);
        status = clGetProgramBuildInfo(
				program,
			selectedDevice,
				CL_PROGRAM_BUILD_LOG,
				retValSize,
				buildLog,
				NULL);
		if (status != CL_SUCCESS) {
            fprintf(stderr, "Failed to get OpenCL kernel info: %d\n", status);
			exit(-1);
		}
        buildLog[retValSize] = '\0';

		fprintf(stderr, "OpenCL Programm Build Log: %s\n", buildLog);
		exit(-1);
    }

	kernel = clCreateKernel(program, "RayTracing", &status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to create OpenCL kernel: %d\n", status);
		exit(-1);
    }

	// for better workGroupSize
	size_t gsize = 0;
	status = clGetKernelWorkGroupInfo(kernel,
		selectedDevice,
			CL_KERNEL_WORK_GROUP_SIZE,
			sizeof(size_t),
			&gsize,
			NULL);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to get OpenCL kernel work group size info: %d\n", status);
		exit(-1);
	}

	workGroupSize = (unsigned int) gsize;
	fprintf(stderr, "Kernel work group size = %d\n", workGroupSize);

	if (forceWorkSize > 0) {
		fprintf(stderr, "Forced kernel work group size = %d\n", forceWorkSize);
		workGroupSize = forceWorkSize;
	}



	/*------------------------------------------------------------------------*/

	AllocateOpenCLBuffers();

	/*------------------------------------------------------------------------*/
}


static void SetupCM() {

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
	CmKernel* cmKernel = nullptr;
	result = pCmDev->CreateKernel(cmProgram, CM_KERNEL_FUNCTION(RayTracing), cmKernel);
	if (result != CM_SUCCESS) {
		perror ("LoadProgram error");
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




static void ExecuteOpenCLKernel() {

	/* Enqueue a kernel run command */
	size_t globalThreads[1];
	globalThreads[0] = width * height;
	if (globalThreads[0] % workGroupSize != 0)
		globalThreads[0] = (globalThreads[0] / workGroupSize + 1) * workGroupSize;
	size_t localThreads[1];
	localThreads[0] = workGroupSize;

	cl_int status = clEnqueueNDRangeKernel(
			commandQueue,
			kernel,
			1,
			NULL,
			globalThreads,
			localThreads,
			0,
			NULL,
			NULL);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to enqueue OpenCL work: %d\n", status);
		exit(-1);
	}

	clFinish(commandQueue);
}


static void ExecuteCmKernel() {

	std::cerr << "before Host seed " << 0 << " x value:" << hostSeeds[0] << std::endl;
	std::cerr << "before Host seed " << 1 << " x value:" << hostSeeds[1] << std::endl;

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


	// tmp, we do not need to read back seed
	seedsBuffer->ReadSurface(reinterpret_cast<unsigned char*>(hostSeeds), pCmEvent);

	colorBuffer->ReadSurface(reinterpret_cast<unsigned char*>(hostColor), pCmEvent);

	pCmEvent->WaitForTaskFinished();


	// test

	std::cerr << "Tmp Test Seed" << std::endl;
	//for (auto i = 0; i < width * height; ++i) {
	//	std::cerr << "Host seed " << i << " x value:" <<hostSeeds[i] << std::endl;
	//}

	std::cerr << "Host seed " << 0 << " x value:" << hostSeeds[0] << std::endl;
	std::cerr << "Host seed " << 1 << " x value:" << hostSeeds[1] << std::endl;


}


void UpdateRendering() {
	double startTime = WallClockTime();
	int startSampleCount = currentSample;

	SetUpOpenCLKernelArguments();

	//--------------------------------------------------------------------------

	//if (1 /*currentSample < 200*/) {

	ExecuteOpenCLKernel();
	++currentSample;

	//	printf("done: %d\n", currentSample);
	//} else {
	//	/* After first 20 samples, continue to execute kernels for more and more time */
	//	const float k = min(currentSample - 20, 100) / 100.f;
	//	const float tresholdTime = 0.5f * k;
	//	while (1) {
	//		ExecuteKernel();
	//		clFinish(commandQueue);
	//		currentSample++;

	//		const float elapsedTime = WallClockTime() - startTime;
	//		if (elapsedTime > tresholdTime)
	//			break;
	//	}
	//}

	//--------------------------------------------------------------------------



	/*===========================================================================*/

	const double elapsedTime = WallClockTime() - startTime;
	const int samples = currentSample - startSampleCount;
	const double sampleSec = samples * height * width / elapsedTime;
	sprintf(captionBuffer, "Rendering time %.3f sec (pass %d)  Sample/sec  %.1fK\n",
			elapsedTime, currentSample, sampleSec / 1000.f);
}


void ReInitScene() {
	currentSample = 0;

	// Reload the scene

}

void ReInit(const int reallocBuffers) {

	if (reallocBuffers) {
		FreeOpenCLBuffers();
		UpdateCamera(cameraPtr);
		AllocateOpenCLBuffers();
	} else {
		UpdateCamera(cameraPtr);
	}


	currentSample = 0;
}

int main(int argc, char *argv[]) {

	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, "Usage: %s <use CPU/GPU (0/1)> <workgroup size (power of 2, 0=default)> <kernel> <width> <height> <scene>\n", argv[0]);
	fprintf(stderr, "Usage: %s <use CPU/GPU (0/1)> <width> <height>\n", argv[0]);

	SetUpOpenCL();

	// tmp
	SetupCM();

	if (argc == 1) {
		SetupOpenCLDefaultScene();
	} else if (argc == 4) {
		useGPU = atoi(argv[1]);
		width = atoi(argv[2]);
		height = atoi(argv[3]);
		SetupOpenCLDefaultScene();

	} else if (argc == 7) {
		useGPU = atoi(argv[1]);
		forceWorkSize = atoi(argv[2]);
		kernelFileName = argv[3];
		width = atoi(argv[4]);
		height = atoi(argv[5]);
		ReadScene(argv[6]);
	} else {
		exit(EXIT_FAILURE);
	}

	UpdateCamera(cameraPtr);


	// test
	//SetupCmDefaultScene();
	UpdateCameraCmBuffer();
	ExecuteCmKernel();

	// set OpenGL pixel pointer here !

	pixels = clPixels;
	

	InitGlut(argc, argv, "OpenCL Ray Tracing Experiment");
	

    glutMainLoop();


	FreeOpenCLBuffers();



	return 0;
}
