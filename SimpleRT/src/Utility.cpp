#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include "Utility.hpp"

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

	 

//
//void ReadScene(char *fileName) {
//	fprintf(stderr, "Reading scene: %s\n", fileName);
//
//	FILE *f = fopen(fileName, "r");
//	if (!f) {
//		fprintf(stderr, "Failed to open file: %s\n", fileName);
//		exit(-1);
//	}
//
//	/* Read the camera position */
//	int c = fscanf(f, "camera %f %f %f  %f %f %f\n",
//		&cameraPtr->orig.x, &cameraPtr->orig.y, &cameraPtr->orig.z,
//		&cameraPtr->target.x, &cameraPtr->target.y, &cameraPtr->target.z);
//	if (c != 6) {
//		fprintf(stderr, "Failed to read 6 camera parameters: %d\n", c);
//		exit(-1);
//	}
//
//	/* Read the sphere count */
//	c = fscanf(f, "size %u\n", &sphereCount);
//	if (c != 1) {
//		fprintf(stderr, "Failed to read sphere count: %d\n", c);
//		exit(-1);
//	}
//	fprintf(stderr, "Scene size: %d\n", sphereCount);
//
//	/* Read all spheres */
//
//	//spheres = (Sphere *)clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER,sizeof(Sphere) * sphereCount, 0);
//	spheres_host_ptr = static_cast<Sphere*>(malloc(sizeof(Sphere) * sphereCount));
//
//	// MAY HAVE BUG HERE !
//#pragma message ( "MAY HAVE BUG HERE ! (ReadScene)" )
//
//	for (unsigned int i = 0; i < sphereCount; i++) {
//		Sphere *s = &spheres_host_ptr[i];
//		int mat;
//		int c = fscanf(f, "sphere %f  %f %f %f  %f %f %f  %f %f %f  %d\n",
//			&s->rad,
//			&s->p.x, &s->p.y, &s->p.z,
//			&s->e.x, &s->e.y, &s->e.z,
//			&s->c.x, &s->c.y, &s->c.z,
//			&mat);
//		switch (mat) {
//		case 0:
//			s->refl = Refl::DIFF;
//			break;
//		case 1:
//			s->refl = Refl::SPEC;
//			break;
//		case 2:
//			s->refl = Refl::REFR;
//			break;
//		default:
//			fprintf(stderr, "Failed to read material type for sphere #%d: %d\n", i, mat);
//			exit(-1);
//			break;
//		}
//		if (c != 11) {
//			fprintf(stderr, "Failed to read sphere #%d: %d\n", i, c);
//			exit(-1);
//		}
//	}
//
//	fclose(f);
//}
