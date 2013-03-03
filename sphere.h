
#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "vec.h"

enum Refl {
	DIFF, SPEC, REFR
}; /* material types, used in radiance() */

typedef struct {
	float rad;			/* radius */
	Vec p, e, c;		/* position, emission, color */
	enum Refl refl;		/* reflection type (DIFFuse, SPECular, REFRactive) */
} Sphere;

#endif

