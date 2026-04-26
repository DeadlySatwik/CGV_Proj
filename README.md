# City Traffic Simulation (AWAS Integration)

A high-fidelity 3D traffic simulation written in C++ and OpenGL. This engine simulates complex city grids, intelligent intersection yielding, traffic light management, and now includes a massive **Grade-Separated 3D AWAS Interchange**.

## Features Overview

### 🚦 Fully Implemented & Working Features

- **Custom Map Engine:** The simulator builds the 3D environment entirely from `exampleRoad.txt` (topology) and `exampleRightOfWay.txt` (intersection priorities).
- **AWAS 3D Cloverleaf Bridge Integration:**
  - Successfully merged modern OpenGL pipeline (`Model` & `Shader` loading via Assimp) alongside the legacy fixed-function city renderer.
  - Dynamically scaled and translated the `.glb` bridge asset to fit perfectly into the city grid at the eastern boundary (`A7` to `E7`).
  - **Bezier Curve AI Pathing:** Vehicles natively navigate the 3D space, realistically curving up and over the off-ramps and underpasses rather than clipping through them.
- **Advanced Intersection Management:**
  - Nodes strictly enforce valid 2-to-4 street limits.
  - Vehicles yield right-of-way using rule-based priorities (`HA`, `VA`, `HB`, etc.).
  - Gridlock and starvation recovery mechanisms automatically flush deadlocked intersections.
- **Player Driveable Vehicles:** 
  - Seamlessly switch to 3rd-person mode to drive the **Car (1)**, **Bus (2)**, or **Bike (3)** using the Arrow Keys or IJKL.
- **Dynamic Time of Day:** 
  - Complete cycle handling Night, Dawn, Morning, Noon, Afternoon, Evening, and Dusk.
  - Fog, sky colors, and ambient lighting continuously interpolate over time.

### 🚧 Partially Implemented / Future Work

- **RL (Reinforcement Learning) Traffic Controller:** 
  - The C++ side (`RLTrafficClient.cpp`) is fully written and attempts to connect to a Python ML server via TCP socket (`localhost:9999`).
  - *Current Status:* Since the server isn't provided, it gracefully falls back to local timer-based traffic light management (logging "RL Fallbacks").
- **Dynamic AI Routing:** 
  - Vehicles currently choose random valid exits at intersections or follow strict priority paths. A true global A* pathfinding algorithm is stubbed but not actively used for individual vehicle trip planning.

## Installation & Build

### Dependencies
The engine requires several system-level libraries:
- OpenGL & GLU
- GLFW3 & GLEW
- Assimp (for the AWAS 3D model)
- GLM (mathematics)
- X11 (for Linux windowing)

You can install these on Ubuntu/Debian using the provided script:
```bash
cd without_traffic_light
chmod +x install_project_deps.sh
./install_project_deps.sh
cd ..
```

### Compilation
The project uses a standard Makefile. From the root directory, compile with all cores:
```bash
make clean && make -j$(nproc)
```

## Running the Simulation

Start the simulation by running the executable:
```bash
./traffic
```

### Controls

| Key | Action |
| --- | --- |
| `W`, `A`, `S`, `D` | Move Camera (Free look) |
| `Q`, `E` | Move Camera Vertically |
| `Left Click + Drag`| Rotate Camera |
| `F` | Toggle 3rd-person Drive Mode |
| `1`, `2`, `3` | Switch Vehicle (Car, Bus, Bike) |
| `Arrow Keys` | Drive (in 3rd person mode) |
| `T`, `Y` | Decrease / Increase Simulation Speed |
| `G`, `H` | Decrease / Increase Time scale (Day/Night) |
| `ESC` | Exit Simulation |

---
*Copyright (C) DeadlyS 2026*
