# RayTracing_Simple

This is a implementation of the Ray-Tracing algorithm using OpenCL for demonstration purpose.
The code has been kept as simple as possible with minimum dependencies to 3rd party libraries.

## System requirements and Dependencies

- [OpenCL SDK](https://software.intel.com/en-us/intel-opencl)
- [freeglut](http://freeglut.sourceforge.net/)

## Build

### Windows & Mac OS X & Linux

We have recently changed from premake to [cmake](https://cmake.org/).
Belows are instructions of usnig a GCC-based compiler as an example.

1. Create your build directory `mkdir RT_build`
2. Run CMake `cmake ..` or `cmake -DCMAKE_BUILD_TYPE=Debug` for debugging
3. Compile by running `make`
4. The binary is named Radiance.

