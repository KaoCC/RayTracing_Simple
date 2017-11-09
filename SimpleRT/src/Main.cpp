

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "Camera.hpp"

#include "SetupGL.hpp"


#include "Config.hpp"
//#include "CmSVMAllocator.hpp"

#include "Scene.hpp"
#include "Utility.hpp"

int main(int argc, char *argv[]) {

	bool useGPU = true;
	bool useSVM = false;


	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, "Usage: %s <framework ID> <use CPU/GPU (0/1)> <use SVM (0/1)> <scene> <workgroup size (power of 2, 0=default)> <kernel> <width> <height>\n", argv[0]);
	fprintf(stderr, "Usage: %s <framework ID> <use CPU/GPU (0/1)> <use SVM (0/1)> <scene>\n", argv[0]);


	std::unique_ptr<Config> frameworkConfig = [&]() {
		if (argc == 1) {
			// the default one ... however it is for tmp use only..
			return createConfig(glWidth, glHeight, selectType(0), useGPU, useSVM);
		} else if (argc >= 4) {

			useGPU = (std::atoi(argv[2]) == 1) ? true : false;
			useSVM = (std::atoi(argv[3]) == 1) ? true : false;

			return createConfig(glWidth, glHeight, selectType(std::atoi(argv[1])), useGPU, useSVM);


		} else {
			exit(EXIT_FAILURE);
		}
	}();


	
	Vec orig;
	Vec target;
	std::vector<Sphere> spheres; 

	// be careful, the current design requires the spheres to exist until termination. !!!
	// This may changed later.

	if (argc >= 5) {

		spheres = readScene(argv[4], orig, target);

	} else {
		// the default scene
		spheres = DemoSpheres;
		orig = { 20.f, 100.f, 120.f };
		target = { 0.f, 25.f, 0.f };
	}

	frameworkConfig->sceneSetup(spheres, orig, target);		// test
	frameworkConfig->updateCamera();


	// OpenGL setup
	InitGlut(argc, argv, "Ray Tracing Demonstration", *frameworkConfig);
    glutMainLoop();


	return 0;
}
