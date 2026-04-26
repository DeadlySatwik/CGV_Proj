cmake_minimum_required(VERSION 3.14)
project(CGV_Traffic_Sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Silence OpenGL policy warning and prefer modern GLVND libraries.
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()
set(OpenGL_GL_PREFERENCE GLVND)

# Find OpenGL, GLFW, and GLEW
find_package(OpenGL REQUIRED)
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
endif()

find_package(GLEW REQUIRED)

# GLM is header-only; use system package when available, otherwise fetch it.
find_package(glm CONFIG QUIET)
if (NOT glm_FOUND)
    include(FetchContent)
    message(STATUS "GLM package not found. Fetching GLM from source...")

    FetchContent_Declare(
        glm_ext
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glm_ext)
endif()

# Prefer system Assimp package. If not available, fetch and build Assimp locally.
find_package(assimp CONFIG QUIET)
if (NOT assimp_FOUND)
    include(FetchContent)
    message(STATUS "Assimp package not found. Fetching assimp from source...")

    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        assimp_ext
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(assimp_ext)
endif()

add_executable(TrafficSim src/main.cpp)

target_include_directories(TrafficSim PRIVATE
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)

if (TARGET assimp::assimp)
    set(ASSIMP_TARGET assimp::assimp)
elseif (TARGET assimp)
    set(ASSIMP_TARGET assimp)
else()
    message(FATAL_ERROR "Assimp target not available after discovery/build.")
endif()

target_link_libraries(TrafficSim
    ${OPENGL_LIBRARIES}
    glfw
    GL
    m
    X11
    pthread
    dl
    Xrandr
    Xi
    ${GLEW_LIBRARIES}
    glm::glm
    ${ASSIMP_TARGET}
)
