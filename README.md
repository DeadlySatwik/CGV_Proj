# City Traffic Simulation (AWAS Integration)

A high-fidelity 3D traffic simulation written in C++ and OpenGL. This engine simulates complex city grids, intelligent intersection yielding, traffic light management, and includes a comprehensive **Driver Training & Examination** system.

## Features Overview

### 🚦 Fully Implemented & Working Features

- **Driver Training Mode (R):** 
  - Interactive lessons including **General Driving**, **Quiet Zone** management, and **Precision Parking**.
  - Visual objective markers (Quiet Zones, Parking Boxes) rendered in the 3D world.
  - Interactive triggers: Press **P** to submit a parking attempt once inside the target zone.
- **Driver Exam Mode (R):**
  - Scripted examination routes defined via configuration files (`exampleExam.txt`).
  - Strict evaluation: Automatic failure on timeout, score depletion, or critical violations.
  - **Automatic Reports:** Generates a detailed Markdown (`.md`) performance report in the `reports/` folder upon completion.
- **Rule Enforcement Engine:**
  - Real-time detection of: Red Light violations, Stop Sign jumping, Wrong-way driving, and Failure to Yield at intersections.
  - Dynamic scoring system that rewards perfect maneuvers and penalizes infractions.
- **AWAS 3D Cloverleaf Bridge Integration:**
  - Assimp-loaded 3D bridge asset integrated into the city grid.
  - Bezier Curve AI pathing for realistic ramp navigation.
- **Player Driveable Vehicles:** 
  - Switch between **Car (1)**, **Bus (2)**, and **Bike (3)**.
  - Full driving physics with horn (`H`) and collision detection.

### 🚧 Partially Implemented / Future Work

- **RL (Reinforcement Learning) Traffic Controller:** 
  - Logic is ready (`RLTrafficClient.cpp`) to connect to a Python ML server.
  - Falls back to local timer-based management if no server is detected.
- **Dynamic AI Routing:** 
  - AI chooses random valid paths; a global A* navigation system is ready for future integration.

## Installation & Build

### Compilation
The project uses a standard Makefile. From the root directory:
```bash
make clean && make -j$(nproc)
```

## Running the Simulation
```bash
./traffic
```

### Controls

| Key | Action |
| --- | --- |
| `W`, `A`, `S`, `D` | Move Camera / Camera Control |
| `Q`, `E` / `Space` | Vertical Camera Movement |
| `F` | Toggle 3rd-person Drive Mode |
| `Arrow Keys` / `IJKL` | Drive Vehicle (in 3rd person) |
| `H` | Honk Horn |
| `P` | Park (in Training/Exam mode) |
| `1`, `2`, `3` | Switch Vehicle (Car, Bus, Bike) |
| `R` | Cycle Game Modes (Basic -> Training -> Exam) |
| `V` | Cycle Training Lessons |
| `O` | Toggle Perspective / Orthographic Projection |
| `[`, `]` | Decrease / Increase Time Scale |
| `N`, `M` | Next Day Phase / Toggle Time Flow |
| `I` | Print Telemetry to Console |
| `ESC` | Exit Simulation |

---
*Copyright (C) DeadlyS 2026*
