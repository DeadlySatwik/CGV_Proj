cmake_minimum_required(VERSION 3.10)
project(CGV_Traffic_Sim)

set(CMAKE_CXX_STANDARD 17)

# Find OpenGL, GLFW, and GLEW
find_package(OpenGL REQUIRED)
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
endif()

find_package(GLEW REQUIRED)
find_package(assimp REQUIRED)

include_directories(${OPENGL_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS})

add_executable(TrafficSim src/main.cpp)
target_link_libraries(TrafficSim ${OPENGL_LIBRARIES} glfw GL m X11 pthread dl Xrandr Xi ${GLEW_LIBRARIES} assimp)
