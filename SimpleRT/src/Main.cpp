

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
#include "CmConfig.hpp"



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
	//ExecuteCmKernel();

	// set OpenGL pixel pointer here !

	//pixels = clPixels;

	pixels = hostPixels;
	
	InitGlut(argc, argv, "Ray-Tracing Experiment");
	
    glutMainLoop();


	FreeOpenCLBuffers();

	return 0;
}
