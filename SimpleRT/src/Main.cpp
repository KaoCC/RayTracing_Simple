

#include <cstdio>
#include <cstdlib>
#include <ctime>


#include "SetupGL.hpp"
#include "OpenCLConfig.hpp"

#include "Config.hpp"



int main(int argc, char *argv[]) {

	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, "Usage: %s <use CPU/GPU (0/1)> <workgroup size (power of 2, 0=default)> <kernel> <width> <height> <scene>\n", argv[0]);
	fprintf(stderr, "Usage: %s <use CPU/GPU (0/1)> <width> <height>\n", argv[0]);


	std::unique_ptr<Config> frameworkConfig = createConfig(glWidth, glHeight, SupportType::OpenCL);

	//SetUpOpenCL();

	//if (argc == 1) {
	//	DefaultSceneSetup();
	//} else if (argc == 4) {
	//	useGPU = atoi(argv[1]);
	//	width = atoi(argv[2]);
	//	height = atoi(argv[3]);
	//	DefaultSceneSetup();

	//} else if (argc == 7) {
	//	useGPU = atoi(argv[1]);
	//	forceWorkSize = atoi(argv[2]);
	//	kernelFileName = argv[3];
	//	width = atoi(argv[4]);
	//	height = atoi(argv[5]);
	//	ReadScene(argv[6]);
	//} else {
	//	exit(EXIT_FAILURE);
	//}

//	UpdateCamera();


	frameworkConfig->sceneSetup(DemoSpheres, { 20.f, 100.f, 120.f }, { 0.f, 25.f, 0.f });		// test
	frameworkConfig->updateCamera();


	// OpenGL setup
	InitGlut(argc, argv, "Ray Tracing Demonstration", *frameworkConfig);
    glutMainLoop();

	//FreeOpenCLBuffers();

	return 0;
}
