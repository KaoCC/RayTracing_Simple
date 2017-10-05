cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)



set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

set(3RDPARTY_DIR ${CMAKE_SOURCE_DIR}/3rdparty)

set(RT_DIR ${CMAKE_SOURCE_DIR}/SimpleRT)


# for Cm Runtime and Compiler
set(CM_DIR C:/Intel/MDF)
set(CM_RUNTIME_DIR ${CM_DIR}/runtime)
set(CM_COMPILER_DIR ${CM_DIR}/compiler)

set(CM_RUNTIME_INC ${CM_RUNTIME_DIR}/include)
set(CM_COMPILER_INC ${CM_COMPILER_DIR}/include)

# add default settings and configurations for Cm compilers and env

set(EXTRA_INC -I ${CM_COMPILER_INC} -I ${CM_RUNTIME_INC})
set(INSTALLDIR ${CM_DIR})
set(ICLDIR ${INSTALLDIR}/compiler)
set(GENX_CC ${ICLDIR}/bin/cmc)

set(GENX_PATH -mCM_genx_assembler=${ICLDIR}/bin/GenX_IR)



# add ARCH check
if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
    set(PATH_MODIFIER "x86")
    set(POSTFIX_MODIFIER "")
    set(FILENAME_MODIFIER "32")
elseif("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
    set(PATH_MODIFIER "x64")
    set(POSTFIX_MODIFIER "_x64")
    set(FILENAME_MODIFIER "64")
else()
    message(FATAL_ERROR "${CMAKE_GENERATOR} missing! Can't continue")
endif()


message("GENX_CC is ${GENX_CC}")


project(SimpleRT)



set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${3RDPARTY_DIR}/freeglut)
endif ()

# find packages

find_package(OpenCL REQUIRED)
find_package(GLUT REQUIRED)

add_subdirectory(SimpleRT)

