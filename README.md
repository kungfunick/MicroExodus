# Micro Exodus

**Roguelike Swarm Survival** — Unreal Engine 5.7

> 100 robots. 100 hats. Every run writes a story.

Miniaturised mannequin robots navigate procedurally generated hazard levels. Between runs, a factory lobby lets players recruit, equip hats (run modifiers), specialise, and upgrade. Robots have persistent identities — names, personalities, visual wear from damage, and narrative commentary via a dynamic event messaging system.

## Current State

**Phase 2 — Editor Integration.** The C++ engine logic (120+ files, 14 modules) compiles and is being connected to Unreal Editor visuals. A spawn test level exists with miniaturised robots, name display, and an RTS camera controller. Next: linking the SandboxCharacter_CMC Blueprint, tilt-shift camera activation, and factory lobby prototyping.

See [CHANGE_LOG.md](CHANGE_LOG.md) for detailed session history.

## Project Structure

```
MicroExodus/
├── Source/MicroExodus/       ← All C++ source files (flat structure, 120+ files)
├── Content/
│   ├── Blueprints/           ← BP_MXGameInstance, BP_MXRobot, BP_MXSpawnTestGameMode,
│   │                            SandboxCharacter_CMC (from Game Animation Sample)
│   ├── Maps/                 ← L_SpawnTest
│   └── Materials/            ← (planned: M_Robot_Master)
├── Config/                   ← DefaultEngine.ini, DefaultGame.ini
├── Claude.md                 ← AI assistant orientation (read this first)
├── Agents.md                 ← Module registry with per-module status
├── CHANGE_LOG.md             ← Chronological project history
└── README.md                 ← This file
```

## Architecture

The C++ engine handles all game logic — robot profiles, events, procedural generation, camera behaviors, hat economies, run management — for speed, determinism, and multi-project reuse. Unreal handles rendering, animation, audio, and UI through Blueprints and UMG widgets.

All cross-module communication uses the **EventBus** (delegate-based) and **provider interfaces** (IMXRobotProvider, IMXHatProvider, IMXRunProvider). No module directly references another module's concrete class.

### Module Overview

| # | Module | Key Class | Purpose |
|---|--------|-----------|---------|
| 0 | Shared | MXTypes.h, MXInterfaces.h | Cross-module contracts: enums, structs, interfaces, events |
| 1 | DEMS | UMXMessageBuilder | Dynamic Event Messaging — narrative sentence construction |
| 2 | Identity | UMXRobotManager | Robot profiles, names, personalities, aging, life logs |
| 3 | Hats | UMXHatManager | 100-hat collection, stacks, combos, level-specific unlocks |
| 4 | Evolution | UMXEvolutionLayerSystem | Visual wear: burns, cracks, patina, weld marks |
| 5 | Specialization | UMXSpecTree | Skill trees: Scout / Guardian / Engineer branches |
| 6 | Roguelike | UMXRunManager | Run management, tiers, modifiers, XP, procedural levels |
| 7 | Camera | UMXSwarmCamera | Swarm-scaled zoom, tilt-shift, time dilation, DEMS reactions |
| 8 | Swarm | UMXSwarmAI | Boid AI, hazard base classes, swarm gates, rescue |
| 9 | Reports | UMXRunReportEngine | Post-run stats, highlights, awards, eulogies |
| 10 | Persistence | UMXSaveManager | Save/load all module state, backup |
| 11 | UI | UMXWidgetBase | HUD, roster, hat collection, report display widgets |
| 13 | Assembly | UMXRobotAssembler | Modular mesh system, per-part destruction, LOD |
| 14 | Factory | UMXRobotFactory | Pre-level lobby: recruit pools, friend codes, upgrades |

### Key Config

```ini
# Config/DefaultEngine.ini
GameInstanceClass=/Game/Blueprints/BP_MXGameInstance.BP_MXGameInstance_C
GameDefaultMap=/Engine/Maps/Templates/OpenWorld
```

## Tech Stack

- **Engine:** Unreal Engine 5.7 (Lumen GI, Virtual Shadow Maps, Substrate, Nanite, DX12/SM6)
- **Language:** C++ with Blueprint integration
- **Character:** UE5 Manny/Quinn mannequin at 0.20 scale (~36cm tall)
- **Assets:** Quixel Megascans (free), Derelict Corridor (free), Steve's UE Helpers (free)
- **Target:** PC, 60fps at 1080p with 100 on-screen robots

## Build Prerequisites

- Unreal Engine 5.7
- Visual Studio 2022 with C++ game development workload
- Windows 10/11 SDK (10.0.22621.0+)

## Development Workflow

Development uses **Claude.ai Projects** with specialised continuation prompts per work stream. Each session produces updated tracking documents:

| Document | Purpose |
|----------|---------|
| `Claude.md` | AI orientation — current state, conventions, known issues |
| `Agents.md` | Module registry — files, status, dependencies per module |
| `CHANGE_LOG.md` | Session history — what changed, decisions made, issues found |
| `README.md` | Public overview — project structure, tech stack, build info |

**Source of truth:** GitHub repo (`kungfunick/MicroExodus`). Claude.ai Project knowledge is synced from GitHub.

## License

Proprietary. All rights reserved.
