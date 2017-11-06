#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include "Utility.hpp"
#include "Sphere.hpp"

//#define NULL 0

double WallClockTime() 
{
#if defined(__linux__) || defined(__APPLE__)

	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (_WIN32)
	return GetTickCount() / 1000.0;
#else
	Unsupported Platform
#endif

}

std::vector<char> ReadKernelSourcesFile(const std::string& fileName) { 
	
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
	 //  fprintf(stderr, "Failed to allocate memory for file '%s'\n", fileName.c_str()); 
	 //  exit(-1); 
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


void computeCameraVariables(Camera* cameraPtr, int width, int height) {

	cameraPtr->dir = cameraPtr->target - cameraPtr->orig;
	cameraPtr->dir.norm();

	const Vec up{ 0.f, 1.f, 0.f };
	const float fov = static_cast<float>((M_PI / 180.f) * 45.f);

	cameraPtr->x = cameraPtr->dir.cross(up);
	cameraPtr->x.norm();
	cameraPtr->x = cameraPtr->x * (width * fov / height);
	cameraPtr->y = cameraPtr->x.cross(cameraPtr->dir);
	cameraPtr->y.norm();
	cameraPtr->y = cameraPtr->y * fov;
}

	 


std::vector<Sphere> readScene(const std::string& fileName, Vec& orig, Vec& target) {

	fprintf(stderr, "Reading scene: %s\n", fileName.c_str());

	FILE *f = std::fopen(fileName.c_str(), "r");
	if (!f) {
		fprintf(stderr, "Failed to open file: %s\n", fileName.c_str());
		exit(-1);
	}

	/* Read the camera position */
	int c = std::fscanf(f, "camera %f %f %f  %f %f %f\n",
		&orig.x, &orig.y, &orig.z,
		&target.x, &target.y, &target.z);
	if (c != 6) {
		fprintf(stderr, "Failed to read 6 camera parameters: %d\n", c);
		exit(-1);
	}

	std::size_t sphereCount = 0;

	/* Read the sphere count */
	c = std::fscanf(f, "size %u\n", &sphereCount);
	if (c != 1) {
		fprintf(stderr, "Failed to read sphere count: %d\n", c);
		exit(-1);
	}
	fprintf(stderr, "Scene size: %d\n", sphereCount);

	/* Read all spheres */
	std::vector<Sphere> spheres(sphereCount);

	for (unsigned i = 0; i < sphereCount; ++i) {

		Sphere s;
		int mat = 0;
		int c = std::fscanf(f, "sphere %f  %f %f %f  %f %f %f  %f %f %f  %d\n",
			&s.rad,
			&s.p.x, &s.p.y, &s.p.z,
			&s.e.x, &s.e.y, &s.e.z,
			&s.c.x, &s.c.y, &s.c.z,
			&mat);

		switch (mat) {
		case 0:
			s.refl = Refl::DIFF;
			break;
		case 1:
			s.refl = Refl::SPEC;
			break;
		case 2:
			s.refl = Refl::REFR;
			break;
		default:
			fprintf(stderr, "Failed to read material type for sphere #%d: %d\n", i, mat);
			exit(-1);
			break;
		}

		if (c != 11) {
			fprintf(stderr, "Failed to read sphere #%d: %d\n", i, c);
			exit(-1);
		}

		spheres.push_back(s);	
	}

	std::fclose(f);

	return spheres;
}

