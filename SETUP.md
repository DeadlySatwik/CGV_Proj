# CityTrafficSimulation Environment Requirements

This project is a C++11 OpenGL traffic simulator that uses X11 on Linux and external text files for map loading.

## Required system packages on Ubuntu

Run this once to install everything needed to build and run the project:

```bash
./install_project_deps.sh
```

If you prefer installing manually:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  git \
  pkg-config \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  freeglut3-dev \
  libglew-dev \
  libglfw3-dev \
  libx11-dev \
  libxi-dev \
  libxrandr-dev \
  libxinerama-dev \
  libxcursor-dev \
  mesa-utils
```

## What these provide

- `build-essential`: compiler, linker, and basic build tools
- `cmake`: modern build system support
- `git`: version control
- `pkg-config`: helps detect library flags
- `libgl1-mesa-dev`, `libglu1-mesa-dev`: OpenGL headers and Mesa development files
- `freeglut3-dev`: GLUT development headers and libraries
- `libglew-dev`: GLEW OpenGL extension loader
- `libglfw3-dev`: GLFW window/input library
- `libx11-dev`, `libxi-dev`, `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`: X11 development libraries
- `mesa-utils`: useful for checking OpenGL support with `glxinfo`

## Build standard

The project should be compiled with:

- C++11
- OpenGL
- X11 on Linux

## Quick build check

```bash
make clean
make -j$(nproc)
./traffic
```

## Notes

- Run the executable from the repository root so the loader can find `exampleRoad.txt` and `exampleRightOfWay.txt`.
- The project loads external text files, so moving the executable away from the data files may break startup.
