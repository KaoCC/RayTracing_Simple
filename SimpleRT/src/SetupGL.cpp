#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Camera.hpp"
#include "Sphere.hpp"
#include "SetupGL.hpp"

extern void ReInit(const int);
extern void ReInitScene();
extern void UpdateRendering();
extern void UpdateCamera();

//extern Camera camera;
extern Camera* cameraPtr;
extern Sphere *spheres;
extern Sphere* spheres_host_ptr;
extern unsigned int sphereCount;


int width = 800;
int height = 600;
unsigned int *pixels;
char captionBuffer[256];

static int currentSphere;


static void PrintString(void *font, const char *str) {

	size_t len = (int)strlen(str);
	for (size_t i = 0; i < len; ++i)
		glutBitmapCharacter(font, str[i]);
}

void ReadScene(char *fileName) {
	fprintf(stderr, "Reading scene: %s\n", fileName);

	FILE *f = fopen(fileName, "r");
	if (!f) {
		fprintf(stderr, "Failed to open file: %s\n", fileName);
		exit(-1);
	}

	/* Read the camera position */
	int c = fscanf(f,"camera %f %f %f  %f %f %f\n",
			&cameraPtr->orig.x, &cameraPtr->orig.y, &cameraPtr->orig.z,
			&cameraPtr->target.x, &cameraPtr->target.y, &cameraPtr->target.z);
	if (c != 6) {
		fprintf(stderr, "Failed to read 6 camera parameters: %d\n", c);
		exit(-1);
	}

	/* Read the sphere count */
	c = fscanf(f,"size %u\n", &sphereCount);
	if (c != 1) {
		fprintf(stderr, "Failed to read sphere count: %d\n", c);
		exit(-1);
	}
	fprintf(stderr, "Scene size: %d\n", sphereCount);

	/* Read all spheres */

	//spheres = (Sphere *)clSVMAlloc(context, CL_MEM_SVM_FINE_GRAIN_BUFFER,sizeof(Sphere) * sphereCount, 0);
	spheres_host_ptr = static_cast<Sphere*>(malloc(sizeof(Sphere) * sphereCount));

	// MAY HAVE BUG HERE !
#pragma message ( "MAY HAVE BUG HERE ! (ReadScene)" )

	for (unsigned int i = 0; i < sphereCount; i++) {
		Sphere *s = &spheres_host_ptr[i];
		int mat;
		int c = fscanf(f,"sphere %f  %f %f %f  %f %f %f  %f %f %f  %d\n",
				&s->rad,
				&s->p.x, &s->p.y, &s->p.z,
				&s->e.x, &s->e.y, &s->e.z,
				&s->c.x, &s->c.y, &s->c.z,
				&mat);
		switch (mat) {
			case 0:
				s->refl = Refl::DIFF;
				break;
			case 1:
				s->refl = Refl::SPEC;
				break;
			case 2:
				s->refl = Refl::REFR;
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
	}

	fclose(f);
}

void UpdateCamera() {

	//vsub(&cameraPtr->dir, &cameraPtr->target, &cameraPtr->orig);
	cameraPtr->dir = cameraPtr->target - cameraPtr->orig;

	//vnorm(&cameraPtr->dir);
	cameraPtr->dir.norm();

	const Vec up {0.f, 1.f, 0.f};
	const float fov = static_cast<float>((M_PI / 180.f) * 45.f);
	//vxcross(&cameraPtr->x, &cameraPtr->dir, &up);
	cameraPtr->x = cameraPtr->dir.cross(up);

	//vnorm(&cameraPtr->x);
	cameraPtr->x.norm();

	//vsmul(&cameraPtr->x, width * fov / height, &cameraPtr->x);
	cameraPtr->x = cameraPtr->x * (width * fov / height);

	//vxcross(&cameraPtr->y, &cameraPtr->x, &cameraPtr->dir);
	cameraPtr->y = cameraPtr->x.cross(cameraPtr->dir);

	//vnorm(&cameraPtr->y);
	cameraPtr->y.norm();

	//vsmul(&cameraPtr->y, fov, &cameraPtr->y);
	cameraPtr->y = cameraPtr->y * fov;
}

void idleFunc(void) {
	UpdateRendering();

	glutPostRedisplay();
}

void displayFunc(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glRasterPos2i(0, 0);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Title
	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(4, height - 16);
	PrintString(GLUT_BITMAP_HELVETICA_18, "Ray Tracing Experiment");

	// Caption line 0
	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(4, 10);
	PrintString(GLUT_BITMAP_HELVETICA_18, captionBuffer);


	glutSwapBuffers();
}



void InitGlut(int argc, char *argv[], char *windowTittle) {
    glutInitWindowSize(width, height);
    glutInitWindowPosition(0,0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInit(&argc, argv);

	glutCreateWindow(windowTittle);

    glutDisplayFunc(displayFunc);
	glutIdleFunc(idleFunc);

	glViewport(0, 0, width, height);
	glLoadIdentity();
	glOrtho(0.f, width - 1.f, 0.f, height - 1.f, -1.f, 1.f);
}
