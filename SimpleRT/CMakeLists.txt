



include_directories(${RT_DIR}/include)

include_directories(${GLUT_INCLUDE_DIR})
include_directories(${OpenCL_INCLUDE_DIRS})


add_executable(
    SimpleRT

    ./include/camera.h
    ./include/ray.h
    ./include/scene.h
    ./include/setupGL.h
    ./include/sphere.h
    ./include/utility.h
    ./include/vec.h

    ./src/Main.c
    ./src/setupGL.c
    ./src/utility.c
    ./src/vec.c
)


target_link_libraries(SimpleRT ${GLUT_LIBRARIES} ${GLUT_glut_LIBRARY} ${OpenCL_LIBRARIES})

# load the kernel automatically

add_custom_command(TARGET SimpleRT POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${RT_DIR}/kernel
$<TARGET_FILE_DIR:SimpleRT>/)

add_custom_command(TARGET SimpleRT POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${RT_DIR}/include
$<TARGET_FILE_DIR:SimpleRT>/)

if (WIN32)
    #set(WK_DIR "$<TARGET_FILE_DIR:Radiance>")
    set_target_properties(SimpleRT PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR})
endif()

