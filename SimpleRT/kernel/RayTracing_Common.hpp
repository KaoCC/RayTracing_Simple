
#include <cm/cm.h>

#define FLOAT_PI 3.14159265358979323846f

#define sign(x) ((x) > 0 ? 1 : -1)

/* material types */
enum Refl {
	DIFF, SPEC, REFR
};

// simple impl.
float clamp(float x, float low, float high) {
	if (x < low) {
		return low;
	} else if (x > high) {
		return high;
	} else {
		return x;
	}
}

const float kEpsilon = 0.01f;

const unsigned kWidth = 800;
const unsigned kHeight = 600;

constexpr const unsigned threadCountX = 100;
constexpr const unsigned threadCountY = 100;


constexpr const unsigned kSphereClassFloatcount =  12;       // padding
constexpr const unsigned kCameraClassFloatcount =  15 + 1;       // change to 64 byte for alignment


constexpr const unsigned cameraVecSize = kCameraClassFloatcount;
constexpr const unsigned rayVecSize = 6;

constexpr const unsigned kColorFloatcount = 4;



#define toInt(x) ((int)(cm_pow(clamp(x, 0.f, 1.f), 1.f / 2.2f) * 255.f + .5f))

using CmRay = vector<float, rayVecSize>;
using CmRay_ref = vector_ref<float, rayVecSize>;

using CmCamera = vector<float, cameraVecSize>;
using CmCamrea_ref = vector_ref<float, cameraVecSize>;

using CmSeed_ref = vector_ref<unsigned, 2>;

using CmSphere = vector<float, kSphereClassFloatcount>;
using CmSphere_ref = vector_ref<float, kSphereClassFloatcount>;



_GENX_ float vecDot(const vector_ref<float, 3> a, const vector_ref<float, 3> b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

_GENX_ bool vecIsZero(const vector_ref<float, 3> v) {
    return (v[0] == 0) && (v[1] == 0) && (v[2] == 0);
}

_GENX_ vector<float, 3> vecCross(const vector_ref<float, 3> a, const vector_ref<float, 3> b) {
    vector<float, 3> ret;

    ret[0] = a[1] * b[2] - a[2] * b[1];
    ret[1] = a[2] * b[0] - a[0] * b[2];
    ret[2] = a[0] * b[1] - a[1] * b[0];

    return ret;
}


_GENX_ void normalize(vector_ref<float, 3> v) {

    // dot product
    float dp = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    float f = 1.f / cm_sqrt(dp);        // Note: can change to rsqrt ...
    v = f * v;
}

_GENX_ float getRandom(CmSeed_ref seeds) {

    unsigned seed0 = seeds[0];
    unsigned seed1 = seeds[1];

    //printf("Cm before seed 0,1: %u, %u\n",seed0, seed1);

    seeds[0] = 36969 * ((seed0) & 65535) + ((seed0) >> 16);
	seeds[1] = 18000 * ((seed1) & 65535) + ((seed1) >> 16);
    
    unsigned int ires = ((seeds[0]) << 16) + (seeds[1]);

    //printf("Cm seed 0,1: %u, %u  ires: %u\n",seed0, seed1, ires);
    
    /* Convert to float */
    union {
        float f;
        unsigned int ui;
    } res;
    res.ui = (ires & 0x007fffff) | 0x40000000;

    return (res.f - 2.f) / 2.f;
}


_GENX_ vector<float, 3> uniformSampleSphere(const float u1, const float u2) {

    const float z = 1.f - 2.f * u1;
    const float r = cm_sqrt(cm_max<float>(0.f, 1.f - z * z));    //distance from the center

    const float phi = 2.f * FLOAT_PI * u2;
	const float x = r * cm_cos(phi);
    const float y = r * cm_sin(phi);
    

    vector<float, 3> result;

    result[0] = x;
    result[1] = y;
    result[2] = z;

    return result;
}


_GENX_ float sphereIntersect(const CmSphere_ref sphere, const CmRay_ref ray) {


    /* returns distance, 0 if nohit */
    /* Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0 */

    vector<float, 3> op = sphere.select<3, 1>(1) - ray.select<3, 1>(0);         // s->p - r->o
    float b = vecDot(op, ray.select<3, 1>(3));     // op , r->d
    float det = (b * b) - vecDot(op, op) + (sphere[0] * sphere[0]);

    if (det < 0.f)
        return 0.f;
    else
        det = cm_sqrt(det);



    float t = b - det;
    if (t >  kEpsilon)
        return t;
    else {
        t = b + det;

        if (t >  kEpsilon)
            return t;
        else
            return 0.f;
    }

}


_GENX_ void generateCameraRay(const CmCamrea_ref camera, vector_ref<unsigned, 2> seeds, const int width, const int height, const int x, const int y, CmRay_ref ray) {

    //test
    //seeds(0) = 10000;

    const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	const float r1 = getRandom(seeds) - 0.5f;
	const float r2 = getRandom(seeds) - 0.5f;
	const float kcx = (x + r1) * invWidth - 0.5f;
    const float kcy = (y + r2) * invHeight - 0.5f;
    

    vector<float, 3> rdir;


    rdir = (camera.select<3, 1>(9) * kcx) + (camera.select<3, 1>(12) * kcy) + camera.select<3, 1>(6);

    vector<float, 3> rorig = (0.1 * rdir) + camera.select<3, 1>(0);


    // KAOCC:  rdir match with CL

    // normalize rdir !!!

    normalize(rdir);


    ray.select<3, 1>(0) = rorig;
    ray.select<3, 1>(3) = rdir;


}

