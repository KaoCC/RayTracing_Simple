
#include <math.h>

#include "vec.h"



void vinit(Vec *v, float a, float b, float c)
{
	v->x = a;
	v->y = b;
	v->z = c;
}


void vassign(Vec *v,  const  Vec *a)
{
	v->x = a->x;
	v->y = a->y;
	v->z = a->z;
}


void vclr(Vec *v)
{
	v->x = 0;
	v->y = 0;
	v->z = 0;
}

void vadd(Vec *v, const  Vec *a, const  Vec *b)
{
	v->x = a->x + b->x;
	v->y = a->y + b->y;
	v->z = a->z + b->z;
}

void vsub(Vec *v, const  Vec *a, const  Vec *b)
{
	v->x = a->x - b->x;
	v->y = a->y - b->y;
	v->z = a->z - b->z;
}

void vssadd(Vec *v, float val, const  Vec *b)
{
	v->x = b->x + val;
	v->y = b->y + val;
	v->z = b->z + val;
}

void vssub(Vec *v, float val, const  Vec *b)
{
	v->x = b->x - val;
	v->y = b->y - val;
	v->z = b->z - val;
}

void vmul(Vec *v, const  Vec *a, const  Vec *b)
{
	v->x = a->x * b->x;
	v->y = a->y * b->y;
	v->z = a->z * b->z;
}

void vsmul(Vec *v, float val, const  Vec *b)
{
	v->x = b->x * val;
	v->y = b->y * val;
	v->z = b->z * val;
}

float vdot(const Vec *a, const  Vec *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

void vnorm(Vec *v)
{
	float f = 1.f / sqrt(vdot(v, v)); 
	vsmul(v, f, v); 
}

void vxcross(Vec *v,  const  Vec *a, const  Vec *b)
{
	vinit(v, a->y * b->z - a->z * b->y, a->z * b->x - a->x * b->z, a->x * b->y - a->y * b->x);
}

//extern void vfilter();

int viszero(const  Vec *v)
{
	return ((v->x == 0.f) && (v->x == 0.f) && (v->z == 0.f));
}

