# About This Project

> A concise summary of what this project **is** and what it **does** — to help you pick a name and design an icon.

---

## What It Is

A **3D city traffic simulation** built from scratch in **C++11 with raw OpenGL** (no game engine, no GLUT). It combines playable driving, AI traffic, rule enforcement, and a driver's license exam system — all rendered in real-time with a full day/night cycle.

---

## Core Features

### 🚗 Playable Driving
- Third-person driving with **3 switchable vehicles**: Car, Bus, Bike
- Each vehicle has unique physics (acceleration, top speed, handling)
- Free-fly camera mode for spectating

### 🏙️ Procedural City
- **5×7 grid** of intersections with streets, garages, trees, lampposts, and benches
- Data-driven map — the entire city is defined in plain text files, no recompilation needed
- **AWAS Bridge** — elevated road with smooth ramp transitions and pitch-aware vehicles

### 🚦 AI Traffic System
- NPC vehicles (cars, buses, bikes) spawn from garages and drive autonomously
- **Car-following model** with safe braking and gap control — no collisions
- Right-of-way yield tables, traffic lights, and gridlock recovery
- Turn blinkers on NPC vehicles

### 🔴🟢 Traffic Lights & Signals
- Signalized intersections with phase-based green/red cycling
- Starvation and gridlock recovery state machines
- Optional **Reinforcement Learning** integration — a Python TCP server can control lights via a PPO policy

### 🌅 Day/Night Cycle
- 7 time phases: Night → Dawn → Morning → Noon → Afternoon → Evening → Dusk
- Dynamic sky color, fog, ambient/diffuse lighting, and sun direction
- Lampposts glow at night, vehicles switch on headlights, building windows change

### 🚶 Pedestrian System
- Zebra crossings with walk/don't-walk signals
- Animated pedestrians that spawn, wait, cross, and despawn
- Player is penalized for driving through active crosswalks

### 📋 Three Game Modes
| Mode | Purpose |
|------|---------|
| **Basic Driving** | Free roam, no rules |
| **Training** | Lessons (General, Quiet Zone, Parking, Pedestrian Safety) with scoring |
| **Exam** | Timed checkpoint-based driving test with pass/fail and report generation |

### 📝 Exam & Licensing
- Timed exam with ordered checkpoints and named objectives
- Navigation aids: golden rings, beacon pillars, directional arrows
- On **PASS** → generates a Markdown exam report + driving license
- On **FAIL** → generates only the report
- Vehicle type is locked during exam

### 🎮 HUD Overlay
- Live on-screen display: mode, score, time, warnings, violations, controls
- Toggle on/off with a single key

### 🖥️ Cross-Platform Engine
- Custom engine core — Linux (X11 + GLX) and Windows (Win32) backends
- Fixed-function OpenGL rendering — everything drawn from scaled cubes
- No external game libraries — just OpenGL and OS APIs

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Language | C++11 |
| Rendering | OpenGL (immediate-mode, fixed-function) |
| Windowing | X11/GLX (Linux), Win32 (Windows) |
| Build | Makefile |
| RL (optional) | Python, stable-baselines3, TCP sockets |

---

## Visual Identity Keywords

Things to consider when naming / creating an icon:

- **City traffic** — streets, intersections, traffic lights
- **Driving simulation** — player-controlled vehicles
- **Learning / Exam** — training, rules, driving license
- **Day/Night** — atmospheric transitions
- **3D / OpenGL** — rendered city environment
- **Bridge (AWAS)** — elevated interchange, signature landmark
- **AI + RL** — smart traffic management

---

*Copyright (C) DeadlyS 2026*
