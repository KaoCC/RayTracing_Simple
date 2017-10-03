
#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "Vec.hpp"

enum class Refl {
	DIFF, SPEC, REFR
}; /* material types, used in radiance() */

struct Sphere {
	float rad;			/* radius */
	Vec p, e, c;		/* position, emission, color */
	Refl refl;		/* reflection type (DIFFuse, SPECular, REFRactive) */
};

#endif

