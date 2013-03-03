

#ifndef _RAY_H_
#define	_RAY_H_

#include "vec.h"

typedef struct {
	Vec o, d;
} Ray;

#define rinit(r, a, b) { vassign(&((r).o), &a); vassign(&((r).d), &b); }
#define rassign(a, b) { vassign(&((a).o), &((b).o)); vassign(&((a).d), &((b).d)); }


#endif

