
#ifndef _SCENE_H_
#define	_SCENE_H_

#include "Sphere.hpp"

Sphere DemoSpheres[] = {
	{1000, {0, -1000, 0}, {0, 0, 0}, {0.75f, 0.75f, 0.75f}, Refl::DIFF},
	{12, {40, 20, 0}, {0, 0, 0}, {0.9f, 0.f, 0.f}, Refl::REFR},
	{11, {-35, 20, 0}, {0, 0, 0}, {0.f, 0.9f, 0.f}, Refl::REFR},
	{10, {0, 25, -10}, {0, 0, 0}, {0.f, 0.f, 0.9f}, Refl::REFR},
	{9, {20, 10, -5}, {0, 0, 0}, {0.9f, 0.f, 0.9f}, Refl::REFR},
	{7, {0, 60, 0}, {12, 12, 12}, {0.f, 0.f, 0.f}, Refl::DIFF}
};


#endif

