
#ifndef _CAMERA_H_
#define	_CAMERA_H_

#include "Vec.hpp"

struct Camera {

	/* User defined values */
	Vec orig, target;

	/* Calculated values */
	Vec dir, x, y;
};

#endif

