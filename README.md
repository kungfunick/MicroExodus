# Micro Exodus

**Roguelike Swarm Survival** — Unreal Engine 5.7

> 100 robots. 100 hats. Every run writes a story.

Miniaturised mannequin robots navigate procedurally generated hazard levels. Between runs, a factory lobby lets players recruit, equip hats (run modifiers), specialise, and upgrade. Robots have persistent identities — names, personalities, visual wear from damage, and narrative commentary via a dynamic event messaging system.

## Current State

**Phase 2C-Move — Selection, Click-to-Move, Animation, HUD.** The C++ engine logic (~135 files, 14 modules) compiles and is connected to Unreal Editor visuals. A spawn test level features miniaturised robots on a procedurally generated collision floor with idle/walk/run animation via BS_Idle_Walk_Run BlendSpace. Players select robots (click, box-drag, Shift+multi, Shift+1-9 groups) and left-click ground to issue move commands. Robots walk to nearby targets and run to distant ones. RTS camera with zoom, tablecloth pan, yaw/pitch rotation, and reset view.

See [CHANGE_LOG.md](CHANGE_LOG.md) for detailed session history.

## Controls

| Input | Action |
|-------|--------|
| **LMB click robot** | Select (Shift = additive) |
| **LMB click ground** (with selection) | Move selected robots |
| **LMB drag** | Box select (green rectangle) |
| **RMB drag** | Tablecloth camera pan |
| **MMB drag** | Camera yaw (X) + pitch (Y) |
| Scroll wheel | Zoom in/out |
| WASD / Arrows | Keyboard pan (follows camera yaw) |
| Home | Reset view |
| Double-click robot | Center + zoom in |
| Double-click ground | Center camera |
| Shift+1-9 | Save control group |
| 1-9 | Recall control group |
| Ctrl+A | Select all |

## Project Structure

```
MicroExodus/
├── Source/MicroExodus/       ← All C++ source files (flat structure, ~135 files)
├── Content/
│   ├── Blueprints/           ← BP_MXGameInstance, BP_MXRobot, BP_MXSpawnTestGameMode,
│   │                            ABP_Unarmed, SandboxCharacter_CMC
│   ├── Maps/                 ← L_SpawnTest
│   └── Materials/            ← (planned: M_Robot_Master)
├── Config/                   ← DefaultEngine.ini, DefaultGame.ini
├── Claude.md                 ← AI assistant orientation (read this first)
├── Agents.md                 ← Module registry with per-module status
├── CHANGE_LOG.md             ← Chronological project history
├── ISSUES.md                 ← Bug tracker with open/resolved issues
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

### Phase 2 Editor Integration

| Class | Purpose |
|-------|---------|
| AMXRobotActor | Robot actor: selection, movement, name display, animation |
| AMXRTSPlayerController | Camera + selection + movement input |
| UMXSelectionManager | Click/box/group selection logic |
| AMXTestFloorGenerator | Procedural grid floor |
| AMXSpawnTestGameMode | Test level setup |
| AMXRTSHUD | Box select rectangle + control group display |
| UMXAnimBridge | CMC → animation variable bridge |
| UMXAnimInstance | C++ AnimInstance base for BlendSpace |

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
- **Animation:** BS_Idle_Walk_Run BlendSpace via MXAnimInstance + UMXAnimBridge
- **Assets:** Quixel Megascans (free), Derelict Corridor (free), Steve's UE Helpers (free)
- **Target:** PC, 60fps at 1080p with 100 on-screen robots

## Build Prerequisites

- Unreal Engine 5.7
- Visual Studio 2022 with C++ game development workload
- Windows 10/11 SDK (10.0.22621.0+)

## Development Workflow

Development uses **Claude.ai Projects** with specialised continuation prompts per work stream:

| Document | Purpose |
|----------|---------|
| `Claude.md` | AI orientation — current state, conventions, technical patterns |
| `Agents.md` | Module registry — files, status, dependencies per module |
| `CHANGE_LOG.md` | Session history — what changed, decisions made, issues found |
| `ISSUES.md` | Bug tracker — open issues, resolved with root cause analysis |
| `README.md` | Public overview — project structure, tech stack, build info |

**Source of truth:** GitHub repo (`kungfunick/MicroExodus`). Claude.ai Project knowledge is synced from GitHub.

## License

Proprietary. All rights reserved.
