



include_directories(${RT_DIR}/include)

include_directories(${GLUT_INCLUDE_DIR})
include_directories(${OpenCL_INCLUDE_DIRS})


add_executable(
    SimpleRT

    ./include/Camera.hpp
    ./include/Ray.hpp
    ./include/Scene.hpp
    ./include/SetupGL.hpp
    ./include/Sphere.hpp
    ./include/Utility.hpp
    ./include/Vec.hpp
    ./include/OpenCLConfig.hpp
    ./include/Config.hpp

    ./src/Main.cpp
    ./src/SetupGL.cpp
    ./src/Utility.cpp
    ./src/Vec.cpp
    ./src/Scene.cpp
    ./src/OpenCLConfig.cpp
)


target_link_libraries(SimpleRT ${GLUT_LIBRARIES} ${GLUT_glut_LIBRARY} ${OpenCL_LIBRARIES})

# load the kernel automatically

add_custom_command(TARGET SimpleRT POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${RT_DIR}/kernel
$<TARGET_FILE_DIR:SimpleRT>/)


if (WIN32)
    #set(WK_DIR "$<TARGET_FILE_DIR:Radiance>")
    set_target_properties(SimpleRT PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR})
endif()

