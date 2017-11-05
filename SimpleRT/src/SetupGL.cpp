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

#include "Config.hpp"

//extern void ReInit(const int);
//extern void ReInitScene();


//extern void UpdateRendering();


//extern Camera camera;
//extern Camera* cameraPtr;
//extern Sphere *spheres;
//extern Sphere* spheres_host_ptr;
//extern unsigned int sphereCount;


static Config* pConfig;

int glWidth = 800;
int glHeight = 600;


static unsigned int *pixels;
static char captionBuffer[256];

static int currentSphere;


static void PrintString(void *font, const char *str) {

	size_t len = (int)strlen(str);
	for (size_t i = 0; i < len; ++i)
		glutBitmapCharacter(font, str[i]);
}




void idleFunc(void) {

	pConfig->updateRendering();

	glutPostRedisplay();
}

void displayFunc(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glRasterPos2i(0, 0);
	glDrawPixels(glWidth, glHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Title
	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(4, glHeight - 16);
	PrintString(GLUT_BITMAP_HELVETICA_18, "Ray Tracing Experiment");

	// Caption line 0
	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(4, 10);
	PrintString(GLUT_BITMAP_HELVETICA_18, captionBuffer);


	glutSwapBuffers();
}



void InitGlut(int argc, char *argv[], char *windowTittle, Config& config) {


	pConfig = &config;
	config.setCaptionBuffer(captionBuffer);
	pixels = config.getPixels();

    glutInitWindowSize(glWidth, glHeight);
    glutInitWindowPosition(0,0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInit(&argc, argv);

	glutCreateWindow(windowTittle);

    glutDisplayFunc(displayFunc);
	glutIdleFunc(idleFunc);

	glViewport(0, 0, glWidth, glHeight);
	glLoadIdentity();
	glOrtho(0.f, glWidth - 1.f, 0.f, glHeight - 1.f, -1.f, 1.f);
}

