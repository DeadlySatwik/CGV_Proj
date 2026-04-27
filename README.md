# City Traffic Simulation

A C++11 OpenGL city traffic simulator with:

- Data-driven road network loading from text files
- AI traffic with right-of-way and traffic-light control
- Player-driveable car, bus, and bike
- Training and exam modes with scoring and reports
- Day/night cycle with dynamic lighting and fog
- Elevation-aware road segments and bridge transitions
- Optional RL traffic-light policy server integration

This README is the authoritative usage and architecture guide for the current codebase in this repository.

## Table of Contents

1. Project Goals
2. What You Get
3. Repository Structure
4. Build and Runtime Requirements
5. Build and Run
6. Controls (Current)
7. Simulation Modes
8. Driving Model
9. Traffic and Intersection Logic
10. Day/Night and Environmental Systems
11. Map, Right-of-Way, and Exam File Formats
12. RL Integration
13. Runtime Outputs and Reports
14. Tuning Guide
15. Known Limitations
16. Troubleshooting
17. License and Credits

## 1. Project Goals

The simulator is designed to combine:

- Playable third-person driving
- Rule-constrained traffic flow
- Procedural city ambience
- Deterministic fallback behavior when optional systems fail

It targets practical simulation behavior rather than photorealism.

## 2. What You Get

### Core Engine

- Fixed-function OpenGL renderer
- Cross-platform engine core (Linux and Windows backends)
- Frame loop with separate per-frame and sub-step simulation updates

### City Simulation

- Streets, intersections, traffic lights, garages, and static props
- Vehicle spawning and retirement
- Lane-aware spacing and intersection crossing state machines
- Jam/starvation mitigation at intersections

### Player Features

- Switchable vehicle classes: car, bus, bike
- Third-person driving mode
- Free camera mode
- Collision and road-boundary constraints

### Learning/Evaluation Modes

- Training mode with penalties/warnings/objectives
- Exam mode with checkpoints, timer, pass threshold, and report export

### Time and Atmosphere

- World clock and day phases
- Interpolated sky, fog, and light changes
- Night-aware visual behavior (streetlights, windows, vehicle lights)

### Optional RL Controller

- TCP client in C++ for traffic-light action requests
- Python server bridge with heuristic fallback policy
- Automatic C++ fallback when RL is unavailable

## 3. Repository Structure

Top-level important paths:

- `src/`
- `src/main.cpp`: app entry point
- `src/simulator/`: simulation and rendering systems
- `src/simulator/Simulator.*`: top-level game loop logic and input
- `src/simulator/Road.*`: roads, crosses, traffic lights, right-of-way
- `src/simulator/Vehicle.*`: AI vehicles
- `src/simulator/PlayerCar.*`: player vehicles and driving dynamics
- `src/simulator/Garage.*`: spawn/delete logic and garage visuals
- `src/simulator/TrainingManager.*`: training rules/objectives
- `src/simulator/ExamManager.*`: exam lifecycle and report generation
- `src/simulator/ModeManager.*`: BASIC/TRAINING/EXAM mode switching
- `src/simulator/EngineCore/`: platform loop, input, graphics helpers
- `exampleRoad.txt`: map topology and object definitions
- `exampleRightOfWay.txt`: intersection ordering and right-of-way mapping
- `exampleExam.txt`: exam settings and checkpoints
- `rl/traffic_rl_server.py`: TCP server returning traffic-light actions
- `rl/train_ppo.py`: basic PPO training stub
- `reports/`: generated exam reports
- `without_traffic_light/`: separate/legacy sandbox content

### 3.1 File-by-File Guide (What each file does)

This section explains each meaningful file in simple terms, with technical notes where useful.

Note on generated artifacts:

- Files like `.o`, `traffic`, `traffic.exe`, and most files inside `without_traffic_light/build/` are build outputs and are not hand-maintained source logic.
- `reports/*.md` are generated runtime outputs from exam sessions.

#### Root-level files

- `.gitignore`
	- Use: ignores temporary/build artifacts.
	- How: excludes object files (`*.o`) and built executable names so they are not committed.

- `Makefile`
	- Use: builds the root simulation executable (`traffic`).
	- How: compiles all C++ translation units under `src/` and links against OpenGL/X11 on Linux (or Win32 OpenGL libs on Windows).

- `install_project_deps.sh`
	- Use: one-command dependency installer for Ubuntu/Debian.
	- How: installs compiler/toolchain and OpenGL/X11 related development packages via `apt`.

- `README.md`
	- Use: primary technical and user documentation (this file).

- `SETUP.md`
	- Use: setup-oriented notes for environment preparation.

- `implementation_plan.md`
	- Use: design/roadmap notes for phase-wise features and robustness goals.
	- How: documents intended implementation behavior (time phases, jam handling, bridge logic, etc.).

- `exampleRoad.txt`
	- Use: map topology and object definitions loaded at startup.
	- How: line-based DSL declaring intersections, streets, garages, and props.

- `exampleRightOfWay.txt`
	- Use: intersection street ordering/right-of-way mapping.
	- How: per-intersection street list that seeds yield/priority tables.

- `exampleExam.txt`
	- Use: exam configuration values and checkpoint positions.
	- How: directive-based config parsed by `ExamManager`.

- `traffic` / `traffic.exe`
	- Use: compiled executables.
	- How: produced by build process; not source.

#### Root folders

- `src/`
	- Use: authoritative simulation source code.

- `rl/`
	- Use: optional reinforcement learning server/training scripts for traffic light decisions.

- `reports/`
	- Use: generated exam results in Markdown.

- `Phases/`
	- Use: screenshot history of development milestones.

- `without_traffic_light/`
	- Use: separate experimental/legacy OpenGL+CMake sandbox variant.

#### `Phases/` files

- `Phases/0.1/firstPhase.png`, `Phases/0.1/screenshot_2.png`, `Phases/0.1/screenshot_3.png`, `Phases/0.1/screenshot_4.png`
	- Use: visual snapshot set for phase 0.1.

- `Phases/1.0/secondPhase.png` and `Phases/1.0/Screenshot from ...` images
	- Use: visual record of phase 1.0 behavior.

- `Phases/2.0/Screenshot from ...` images
	- Use: visual record of phase 2.0 behavior.

- `Phases/3.0/Screenshot from ...` images
	- Use: visual record of phase 3.0 behavior.

#### `rl/` files

- `rl/traffic_rl_server.py`
	- Use: TCP bridge server that accepts state from C++ and returns an action.
	- How: parses one-line numeric state, runs PPO policy (or heuristic fallback), responds with discrete action `0..3`.

- `rl/train_ppo.py`
	- Use: baseline PPO training stub.
	- How: trains against a synthetic `gymnasium` environment and writes `ppo_traffic.zip` model files.

#### `src/` top-level file

- `src/main.cpp`
	- Use: application entry.
	- How: sets command-line args, prints controls, loads road and right-of-way files, and starts `Simulator`.

#### `src/simulator/` files

- `src/simulator/GameObject.h`
	- Use: abstract/common object interface for all world entities.
	- How: defines shared transform/state methods used polymorphically.

- `src/simulator/GameObject.cpp`
	- Use: base object behavior implementation.
	- How: provides draw/update wrappers and common object operations.

- `src/simulator/Simulator.h`
	- Use: central orchestrator class declaration.
	- How: stores camera/player/world-time/mode state and declares input-update-render pipeline methods.

- `src/simulator/Simulator.cpp`
	- Use: main runtime behavior implementation.
	- How: handles controls, world updates, object lifecycle, day/night transitions, camera modes, projection, telemetry.

- `src/simulator/ObjectsLoader.h`
	- Use: data loader API for road/right-of-way files.
	- How: defines parsing entry points and callback hooks for created objects.

- `src/simulator/ObjectsLoader.cpp`
	- Use: parser implementation for map and right-of-way DSL files.
	- How: tokenizes each line, instantiates typed objects, resolves cross references, configures intersections.

- `src/simulator/Road.h`
	- Use: declarations for roads (`Driveable`), intersections (`Cross`), and signalized intersections (`CrossLights`).
	- How: defines geometry access, lane queues, right-of-way structures, telemetry counters, and light-control state.

- `src/simulator/Road.cpp`
	- Use: road/intersection geometry and traffic-control logic implementation.
	- How: draws elevated/flat road surfaces, computes free space, controls cross passing rules, handles starvation/gridlock recovery, runs RL-assisted light actions.

- `src/simulator/Vehicle.h`
	- Use: base AI vehicle declaration and class variants (`Car`, `Bus`, `Bike`).
	- How: defines movement specs, cross transition state machine, blinkers, and bridge anomaly telemetry.

- `src/simulator/Vehicle.cpp`
	- Use: AI vehicle behavior implementation.
	- How: applies speed control, spacing/braking, cross registration/permission, lane transitions, and ramp pitch/height corrections.

- `src/simulator/PlayerCar.h`
	- Use: player-controlled vehicle class declarations.
	- How: defines input flags, camera helpers, and subclasses for player bus and bike.

- `src/simulator/PlayerCar.cpp`
	- Use: player driving and rendering implementation.
	- How: applies acceleration/braking/steering dynamics, vehicle-specific tuning, and third-person camera offsets.

- `src/simulator/Garage.h`
	- Use: vehicle spawn/deletion source declaration.
	- How: defines garage timing, limits, gate animation states, and vehicle factory method interface.

- `src/simulator/Garage.cpp`
	- Use: garage logic and procedural building visuals.
	- How: modulates spawn rates by day phase, manages queue/gate behavior, and draws style-varied buildings/windows.

- `src/simulator/Environment.h`
	- Use: decorative prop declarations (tree, lamppost, bench, dustbin).
	- How: defines per-prop draw/update interfaces.

- `src/simulator/Environment.cpp`
	- Use: decorative prop drawing implementation.
	- How: procedurally varies tree/bench look and toggles lamp glow by day phase.

- `src/simulator/ModeManager.h`
	- Use: mode state declaration (`BASIC_DRIVING`, `TRAINING`, `EXAM`).
	- How: exposes mode switching and access to training/exam managers.

- `src/simulator/ModeManager.cpp`
	- Use: mode transition implementation.
	- How: cycles modes, resets managers appropriately, and triggers exam loading/start.

- `src/simulator/TrainingManager.h`
	- Use: training mode rules declaration.
	- How: defines score/warning/objective state, lesson types, and rule-check methods.

- `src/simulator/TrainingManager.cpp`
	- Use: training mode rule logic implementation.
	- How: checks speeding, wrong-way, yield/light/stop behavior, quiet-zone horn penalties, and parking challenge completion.

- `src/simulator/ExamManager.h`
	- Use: exam mode declaration.
	- How: defines exam state, checkpoints, scoring thresholds, and report API.

- `src/simulator/ExamManager.cpp`
	- Use: exam lifecycle implementation.
	- How: parses exam config, tracks timer/checkpoints/score, records violations, and exports pass/fail report markdown.

- `src/simulator/RLTrafficClient.h`
	- Use: RL traffic-light client declaration.
	- How: defines socket connection, request/response API, and lifecycle methods.

- `src/simulator/RLTrafficClient.cpp`
	- Use: RL client networking implementation.
	- How: on Linux, sends state over TCP and parses action replies; on Windows, returns fallback invalid action.

- `src/simulator/RuleManager.h`
	- Use: legacy/parallel rule-manager declaration.
	- How: mirrors many checks now handled by `TrainingManager`.

- `src/simulator/RuleManager.cpp`
	- Use: legacy/parallel rule-manager implementation.
	- How: applies score penalties for speed, wrong-way, yield/stop/light violations; generally superseded by training/exam flow.

#### `src/simulator/EngineCore/` files

- `src/simulator/EngineCore/EngineCore.h`
	- Use: platform abstraction selector header.
	- How: includes Linux or Windows backend based on compile target.

- `src/simulator/EngineCore/EngineCoreBase.h`
	- Use: core engine-loop abstraction declaration.
	- How: defines virtual hooks for input/update/render and timing constraints.

- `src/simulator/EngineCore/EngineCoreBase.cpp`
	- Use: frame loop implementation.
	- How: executes event polling, computes delta, applies `timeScale`, performs `singleUpdate` and repeated `update` steps, then redraw/swap.

- `src/simulator/EngineCore/EngineCoreLinux.h`
	- Use: Linux backend declaration.
	- How: stores X11/GLX context data and event state.

- `src/simulator/EngineCore/EngineCoreLinux.cpp`
	- Use: Linux backend implementation.
	- How: creates X11 window/GL context, translates keyboard/mouse events, maintains held-key map, calls simulator callbacks.

- `src/simulator/EngineCore/EngineCoreWindows.h`
	- Use: Windows backend declaration.
	- How: declares Win32/OpenGL window and input management methods.

- `src/simulator/EngineCore/EngineCoreWindows.cpp`
	- Use: Windows backend implementation.
	- How: creates Win32 window/context, polls keyboard state, dispatches events to simulator hooks.

- `src/simulator/EngineCore/Graphics.h`
	- Use: rendering helper API declaration.
	- How: wraps common OpenGL immediate-mode operations (cube/quads/transform/color/normal helpers).

- `src/simulator/EngineCore/Graphics.cpp`
	- Use: rendering helper implementation.
	- How: provides utility draw methods and interpolation helpers used by object draw code.

- `src/simulator/EngineCore/Vec3.h`
	- Use: 3D vector math type declaration.
	- How: defines vector operations, interpolation, distances, normalization, cross product utilities.

- `src/simulator/EngineCore/Vec3.cpp`
	- Use: vector math implementation.
	- How: implements all geometric arithmetic used by roads, vehicles, cameras, and intersections.

- `src/simulator/EngineCore/Colors.h`
	- Use: color utility declarations.
	- How: provides helper methods (for example random color selection) for visuals.

- `src/simulator/EngineCore/Colors.cpp`
	- Use: color helper implementation.
	- How: generates/returns color constants used in vehicle/building drawing.

- `src/simulator/EngineCore/ExceptionClass.h`
	- Use: custom exception type declaration.
	- How: defines lightweight error wrapper used by parser/loader systems.

- `src/simulator/EngineCore/ExceptionClass.cpp`
	- Use: custom exception implementation.
	- How: stores/returns error messages for diagnostics.

#### `without_traffic_light/` files (separate sandbox variant)

- `without_traffic_light/CMakeLists.txt`
	- Use: CMake build definition for the alternate simulation variant.

- `without_traffic_light/src/main.cpp`
	- Use: alternate app entry for the sandbox.

- `without_traffic_light/src/Mesh.h`
	- Use: mesh container/attribute layout declarations for modern OpenGL path.

- `without_traffic_light/src/Model.h`
	- Use: model loading abstraction (Assimp-based workflow).

- `without_traffic_light/src/Shader.h`
	- Use: GLSL shader compile/link/use wrapper.

- `without_traffic_light/shaders/vertex.glsl`
	- Use: vertex transformation and per-vertex stage logic.

- `without_traffic_light/shaders/fragment.glsl`
	- Use: pixel color output stage logic.

#### Generated/runtime files and folders

- `src/simulator/*.o`, `src/simulator/EngineCore/*.o`
	- Use: object files produced during compilation.

- `without_traffic_light/build/` and its nested files (`CMakeCache.txt`, generated `Makefile`, dependency sub-build trees, etc.)
	- Use: CMake-generated build workspace and third-party dependency build outputs.

- `reports/exam_*.md`
	- Use: per-run exam result records generated by `ExamManager`.

## 4. Build and Runtime Requirements

### Compiler and Standard

- g++
- C++11

### Linux packages

Use the provided installer:

```bash
chmod +x install_project_deps.sh
./install_project_deps.sh
```

It installs:

- build-essential
- cmake
- git
- pkg-config
- libgl1-mesa-dev
- libglu1-mesa-dev
- freeglut3-dev
- libglew-dev
- libglfw3-dev
- libx11-dev
- libxi-dev
- libxrandr-dev
- libxinerama-dev
- libxcursor-dev
- mesa-utils

Note: the root Makefile currently links against `-lGL -lX11` and does not require a CMake build for the root simulator.

## 5. Build and Run

From repository root:

```bash
make clean
make -j$(nproc)
./traffic
```

The executable expects data files in the working directory, so run it from the repository root.

## 6. Controls (Current)

### Global

- `ESC`: exit simulation

### Camera (Free-fly mode)

- `W/A/S/D`: move camera
- `Q`: move down
- `E`: move up
- `Space`: move up
- `Mouse drag`: rotate camera

### Driving / View

- `F`: toggle free-fly and third-person driving mode
- `1`: switch to player car
- `2`: switch to player bus
- `3`: switch to player bike

In third-person driving mode:

- `Arrow Up`: accelerate
- `Arrow Down`: brake/reverse
- `Arrow Left`: steer left
- `Arrow Right`: steer right

### Simulation pacing

- `Y`: increase updates-per-frame
- `T`: decrease updates-per-frame
- `]`: increase global time scale
- `[`: decrease global time scale

### Time-of-day controls

- `N`: jump to next day phase boundary
- `M`: pause/resume world clock flow

### Modes and diagnostics

- `R`: cycle mode (BASIC -> TRAINING -> EXAM -> BASIC)
- `V`: cycle training lesson (only in TRAINING)
- `P`: attempt parking completion (only in PARKING lesson)
- `I`: print telemetry snapshot
- `O`: toggle projection (Perspective/Orthographic)

## 7. Simulation Modes

Mode switching is handled by `ModeManager`.

### BASIC_DRIVING

- Free roam
- No training/exam scoring effects

### TRAINING

Uses `TrainingManager` checks:

- Speed limit penalty over sustained overspeed
- Wrong-way penalty after threshold duration
- Yield/stop-sign/light compliance checks
- Quiet zone lesson with anti-honking rule
- Parking lesson with target marker and completion action

### EXAM

Uses `ExamManager` on top of training signals:

- Time limit
- Pass score threshold
- Ordered checkpoints
- Final PASS/FAIL result
- Markdown report written to `reports/exam_YYYYMMDD_HHMMSS.md`

## 8. Driving Model

Player vehicles:

- `PlayerCar`: balanced baseline
- `PlayerBus`: lower acceleration/top speed, wider camera
- `PlayerBike`: higher agility and top speed, tighter camera

### Current steering intent

The steering model is speed-dependent:

- Stronger authority at very low speeds to allow tighter cornering
- Dampened authority at higher speeds for stability

Position update and constraints:

- Vehicle heading determines forward motion direction
- Position sampled against drivable geometry
- Out-of-road or blocked moves are reverted

## 9. Traffic and Intersection Logic

### Vehicle AI

AI vehicles:

- Follow lane distance rules
- Register with upcoming intersections
- Request crossing permission
- Reserve target-road space before committing
- Transition road -> cross -> next road with continuity checks

### Right-of-way

At non-light intersections (`Cross`):

- Yield relationships are encoded per street and turn
- Vehicles pass only when yielding constraints are satisfied
- Round-robin and forced-pass recovery prevent persistent deadlock

### Traffic lights

At signalized intersections (`CrossLights`):

- Phase-controlled priorities (NS/EW)
- Decision interval updates
- Starvation and gridlock recovery states
- RL-requested action support with safe fallback

### Elevation and bridge robustness

Road segments can have non-zero endpoint heights:

- Height interpolation along segment
- Ramp pitch-aware orientation updates
- Collision layer tagging (ground/elevated)
- Anomaly counter increments if vehicle height is corrected

## 10. Day/Night and Environmental Systems

World time:

- Maintained in hours [0, 24)
- Configured by world-time speed and pause toggle

Phases:

- Night
- Dawn
- Morning
- Noon
- Afternoon
- Evening
- Dusk

Dynamic visuals:

- Sky color interpolation
- Ambient and diffuse light updates
- Directional light movement
- Fog color and distance interpolation
- Ground color interpolation

Environment behaviors:

- Lamppost glow depends on phase
- Vehicle light appearance depends on phase
- Building windows switch day/night appearance

## 11. Map, Right-of-Way, and Exam File Formats

### 11.1 Road/map file (`exampleRoad.txt`)

Supported object declarations:

- `CR <id> <x> <y> <z>`: unsignaled intersection
- `CL <id> <x> <y> <z>`: signalized intersection
- `ST <id> <begCrossId> <endCrossId>`: street segment
- `GA <id> <vehType> <crossId> <x> <y> <z> <spawnFreq> <maxVehicles>`: garage
- `TR <id> <x> <y> <z>`: tree
- `LP <id> <x> <y> <z>`: lamppost
- `BN <id> <x> <y> <z>`: bench

Garage vehicle type tokens:

- `C` or `CAR`
- `B` or `BUS`
- `K` or `BIKE`

Notes:

- IDs must be unique
- Crosses must exist before streets/garages reference them
- `y` on crosses supports elevated nodes/bridge sections

### 11.2 Right-of-way file (`exampleRightOfWay.txt`)

Per-line format:

```text
<crossId> <streetCount> <streetId0> <streetId1> ...
```

Rules:

- `streetCount` must match the real connected street count (2..4)
- Listed street IDs are mapped into default priority/yield templates

### 11.3 Exam file (`exampleExam.txt`)

Supported directives:

- `TIME_LIMIT <seconds>`
- `PASS_SCORE <score>`
- `START_SCORE <score>`
- `CHECKPOINT <x> <y> <z>`

If loading fails, exam manager falls back to built-in defaults.

## 12. RL Integration

### C++ side

`RLTrafficClient` sends one-line state payloads:

```text
carsN carsS carsE carsW phase greenDuration reward
```

and expects an integer action response `0..3`.

Actions:

- `0`: keep current phase
- `1`: switch phase
- `2`: increase green duration
- `3`: decrease green duration

### Python side

Run server:

```bash
python3 rl/traffic_rl_server.py --host 127.0.0.1 --port 5555 --model rl/ppo_traffic.zip
```

Behavior:

- Loads PPO model if available and `stable-baselines3` is installed
- Otherwise uses built-in heuristic policy

Training stub:

```bash
python3 rl/train_ppo.py --steps 200000 --out rl/ppo_traffic
```

Dependencies for RL scripts (Python side):

- python3
- numpy
- gymnasium
- stable-baselines3

### Failure handling

If RL responses fail repeatedly, C++ automatically:

- Disables RL for a cooldown window
- Switches to deterministic signal actions
- Tracks fallback telemetry counters

## 13. Runtime Outputs and Reports

Console startup includes:

- Controls summary
- Initial day phase/time

Telemetry (`I`) prints mode-aware snapshots including:

- Time and phase
- Mode state
- Active vehicle estimate
- Objective/warnings/scores when relevant

Exam reports:

- Generated in `reports/`
- Include result, score, time usage, checkpoint progress, and violations

## 14. Tuning Guide

Common tuning files and parameters:

- `src/simulator/PlayerCar.cpp`
- `turnSpeed`, `acceleration`, `friction`, low-speed steering boost
- `src/simulator/EngineCore/EngineCoreBase.cpp`
- `timeScale`, `updatesPerFrame` defaults
- `src/simulator/Garage.cpp`
- Spawn frequency and day-phase multipliers
- `src/simulator/Road.cpp`
- Cross recovery thresholds and traffic-light control windows

Recommended process:

1. Adjust one subsystem at a time
2. Rebuild and test in both free roads and tight intersections
3. Confirm no mode regressions (BASIC/TRAINING/EXAM)

## 15. Known Limitations

- Rendering is fixed-function and not physically-based
- AI routing is local/intersection-driven, not global shortest-path navigation
- Linux key handling relies on X11 translation logic
- Root simulator build uses direct Makefile flow; separate folders may have different build systems and assumptions

## 16. Troubleshooting

### Build fails on missing GL/X11 symbols

- Re-run dependency installer
- Confirm Linux dev packages are installed

### Executable starts then exits or cannot find map data

- Run `./traffic` from repository root
- Ensure `exampleRoad.txt` and `exampleRightOfWay.txt` are present

### Arrow keys not affecting driving

- Make sure you are in third-person mode (`F`)
- Verify window focus is on simulator window

### RL integration not active

- Ensure Python server is running on host/port expected by C++ client
- If unavailable, simulator safely falls back to deterministic control

### Exam report not generated

- Ensure `reports/` is writable
- Confirm exam reached pass/fail terminal state

## 17. License and Credits

- Copyright (C) DeadlyS 2026

If you evolve the simulator, keep this README aligned with code behavior so controls and tuning expectations remain accurate.
