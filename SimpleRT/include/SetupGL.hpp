
#ifndef _SETUPGL_H_
#define	_SETUPGL_H_

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "Config.hpp"

extern int glWidth;
extern int glHeight;
extern unsigned int renderingFlags;


void InitGlut(int argc, char *argv[], char *windowTittle, Config& config);

//void ReadScene(char *);
//void UpdateCamera();






#endif

