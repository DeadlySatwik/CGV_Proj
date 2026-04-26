# Phase 6: Living City — Full Logic Plan (Visual + Traffic Robustness)

## Objective

Deliver a complete, failure-aware implementation plan for a dynamic city cycle with strict handling of edge cases, including:

- day/night continuity,
- bridge/elevation transitions (up/down ramps),
- intersection control failures,
- full-stop traffic jam conditions,
- deterministic recovery behavior.

This plan is implementation-ready and removes unresolved logic gaps.

---

## Scope and Constraints

- Keep existing fixed-function OpenGL architecture.
- Do not change map file schema.
- Preserve current object ownership model (`Simulator` owns `objects`, `spots`; roads/crosses own traffic queues).
- Ensure every new behavior degrades safely when optional systems fail (e.g., RL server offline).

---

## Sub-Phase 6A — World Clock and Phase Model

### Files

- `Simulator.h`
- `Simulator.cpp`

### Required State

- `worldTime` in `[0, 24)` hours.
- `timeSpeed` in sim-minutes per real-second.
- `timeFlowing` toggle.
- `dayPhase` derived from time boundaries.

### Phase Table (authoritative)

| Phase     | Hours                       | Index |
| --------- | --------------------------- | ----- |
| Night     | 21:00–24:00 and 00:00–05:00 | 0     |
| Dawn      | 05:00–07:00                 | 1     |
| Morning   | 07:00–09:00                 | 2     |
| Noon      | 09:00–15:00                 | 3     |
| Afternoon | 15:00–17:00                 | 4     |
| Evening   | 17:00–19:00                 | 5     |
| Dusk      | 19:00–21:00                 | 6     |

### Invariants

- `worldTime` always normalized to `[0, 24)` after update.
- `dayPhase` must be recomputed after every `worldTime` change (including hotkey jumps).
- Phase progress is always clamped `[0,1]`.
- Logging format fixed as `[HH:MM] PhaseName`.

### Controls

- `N`: jump to next phase boundary.
- `M`: pause/resume time flow.
- `I`: print instant snapshot (time, phase, object/vehicle count).

---

## Sub-Phase 6B — Dynamic Sky, Lighting, Fog

### Files

- `Simulator.cpp` (`redraw`, lighting updater)
- `EngineCoreBase.cpp` (static light setup only)

### Rules

- Apply camera/view first.
- Apply dynamic `GL_LIGHT0` (`AMBIENT`, `DIFFUSE`, `POSITION`) after view matrix and before world-object transforms.
- Interpolate from current phase palette to next phase palette via phase progress `t`.
- Fog color follows sky color.

### Fog Profile

- Night-biased near fog; noon-biased far fog.
- Always enforce `fogEnd > fogStart + epsilon` to avoid degenerate fog state.

### Failure Guards

- If interpolation index computation fails, fall back to current phase palette.
- If `t` invalid (`NaN/inf`), force `t=0`.

---

## Sub-Phase 6C — Ground/Terrain Layers

### File

- `Simulator.cpp`

### Rendering Order

1. Base ground (large quad, interpolated by phase).
2. Existing road/cross geometry.
3. Optional enhancement overlays only if geometry data available and cheap.

### Hard Rule

- Do not introduce z-fighting with road surfaces: all decorative ground surfaces must remain below road top or with deterministic epsilon.

---

## Sub-Phase 6D — Streetlights

### Files

- `Environment.cpp`
- `Environment.h`

### Behavior Matrix

- Morning/Noon/Afternoon: lamp off.
- Evening/Dusk: warm on.
- Night: brightest on.
- Dawn: dim/transition behavior.

### Access Pattern

- Query phase via `Simulator::getInstance().getDayPhase()`.

### Failure Guard

- If phase is out-of-range, default to lamp-on safe mode for visibility.

---

## Sub-Phase 6E — Vehicle Night Mode

### File

- `Vehicle.cpp`

### Behavior

- Night phases (`Night`, `Dawn`, `Evening`, `Dusk`) enable brighter headlight lenses and beam hints.
- Stronger taillight glow under low-light phases.
- Bus interior glass transitions to warm lit tint at night.

### Constraint

- Visual effects must remain purely cosmetic; no physics/path changes in draw path.

---

## Sub-Phase 6F — Building Window Occupancy Glow

### File

- `Garage.cpp`

### Deterministic Lit Pattern

- Use hash-based deterministic per-window selection.
- Night/evening lit ratio target: ~60%.
- Dawn lit ratio target: ~30%.
- Daytime: reflective tint only.

### Guard

- Pattern must remain stable frame-to-frame (no temporal flicker unless explicitly designed).

---

## Sub-Phase 6G — Time-Based Traffic Density

### Files

- `Garage.cpp`
- `Garage.h` (optional constants)

### Multipliers (authoritative)

| Phase     | Spawn Mult | Max Vehicle Mult |
| --------- | ---------: | ---------------: |
| Night     |       0.15 |             0.20 |
| Dawn      |       0.50 |             0.40 |
| Morning   |       2.00 |             1.50 |
| Noon      |       1.00 |             1.00 |
| Afternoon |       1.20 |             1.10 |
| Evening   |       2.00 |             1.50 |
| Dusk      |       0.60 |             0.50 |

### Effective Values

- `effectiveFrec = frecSpot / spawnMult`
- `effectiveMax = max(2, int(maxVehicles * maxMult))`

### Bus-Specific Rule

- Apply additional bus multiplier:
  - Morning/Evening: ×2.5
  - Night: ×0.05
  - others: ×1.0

---

## Sub-Phase 6H — Runtime Info Output

### File

- `Simulator.cpp`

### Output Events

- Phase transition print.
- `I` key print for point-in-time diagnostics.

### Required Fields

- Time (`HH:MM`)
- Phase name
- Vehicle count estimate
- Object count

---

## Sub-Phase 6I — Bridge/Elevation Robustness (Critical)

### Files

- `Road.cpp`
- `Vehicle.cpp`
- `Garage.cpp`
- `Simulator.cpp` (diagnostic counters)

### Problem Class

Elevation roads and intersections (`position.y > 0`) create edge cases where vehicles can:

- visually float,
- clip into ramps/intersections,
- stall at grade transitions,
- deadlock due to layer mismatches or queue ordering.

### Mandatory Invariants

1. **Height continuity**
   - For any transition `road -> cross -> nextRoad`, end height and entry height must be continuous within epsilon.
2. **Collision layer coherence**
   - Vehicles on elevated segments must keep elevated collision layer until fully transitioned.
3. **Monotonic progress**
   - During cross/turn state, progress toward next road cannot oscillate backward.
4. **Queue correctness**
   - Enter/leave events must preserve queue push/pop symmetry on both directions.

### Edge Cases and Logic

#### Case A: Uphill entry congestion

- Risk: front vehicles slow sharply; trailing vehicles compress and stop permanently.
- Rule: clamp minimum progress speed for vehicles already granted crossing permission.

#### Case B: Downhill overrun near cross

- Risk: overshoot causes invalid position or abrupt snapping.
- Rule: cap `xPos` to segment length before state transition; transition atomically.

#### Case C: Elevated-to-ground mismatch

- Risk: sudden y-jump when next road has lower height.
- Rule: transition interpolation uses joint points from both segments; never mix flat and elevated anchors.

#### Case D: Bridge pass-through nodes (2-way links)

- Risk: `allowedVeh` accumulation causes phantom grants/stalls.
- Rule: pass-through nodes must not accumulate grants across frames; treat as immediate-through logic.

#### Case E: Spawn on steep ramp blocked forever

- Risk: garage keeps trying to spawn but head cannot clear.
- Rule: spawn timer advances only when headway threshold is satisfied; otherwise backoff without burst spawning.

### Runtime Assertions (debug mode)

- Vehicle y-height deviation from road profile > epsilon increments anomaly counter.
- Negative queue sizes / impossible pop attempts raise hard error.
- `reservedSpace` underflow/overflow is blocked and logged.

---

## Sub-Phase 6J — Traffic Control Failure/Jam Recovery (Critical)

### Files

- `Road.cpp` (`Cross`, `CrossLights`)
- `Garage.cpp`
- `Simulator.cpp` (global diagnostics)

### Failure Types

1. **Local starvation**: one approach never gets clearance.
2. **Global freeze**: many vehicles stop, throughput near zero.
3. **RL control degradation**: RL server unreachable/invalid actions.
4. **Crosslock cycle**: vehicles mutually waiting under right-of-way constraints.

### Detection Metrics

- Per-intersection:
  - queue lengths per approach,
  - time since last vehicle granted,
  - grants per rolling window.
- Global:
  - moving count of vehicles with near-zero velocity,
  - throughput per time window,
  - number of intersections with `allowedVeh == 0` while queues exist.

### Recovery State Machine (per controlled intersection)

| State               | Entry Condition                                         | Action                                                        | Exit Condition              |
| ------------------- | ------------------------------------------------------- | ------------------------------------------------------------- | --------------------------- |
| NORMAL              | default                                                 | regular priority / RL assist                                  | starvation timer exceeded   |
| STARVATION_RECOVERY | one approach starving                                   | force grant from starved approach if safe                     | starvation cleared          |
| GRIDLOCK_RECOVERY   | no grants + queues on all/most approaches for threshold | temporary all-red break then deterministic alternating grants | throughput resumes          |
| FAILSAFE            | repeated RL/action failures                             | disable RL, fixed deterministic schedule                      | stable operation window met |

### Non-Negotiable Safeguards

- RL action out-of-range => ignore and keep deterministic fallback.
- No intersection may remain with waiting queues and zero grants beyond timeout without escalation.
- If all vehicles nearly stopped for prolonged window, reduce spawn multipliers temporarily (anti-jam dampening).

### Deterministic Unjam Procedure

1. Freeze new grants for short break interval.
2. Pick approach with oldest waiting head vehicle.
3. Grant one vehicle if downstream free-space allows.
4. Repeat with round-robin fairness and timeout cap.
5. Restore normal mode once rolling throughput threshold recovered.

---

## Sub-Phase 6K — Diagnostics and Acceptance Gates

### Files

- `Simulator.cpp`
- `Road.cpp`
- `Garage.cpp`

### Required Counters

- `vehiclesSpawned`, `vehiclesDeleted`
- `crossGrantsPerMinute`
- `stalledVehicleCount`
- `jamRecoveryActivations`
- `bridgeAnomalyCount`

### Acceptance Criteria

1. **Visual continuity**: full 24h cycle with no palette discontinuities.
2. **Bridge stability**: no persistent floating/clipping on elevated network over long run.
3. **Traffic liveness**: no permanent standstill under sustained simulation.
4. **Control resilience**: RL disconnection does not halt traffic lights or movement.
5. **Deterministic fallback**: repeated runs with same inputs produce same control fallback behavior.

---

## File Impact Summary (Complete)

| File                 | Planned Logic Impact                                                                        |
| -------------------- | ------------------------------------------------------------------------------------------- |
| `Simulator.h`        | clock state, phase accessors, diagnostic access points                                      |
| `Simulator.cpp`      | world-time update, sky/light/fog, ground base, logging, jam diagnostics                     |
| `EngineCoreBase.cpp` | static-only light baseline, dynamic values set in redraw path                               |
| `Environment.cpp`    | phase-aware lamppost visuals                                                                |
| `Vehicle.cpp`        | night-mode visuals, bridge transition stability hooks                                       |
| `Garage.cpp`         | window glow, phase-based spawn control, anti-jam spawn dampening                            |
| `Garage.h`           | optional static multiplier tables and constants                                             |
| `Road.cpp`           | cross priority, light-controller resilience, deadlock recovery, elevation continuity checks |
| `ObjectsLoader.cpp`  | unchanged schema; source of map/elevation inputs                                            |

---

## Execution Order (No Logic Gaps)

1. Stabilize clock/phase invariants (`6A`).
2. Apply dynamic sky/light/fog with strict matrix ordering (`6B`).
3. Finalize base ground layering (`6C`).
4. Complete phase-bound visual actors (`6D`, `6E`, `6F`).
5. Enable time-based spawn modulation (`6G`).
6. Add bridge/elevation invariants and anomaly instrumentation (`6I`).
7. Add jam detection + recovery FSM + deterministic fallback (`6J`).
8. Add diagnostics and verify acceptance gates (`6K`).

---

## Final Control Keys

| Key | Action                               |
| --- | ------------------------------------ |
| `N` | Jump to next day phase boundary      |
| `M` | Pause/resume world time              |
| `I` | Print time/phase/traffic diagnostics |
| `P` | Projection toggle                    |

---

## Plan Decisions (Resolved)

- Time acceleration default: **2.0 sim-minutes/sec**.
- Fog: **enabled with phase-dependent distance profile**.
- Sidewalk/ground complexity: **retain simple base-ground-first approach; no geometry-heavy per-road expansion unless profiling permits**.

This phase plan is complete and includes bridge transitions plus traffic-control failure handling for full-stop jam scenarios.
