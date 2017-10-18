

#include <cm/cm.h>


// tmp
#include "../include/vec.hpp"
// #include "../include/Ray.hpp"


#define FLOAT_PI 3.14159265358979323846f

#define sign(x) ((x) > 0 ? 1 : -1)


// we have to use C struct ...

/*struct Vec {
    float x;
    float y;
    float z;
}; */


// tmp

/* material types */
enum Refl {
	DIFF, SPEC, REFR
};

const float kEpsilon = 0.01f;


const unsigned kWidth = 800;
const unsigned kHeight = 600;


constexpr const unsigned sphereClassFloatcount =  12;       // padding
constexpr const unsigned cameraClassFloatcount =  15;       // check alignment

constexpr const unsigned cameraVecSize = cameraClassFloatcount;
constexpr const unsigned rayVecSize = 6;


// not used, for reference only
struct Camera {

	/* User defined values */
	Vec orig, target;
    /* Calculated values */
    Vec dir, x, y;
};

// not used, for refernce only
/*struct Sphere {

    float rad;
    Vec p, e, c;
    int refl;
};*/



// KAOCC: not in used
// Vec o, d
//struct CmRay {
//};


// test 
using CmRay = vector<float, rayVecSize>;
using CmRay_ref = vector_ref<float, rayVecSize>;

using CmCamera = vector<float, cameraVecSize>;
using CmCamrea_ref = vector_ref<float, cameraVecSize>;

using CmSeed_ref = vector_ref<unsigned, 2>;

using CmSphere = vector<float, sphereClassFloatcount>;
using CmSphere_ref = vector_ref<float, sphereClassFloatcount>;


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


// Ray : Vec o, d => vector<float, 6>

// for single ray ?
_GENX_ void generateCameraRay(const CmCamrea_ref camera, vector_ref<unsigned, 2> seeds, const int width, const int height, const int x, const int y, CmRay_ref ray) {

    //test
    //seeds(0) = 10000;



    const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	const float r1 = getRandom(seeds) - 0.5f;
	const float r2 = getRandom(seeds) - 0.5f;
	const float kcx = (x + r1) * invWidth - 0.5f;
    const float kcy = (y + r2) * invHeight - 0.5f;
    

//    if (x == 0 && y == 0) {
//        printf("seed0, seed1 r1, r2: %u %u %f, %f | %f, %f: \n", seeds[0], seeds[1], r1, r2,  kcx, kcy);
//    }

    vector<float, 3> rdir;

//    rdir(0) = camera(9) * kcx + camera(12) * kcy + camera(6);               // for x
//    rdir(1) = camera(9 + 1) * kcx + camera(12 + 1) * kcy + camera(6 + 1);   // for y
//    rdir(2) = camera(9 + 2) * kcx + camera(12 + 2) * kcy + camera(6 + 2);   // for z



//    if (x == 0 && y == 0) {
        //printf("camera x.x: %f \n", camera(9));
//        printf("cam : %f, %f, %f, %f, %f, %f, %f, %f, %f\n", camera(6), camera(7), camera(8), camera(9), camera(10), camera(11) ,camera(12), camera(13), camera(14));


//        printf("cam select: %f, %f\n", camera.select<3, 1>(9)[0], camera.select<3, 1>(9)[1]);

//    }

    rdir = (camera.select<3, 1>(9) * kcx) + (camera.select<3, 1>(12) * kcy) + camera.select<3, 1>(6);

//   if (x == 0 && y == 0) {
//		printf("rdir: x, y, z %f %f %f\n", rdir[0], rdir[1], rdir[2]);
//	}

    vector<float, 3> rorig = (0.1 * rdir) + camera.select<3, 1>(0);


    // KAOCC:  rdir match with CL


    // normalize rdir !!!

    normalize(rdir);


    ray.select<3, 1>(0) = rorig;
    ray.select<3, 1>(3) = rdir;

/*    for (auto i = 0 ; i < 3; ++i) {
        ray[i] = rorig[i];
        ray[i + 3] = rdir[i];
    }*/

//    if (x == 0 && y == 0) {
//        printf("Ray Gen: %f ,%f, %f | %f, %f, %f\n", ray[0], ray[1], ray[2], ray[3], ray[4], ray[5]);
//    }
}



_GENX_ float sphereIntersect(const CmSphere_ref sphere, const CmRay_ref ray) {


    /* returns distance, 0 if nohit */
    /* Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0 */

    vector<float, 3> op = sphere.select<3, 1>(1) - ray.select<3, 1>(0);         // s->p - r->o
    float b = vecDot(op, ray.select<3, 1>(3));     // op , r->d
    float det = b * b - vecDot(op, op) + sphere[0] * sphere[0];

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



// intersectResult: distance, sphere id

// yet to be done
_GENX_ bool intersect(SurfaceIndex spheresIndex, const unsigned kSphereCount, const CmRay_ref currentRay, vector_ref<float, 2> intersectResult) {

    float inf = intersectResult[0] = 1e20f;

    unsigned sphereOffset = 0;

    // test with all the spheres in the scene 
    for (unsigned i = 0; i < kSphereCount; ++i) {

        // read from SphereIndex and pass to the function
        sphereOffset = sphereClassFloatcount * sizeof(float) * i;

        CmSphere sphere;
        read(spheresIndex, sphereOffset, sphere);

        // KAOCC: check the sphere information

        const float d = sphereIntersect(sphere, currentRay);
        
		if ((d != 0.f) && (d < intersectResult[0])) {
			intersectResult[0] = d;
			intersectResult[1] = i;
        }    
    }
    
    return (intersectResult[0] < inf);
}

// single ray version
_GENX_ void radiancePathTracing(SurfaceIndex spheresIndex, const unsigned kSphereCount, const CmRay_ref startRay, CmSeed_ref seeds, vector_ref<float, 3> result) {


    // current ray being processed
    CmRay currentRay = startRay;


    // move to somewhere else
    const unsigned kMaxNumOfBounce = 7;
    int specularBounce = 1;
    
    vector<float, 3> rad;

    // init rad to 0,0,0
    for (unsigned i = 0 ; i < 3; ++i) {
        rad[i] = 0.f;
    }

    vector<float, 3> throughput;
    
    // init throughput to 1,1,1
    for (unsigned i = 0 ; i < 3; ++i) {
        throughput[i] = 1.f;
    }

    // init current ray ?

    for (unsigned int depth = 0; depth < kMaxNumOfBounce; ++depth) {

        //float t = 0;            /* distance to intersection */
        //unsigned int id = 0;    /* id of intersected object */

        vector<float, 2> intersectResult;
        intersectResult[0] = 0.f;       /* distance to intersection */
        intersectResult[1] = 0;         /* id of intersected object */

		if (!intersect(spheresIndex, kSphereCount, currentRay, intersectResult)) {
			break;      /* if miss, break */
        }
        
        // hit point infoirmation
        vector<float, 3> hitpoint = intersectResult[0] * currentRay.select<3, 1>(3);     // t * r->d
        hitpoint = currentRay.select<3, 1>(0) + hitpoint;



        // Normal

        // load the hit object first
        unsigned hitID = intersectResult[1];
        unsigned hitSphereOffset = sphereClassFloatcount * sizeof(float) * hitID;

        printf("Hit!! %f %f %f, %u\n", hitpoint[0], hitpoint[1], hitpoint[2], hitID);

        CmSphere hitSphere;
        read(spheresIndex, hitSphereOffset, hitSphere);

        vector<float, 3> normal = hitpoint - hitSphere.select<3, 1>(1);

        // normailze the normal
        normalize(normal);

        const float dp = vecDot(normal, currentRay.select<3, 1>(3));

        // Orienting Normal Vector

		const float invSignDP = -1.f * sign(dp);
        vector<float, 3> nl = invSignDP * normal;


        /* Add emitted light */

        vector<float, 3> emitColor = hitSphere.select<3, 1>(4);       // s->e

        if (!vecIsZero(emitColor)) {

            if (specularBounce) {
                emitColor = cm_abs<float>(dp) * emitColor;
                emitColor = throughput * emitColor;
                rad = rad + emitColor;
            }
            
            break;
        }


        // Applying Rendering based on surface propery 

        int refl = hitSphere[10];

        if (refl == DIFF) {
            specularBounce = 0;

            throughput = throughput *  hitSphere.select<3, 1>(7);

            /* Direct lighting component */

            vector<float, 3> Ld;


            // --- KAOCC:  sample light ?!??!??!?!



            Ld = throughput * Ld;
            rad = rad + Ld;

            /* Diffuse component */
            float r1 = 2.f * FLOAT_PI * getRandom(seeds);	// Random angle
            float r2 = getRandom(seeds);
            float r2s = cm_sqrt(r2); // Random distance from center

            // Create coordinate system for sphere: w u v

            vector<float, 3> w = nl;


            // ...

            vector<float, 3> newDir;

            currentRay.select<3, 1>(0) = hitpoint;
            currentRay.select<3, 1>(3) = newDir;

        } else if (refl == SPEC) {
            specularBounce = 1;

            // R = D - 2(Nornal dot D)Normal
            vector<float, 3> newDir = (2 * vecDot(normal, currentRay.select<3, 1>(3))) * normal;

            newDir = currentRay.select<3, 1>(3) - newDir;

            throughput = throughput *  hitSphere.select<3, 1>(7);

            currentRay.select<3, 1>(0) = hitpoint;
            currentRay.select<3, 1>(3) = newDir;

        } else if (refl == REFR) {

        } else {
            // Error here
        }

    }


    result = rad;

}



//constexpr const unsigned seedOffset = sizeof(unsigned) * 2;

_GENX_MAIN_ void
RayTracing(SurfaceIndex cameraIndex, SurfaceIndex seedIndex, SurfaceIndex colorIndex, SurfaceIndex spheresIndex, unsigned sphereCount) {
    
    int x = get_thread_origin_x();
    int y = get_thread_origin_y();

    printf("(x, y) : (%d, %d)\n", x, y);


    //printf("Sphere count: %d\n", sphereCount);

    // test

    //vector<svmptr_t, 8> camera = cam;

    //Camera* camera = reinterpret_cast<Camera*>(cam);

    //vector<float, 8> vals;

    //cm_svm_scatter_read(camera + sizeof(Vec), vals);

    //Sphere aa = *(reinterpret_cast<Sphere*>(&spheresInedx));
    //Sphere* s = reinterpret_cast<Sphere*>(tt);



    unsigned sphereOffset = 0;

    // test
    /*for (unsigned i = 0 ; i < sphereCount; ++i ) {
            // Sphere buffer
        vector<float, sphereClassFloatcount> sphereParam;
        read(spheresInedx, sphereOffset, sphereParam);

        printf("[%d] Sphere rad: %f\n, p: %f ,%f, %f\n, e: %f, %f, %f\n", i, sphereParam(0), sphereParam(1), sphereParam(2), sphereParam(3), sphereParam(4), sphereParam(5), sphereParam(6) );

        sphereOffset += sphereClassFloatcount * sizeof(float);
    }*/


    const unsigned width = kWidth;
    const unsigned height = kHeight;


    // tmp
    const unsigned seedOffset = (y * width + (x / 2) * 2) * sizeof(unsigned);
//    printf("Seed Offset: %d\n", seedOffset);
    
    vector<unsigned int, 2> seedIn;
    read(seedIndex, seedOffset, seedIn);

//    printf("(%d, %d): before in(0): %u\n", x, y, seedIn(0));


    // Camera
    CmCamera camera;
        
    read(cameraIndex, 0, camera);
//    printf("cam : %f, %f, %f, %f, %f, %f, %f, %f, %f\n", camera(6), camera(7), camera(8), camera(9), camera(10), camera(11) ,camera(12), camera(13), camera(14));


    CmRay ray;
    generateCameraRay(camera, seedIn, width, height, x,  y, ray);

    vector<float, 3> result;
    radiancePathTracing(spheresIndex, sphereCount, ray, seedIn, result); 




    // convert to pixel color



//    if (x == 0 && y == 0) {
//        printf("after function seedIn(0): %u\n", seedIn(0));
//    }

    //vector<unsigned int, 8> out;

    //vector<Vec, 10> outColor;

    //vector<float, 3> outTmpColor;

    //printf("cam : %f, %f, %f, %f, %f, %f, %f, %f\n", vals(0), vals(1), vals(2), vals(3), vals(4), vals(5), vals(6), vals(7));

    //printf("before out(0): %d\n", out(0));

    //out(0) = 3000;
    //out(1) = 5000;

    //printf("out(0): %d\n", out(0));
    //printf("out(1): %d\n", out(1));

    //write(seedIndex, 0 , out);
    //write(colorIndex, x, y, outColor);

}