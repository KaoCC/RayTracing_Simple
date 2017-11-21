

#include <cm/cm.h>


// tmp
//#include "../include/vec.hpp"
// #include "../include/Ray.hpp"

#include "RayTracing_Common.hpp"


// not used, for reference only
//struct Camera {

	/* User defined values */
//	Vec orig, target;
    /* Calculated values */
//    Vec dir, x, y;
//};

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

// Ray : Vec o, d => vector<float, 6>


// intersectResult: distance, sphere id

// yet to be checked
_GENX_ bool intersect(svmptr_t spheresSVMPtr, const unsigned kSphereCount, const CmRay_ref currentRay, vector_ref<float, 2> intersectResult) {

    float inf = intersectResult[0] = 1e20f;

    // test with all the spheres in the scene 
    for (unsigned i = 0; i < kSphereCount; ++i) {

        // read from spheresSVMPtr and pass to the function
        unsigned sphereOffset = kSphereClassFloatcount * sizeof(float) * i;

        CmSphere sphere;
        cm_svm_block_read(spheresSVMPtr + sphereOffset, sphere.select<4, 1>(0));
        cm_svm_block_read(spheresSVMPtr + sphereOffset + 4 * sizeof(float), sphere.select<8, 1>(4));
        //cm_svm_block_read(spheresSVMPtr + sphereOffset + 4 * sizeof(float) + 4 * sizeof(float), sphere.select<4, 1>(8));

        /*printf("[%d] Sphere rad: %f\n, p: %f ,%f, %f\n, e: %f, %f, %f\n, c: %f, %f, %f\n, refl: %f\n", i,
         sphere(0), sphere(1), sphere(2), sphere(3), sphere(4), sphere(5), sphere(6), sphere(7), sphere(8), sphere(9), sphere(10) ); */

        // KAOCC: check the sphere information

        const float d = sphereIntersect(sphere, currentRay);
        
		if ((d != 0.f) && (d < intersectResult[0])) {
			intersectResult[0] = d;
			intersectResult[1] = i;
        }    
    }
    
    return (intersectResult[0] < inf);
}


_GENX_ bool intersectP(svmptr_t spheresSVMPtr, const unsigned kSphereCount, const CmRay_ref currentRay, const float maVal) {


    for (unsigned i = 0 ; i < kSphereCount; ++i) {

        // read the sphere
        unsigned sphereOffset = kSphereClassFloatcount * sizeof(float) * i;
        
        CmSphere sphere;
        //cm_svm_block_read(spheresSVMPtr + sphereOffset, sphere);

        cm_svm_block_read(spheresSVMPtr + sphereOffset, sphere.select<4, 1>(0));
        cm_svm_block_read(spheresSVMPtr + sphereOffset + 4 * sizeof(float), sphere.select<8, 1>(4));
        //cm_svm_block_read(spheresSVMPtr + sphereOffset + 4 * sizeof(float) + 4 * sizeof(float), sphere.select<4, 1>(8));


        const float d = sphereIntersect(sphere, currentRay);

        if ((d != 0.f) && (d < maVal)) {
            return true;
        }

    }

    return false;
}


_GENX_ void sampleLights(svmptr_t spheresSVMPtr, const unsigned kSphereCount, CmSeed_ref seeds,  const vector_ref<float, 3> hitPoint, const vector_ref<float, 3> normal, vector_ref<float, 3> result) {

    // clear
    for (unsigned i = 0; i < 3; ++i) {
        result[i] = 0;
    }

    for (unsigned i = 0 ; i < kSphereCount; ++i) {

        // read sphere
        unsigned lightOffset = kSphereClassFloatcount * sizeof(float) * i;
        CmSphere light;
        //cm_svm_block_read(spheresSVMPtr + lightOffset, light);


        cm_svm_block_read(spheresSVMPtr + lightOffset, light.select<4, 1>(0));
        cm_svm_block_read(spheresSVMPtr + lightOffset + 4 * sizeof(float), light.select<8, 1>(4));
        //cm_svm_block_read(spheresSVMPtr + lightOffset + 4 * sizeof(float) + 4 * sizeof(float), light.select<4, 1>(8));

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
            if ((wi > 0.f) && (!intersectP(spheresSVMPtr, kSphereCount, shadowRay, len - kEpsilon))) {       // note :  if wi <= 0, the hitpoint is on the back of the sphere

                vector<float, 3> c = light.select<3, 1>(4);
                const float s = (4.f * FLOAT_PI * light[0] * light[0]) * wi * wo / (len * len);
                c = s * c;
                result = result + c;
            }
            

        }


    }

}


// single ray version
_GENX_ void radiancePathTracing(svmptr_t spheresSVMPtr, const unsigned kSphereCount, const CmRay_ref startRay, CmSeed_ref seeds, vector_ref<float, 3> result) {


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

		if (!intersect(spheresSVMPtr, kSphereCount, currentRay, intersectResult)) {
			break;      /* if miss, break */
        }
        
        // hit point infoirmation
        vector<float, 3> hitpoint = intersectResult[0] * currentRay.select<3, 1>(3);     // t * r->d
        hitpoint = currentRay.select<3, 1>(0) + hitpoint;           // new hit  = o + td


        // Normal

        // load the hit object first
        unsigned hitID = intersectResult[1];
        unsigned hitSphereOffset = kSphereClassFloatcount * sizeof(float) * hitID;

        //printf("Hit!! %f %f %f, ID: %u, depth: %u\n", hitpoint[0], hitpoint[1], hitpoint[2], hitID, depth);

        CmSphere hitSphere;
        //cm_svm_block_read(spheresSVMPtr + hitSphereOffset, hitSphere);


        cm_svm_block_read(spheresSVMPtr + hitSphereOffset, hitSphere.select<4, 1>(0));
        cm_svm_block_read(spheresSVMPtr + hitSphereOffset + 4 * sizeof(float), hitSphere.select<8, 1>(4));
        //cm_svm_block_read(spheresSVMPtr + hitSphereOffset + 4 * sizeof(float) + 4 * sizeof(float), hitSphere.select<4, 1>(8));

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
            sampleLights(spheresSVMPtr, kSphereCount, seeds, hitpoint, nl, Ld);


            Ld = throughput * Ld;
            rad = rad + Ld;

            // test
            //printf("DIFF rad: %f %f %f | ID: %u, depth: %u\n", rad[0], rad[1], rad[2], hitID, depth);

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

            //printf("SPEC | depth: %u \n", depth);

            // R = D - 2(Nornal dot D)Normal
            vector<float, 3> newDir = (2 * vecDot(normal, currentRay.select<3, 1>(3))) * normal;

            newDir = currentRay.select<3, 1>(3) - newDir;

            throughput = throughput *  hitSphere.select<3, 1>(7);

            currentRay.select<3, 1>(0) = hitpoint;
            currentRay.select<3, 1>(3) = newDir;

        } else if (refl == REFR) {
            specularBounce = 1;

            //printf("REFR | depth: %u \n", depth);

            // tmp !
            //break;


            vector<float, 3> newDir = (2 * vecDot(normal, currentRay.select<3, 1>(3))) * normal;
            newDir = currentRay.select<3, 1>(3) - newDir;

            CmRay reflRay; 
            reflRay.select<3, 1>(0) = hitpoint;  /* Ideal dielectric REFRACTION */
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

            printf("Error !!!! refl: %f\n", refl);
        }

    }


    result = rad;

}


//constexpr const unsigned seedOffset = sizeof(unsigned) * 2;

_GENX_MAIN_ void
RayTracing(svmptr_t cameraSVMPtr, svmptr_t seedSVMPtr, svmptr_t colorSVMPtr, svmptr_t spheresSVMPtr, svmptr_t pixelSVMPtr, unsigned sphereCount, unsigned inputSampleCount) {
    
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
        
    cm_svm_block_read(cameraSVMPtr + 0, camera);
 //   printf("cam : %f, %f, %f, %f, %f, %f, %f, %f, %f\n", camera(6), camera(7), camera(8), camera(9), camera(10), camera(11) ,camera(12), camera(13), camera(14));


    // W, H, 
    // Tx = x thread count, Ty

    // upper left pixel index: (H/Ty) * y * width + (W/Tx) * x

    // seed index = (2 * pixel index, 2 * pixel index + 1)


    unsigned upperLeftPixelNumber = (height / threadCountY) * y * width + (width / threadCountX) * x;
    //unsigned lowerRightPixelBound = (height / threadCountY) * (y + 1) * width + (width / threadCountX) * (x + 1);

    unsigned lowerLeftPixelNumber = (height / threadCountY) * (y + 1) * width + (width / threadCountX) * x;
  
    // test
    //printf("x, y : (%u, %u) Upper left pixel number (id): %u\n", x , y,  upperLeftPixelNumber);

    for (unsigned startNum = upperLeftPixelNumber; startNum < lowerLeftPixelNumber; startNum += width) {

        for (unsigned iNum = startNum; iNum < startNum + (width / threadCountX); ++iNum) {

            unsigned seedNum = 2 * iNum;

            const unsigned offsetStart = (seedNum / 4) * 4;
            const unsigned localIndex = seedNum % 4;
        
            const unsigned seedOffset = offsetStart * 4;

            vector<unsigned int, 4> seedIn;
            cm_svm_block_read(seedSVMPtr + seedOffset , seedIn);


            vector_ref<unsigned, 2> seedFinal = seedIn.select<2, 1>(localIndex);

            CmRay ray;
            generateCameraRay(camera, seedFinal, width, height, iNum % width,  iNum / width, ray);        // check x, y
        
            vector<float, 3> result;
            radiancePathTracing(spheresSVMPtr, sphereCount, ray, seedFinal, result); 


            unsigned currentColorNumber = iNum;
            unsigned colorOffset = currentColorNumber * 16;
        
            // single color?
            vector<float, kColorFloatcount> color;
        
            // read color ?
            cm_svm_block_read(colorSVMPtr + colorOffset, color);
        
            //printf("read color: %f %f %f %f\n", color[0], color[1], color[2], color[3]);
            
        
            // TMP
            const unsigned currentSample = inputSampleCount;       // Eventually it should be written from host
            const float k1 = currentSample;
            const float k2 = 1.f / (currentSample + 1.f);
        
            color.select<3,1>(0) = ((color.select<3,1>(0) * k1) + result) * k2;
        
            // test only
            color[3] = -1;
        

            //printf("current sample: %u\n", currentSample);
        
            //printf("(%d, %d) write color: %f %f %f %f\n", x , y, color[0], color[1], color[2], color[3]);
        
            cm_svm_block_write(colorSVMPtr + colorOffset, color);        


            // missing : write back the seeds ...


            // not optimized ...
            cm_svm_block_write(seedSVMPtr + seedOffset , seedIn);

            // pixel
            const unsigned pixelOffsetStart = (iNum / 4) * 4;
            const unsigned pixelLocalIndex = iNum % 4;
            const unsigned pixelOffset = pixelOffsetStart * 4;      // sizeof(unsigned)
            
            vector<unsigned int, 4> pixelIn;
            //read(pixelIndex, pixelOffset, pixelIn);


            pixelIn[pixelLocalIndex] = (toInt(color[0])) | (toInt(color[1]) << 8) | (toInt(color[2]) << 16);

            // write the pixel back
            if (iNum % 4 == 3) {
                cm_svm_block_write(pixelSVMPtr + pixelOffset, pixelIn);
            }

        }


    }




}
