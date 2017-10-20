

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

constexpr const unsigned threadCountX = 20;
constexpr const unsigned threadCountY = 20;


constexpr const unsigned kSphereClassFloatcount =  12;       // padding
constexpr const unsigned kCameraClassFloatcount =  15;       // check alignment

constexpr const unsigned cameraVecSize = kCameraClassFloatcount;
constexpr const unsigned rayVecSize = 6;

constexpr const unsigned kColorFloatcount = 4;

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



// intersectResult: distance, sphere id

// yet to be checked
_GENX_ bool intersect(SurfaceIndex spheresIndex, const unsigned kSphereCount, const CmRay_ref currentRay, vector_ref<float, 2> intersectResult) {

    float inf = intersectResult[0] = 1e20f;

    // test with all the spheres in the scene 
    for (unsigned i = 0; i < kSphereCount; ++i) {

        // read from spheresIndex and pass to the function
        unsigned sphereOffset = kSphereClassFloatcount * sizeof(float) * i;

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


_GENX_ bool intersectP(SurfaceIndex spheresIndex, const unsigned kSphereCount, const CmRay_ref currentRay, const float maVal) {


    for (unsigned i = 0 ; i < kSphereCount; ++i) {

        // read the sphere
        unsigned sphereOffset = kSphereClassFloatcount * sizeof(float) * i;
        
        CmSphere sphere;
        read(spheresIndex, sphereOffset, sphere);

        const float d = sphereIntersect(sphere, currentRay);

        if ((d != 0.f) && (d < maVal)) {
            return true;
        }

    }

    return false;
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

_GENX_ void sampleLights(SurfaceIndex spheresIndex, const unsigned kSphereCount, CmSeed_ref seeds,  const vector_ref<float, 3> hitPoint, const vector_ref<float, 3> normal, vector_ref<float, 3> result) {

    // clear
    for (unsigned i = 0; i < 3; ++i) {
        result[i] = 0;
    }

    for (unsigned i = 0 ; i < kSphereCount; ++i) {

        // read sphere
        unsigned lightOffset = kSphereClassFloatcount * sizeof(float) * i;
        CmSphere light;
        read(spheresIndex, lightOffset, light);

        if (!vecIsZero(light.select<3, 1>(4))) {

            // it is a light source !

            // test
            //printf("Light source !!\n");

            CmRay shadowRay;
            shadowRay.select<3, 1>(0) = hitPoint;       // r.o
            
            /* Choose a point over the light source */

            vector<float, 3> unitSpherePoint = uniformSampleSphere(getRandom(seeds), getRandom(seeds));

            vector<float, 3> spherePoint = light[0] * unitSpherePoint;
            spherePoint = spherePoint + light.select<3, 1>(1);
            
            /* Build the shadow ray direction */

            shadowRay.select<3, 1>(3) = spherePoint - hitPoint;         // r.d
            const float len = cm_sqrt(vecDot(shadowRay.select<3, 1>(3), shadowRay.select<3, 1>(3)));
            shadowRay.select<3, 1>(3) = (1.f / len) * shadowRay.select<3, 1>(3);

            float wo = vecDot(shadowRay.select<3, 1>(3), unitSpherePoint);
			if (wo > 0.f) {	
                /* It is on the other half of the sphere */
                continue;
            } else {
                wo = -wo;
            }



            /* Check if the light is visible */

            const float wi = vecDot(shadowRay.select<3, 1>(3), normal);
            if ((wi > 0.f) && (!intersectP(spheresIndex, kSphereCount, shadowRay, len - kEpsilon))) {       // note :  if wi <= 0, the hitpoint is on the back of the sphere

                vector<float, 3> c = light.select<3, 1>(4);
                const float s = (4.f * FLOAT_PI * light[0] * light[0]) * wi * wo / (len * len);
                c = s * c;
                result = result + c;
            }
            

        }


    }

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

    for (unsigned int depth = 0; depth < kMaxNumOfBounce; ++depth) {

        //float t = 0;            /* distance to intersection */
        //unsigned int id = 0;    /* id of intersected object */

        vector<float, 2> intersectResult;
        intersectResult[0] = 0.f;       /* distance to intersection */
        intersectResult[1] = -1;         /* id of intersected object */

		if (!intersect(spheresIndex, kSphereCount, currentRay, intersectResult)) {
			break;      /* if miss, break */
        }
        
        // hit point infoirmation
        vector<float, 3> hitpoint = intersectResult[0] * currentRay.select<3, 1>(3);     // t * r->d
        hitpoint = currentRay.select<3, 1>(0) + hitpoint;           // new hit  = o + td


        // Normal

        // load the hit object first
        unsigned hitID = intersectResult[1];
        unsigned hitSphereOffset = kSphereClassFloatcount * sizeof(float) * hitID;

        printf("Hit!! %f %f %f, ID: %u, depth: %u\n", hitpoint[0], hitpoint[1], hitpoint[2], hitID, depth);

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

        int refl = hitSphere[10];       // get refl property

        if (refl == DIFF) {
            specularBounce = 0;

            throughput = throughput * hitSphere.select<3, 1>(7);           // s->c

            /* Direct lighting component */

            vector<float, 3> Ld;


            // KAOCC:  sample light
            sampleLights(spheresIndex, kSphereCount, seeds, hitpoint, normal, Ld);


            Ld = throughput * Ld;
            rad = rad + Ld;

            // test
            printf("DIFF rad: %f %f %f | ID: %u, depth: %u\n", rad[0], rad[1], rad[2], hitID, depth);

            /* Diffuse component */
            float r1 = 2.f * FLOAT_PI * getRandom(seeds);	// Random angle
            float r2 = getRandom(seeds);
            float r2s = cm_sqrt(r2);                        // Random distance from center

            // Create coordinate system for sphere: w u v

            vector<float, 3> w = nl;

            vector<float, 3> u;
            vector<float, 3> a;

            if (cm_abs(w[0]) > .1f) {       // test w.x
                a[0] = 0.f;
                a[1] = 1.f;
                a[2] = 0.f;
            } else {
                a[0] = 1.f;
                a[1] = 0.f;
                a[2] = 0.f;
            }


            // cross
            u = vecCross(a, w);
            normalize(u);

            vector<float, 3> v = vecCross(w, u);

            u = (cm_cos(r1) * r2s) * u;
            v = (cm_sin(r1) * r2s) * v;

            vector<float, 3> newDir = u + v;
            w = cm_sqrt(1 - r2) * w;
            newDir = newDir + w;

            currentRay.select<3, 1>(0) = hitpoint;
            currentRay.select<3, 1>(3) = newDir;

        } else if (refl == SPEC) {
            specularBounce = 1;

            printf("SPEC | depth: %u \n", depth);

            // R = D - 2(Nornal dot D)Normal
            vector<float, 3> newDir = (2 * vecDot(normal, currentRay.select<3, 1>(3))) * normal;

            newDir = currentRay.select<3, 1>(3) - newDir;

            throughput = throughput *  hitSphere.select<3, 1>(7);

            currentRay.select<3, 1>(0) = hitpoint;
            currentRay.select<3, 1>(3) = newDir;

        } else if (refl == REFR) {
            specularBounce = 1;

            printf("REFR | depth: %u \n", depth);

            // tmp !
            //break;


            vector<float, 3> newDir = (2 * vecDot(normal, currentRay.select<3, 1>(3))) * normal;
            newDir = currentRay.select<3, 1>(3) - newDir;

            CmRay reflRay; 
            reflRay.select<3,1>(0) = hitpoint;  /* Ideal dielectric REFRACTION */
            reflRay.select<3, 1>(3) = newDir;

            /* Ray from outside going in? */
            bool isInto = (vecDot(normal, nl) > 0);


            const float nc = 1.f;		// Index of Refraction (IOR) of vacuum
			const float nt = 1.52f;	//Index of Refraction (IOR) for glass is 1.52 
            float nnt = isInto ? nc / nt : nt / nc;
            float ddn = vecDot(currentRay.select<3, 1>(3), nl);

            // Note: currentRay.d and nl are unit vectors
			// nc * Sin(c) = nt * Sin(t)
            // Cos^2(t) = 1 - Sin^2(t) = 1 - (nnt)^2 * (1 - Cos^2(c))
            

            float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

            if (cos2t < 0.f)  { /* Total internal reflection */
				throughput = throughput * hitSphere.select<3, 1>(7);

				currentRay = reflRay; // if the angle is too shallow, all the light is reflected
				continue;
            }
            
            // refraction ray equation
            float kk = (isInto ? 1 : -1) * (ddn * nnt + cm_sqrt(cos2t));
            vector<float, 3> nkk = kk * normal;
            vector<float, 3> transDir = nnt *  currentRay.select<3, 1>(3);
            transDir = transDir - nkk;

            normalize(transDir);

            // Fresnel Reflectance
			float a = nt - nc;
			float b = nt + nc;
            float R0 = a * a / (b * b);
            float c = 1 - (isInto ? -ddn : vecDot(transDir, normal));	// 1 - cos

            float Re = R0 + (1 - R0) * c * c * c * c * c; // Reflectance
			float Tr = 1.f - Re;
            float P = 0.25f + 0.5f * Re;		// probability of reflecting
            
            float RP = Re / P;
            float TP = Tr / (1.f - P);
            
            if (getRandom(seeds) < P) { // Choose one !
				throughput = RP * throughput;
				throughput = throughput * hitSphere.select<3, 1>(7);

				currentRay = reflRay;	//reflect
				continue;
			} else {
				throughput = TP * throughput;
				throughput = throughput * hitSphere.select<3, 1>(7);
                
                currentRay.select<3, 1>(0) = hitpoint;
                currentRay.select<3, 1>(3) = transDir;  // refract

				continue;
			}



        } else {
            // Error here

            printf("Error !!!! \n");
        }

    }


    result = rad;

}



//constexpr const unsigned seedOffset = sizeof(unsigned) * 2;

_GENX_MAIN_ void
RayTracing(SurfaceIndex cameraIndex, SurfaceIndex seedIndex, SurfaceIndex colorIndex, SurfaceIndex spheresIndex, unsigned sphereCount, unsigned inputSampleCount) {
    
    int x = get_thread_origin_x();
    int y = get_thread_origin_y();

    //printf("(x, y) : (%d, %d)\n", x, y);


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
        vector<float, kSphereClassFloatcount> sphereParam;
        read(spheresInedx, sphereOffset, sphereParam);

        printf("[%d] Sphere rad: %f\n, p: %f ,%f, %f\n, e: %f, %f, %f\n", i, sphereParam(0), sphereParam(1), sphereParam(2), sphereParam(3), sphereParam(4), sphereParam(5), sphereParam(6) );

        sphereOffset += kSphereClassFloatcount * sizeof(float);
    }*/


    const unsigned width = kWidth;
    const unsigned height = kHeight;

    // tmp


    // Camera
    CmCamera camera;
        
    read(cameraIndex, 0, camera);
//    printf("cam : %f, %f, %f, %f, %f, %f, %f, %f, %f\n", camera(6), camera(7), camera(8), camera(9), camera(10), camera(11) ,camera(12), camera(13), camera(14));


    // W, H, 
    // Tx = x thread count, Ty

    // upper left pixel index: (H/Ty) * y * width + (W/Tx) * x

    // seed index = (2 * pixel index, 2 * pixel index + 1)


    unsigned upperLeftPixelNumber = (height / threadCountY) * y * width + (width / threadCountX) * x;
    unsigned lowerRightPixelBound = (height / threadCountY) * (y + 1) * width + (width / threadCountX) * (x + 1);

    
    // test
    printf("x, y : (%u, %u) Upper left pixel number (id): %u\n", x , y,  upperLeftPixelNumber);


    for (unsigned iNum = upperLeftPixelNumber; iNum < lowerRightPixelBound; ++iNum) {

        unsigned seedNum = 2 * iNum;

        const unsigned offsetStart = (seedNum / 4) * 4;
        const unsigned localIndex = seedNum % 4;
    
        const unsigned seedOffset = offsetStart * 4;

        vector<unsigned int, 4> seedIn;
        read(seedIndex,  seedOffset , seedIn);


        vector_ref<unsigned, 2> seedFinal = seedIn.select<2, 1>(localIndex);

        CmRay ray;
        generateCameraRay(camera, seedFinal, width, height, iNum % width,  iNum / width, ray);        // check x, y
    
        vector<float, 3> result;
        radiancePathTracing(spheresIndex, sphereCount, ray, seedFinal, result); 


        unsigned currentColorNumber = iNum;
        unsigned colorOffset = currentColorNumber * 16;
    
        // single color?
        vector<float, kColorFloatcount> color;
    
        // read color ?
        read(colorIndex, colorOffset, color);
    
        //printf("read color: %f %f %f %f\n", color[0], color[1], color[2], color[3]);
        
    
        // TMP
        const unsigned currentSample = inputSampleCount;       // Eventually it should be written from host
        const float k1 = currentSample;
        const float k2 = 1.f / (currentSample + 1.f);
    
        color.select<3,1>(0) = (color.select<3,1>(0) * k1 + result) * k2;
    
        // test only
        color[3] = -1;
    
    
        //printf("(%d, %d) write color: %f %f %f %f\n", x , y, color[0], color[1], color[2], color[3]);
    
        write(colorIndex, colorOffset, color);        


        // missing : write back the seeds ...


        // not optimized ...
        write(seedIndex,  seedOffset , seedIn);


    }




/*
    //  ------------ templates -------------

    
    unsigned firstSeedNumber = 2 * upperLeftPixelNumber;

    // tmp
    // Error: OWORD align ...

    const unsigned offsetStart = (firstSeedNumber / 4) * 4;
    const unsigned localIndex = firstSeedNumber % 4;

    const unsigned seedOffset = offsetStart * 4;
    
    // KAOCC: Check offset !!!!!       It must be OWORD aligned !

    vector<unsigned int, 4> seedIn;
    read(seedIndex,  seedOffset , seedIn);

    printf("firstSeedNumber: %u, Seed Offset: %u seedIn<4>: %u %u %u %u\n", firstSeedNumber, seedOffset, seedIn(0), seedIn(1), seedIn(2), seedIn(3));


    // extra:

    //vector<unsigned int, 4> seedIn;
//    read(seedIndex,  16 , seedIn);

//    printf("Extra: firstSeedNumber: %u, Seed Offset: %u seedIn<4>: %u %u %u %u\n", 4, 4 * 16, seedIn(0), seedIn(1), seedIn(2), seedIn(3));


    vector<unsigned, 2> seedFinal = seedIn.select<2, 1>(localIndex);

    CmRay ray;
    generateCameraRay(camera, seedFinal, width, height, x,  y, ray);

    vector<float, 3> result;
    radiancePathTracing(spheresIndex, sphereCount, ray, seedFinal, result); 



    // convert to pixel color


    // ...


    unsigned upperLeftColorNumber = upperLeftPixelNumber;
    unsigned colorOffset = upperLeftColorNumber * 16;

    // single color?
    vector<float, kColorFloatcount> color;

    // read color ?
    read(colorIndex, colorOffset, color);

    //printf("read color: %f %f %f %f\n", color[0], color[1], color[2], color[3]);
    

    // TMP
    const unsigned currentSample = 0;       // Eventually it should be written from host
    const float k1 = currentSample;
    const float k2 = 1.f / (currentSample + 1.f);

    color.select<3,1>(0) = (color.select<3,1>(0) * k1 + result) * k2;

    // test only
    color[3] = -1;


    //printf("(%d, %d) write color: %f %f %f %f\n", x , y, color[0], color[1], color[2], color[3]);

    write(colorIndex, colorOffset, color);


*/

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