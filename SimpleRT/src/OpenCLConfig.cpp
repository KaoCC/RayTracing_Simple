
#include "OpenCLConfig.hpp"

#include "Camera.hpp"
#include "Sphere.hpp"
#include "SetupGL.hpp"
#include "Scene.hpp"
#include "Utility.hpp"



//
//void ReInitScene() {
//	currentSample = 0;
//
//	// Reload the scene
//
//}
//
//void ReInit(const int reallocBuffers) {
//
//	if (reallocBuffers) {
//		FreeOpenCLBuffers();
//		UpdateCamera();
//		AllocateOpenCLBuffers();
//	} else {
//		UpdateCamera();
//	}
//
//
//	currentSample = 0;
//}


// -----------


OpenCLConfig::OpenCLConfig(int width, int height) : Config(width, height) {

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
		(cl_context_properties)platform,
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

	workGroupSize = (unsigned int)gsize;
	fprintf(stderr, "Kernel work group size = %d\n", workGroupSize);

	if (forceWorkSize > 0) {
		fprintf(stderr, "Forced kernel work group size = %d\n", forceWorkSize);
		workGroupSize = forceWorkSize;
	}

}

void OpenCLConfig::updateCamera() {
	computeCameraVariables(pCamera, mWidth, mHeight);
}

unsigned * OpenCLConfig::getPixels() {
	return pPixels;
}



// -----------------

OpenCLConfigBuffer::OpenCLConfigBuffer(int width, int height) : OpenCLConfig(width, height) {
	allocateBuffer();
}

OpenCLConfigBuffer::~OpenCLConfigBuffer() {
	freeBuffer();
}


void OpenCLConfigBuffer::execute() {

	/* Enqueue a kernel run command */
	size_t globalThreads[1];
	globalThreads[0] = mWidth * mHeight;
	if (globalThreads[0] % workGroupSize != 0)
		globalThreads[0] = (globalThreads[0] / workGroupSize + 1) * workGroupSize;
	size_t localThreads[1];
	localThreads[0] = workGroupSize;


	cl_int status = clEnqueueWriteBuffer(
		commandQueue,
		cameraBuffer,
		CL_TRUE,
		0,
		sizeof(Camera),
		pCamera,
		0,
		NULL,
		NULL);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to write the OpenCL camera buffer: %d\n", status);
		exit(-1);
	}

	cl_uint sizeBytes = sizeof(unsigned) * mWidth * mHeight * 2;

	status = clEnqueueWriteBuffer(
		commandQueue,
		seedBuffer,
		CL_TRUE,
		0,
		sizeBytes,
		pSeeds,
		0,
		NULL,
		NULL);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to write the OpenCL seeds buffer: %d\n", status);
		exit(-1);
	}

	status = clEnqueueWriteBuffer(
		commandQueue,
		sphereBuffer,
		CL_TRUE,
		0,
		sizeof(Sphere) * mSphereCount,
		pSpheres,
		0,
		NULL,
		NULL);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to write the OpenCL scene buffer: %d\n", status);
		exit(-1);
	}

	status = clEnqueueNDRangeKernel(
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


	// Read the result if we are using buffers
	status = clEnqueueReadBuffer(
		commandQueue,
		pixelBuffer,
		CL_TRUE,
		0,
		mWidth * mHeight * sizeof(unsigned),
		pPixels,
		0,
		NULL,
		NULL);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to read the OpenCL pixel buffer: %d\n", status);
		exit(-1);
	}

	sizeBytes = sizeof(unsigned) * mWidth * mHeight * 2;
	status = clEnqueueReadBuffer(
		commandQueue,
		seedBuffer,
		CL_TRUE,
		0,
		sizeBytes,
		pSeeds,
		0,
		NULL,
		NULL);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to read the OpenCL seeds buffer: %d\n", status);
		exit(-1);
	}

}

void OpenCLConfigBuffer::setArguments() {

	cl_int status = clSetKernelArg(
		kernel,
		0,
		sizeof(cl_mem),
		(void *)&colorBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #1: %d\n", status);
		exit(-1);
	}


	status = clSetKernelArg(
		kernel,
		1,
		sizeof(cl_mem),
		(void *)&seedBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #2: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
		kernel,
		2,
		sizeof(cl_mem),
		(void *)&sphereBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #3: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
		kernel,
		3,
		sizeof(cl_mem),
		(void *)&cameraBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #4: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
		kernel,
		4,
		sizeof(unsigned int),
		(void *)&mSphereCount);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #5: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
		kernel,
		5,
		sizeof(int),
		(void *)&mWidth);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #6: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
		kernel,
		6,
		sizeof(int),
		(void *)&mHeight);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #7: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
		kernel,
		7,
		sizeof(int),
		(void *)&mCurrentSample);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #8: %d\n", status);
		exit(-1);
	}


	status = clSetKernelArg(
		kernel,
		8,
		sizeof(cl_mem),
		(void *)&pixelBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #9: %d\n", status);
		exit(-1);
	}

}

void OpenCLConfigBuffer::allocateBuffer() {

	const int pixelCount = mWidth * mHeight;

	// raw array space
	pCamera = new Camera();
	pSeeds = new unsigned[pixelCount * 2];
	pPixels = new unsigned[pixelCount];
	pColor = new Vec[pixelCount];

	// CL Buffers

	cl_int status;

	cameraBuffer = clCreateBuffer(
		context,
		CL_MEM_READ_ONLY,
		sizeof(Camera),
		NULL,
		&status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to create OpenCL camera buffer: %d\n", status);
		exit(-1);
	}

	cl_uint sizeBytes = sizeof(Vec) * mWidth * mHeight;

	colorBuffer = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		sizeBytes,
		NULL,
		&status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to create OpenCL colorBuffer: %d\n", status);
		exit(-1);
	}

	sizeBytes = sizeof(unsigned int) * mWidth * mHeight;
	pixelBuffer = clCreateBuffer(
		context,
		CL_MEM_WRITE_ONLY,
		sizeBytes,
		NULL,
		&status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to create OpenCL pixelBuffer: %d\n", status);
		exit(-1);
	}

	sizeBytes = sizeof(unsigned) * mWidth * mHeight * 2;
	seedBuffer = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		sizeBytes,
		NULL,
		&status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to create OpenCL seedBuffer: %d\n", status);
		exit(-1);
	}

	//	// init seed
	for (int i = 0; i < pixelCount * 2; ++i) {
		pSeeds[i] = std::rand();
		if (pSeeds[i] < 2)
			pSeeds[i] = 2;
	}

}

void OpenCLConfigBuffer::freeBuffer() {

	// openCL Buffer: color, pixel, seed, camera
	cl_int status = clReleaseMemObject(colorBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to release OpenCL color buffer: %d\n", status);
		exit(-1);
	}

	status = clReleaseMemObject(pixelBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to release OpenCL pixel buffer: %d\n", status);
		exit(-1);
	}

	status = clReleaseMemObject(seedBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to release OpenCL seed buffer: %d\n", status);
		exit(-1);
	}

	status = clReleaseMemObject(cameraBuffer);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to release OpenCL camera buffer: %d\n", status);
		exit(-1);
	}


	// delete raw array
	delete pCamera;
	delete[] pSeeds;
	delete[] pPixels;
	delete[] pColor;
	delete[] pSpheres;
}

void OpenCLConfigBuffer::sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec target) {

	pSpheres = new Sphere[spheres.size()];
	mSphereCount = spheres.size();

	for (int i = 0; i < mSphereCount; ++i) {
		pSpheres[i] = spheres[i];
	}
	
	// CL buffer ?
	
	cl_int status;
	sphereBuffer = clCreateBuffer(
		context,
		CL_MEM_READ_ONLY,
		sizeof(Sphere) * mSphereCount,
		NULL,
		&status);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to create OpenCL scene buffer: %d\n", status);
		exit(-1);
	}


	pCamera->orig = orig;
	pCamera->target = target;

}

// -------------------

OpenCLConfigSVM::OpenCLConfigSVM(int width, int height) : OpenCLConfig(width, height) {
	allocateBuffer();
}

void OpenCLConfigSVM::sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec target) {

	pSpheres = static_cast<Sphere*>(clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(Sphere) * spheres.size(), 0));
	mSphereCount = spheres.size();
	
	for (unsigned i = 0; i < spheres.size(); ++i) {
		pSpheres[i] = spheres[i];
	}

	pCamera->orig = orig;
	pCamera->target = target;
}

OpenCLConfigSVM::~OpenCLConfigSVM() {
	freeBuffer();
}

void OpenCLConfigSVM::execute() {

	/* Enqueue a kernel run command */
	size_t globalThreads[1];
	globalThreads[0] = mWidth * mHeight;
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

void OpenCLConfigSVM::setArguments() {
	cl_int status = clSetKernelArgSVMPointer(
		kernel,
		0,
		pColor);
	
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #1: %d\n", status);
		exit(-1);
	}
	
	status = clSetKernelArgSVMPointer(
		kernel,
		1,
		pSeeds);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #2: %d\n", status);
		exit(-1);
	}
	
	// wait
	status = clSetKernelArgSVMPointer(
		kernel,
		2,
		pSpheres);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #3: %d\n", status);
		exit(-1);
	}
	
	status = clSetKernelArgSVMPointer(
		kernel,
		3,
		pCamera);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #4: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArg(
		kernel,
		4,
		sizeof(unsigned int),
		(void *)&mSphereCount);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #5: %d\n", status);
		exit(-1);
	}
	
	status = clSetKernelArg(
		kernel,
		5,
		sizeof(int),
		(void *)&mWidth);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #6: %d\n", status);
		exit(-1);
	}
	
	status = clSetKernelArg(
		kernel,
		6,
		sizeof(int),
		(void *)&mHeight);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #7: %d\n", status);
		exit(-1);
	}
	
	status = clSetKernelArg(
		kernel,
		7,
		sizeof(int),
		(void *)&mCurrentSample);
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #8: %d\n", status);
		exit(-1);
	}

	status = clSetKernelArgSVMPointer(
		kernel,
		8,
		pPixels);
	
	if (status != CL_SUCCESS) {
		fprintf(stderr, "Failed to set OpenCL arg. #9: %d\n", status);
		exit(-1);
	}


}

void OpenCLConfigSVM::allocateBuffer() {

	const int pixelCount = mWidth * mHeight;

	pCamera = static_cast<Camera*>(clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(Camera), 0));
	pSeeds = static_cast<unsigned*>(clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(unsigned) * pixelCount * 2, 0));
	pPixels = static_cast<unsigned*>(clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(unsigned) * pixelCount, 0));
	pColor = static_cast<Vec*>(clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER, sizeof(Vec) * pixelCount, 0));


	// init seed
	for (int i = 0; i < pixelCount * 2; ++i) {
		pSeeds[i] = std::rand();
		if (pSeeds[i] < 2)
			pSeeds[i] = 2;
	}	

}

void OpenCLConfigSVM::freeBuffer() {

	clSVMFree(context, pCamera);
	clSVMFree(context, pSeeds);
	clSVMFree(context, pPixels);
	clSVMFree(context, pColor);
	clSVMFree(context, pSpheres);
}
