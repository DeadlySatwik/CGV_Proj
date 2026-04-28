# ExplainME — AWAS City Traffic Simulator: Architecture & Core Logic

A comprehensive technical breakdown of how this C++ / OpenGL traffic simulator works, from rendering primitives to intersection AI to rule enforcement.

---

## Table of Contents
1. [Graphics Pipeline — No GLUT, Pure X11 + GLX](#1-graphics-pipeline)
2. [The Game Loop](#2-the-game-loop)
3. [World Construction — Roads & Intersections](#3-world-construction)
4. [NPC Vehicle AI — How Cars Drive Themselves](#4-npc-vehicle-ai)
5. [Intersection Management — How Collisions Are Prevented](#5-intersection-management)
6. [Traffic Lights — Timer, RL, and Recovery Systems](#6-traffic-lights)
7. [Vehicle Rendering — Fixed-Function OpenGL](#7-vehicle-rendering)
8. [Day/Night Cycle](#8-daynight-cycle)
9. [Player Driving System](#9-player-driving-system)
10. [Rule Enforcement Engine](#10-rule-enforcement-engine)
11. [Training & Exam Mode](#11-training--exam-mode)
12. [Pedestrian System](#12-pedestrian-system)

---

## 1. Graphics Pipeline


This project does use GLUT or GLFW, if the form. It uses **raw X11 windowing** and **GLX** for OpenGL context creation on Linux, and the Win32 API on Windows.

**File:** `EngineCoreLinux.cpp`

```
X11 creates the window → GLX creates the OpenGL context → game loop runs
```

Key steps in `EngineCore::init()`:
1. `XOpenDisplay()` — opens a connection to the X server
2. `glXChooseVisual()` — picks a visual with depth buffer (prefers double-buffered)
3. `glXCreateContext()` — creates the OpenGL rendering context
4. `XCreateWindow()` — creates the actual window
5. `glXMakeCurrent()` — binds the GL context to the window

The event loop in `checkEvents()` manually handles:
- `KeyPress` / `KeyRelease` — translated via `XLookupString` and `XK_*` keysyms
- `MotionNotify` — mouse drag for camera rotation
- `ConfigureNotify` — window resize

Frame timing uses `gettimeofday()` to compute real delta time between frames.

### The `Graphics` Wrapper Class

**File:** `Graphics.h`

Instead of calling raw `glBegin/glEnd/glTranslatef` everywhere, the project wraps them in a clean `Graphics` class:

| Method | GL Equivalent |
| :--- | :--- |
| `drawCube(x,y,z)` | 6 quads with normals |
| `translate(x,y,z)` | `glTranslatef` |
| `pushMatrix()` / `popMatrix()` | `glPushMatrix` / `glPopMatrix` |
| `setColor(r,g,b)` | `glColor3f` + `glMaterialfv` |
| `beginDraw(QUADS)` | `glBegin(GL_QUADS)` |

Everything is **immediate-mode OpenGL (fixed-function pipeline)**. There are no shaders, VBOs, or VAOs.

### Font Rendering

On Linux, fonts are loaded via `glXUseXFont()` using the X11 `"fixed"` font. Display lists are generated for ASCII characters 32–159, enabling `glCallLists()` for bitmap text rendering in the HUD overlay.

---

## 2. The Game Loop

**File:** `EngineCoreBase.cpp` → `performFrame()`

```
while (running) {
    delta = getDeltaTime();
    checkEvents();           // input polling
    
    for (i = 0; i < updatesPerFrame; i++) {
        singleUpdate(delta); // physics + AI tick
    }
    
    redraw();                // OpenGL draw calls
    swapBuffers();           // present to screen
}
```

- `singleUpdate()` calls `GameObject::update(delta)` on every registered object (roads, crosses, vehicles, garages).
- `redraw()` calls `GameObject::draw()` on every object, plus the environment, HUD, and player overlays.
- The time scale (`T`/`Y` keys) multiplies the delta before passing it to game logic.

---

## 3. World Construction — Roads & Intersections

### Map Files

The city is defined by two text files:

**`exampleRoad.txt`** — defines nodes (intersections) and edges (streets):
```
node A1 0 0        // Cross at position (0, 0)
node A2 4 0        // Cross at position (4, 0)
street HA1 A1 A2   // Street connecting A1 to A2
garage GA1 -2 0 A1 // Garage spawning into intersection A1
```

**`exampleRightOfWay.txt`** — defines yield priorities at each intersection:
```
A2 HA1 VA2 HA3 VA1   // Streets listed in counterclockwise priority order
```

### Class Hierarchy

```
GameObject (base)
├── Road (abstract)
│   ├── Driveable (has vehicles, direction, length)
│   │   ├── Street (road segment between two intersections)
│   │   └── Garage (vehicle spawner/despawner at map edge)
│   └── Cross (intersection node)
│       └── CrossLights (intersection with traffic lights)
└── Vehicle
    ├── Car
    ├── Bus
    └── Bike
```

### Key Concepts

- **`Driveable`** — A road segment with a direction vector, normal, length, and two vehicle queues (`vehiclesBeg`, `vehiclesEnd`) for traffic going in each direction.
- **`Cross`** — An intersection node. Stores a list of `OneStreet` structs, each pointing to a connected `Driveable` and its yield table.
- **`Garage`** — Extends `Driveable`. Spawns new vehicles at regular intervals and despawns vehicles that reach the map edge.

### Elevation (AWAS Bridge)

Each `Driveable` stores `begHeight` and `endHeight`. The height at any point along the road is computed using **smoothstep interpolation**:

```cpp
float u = t * t * (3.0f - 2.0f * t);  // smoothstep
return begHeight + (endHeight - begHeight) * u;
```

This creates smooth ramp transitions instead of jarring steps. Vehicles also get **pitch rotation** (`atan2(dH, length)`) to visually tilt on slopes.

---

## 4. NPC Vehicle AI — How Cars Drive Themselves

**File:** `Vehicle.cpp`

Every NPC vehicle is a state machine that drives along `Driveable` segments and negotiates intersections.

### The Update Cycle

Each frame, `Vehicle::update(delta)` runs:

```
1. Drive along current road  → xPos += velocity * delta
2. Compute safe velocity     → setVelocity()
3. Set 3D world position     → setNewPos() (lerp along road)
4. Register to intersection  → registerToCross() (when close enough)
5. Enter intersection        → tryBeAllowedToEnterCross()
6. Cross intersection        → setCornerPosition() (lerp + rotate)
7. Enter next road           → enterNewRoad()
```

### Velocity Control (Why Vehicles Never Crash)

The `setVelocity()` function is the **core anti-collision system**:

```cpp
float gap = getDst() - specs.remainDst;  // distance to car in front
float safeGap = vehicleLength * 0.8 + remainDst;

if (gap <= safeGap) {
    targetVelocity = 0.0;              // STOP — too close
} else {
    float followLimit = (gap - safeGap) / stopTime;
    targetVelocity = min(maxV, followLimit);  // proportional braking
}
```

**Key design:**
- Each road maintains a **queue** of vehicles (`vehiclesBeg`, `vehiclesEnd`).
- Every vehicle has a pointer to the **vehicle directly in front** (`frontVeh`).
- `getDst()` returns the gap to the front vehicle's rear bumper.
- Velocity is smoothly eased toward the target (no instant jumps).

This is essentially a **car-following model** — each vehicle matches the speed of the one ahead, with a safety buffer. If the gap shrinks, the vehicle decelerates proportionally. This guarantees no clipping.

### Position Calculation

Vehicles don't have free 3D movement. They are **parametrically constrained** to their road:

```cpp
float s = xPos / curRoad->getLength();  // 0.0 to 1.0
pos = Vec3::lerp(roadStart, roadEnd, s);
```

This means a vehicle can never leave its lane. It simply slides between the road's start and end points.

### Linked List of Vehicles

Each road has two `std::queue<Vehicle*>`:
- `vehiclesBeg` — vehicles traveling in the "begin" direction
- `vehiclesEnd` — vehicles traveling in the "end" direction

When a vehicle enters a road, it pushes itself onto the queue. When it leaves, it pops. The `frontVeh`/`backVeh` pointers form a doubly-linked chain for distance checks.

---

## 5. Intersection Management — How Collisions Are Prevented

**File:** `Road.cpp` → `Cross::updateCross()`

This is the most critical system. Every intersection (`Cross`) runs a state machine each frame:

### The Algorithm

```
1. Are any vehicles allowed through?         → allowedVeh > 0?
2. If not, try right-of-way based pass       → tryPassVehiclesWithRightOfWay()
3. If still none, try any vehicle             → tryPassAnyVehicle()
4. If stuck for >2.5 seconds, force unjam    → tryForceUnjamPass()
```

### Right-of-Way Yielding

Each street at an intersection has a **yield table**: `streets[i].yield[turn]` = list of street indices that have priority.

When a vehicle at street `i` wants to turn to street `turn`:
1. Look up `yield[turn]` — which streets does it need to yield to?
2. Check if any of those streets have vehicles waiting.
3. If all are clear → **grant permission** (`allowedToCross = true`).
4. If any have vehicles → **wait**.

### 2-Way Pass-Through Nodes

For simple 2-way connections (like bridge ramp connectors):
- `allowedVeh` is always reset to 0
- All waiting vehicles are immediately granted passage
- Cross speed is set to `maxV * 0.85` (near full speed) instead of `cornerVelocity`

This prevents artificial slowdowns at simple road junctions.

### Space Reservation

Before entering an intersection, the vehicle **reserves space** on the destination road:

```cpp
nextRoad->reservedSpaceBeg += reserveAmount;
```

This prevents two vehicles from being simultaneously granted permission to enter the same space on the next road.

### Gridlock Recovery

If `noGrantTimer > 2.5 seconds`, the intersection enters **force-unjam mode**:
- Uses a **round-robin cursor** to fairly grant access to a stuck street
- Counts as a `telemetryJamRecovery` for debugging

---

## 6. Traffic Lights

**File:** `Road.cpp` → `CrossLights`

`CrossLights` extends `Cross` with a phase-based signal system:

### Normal Operation

```
Phase NS (North-South green) → timer runs → switch → Phase EW (East-West green)
```

Each phase has configurable durations:
- `durationGreen1` / `durationGreen2` — green phase lengths
- `durationYellow1` / `durationYellow2` — yellow transition
- `durationBreak` — all-red gap between phases

### RL (Reinforcement Learning) Fallback

The system attempts to connect to a Python ML server via TCP:

```cpp
RLTrafficClient rlClient;  // connects to localhost:9999
```

If the server is available, it sends intersection state (vehicle counts per direction) and receives an action (which streets to make green). If the server is unreachable, it falls back to local timer-based control and increments `telemetryRLFallbacks`.

### Starvation & Gridlock Recovery

The `CrossLights` has a state machine with recovery modes:

| State | Trigger | Action |
| :--- | :--- | :--- |
| `CTRL_NORMAL` | Default | Timer-based or RL-based switching |
| `CTRL_STARVATION_RECOVERY` | One direction starved >8s | Force green for starved direction |
| `CTRL_GRIDLOCK_RECOVERY` | Total waiting >12 vehicles | Rapid cycling between all phases |
| `CTRL_FAILSAFE` | No grants for >4s | Allow all directions briefly |

---

## 7. Vehicle Rendering — Fixed-Function OpenGL

**File:** `Vehicle.cpp` → `Car::draw()`, `Bus::draw()`, `Bike::draw()`

Vehicles are drawn entirely from **scaled cubes** (`drawCube(x, y, z)`) composed with `pushMatrix/popMatrix/translate`:

### Car Structure (from bottom to top):
1. **Wheels** — 4 dark rubber cubes + silver rim accents
2. **Undercarriage** — dark shadow strip
3. **Main Body** — colored chassis + hood + trunk
4. **Side Skirts** — thin dark strips
5. **Cabin/Roof** — windshield (translucent blue) + roof
6. **Front Grille** — dark grey + chrome accents
7. **Headlights** — day/night aware (bright white at night, dim during day)
8. **Taillights** — red cubes, brighter when `isBraking`
9. **Blinkers** — orange cubes that flash on/off when turning

### Day/Night Awareness

Vehicles query `Simulator::getInstance().getDayPhase()`:
- During night/dawn/dusk → headlights render bright white
- During day → headlights render dim

### Blinker System

```cpp
struct Blinker {
    int which;       // -1=left, 0=none, +1=right
    bool isLighting; // toggles on/off
    float time;      // accumulator
    float duration;  // ~0.5 seconds
};
```

The blinker direction is determined by `rotateDirection(begRot, endRot)` when the vehicle registers at an intersection.

---

## 8. Day/Night Cycle

**File:** `Simulator.cpp`

The simulator maintains a virtual clock (`worldTime`, 0.0–24.0 hours). Time advances each frame:

```cpp
worldTime += (delta * timeSpeed) / 60.0f;
```

At `timeSpeed = 2.0`, one real second = 2 simulated minutes → a full 24h cycle in 12 real minutes.

### Seven Day Phases

| Phase | Hours | Sky Color | Characteristics |
| :--- | :--- | :--- | :--- |
| Night | 21:00–05:00 | Deep navy | Dim moonlight, close fog |
| Dawn | 05:00–07:00 | Warm peach | Low sun from east |
| Morning | 07:00–09:00 | Clear blue | Bright and warm |
| Noon | 09:00–15:00 | Bright blue | Full sunlight, far fog |
| Afternoon | 15:00–17:00 | Soft blue | Slightly dimmer |
| Evening | 17:00–19:00 | Golden | Low sun from west |
| Dusk | 19:00–21:00 | Purple | Rapidly dimming |

### Dynamic Lighting

Each frame, `updateSkyAndLighting()` interpolates between the current and next phase:

```cpp
glClearColor(skyR, skyG, skyB, 1.0);        // sky color
glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);  // ambient light
glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);  // sun color
glLightfv(GL_LIGHT0, GL_POSITION, sunDir);  // sun direction
glFogf(GL_FOG_START, fogStart);             // fog near
glFogf(GL_FOG_END, fogEnd);                 // fog far
```

Ground color, road color, and vehicle headlights all respond to the current phase.

---

## 9. Player Driving System

**File:** `PlayerCar.h/cpp`, `Simulator.cpp`

The player doesn't use the same lane-following system as NPCs. Instead, the player has **free 3D movement** constrained to road surfaces.

### Input System

When in 3rd-person mode (`F` key), arrow keys set input flags:

```cpp
INPUT_ACCEL       = 1  // Up arrow
INPUT_BRAKE       = 2  // Down arrow
INPUT_STEER_LEFT  = 4  // Left arrow
INPUT_STEER_RIGHT = 8  // Right arrow
INPUT_HORN        = 16 // H key
```

### Physics

```cpp
// Acceleration
if (inputMap & INPUT_ACCEL)
    speed += acceleration * delta;

// Steering (only when moving)
if (inputMap & INPUT_STEER_LEFT)
    heading -= turnSpeed * delta;

// Movement
pos.x += cos(heading) * speed * delta;
pos.z -= sin(heading) * speed * delta;
```

### Road Constraint

After computing the new position, `Simulator::isOnRoad()` checks if the player is still on a valid road surface. If not, the move is rejected. Height is sampled from the road below using `sampleRoadHeight()`.

Collision detection with NPC vehicles and props (trees, benches) uses simple distance checks in `isPlayerBlocked()`.

---

## 10. Rule Enforcement Engine

**File:** `TrainingManager.cpp`

The `TrainingManager` observes the player and issues penalties:

| Rule | Detection Method | Penalty |
| :--- | :--- | :--- |
| **Red Light** | Player crosses stop line while `CrossLights::isGreenLight()` returns false | -20 pts |
| **Stop Sign** | Player crosses unsignalized intersection without stopping for 0.5s | -20 pts |
| **Wrong-Way** | Player's heading vs road direction angle > 100° | -10 pts |
| **Speeding** | `player->speed > 2.5` sustained for >1.5s | -15 pts |
| **Yield** | Player enters intersection when higher-priority traffic is approaching | Warning |
| **Quiet Zone Horn** | Player honks inside quiet zone radius | -25 pts |
| **Pedestrian** | Player enters active crosswalk while pedestrians are crossing | -30 pts |

### Intersection Rule Check Logic

```cpp
for each Cross near the player:
    1. Get the player's closest street index
    2. If CrossLights → check isGreenLight(streetIdx)
    3. If regular Cross → check if player stopped for stopSignTimer >= 0.5s
    4. Check yield table: does any higher-priority street have approaching vehicles?
```

---

## 11. Training & Exam Mode

### Mode Architecture

```
ModeManager
├── BASIC_DRIVING  → No rules, free roam
├── TRAINING       → Rules active, lessons, scoring
└── EXAM           → Strict evaluation, checkpoints, timer, reports
```

Toggled with the `R` key. Modes cycle: Basic → Training → Exam → Basic.

### Training Mode

- **Lessons** cycled with `V`: General Driving, Quiet Zone, Parking, Pedestrian Safety
- **Parking Challenge**: Green target zone rendered on-road. Press `P` when stopped inside for +50 pts.
- **Quiet Zone**: Red circle rendered. Honking inside = -25 pts per honk.

### Exam Mode

**File:** `ExamManager.cpp`

- Loads a route from `exampleExam.txt` with checkpoints and objectives
- Timer counts down from `TIME_LIMIT`
- Score starts at `START_SCORE`, decreases on violations
- Auto-fails if: time runs out, score drops below `PASS_SCORE`, or player cancels
- On PASS: generates both `exam_report_<timestamp>.md` and `driving_license_<timestamp>.md`
- On FAIL: generates only the report
- Vehicle switching is **locked** during the exam

---

## 12. Pedestrian System

**File:** `PedestrianManager.cpp`, `Pedestrian.cpp`, `CrosswalkZone.cpp`

### Crosswalk Zones

Placed on specific streets at parameterized positions. Each `CrosswalkZone` has:
- A position on the road
- Associated `PedestrianSignal` (WALK / DON'T WALK)
- Visual rendering (striped crosswalk markings)

### Pedestrian AI

Pedestrians spawn at crosswalk edges and walk across when the signal is WALK:
1. Wait at curb until signal changes to WALK
2. Walk across at a random speed
3. Despawn on the other side
4. The `TrainingManager` penalizes the player for entering an active crosswalk while pedestrians are present.

---

## Summary of OpenGL Usage

| Feature | GL Functions Used |
| :--- | :--- |
| **Geometry** | `glBegin(GL_QUADS)`, `glVertex3f`, `glNormal3f`, `glEnd` |
| **Transforms** | `glPushMatrix`, `glPopMatrix`, `glTranslatef`, `glRotatef`, `glScalef` |
| **Materials** | `glColor3f`, `glMaterialfv` |
| **Lighting** | `glLightfv(GL_LIGHT0, ...)`, `glEnable(GL_LIGHTING)` |
| **Fog** | `glFogf(GL_FOG_START/END)`, `glFogfv(GL_FOG_COLOR)` |
| **Projection** | `glMatrixMode(GL_PROJECTION)`, `gluPerspective` / `glOrtho` |
| **Depth** | `glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)` |
| **Blending** | `glEnable(GL_BLEND)`, `glBlendFunc` (for HUD overlay) |
| **Text** | `glXUseXFont`, `glCallLists` (bitmap font rendering) |
| **Windowing** | X11 `XCreateWindow`, GLX `glXCreateContext` (NOT GLUT) |

---

*This document explains the core architecture of the AWAS City Traffic Simulator. Copyright (C) DeadlyS 2026*
