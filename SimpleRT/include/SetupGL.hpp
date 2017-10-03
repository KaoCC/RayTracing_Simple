
#ifndef _SETUPGL_H_
#define	_SETUPGL_H_


#include <GL/glut.h>

extern int width;
extern int height;
extern unsigned int *pixels;
extern unsigned int renderingFlags;
extern char captionBuffer[256];

void InitGlut(int argc, char *argv[], char *windowTittle);

void ReadScene(char *);
void UpdateCamera();

#endif

