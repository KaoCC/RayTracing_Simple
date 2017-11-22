

#include <cstdio>
#include <cstdlib>
#include <ctime>


#include "SetupGL.hpp"
#include "OpenCLConfig.hpp"

#include "Config.hpp"



int main(int argc, char *argv[]) {

	bool useGPU = true;
	bool useSVM = false;


	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, "Usage: %s <framework ID> <use CPU/GPU (0/1)> <use SVM (0/1)> <scene> <workgroup size (power of 2, 0=default)> <kernel> <width> <height>\n", argv[0]);
	fprintf(stderr, "Usage: %s <framework ID> <use CPU/GPU (0/1)> <use SVM (0/1)> <scene>\n", argv[0]);


	std::unique_ptr<Config> frameworkConfig = [&]() {
		if (argc == 1) {
			// the default one ... however it is for tmp use only..
			return createConfig(glWidth, glHeight, SupportType::Cm, useGPU, useSVM);
		} else if (argc >= 4) {

			useGPU = (std::atoi(argv[2]) == 2) ? true : false;
			useSVM = (std::atoi(argv[3]) == 3) ? true : false;

			return createConfig(glWidth, glHeight, selectType(std::atoi(argv[1])), useGPU, useSVM);


		} else {
			exit(EXIT_FAILURE);
		}
	}();


	
	Vec orig;
	Vec target;

	// be careful, the current design requires the spheres to exist until termination. !!!
	// This may changed later.
	std::vector<Sphere> spheres = [&]() {
		if (argc >= 5) {

			return readScene(argv[4], orig, target);

		} else {

			// the default scene
			orig = { 20.f, 100.f, 120.f };
			target = { 0.f, 25.f, 0.f };
			return DemoSpheres;
		}
	}();

	frameworkConfig->sceneSetup(spheres, orig, target);		// test
	frameworkConfig->updateCamera();


	// OpenGL setup
	InitGlut(argc, argv, "Ray Tracing Demonstration", *frameworkConfig);
    glutMainLoop();


	return 0;
}
