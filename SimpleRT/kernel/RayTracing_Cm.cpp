

#include <cm/cm.h>


// tmp
#include "../include/vec.hpp"
// #include "../include/Ray.hpp"


// we have to use C struct ...

/*struct Vec {
    float x;
    float y;
    float z;
}; */


// tmp

const unsigned kWidth = 800;
const unsigned kHeight = 600;


constexpr const unsigned sphereClassFloatcount =  12;
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
struct Sphere {

    float rad;
    Vec p, e, c;
    int refl;
};



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


// Tested with OpenCL
_GENX_ float GetRandom(CmSeed_ref seeds) {

    unsigned seed0 = seeds[0];
    unsigned seed1 = seeds[1];

//    printf("Cm before seed 0,1: %u, %u\n",seed0, seed1);

    seeds[0] = 36969 * ((seed0) & 65535) + ((seed0) >> 16);
	seeds[1] = 18000 * ((seed1) & 65535) + ((seed1) >> 16);
    
    unsigned int ires = (seeds[0] << 16) + seeds[1];

//    printf("Cm seed 0,1: %u, %u  ires: %u\n",seed0, seed1, ires);
    
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
_GENX_ void GenerateCameraRay(CmCamrea_ref camera, vector_ref<unsigned, 2> seeds, const int width, const int height, const int x, const int y, CmRay_ref ray) {

    //test
    //seeds(0) = 10000;



    const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	const float r1 = GetRandom(seeds) - 0.5f;
	const float r2 = GetRandom(seeds) - 0.5f;
	const float kcx = (x + r1) * invWidth - 0.5f;
    const float kcy = (y + r2) * invHeight - 0.5f;
    
    vector<float, 3> rdir;

//    rdir(0) = camera(9) * kcx + camera(12) * kcy + camera(6);               // for x
//    rdir(1) = camera(9 + 1) * kcx + camera(12 + 1) * kcy + camera(6 + 1);   // for y
//    rdir(2) = camera(9 + 2) * kcx + camera(12 + 2) * kcy + camera(6 + 2);   // for z


    rdir = camera.select<3, 1>(9) * kcx + camera.select<3, 1>(12) * kcy + camera.select<3, 1>(6);

    vector<float, 3> rorig = 0.1 * rdir + camera.select<3, 1>(0);


    ray.select<3, 1>(0) = rorig;
    ray.select<3, 1>(3) = rdir;

/*    for (auto i = 0 ; i < 3; ++i) {
        ray[i] = rorig[i];
        ray[i + 3] = rdir[i];
    }*/



}



//constexpr const unsigned seedOffset = sizeof(unsigned) * 2;

extern "C" _GENX_MAIN_ void
RayTracing(SurfaceIndex cameraIndex, SurfaceIndex seedIndex, SurfaceIndex colorIndex, SurfaceIndex spheresInedx, unsigned sphereCount) {
    
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
    printf("Seed Offset: %d\n", seedOffset);
    
    vector<unsigned int, 2> seedIn;
    read(seedIndex, seedOffset, seedIn);

    printf("(%d, %d): before in(0): %u\n", x, y, seedIn(0));


    // Camera
    CmCamera cameraParam;
        
    read(cameraIndex, 0, cameraParam);
    printf("cam : %f, %f, %f, %f, %f, %f, %f, %f\n", cameraParam(0), cameraParam(1), cameraParam(2), cameraParam(3), cameraParam(4), cameraParam(5), cameraParam(6), cameraParam(7));


    CmRay ray;
    GenerateCameraRay(cameraParam, seedIn, width, height, x,  y, ray);

    printf("Ray Gen: %f ,%f, %f | %f, %f, %f\n", ray[0], ray[1], ray[2], ray[3], ray[4], ray[5]);

    printf("after function seedIn(0): %u\n", seedIn(0));

    //vector<unsigned int, 8> out;

    //vector<Vec, 10> outColor;

    vector<float, 3> outTmpColor;

    //printf("cam : %f, %f, %f, %f, %f, %f, %f, %f\n", vals(0), vals(1), vals(2), vals(3), vals(4), vals(5), vals(6), vals(7));

    //printf("before out(0): %d\n", out(0));

    //out(0) = 3000;
    //out(1) = 5000;

    //printf("out(0): %d\n", out(0));
    //printf("out(1): %d\n", out(1));

    //write(seedIndex, 0 , out);
    //write(colorIndex, x, y, outColor);

}