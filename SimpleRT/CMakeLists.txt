



include_directories(${RT_DIR}/include)

include_directories(${GLUT_INCLUDE_DIR})
include_directories(${OpenCL_INCLUDE_DIRS})

# Cm related
include_directories(${CM_RUNTIME_INC})
include_directories(${CM_COMPILER_INC})

link_directories(${CM_RUNTIME_DIR}/lib/${PATH_MODIFIER} ${CM_COMPILER_DIR}/lib/${PATH_MODIFIER})

add_executable(
    SimpleRT

    ./include/Camera.hpp
    ./include/Ray.hpp
    ./include/Scene.hpp
    ./include/SetupGL.hpp
    ./include/Sphere.hpp
    ./include/Utility.hpp
    ./include/Vec.hpp
    ./include/CmSVMAllocator.hpp
    ./include/CmConfig.hpp

    ./src/Main.cpp
    ./src/SetupGL.cpp
    ./src/Utility.cpp
    ./src/Vec.cpp
    ./src/CmSVMAllocator.cpp
    ./src/CmConfig.cpp
    ./src/Scene.cpp
)




target_link_libraries(SimpleRT ${GLUT_LIBRARIES} ${GLUT_glut_LIBRARY} ${OpenCL_LIBRARIES})


# Cm 
set(CMRT_LIB "igfx${DX_MOD}cmrt${FILENAME_MODIFIER}")
list (APPEND CMRT_LIB "libcm" )
target_link_libraries(SimpleRT ${CMRT_LIB})

# load the kernel automatically

add_custom_command(TARGET SimpleRT POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${RT_DIR}/kernel
$<TARGET_FILE_DIR:SimpleRT>/)


add_custom_command(
    TARGET SimpleRT PRE_BUILD
    COMMAND ${GENX_CC} ARGS -c -Qxcm -mCM_emit_common_isa ${RT_DIR}/kernel/RayTracing_Cm.cpp
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}
    COMMENT "Compiling genx CM kernel definitions source file ${RT_DIR}/kernel/RayTracing_Cm.cpp"
    #VERBATIM
   )


#add_custom_command(
#    TARGET SimpleRT PRE_BUILD
#    COMMAND ${CM_ENV_SCRIPT} ARGS x64 gen9
    #WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}
#    COMMENT "SetupEnv"
    #VERBATIM
#   )


if (WIN32)
    #set(WK_DIR "$<TARGET_FILE_DIR:Radiance>")
    set_target_properties(SimpleRT PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR})
endif()

