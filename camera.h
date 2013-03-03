
#ifndef _CAMERA_H_
#define	_CAMERA_H_

#include "vec.h"

typedef struct {

	/* User defined values */
	Vec orig, target;

	/* Calculated values */
	Vec dir, x, y;
} Camera;

#endif

