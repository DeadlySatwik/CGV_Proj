# RoadLogic — How the Data Files Drive the Simulation

This document explains the three configuration files that define the world, the traffic rules, and the exam objectives. All three are plain text files parsed at startup — **no recompilation required** to change the city.

---

## Table of Contents

1. [exampleRoad.txt — The City Blueprint](#1-exampleroadtxt--the-city-blueprint)
2. [exampleRightOfWay.txt — The Traffic Brain](#2-examplerightofwaytxt--the-traffic-brain)
3. [exampleExam.txt — The Exam Route](#3-exampleexamtxt--the-exam-route)
4. [How They Connect](#4-how-they-connect)

---

## 1. exampleRoad.txt — The City Blueprint

**Parsed by:** `ObjectsLoader::loadRoad()` in `ObjectsLoader.cpp`  
**Importance:** 🔴 Critical — without this file, the world is empty.

### What It Does

Every line declares a physical object in the 3D world. The parser reads each line, creates the corresponding C++ object, and registers it into the simulation.

### Supported Line Types

| Token | C++ Class Created | Format | Description |
|:---|:---|:---|:---|
| `CR` | `Cross` | `CR <id> <x> <y> <z>` | Unsignalized intersection (stop-sign/yield rules) |
| `CL` | `CrossLights` | `CL <id> <x> <y> <z>` | Signalized intersection (has traffic lights) |
| `ST` | `Street` | `ST <id> <begCrossId> <endCrossId>` | Road segment connecting two intersections |
| `GA` | `GarageCar/Bus/Bike` | `GA <id> <vehType> <crossId> <x> <y> <z> <spawnFreq> <maxVeh>` | Vehicle spawner at the map edge |
| `TR` | `Tree` | `TR <id> <x> <y> <z>` | Decorative tree |
| `LP` | `Lamppost` | `LP <id> <x> <y> <z>` | Street lamp (glows at night) |
| `BN` | `Bench` | `BN <id> <x> <y> <z>` | Decorative bench |

### Vehicle Type Tokens (for Garages)

| Token | Vehicle |
|:---|:---|
| `C` or `CAR` | Car |
| `B` or `BUS` | Bus |
| `K` or `BIKE` | Bike |

### The Current City Grid

The map defines a **5×7 grid** of intersections (rows A through E, columns 1 through 7):

```
       col1    col2    col3    col4    col5    col6    col7
      X=-12   X=-8    X=-4    X=0     X=4     X=8     X=12
       
Z=8:   A1 ——— A2 ——— A3 ——— A4* ——— A5 ——— A6 ——— A7       Row A
       |       |       |       |       |       |       |
Z=5:   B1 ——— B2 ——— B3 ——— B4* ——— B5 ——— B6 ——— B7       Row B
       |       |       |       |       |       |       |
Z=0:   C1*——— C2*——— C3*   C4EN    C5*——— C6*——— C7*      Row C
       |       |       |     ↕↕↕      |       |       |
Z=-5:  D1 ——— D2 ——— D3 ——— D4* ——— D5 ——— D6 ——— D7       Row D
       |       |       |       |       |       |       |
Z=-8:  E1 ——— E2 ——— E3 ——— E4* ——— E5 ——— E6 ——— E7       Row E

  * = has traffic lights (CL)       ↕↕↕ = AWAS Bridge (elevated)
```

**Key observations:**
- `——` = Horizontal street (`H` prefix, e.g. `HA1` connects A1→A2)
- `|` = Vertical street (`V` prefix, e.g. `VA1` connects A1→B1)
- Nodes marked `*` are `CL` (CrossLights) — they have automated traffic signals
- All others are `CR` (Cross) — they use right-of-way/yield rules

### The AWAS Bridge

The center column at X=0 has a special feature between rows B and D:

```
B4 (Y=0, Z=5)
  ↓  road VB4
C4EN (Y=0.55, Z=2.0)    ← elevated node (on-ramp)
  ↓  road VB4OV
C4ES (Y=0.55, Z=-2.0)   ← elevated node (off-ramp)
  ↓  road VC4
D4 (Y=0, Z=-5)
```

- `C4EN` and `C4ES` have `Y=0.55`, which lifts them **above ground level**
- The road `VB4OV` between them is the **bridge deck** — vehicles drive over the C-row streets below
- Height is interpolated with **smoothstep** for smooth ramp transitions
- Vehicles get **pitch rotation** (`atan2`) on the ramps so they visually tilt

### Garage Examples Explained

```
GA G1  C  A1  -16 0 8   5  20
│   │  │  │    │       │  │
│   │  │  │    │       │  └─ max 20 vehicles alive at once
│   │  │  │    │       └─── spawn one every 5 seconds
│   │  │  │    └─────────── garage position (off-map, west edge)
│   │  │  └──────────────── connects into intersection A1
│   │  └─────────────────── vehicle type: Car
│   └────────────────────── unique ID
└────────────────────────── token type (Garage)
```

The 12 garages are placed at the **edges** of the map, feeding traffic into the grid from all four sides. Their spawn rate is modulated by the day/night cycle (slower at night).

---

## 2. exampleRightOfWay.txt — The Traffic Brain

**Parsed by:** `ObjectsLoader::loadRightOfWay()` in `ObjectsLoader.cpp`  
**Importance:** 🔴 Critical — without this file, intersections have no AI rules and vehicles deadlock.

### What It Does

For every intersection, this file defines the **order of priority** among its connected streets. The parser calls `cross->setDefaultPriority(s0, s1, s2, s3)` which builds a **yield table** — a matrix that tells each street which other streets it must yield to for each possible turn.

### Format

```
<crossId> <streetCount> <street0> <street1> <street2> [street3]
```

### How the Yield Table Is Built

The function `Cross::setDefaultPriority()` in `Road.cpp` takes the streets in the order listed and assigns yield relationships based on position.

#### For 4-Way Intersections (most common)

Take `B2 4 HB2 HB1 VB2 VA2`:

The streets are assigned **positions 0–3** in the listed order:
- Position 0: HB2 (east)
- Position 1: HB1 (west)  
- Position 2: VB2 (south)
- Position 3: VA2 (north)

The base yield template is:
```
Position 0: yields to nobody         → highest priority
Position 1: yields to nobody         → second priority
Position 2: yields to [position 1]   → must wait for position 1
Position 3: yields to [positions 1,2] → must wait for both
```

This template is then **rotated** for each street's perspective using modular arithmetic, so every street knows exactly who it must yield to when making each possible turn (left, right, straight, U-turn).

**Concrete example:** A car on VA2 (position 3) wanting to turn onto HB2 (position 0) must yield to vehicles on HB1 (position 1) and VB2 (position 2). If both are clear → the car proceeds.

#### For 3-Way Intersections (T-junctions)

Take `A3 3 HA3 HA2 VA3`:

A circular yield pattern is created:
- Street 0 (HA3) yields to street 1 (HA2) for some turns
- Street 1 (HA2) yields to street 2 (VA3) for some turns
- Street 2 (VA3) yields to street 0 (HA3) for some turns

#### For 2-Way Intersections (Pass-Through Nodes)

Take `C4EN 2 VB4 VB4OV`:

**No yield table is needed.** These are simple connectors (like the bridge ramp joints). The `updateCross()` method treats them specially:
```cpp
if (streets.size() <= 2) {
    allowedVeh = 0;
    tryPassVehiclesWithRightOfWay();  // immediately pass all
    allowedVeh = 0;                   // reset
    noGrantTimer = 0.0f;              // no jam detection needed
    return;
}
```
Vehicles pass through at near-full speed without stopping.

### The Decision Algorithm (Every Frame)

For each non-trivial intersection, `Cross::updateCross()` runs:

```
Step 1: Are vehicles currently crossing? → wait for them to finish
Step 2: Try right-of-way pass           → check yield tables
Step 3: If nobody qualifies, try ANY     → round-robin fairness
Step 4: If stuck > 2.5 seconds           → FORCE unjam pass
```

**Step 2 in detail** (`tryPassVehiclesWithRightOfWay`):
1. For each waiting vehicle at the intersection
2. Look up which turn it wants to make (`desiredTurn`)
3. Check `streets[i].yield[desiredTurn]` — list of street indices to yield to
4. If ALL those streets have no approaching vehicles → **grant passage**
5. The vehicle sets `allowedToCross = true` and enters the crossing

**Step 4 — Gridlock Recovery:** If no vehicle has been granted passage for 2.5 seconds, the intersection uses a **round-robin cursor** to forcefully let one vehicle through. This prevents permanent deadlock. Counted as `telemetryJamRecovery`.

### Traffic Lights Override

For `CrossLights` intersections (marked `CL` in the road file), the yield table is **overridden** by signal phases:

```
dontCheckStreet() returns true for RED streets
→ vehicles on RED streets are excluded from tryPassVehiclesWithRightOfWay()
→ only GREEN streets get checked
```

The light phases cycle automatically (NS green → yellow → EW green → yellow → repeat).

---

## 3. exampleExam.txt — The Exam Route

**Parsed by:** `ExamManager::loadExam()` in `ExamManager.cpp`  
**Importance:** 🟡 Important for Exam Mode — falls back to hardcoded defaults if missing.

### What It Does

Defines the parameters and route for the Driver's License Exam.

### Format

```
TIME_LIMIT <seconds>                          ← total exam duration
PASS_SCORE <score>                            ← minimum score to pass
START_SCORE <score>                           ← starting score
CHECKPOINT <x> <y> <z> [objective_name]       ← waypoint to reach
```

### Current Configuration

```
TIME_LIMIT 180                                ← 3 minutes
PASS_SCORE 70                                 ← need ≥70 to pass
START_SCORE 100                               ← begin with 100 points
CHECKPOINT -4 0 8 Navigate_Intersection       ← at intersection A3
CHECKPOINT 4 0 -5 Stop_at_Red_Light           ← at intersection D5
CHECKPOINT 8 0 8 Park_Safely                  ← at intersection A6
```

### The Checkpoint Coordinates on the Map

```
       X=-12   X=-8    X=-4    X=0     X=4     X=8     X=12
       
Z=8:   A1 ——— A2 ——— [CP1] —— A4 ——— A5 ——— [CP3] —— A7
       |       |       |       |       |       |       |
Z=5:   B1 ——— B2 ——— B3 ——— B4 ——— B5 ——— B6 ——— B7
       |       |       |       |       |       |       |
Z=0:   C1 ——— C2 ——— C3    bridge    C5 ——— C6 ——— C7
       |       |       |     ↕↕↕      |       |       |
Z=-5:  D1 ——— D2 ——— D3 ——— D4 ——— [CP2] —— D6 ——— D7
       |       |       |       |       |       |       |
Z=-8:  E1 ——— E2 ——— E3 ——— E4 ——— E5 ——— E6 ——— E7

[CP1] = Checkpoint 1 at (-4, 0, 8)  → "Navigate_Intersection"
[CP2] = Checkpoint 2 at (4, 0, -5)  → "Stop_at_Red_Light"
[CP3] = Checkpoint 3 at (8, 0, 8)   → "Park_Safely"
```

### How It Works at Runtime

1. **Exam starts:** Vehicle type is locked. Timer begins counting down from `TIME_LIMIT`.
2. **Navigation aids appear:**
   - **Golden ring** on the ground at the active checkpoint
   - **Tall beacon pillar** (gold line + diamond) rising above the checkpoint — visible from far away
   - **Green direction arrow** on the ground near the player, always pointing toward the checkpoint
3. **Reaching a checkpoint:** When the player drives within the checkpoint's radius, `currentCheckpointIndex++` and the next checkpoint becomes active.
4. **Violations:** The `TrainingManager` continues checking traffic rules. Each violation (red light, speeding, wrong-way, etc.) reduces the score. The `ExamManager` detects score drops and records them.
5. **End conditions:**
   - **PASS:** All checkpoints reached AND score ≥ `PASS_SCORE` → Report + Driving License generated
   - **FAIL (score):** Score drops below `PASS_SCORE` → Report only
   - **FAIL (time):** Timer reaches 0 → Report only
   - **FAIL (cancel):** Player presses `R` during exam → Report only

### HUD Display During Exam

```
┌──────────────────────────────┐
│  EXAM                        │
│  Status: IN PROGRESS         │
│  Time: 02:43                 │
│  Score: 100 / 70 pass        │
│  CP: [>>1] [2] [3]          │
│  Next: (-4, 8)               │
│  Obj: Navigate_Intersection  │
│                              │
│  R=Cancel Exam               │
└──────────────────────────────┘
```

---

## 4. How They Connect

The three files form a pipeline:

```
┌─────────────────┐     ┌──────────────────────┐     ┌─────────────────┐
│ exampleRoad.txt  │────▶│ exampleRightOfWay.txt │────▶│ exampleExam.txt │
│                  │     │                        │     │                 │
│ Creates the      │     │ Teaches intersections  │     │ Places goals    │
│ physical world:  │     │ HOW to manage traffic: │     │ ON the world:   │
│ - Intersections  │     │ - Yield tables         │     │ - Checkpoints   │
│ - Roads          │     │ - Priority order       │     │ - Time limit    │
│ - Garages        │     │ - Turn permissions     │     │ - Pass score    │
│ - Props          │     │                        │     │                 │
└─────────────────┘     └──────────────────────┘     └─────────────────┘
      STEP 1                    STEP 2                     STEP 3
   (loaded first)         (loaded second,              (loaded when
                           references roads)            entering exam)
```

**Loading order matters:**
1. `exampleRoad.txt` **must** be loaded first — it creates all the `Cross` and `Street` objects
2. `exampleRightOfWay.txt` **must** be loaded second — it references objects created by step 1
3. `exampleExam.txt` is loaded on-demand when entering Exam Mode — checkpoint coordinates must align with actual road positions

**To create a new city:** Edit `exampleRoad.txt` with new nodes and edges → update `exampleRightOfWay.txt` with the yield rules for each new intersection → update `exampleExam.txt` with checkpoint coordinates that sit on valid roads → run `./traffic`. No recompilation needed.

---

*This document explains the data-driven configuration layer of the AWAS City Traffic Simulator. Copyright (C) DeadlyS 2026*
