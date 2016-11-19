
#ifndef _VEC_H_
#define	_VEC_H_

typedef struct {
	float x, y, z; // for position and color (r,g,b)
} Vec;


extern void vinit(Vec *v, float a, float b, float c);
extern void vassign(Vec *v, const  Vec *a);
extern void vclr(Vec *v);
extern void vadd(Vec *v, const  Vec *a, const Vec *b);
extern void vsub(Vec *v, const  Vec *a, const Vec *b);
extern void vssadd(Vec *v, float val, const  Vec *b);
extern void vssub(Vec *v, float val, const  Vec *b);
extern void vmul(Vec *v, const Vec  *a, const  Vec *b);
extern void vsmul(Vec *v, float val, const  Vec *b);
extern float vdot(const  Vec *a, const  Vec *b);
extern void vnorm(Vec *v);
extern void vxcross(Vec *v,  const  Vec *a, const  Vec *b);
//extern void vfilter();
extern int viszero(const  Vec *v);


#ifndef GPU_KERNEL
#define clamp(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define max(x, y) ( (x) > (y) ? (x) : (y))
#define min(x, y) ( (x) < (y) ? (x) : (y))
#define sign(x) ((x) > 0 ? 1 : -1)
#endif

#define toInt(x) ((int)(pow(clamp(x, 0.f, 1.f), 1.f / 2.2f) * 255.f + .5f))


#endif	/* _VEC_H */

