

#define GPU_KERNEL

#include "camera.h"
#include "vec.h"
#include "sphere.h"
#include "ray.h"

#define OCL_CONSTANT_BUFFER __constant

#define EPSILON 0.01f
#define FLOAT_PI 3.14159265358979323846f

// Vector operation, copy from vec.c ----------------------
void vinit(Vec *v, float a, float b, float c)
{
	v->x = a; v->y = b; v->z = c;
}

void vassign(Vec *v,  const  Vec *a)
{
	v->x = a->x; v->y = a->y; v->z = a->z;
}

void vclr(Vec *v)
{
	v->x = 0; v->y = 0; v->z = 0;
}

void vadd(Vec *v, const  Vec *a, const  Vec *b)
{
	v->x = a->x + b->x; v->y = a->y + b->y; v->z = a->z + b->z;
}

void vsub(Vec *v, const  Vec *a, const  Vec *b)
{
	v->x = a->x - b->x; v->y = a->y - b->y; v->z = a->z - b->z;
}

void vssadd(Vec *v, float val, const  Vec *b)
{
	v->x = b->x + val; v->y = b->y + val; v->z = b->z + val;
}

void vssub(Vec *v, float val, const  Vec *b)
{
	v->x = b->x - val; v->y = b->y - val; v->z = b->z - val;
}

void vmul(Vec *v, const  Vec *a, const  Vec *b)
{
	v->x = a->x * b->x; v->y = a->y * b->y; v->z = a->z * b->z;
}

void vsmul(Vec *v, float val, const  Vec *b)
{
	v->x = b->x * val; v->y = b->y * val; v->z = b->z * val;
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

// ----------------------

// http://en.wikipedia.org/wiki/Random_number_generator
float GetRandom(unsigned int *seed0, unsigned int *seed1) 
{
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	unsigned int ires = ((*seed0) << 16) + (*seed1);

	/* Convert to float */
	union {
		float f;
		unsigned int ui;
	} res;
	res.ui = (ires & 0x007fffff) | 0x40000000;

	return (res.f - 2.f) / 2.f;
}



static float SphereIntersect( OCL_CONSTANT_BUFFER const Sphere *s_, const Ray *r) 
{ 
	
	Sphere _s = *s_;
	Sphere *s = &_s;
		
	/* returns distance, 0 if nohit */
	Vec op; /* Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0 */
	vsub(&op, &s->p, &r->o);

	float b = vdot(&op, &r->d);
	float det = b * b - vdot(&op, &op) + s->rad * s->rad;
	if (det < 0.f)
		return 0.f;
	else
		det = sqrt(det);

	float t = b - det;
	if (t >  EPSILON)
		return t;
	else {
		t = b + det;

		if (t >  EPSILON)
			return t;
		else
			return 0.f;
	}
}

static void UniformSampleSphere(const float u1, const float u2, Vec *v) {

	//select a random point on the surface
	const float z_ = 1.f - 2.f * u1;
	const float r = sqrt(max(0.f, 1.f - z_ * z_)); //distance from the center
	const float phi = 2.f * FLOAT_PI * u2;
	const float x_ = r * cos(phi);
	const float y_ = r * sin(phi);

	vinit(v, x_, y_, z_);
}

static int Intersect( OCL_CONSTANT_BUFFER const Sphere *spheres, const unsigned int sphereCount,
	const Ray *r,
	float *t,
	unsigned int *id) 
{
	float inf = (*t) = 1e20f;

	unsigned int i = 0;
	for (i = 0; i < sphereCount; ++i) {
		const float d = SphereIntersect(&spheres[i], r);
		if ((d != 0.f) && (d < *t)) {
			*t = d;
			*id = i;
		}
	}

	return (*t < inf);
}

static int IntersectP( OCL_CONSTANT_BUFFER const Sphere *spheres, const unsigned int sphereCount,
	const Ray *r,
	const float max_t) 
{
	
	unsigned int i = 0;
	for (i = 0; i < sphereCount; ++i) {
		const float d = SphereIntersect(&spheres[i], r);
		if ((d != 0.f) && (d < max_t))
			return 1;
	}

	return 0;
}

static void SampleLights( OCL_CONSTANT_BUFFER const Sphere *spheres,
	const unsigned int sphereCount, unsigned int *seed0, unsigned int *seed1,
	const Vec *hitPoint,
	const Vec *normal,
	Vec *result) 
{

	vclr(result);

	/* For each light */
	unsigned int i;
	for (i = 0; i < sphereCount; i++) {
		OCL_CONSTANT_BUFFER const Sphere *light_ = &spheres[i];

		Sphere _l = *light_;
		const Sphere *light = &_l;

		if (!viszero(&light->e)) {
			/* It is a light source */
			Ray shadowRay;
			shadowRay.o = *hitPoint;

			/* Choose a point over the light source */
			//this is a simple, non-optimized version to find the point on the light source
			//it may create too much noises
			Vec unitSpherePoint;
			UniformSampleSphere(GetRandom(seed0, seed1), GetRandom(seed0, seed1), &unitSpherePoint);
			Vec spherePoint;
			vsmul(&spherePoint, light->rad, &unitSpherePoint);
			vadd(&spherePoint, &spherePoint, &light->p);

			/* Build the shadow ray direction */
			vsub(&shadowRay.d, &spherePoint, hitPoint);
			const float len = sqrt(vdot(&shadowRay.d, &shadowRay.d));
			vsmul(&shadowRay.d, 1.f / len, &shadowRay.d);

			float wo = vdot(&shadowRay.d, &unitSpherePoint);
			if (wo > 0.f) {	

				/* It is on the other half of the sphere */
				continue;
			} else
				wo = -wo;

			/* Check if the light is visible */
			const float wi = vdot(&shadowRay.d, normal);
			if ((wi > 0.f) && (!IntersectP(spheres, sphereCount, &shadowRay, len - EPSILON))) {	// if wi <= 0, the hitpoint is on the back of the sphere
				Vec c; vassign(&c, &light->e);
				const float s = (4.f * FLOAT_PI * light->rad * light->rad) * wi * wo / (len *len);
				vsmul(&c, s, &c);
				vadd(result, result, &c);
			}
		}
	}
}

static void RadiancePathTracing( OCL_CONSTANT_BUFFER const Sphere *spheres, const unsigned int sphereCount,
	const Ray *startRay, unsigned int *seed0, unsigned int *seed1, Vec *result) 
{
	
	Ray currentRay; 
	rassign(currentRay, *startRay);
	Vec rad; 
	vinit(&rad, 0.f, 0.f, 0.f);
	Vec throughput; 
	vinit(&throughput, 1.f, 1.f, 1.f);

	unsigned int depth = 0;
	int specularBounce = 1;
	for (;; ++depth) {
		
		if (depth > 7) {
			*result = rad;
			return;
		}

		float t = 0; /* distance to intersection */
		unsigned int id = 0; /* id of intersected object */
		if (!Intersect(spheres, sphereCount, &currentRay, &t, &id)) {
			*result = rad; /* if miss, return */
			return;
		}


		OCL_CONSTANT_BUFFER const Sphere *obj_ = &spheres[id]; /* the hit object */

		Sphere _obj = *obj_;
		const Sphere *obj = &_obj;

		Vec hitPoint;
		vsmul(&hitPoint, t, &currentRay.d);
		vadd(&hitPoint, &currentRay.o, &hitPoint);

		Vec normal;
		vsub(&normal, &hitPoint, &obj->p);
		vnorm(&normal);

		const float dp = vdot(&normal, &currentRay.d);

		// Orienting Normal Vector
		Vec nl;

		const float invSignDP = -1.f * sign(dp);
		vsmul(&nl, invSignDP, &normal);

		/* Add emitted light */
		Vec eCol; vassign(&eCol, &obj->e);
		if (!viszero(&eCol)) {
			if (specularBounce) {
				vsmul(&eCol, fabs(dp), &eCol);
				vmul(&eCol, &throughput, &eCol);
				vadd(&rad, &rad, &eCol);
			}

			*result = rad;
			return;
		}

		if (obj->refl == DIFF) { /* Ideal DIFFUSE reflection */
			specularBounce = 0;
			vmul(&throughput, &throughput, &obj->c);

			/* Direct lighting component */
			Vec Ld;
			SampleLights(spheres, sphereCount, seed0, seed1, &hitPoint, &nl, &Ld);
			vmul(&Ld, &throughput, &Ld);
			vadd(&rad, &rad, &Ld);

			/* Diffuse component */
			float r1 = 2.f * FLOAT_PI * GetRandom(seed0, seed1);	// Random angle
			float r2 = GetRandom(seed0, seed1);
			float r2s = sqrt(r2); // Random distance from center

			// Create coordinate system for sphere: w u v
			Vec w; 
			vassign(&w, &nl);

			Vec u, a;
			if (fabs(w.x) > .1f) {
				vinit(&a, 0.f, 1.f, 0.f);
			} else {
				vinit(&a, 1.f, 0.f, 0.f);
			}
			vxcross(&u, &a, &w);
			vnorm(&u);

			Vec v;
			vxcross(&v, &w, &u);

			Vec newDir;
			vsmul(&u, cos(r1) * r2s, &u);
			vsmul(&v, sin(r1) * r2s, &v);
			vadd(&newDir, &u, &v);
			vsmul(&w, sqrt(1 - r2), &w);
			vadd(&newDir, &newDir, &w);

			currentRay.o = hitPoint;
			currentRay.d = newDir;
			continue;
		} else if (obj->refl == SPEC) { /* Ideal SPECULAR reflection */
			specularBounce = 1;

			// R = D - 2(Nornal dot D)Normal
			Vec newDir;
			vsmul(&newDir,  2.f * vdot(&normal, &currentRay.d), &normal);
			vsub(&newDir, &currentRay.d, &newDir);

			vmul(&throughput, &throughput, &obj->c);

			rinit(currentRay, hitPoint, newDir);
			continue;
		} else {		//REFRACTION, (Standard Glass)
			specularBounce = 1;

			// The reflection part
			Vec newDir;
			vsmul(&newDir,  2.f * vdot(&normal, &currentRay.d), &normal);
			vsub(&newDir, &currentRay.d, &newDir);

			Ray reflRay; 
			rinit(reflRay, hitPoint, newDir); /* Ideal dielectric REFRACTION */
			int into = (vdot(&normal, &nl) > 0); /* Ray from outside going in? */

			const float nc = 1.f;		// Index of Refraction (IOR) of vacuum
			const float nt = 1.52f;	//Index of Refraction (IOR) for glass is 1.52 
			float nnt = into ? nc / nt : nt / nc;
			float ddn = vdot(&currentRay.d, &nl);
			
			// Note: currentRay.d and nl are unit vectors
			// nc * Sin(c) = nt * Sin(t)
			// Cos^2(t) = 1 - Sin^2(t) = 1 - (nnt)^2 * (1 - Cos^2(c))
			float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

			if (cos2t < 0.f)  { /* Total internal reflection */
				vmul(&throughput, &throughput, &obj->c);

				rassign(currentRay, reflRay); // if the angle is too shallow, all the light is reflected
				continue;
			}

			// refraction ray equation
			float kk = (into ? 1 : -1) * (ddn * nnt + sqrt(cos2t));
			Vec nkk;
			vsmul(&nkk, kk, &normal);
			Vec transDir;
			vsmul(&transDir, nnt, &currentRay.d);
			vsub(&transDir, &transDir, &nkk);
			vnorm(&transDir);

			// Fresnel Reflectance
			float a = nt - nc;
			float b = nt + nc;
			float R0 = a * a / (b * b);
			float c = 1 - (into ? -ddn : vdot(&transDir, &normal));	// 1 - cos

			float Re = R0 + (1 - R0) * c * c * c * c * c; // Reflectance
			float Tr = 1.f - Re;
			float P = 0.25f + 0.5f * Re;		// probability of reflecting

			float RP = Re / P;
			float TP = Tr / (1.f - P);

			if (GetRandom(seed0, seed1) < P) { // Choose one !
				vsmul(&throughput, RP, &throughput);
				vmul(&throughput, &throughput, &obj->c);

				rassign(currentRay, reflRay);	//reflect
				continue;
			} else {
				vsmul(&throughput, TP, &throughput);
				vmul(&throughput, &throughput, &obj->c);

				rinit(currentRay, hitPoint, transDir); // refract
				continue;
			}
		}
	}
}


static void GenerateCameraRay(OCL_CONSTANT_BUFFER Camera *camera_,
		unsigned int *seed0, unsigned int *seed1,
		const int width, const int height, const int x, const int y, Ray *ray) 
{
	const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	const float r1 = GetRandom(seed0, seed1) - 0.5f;
	const float r2 = GetRandom(seed0, seed1) - 0.5f;
	const float kcx = (x + r1) * invWidth - 0.5f;
	const float kcy = (y + r2) * invHeight - 0.5f;

	/*local variables*/
	Camera _camera = *camera_;
	const Camera *camera = &_camera;

	Vec rdir;
	vinit(&rdir,
			camera->x.x * kcx + camera->y.x * kcy + camera->dir.x,
			camera->x.y * kcx + camera->y.y * kcy + camera->dir.y,
			camera->x.z * kcx + camera->y.z * kcy + camera->dir.z);

	Vec rorig;
	vsmul(&rorig, 0.1f, &rdir);
	vadd(&rorig, &rorig, &camera->orig);

	vnorm(&rdir);
	rinit(*ray, rorig, rdir);
}

__kernel void RayTracing(
    __global Vec *colors, __global unsigned int *seedsInput,
	OCL_CONSTANT_BUFFER Sphere *sphere, OCL_CONSTANT_BUFFER Camera *camera,
	const unsigned int sphereCount,
	const int width, const int height,
	const int currentSample,
	__global int *pixels) 
{
	
    const int gid = get_global_id(0);
	const int gid2 = 2 * gid;
	const int x = gid % width;
	const int y = gid / width;

	/* Check if we have to do something */
	if (y >= height)
		return;

	/* move seed to local store */
	unsigned int seed0 = seedsInput[gid2];
	unsigned int seed1 = seedsInput[gid2 + 1];

	Ray ray;
	GenerateCameraRay(camera, &seed0, &seed1, width, height, x, y, &ray);

	Vec r;
	RadiancePathTracing(sphere, sphereCount, &ray, &seed0, &seed1, &r);

	const int i = (height - y - 1) * width + x;
	if (currentSample == 0) {

		colors[i] = r;
	} else {
		const float k1 = currentSample;
		const float k2 = 1.f / (currentSample + 1.f);
		colors[i].x = (colors[i].x * k1  + r.x) * k2;
		colors[i].y = (colors[i].y * k1  + r.y) * k2;
		colors[i].z = (colors[i].z * k1  + r.z) * k2;
	}

	// result
	pixels[y * width + x] = toInt(colors[i].x) |
			(toInt(colors[i].y) << 8) |
			(toInt(colors[i].z) << 16);

	seedsInput[gid2] = seed0;
	seedsInput[gid2 + 1] = seed1;
}
