#!/usr/bin/env bash
set -euo pipefail

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

echo "Installed project dependencies successfully."
