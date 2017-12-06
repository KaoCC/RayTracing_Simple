

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
	//bool useSVM = false;


	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, "Usage: %s <framework ID> <CPU/GPU (0/1)> <mem [Buffer/SVM/UP] (0/1/2)> <scene> <workgroup size (power of 2, 0=default)> <kernel> <width> <height>\n", argv[0]);
	fprintf(stderr, "Usage: %s <framework ID> <CPU/GPU (0/1)> <mem [Buffer/SVM/UP] (0/1/2)> <scene>\n", argv[0]);


	std::unique_ptr<Config> frameworkConfig = [&]() {
		if (argc == 1) {
			// the default one ... however it is for tmp use only..
			return createConfig(glWidth, glHeight, selectType(0), useGPU, MemType::Buffer);
		} else if (argc >= 4) {

			useGPU = (std::atoi(argv[2]) == 1) ? true : false;
			//useSVM = (std::atoi(argv[3]) == 1) ? true : false;

			auto memType = [&]() {
				switch (std::atoi(argv[3])) {
				case 0:
					return MemType::Buffer;
					break;
				case 1:
					return MemType::SVM;
					break;
				case 2:
					return MemType::UserProvidedZeroCopy;
					break;
				default:
					throw std::runtime_error("Unsupported Memory Type");
					break;
				}
			}();

			return createConfig(glWidth, glHeight, selectType(std::atoi(argv[1])), useGPU, memType);


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
