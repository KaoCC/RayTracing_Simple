

#include <cstdio>
#include <cstdlib>
#include <ctime>


#include "SetupGL.hpp"
#include "OpenCLConfig.hpp"

extern int forceWorkSize;
extern int useGPU;
extern std::string kernelFileName;

int main(int argc, char *argv[]) {

	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, "Usage: %s <use CPU/GPU (0/1)> <workgroup size (power of 2, 0=default)> <kernel> <width> <height> <scene>\n", argv[0]);
	fprintf(stderr, "Usage: %s <use CPU/GPU (0/1)> <width> <height>\n", argv[0]);

	SetUpOpenCL();

	if (argc == 1) {
		DefaultSceneSetup();
	} else if (argc == 4) {
		useGPU = atoi(argv[1]);
		width = atoi(argv[2]);
		height = atoi(argv[3]);
		DefaultSceneSetup();

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

	UpdateCamera();

	InitGlut(argc, argv, "Ray Tracing Demonstration");	
    glutMainLoop();

	FreeOpenCLBuffers();

	return 0;
}
