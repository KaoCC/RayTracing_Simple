cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)



set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

set(3RDPARTY_DIR ${CMAKE_SOURCE_DIR}/3rdparty)

set(RT_DIR ${CMAKE_SOURCE_DIR}/SimpleRT)

project(SimpleRT)


set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${3RDPARTY_DIR}/freeglut)
endif ()


# options

option (USE_OPENCL "Use OpenCL" ON)

# add defs

if (USE_OPENCL)
    add_definitions(-DENABLE_OPENCL=1)
endif()


# find packages

if (USE_OPENCL)
    find_package(OpenCL REQUIRED)
endif()

find_package(GLUT REQUIRED)

add_subdirectory(SimpleRT)

