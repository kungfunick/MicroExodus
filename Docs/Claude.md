# MicroExodus — Claude Project Orientation

**Last Updated:** 2026-03-13
**Repo:** github.com/kungfunick/MicroExodus
**Engine:** Unreal Engine 5.7 (C++, DX12/SM6, Lumen, Substrate)

---

## What Is MicroExodus?

A UE5 roguelike where players manage swarms of miniaturised mannequin robots (~36cm tall at 0.20 scale) through hazardous procedurally generated levels. Between levels, a factory lobby lets players recruit, equip hats (run modifiers), and specialise robots. Robots have persistent identities — names, personalities, visual evolution from damage, and emotional narratives via the DEMS (Dynamic Event Messaging System).

## Architecture Philosophy

The C++ engine handles all game logic — calculations, profiles, events, procedural generation — for speed and determinism. Unreal handles rendering, animation, audio, and UI. This separation means the entire game logic layer is pure code, while visual presentation is handled through Blueprints, materials, and UMG widgets.

All cross-module communication uses: events (MXEvents.h delegates), interfaces (MXInterfaces.h), and DataTables. No module directly references another module's concrete class — they communicate through the EventBus and provider interfaces.

## Current State (Phase 2C-Move + Anim-A)

**Compiling:** All 132+ source files compile. Phase Anim-A adds 2 new files (MXAnimBridge.h/.cpp) and patches 3 (MXTypes.h, MXRobotActor.h/.cpp).

**Phase 2A (Complete):** Blueprint-to-C++ integration ✓
**Phase 2B (Complete):** RTS Camera Controller ✓

**Phase 2C-Move (In Progress):** Selection + Click-to-Move + Procedural Floor:
- UMXSelectionManager: click-select, box-select, Shift+multi, Ctrl+1-9 groups, Ctrl+A select all
- AMXRobotActor: selection/hover state, MoveToLocation with CMC, conditional name display, full profile binding
- AMXTestFloorGenerator: procedural grid floor with collision
- AMXRTSPlayerController: WASD/arrow pan, middle-click drag, right-click rotate, scroll zoom, double-click, Home reset
- AMXSpawnTestGameMode: spawns floor → robots → camera, configurable SpawnRadius

**Phase Anim-A (Complete):** AnimBridge Foundation:
- UMXAnimBridge: ActorComponent on AMXRobotActor, reads CMC state each tick, exposes Speed/Direction/bIsMoving/LocomotionState/TurnAngle/LeanAngle/IdleTime/IdleVariant to any AnimBP
- Three new enums in MXTypes.h: EMXLocomotionState, ETraversalType, EMXActionMontage
- Action API: PlayTraversalAction(), PlayActionMontage(), SetIdleVariant()
- **GASP removed** — using built-in UE5 Manny animations (MM_Idle, MM_Walk_Fwd, MM_Run_Fwd). AnimBridge has zero GASP dependencies; animation layer fully swappable.
- SandboxCharacter_CMC.uasset deleted. GASP integration prompt retired.

**Themed Name Evolution:** Per-robot naming themes (6 themes, ~420 names). Compiled but not yet wired (ISS-007).

**Known Issues:** See `ISSUES.md`. Key open items:
1. Robots in T-pose (AnimBridge exists, need ABP_MXRobot AnimBP in editor — see locomotion guide)
2. Box select rectangle not drawn
3. Themed naming not wired
4. Floor tile material "Color" parameter may not work

## File Structure

All C++ source is in `Source/MicroExodus/` — **flat structure, no subdirectories**.

```
Source/MicroExodus/           ← All .h/.cpp files (flat, ~132 files)
Content/Blueprints/           ← BP_MXGameInstance, BP_MXRobot, BP_MXSpawnTestGameMode
Content/Blueprints/Animation/ ← (planned: ABP_MXRobot, BS_MXRobot_Locomotion)
Content/Maps/                 ← L_SpawnTest
Config/                       ← DefaultEngine.ini, DefaultGame.ini
```

## Key Config

```ini
# DefaultEngine.ini
GameInstanceClass=/Game/Blueprints/BP_MXGameInstance.BP_MXGameInstance_C
GameDefaultMap=/Engine/Maps/Templates/OpenWorld
```

## Build Dependencies (MicroExodus.Build.cs)

Core, CoreUObject, Engine, InputCore, Json, JsonUtilities, UMG, Slate, SlateCore

## Module Initialisation Order (in UMXGameInstance::Init)

1. EventBus (via UMXEventBusSubsystem — auto-created as GameInstance subsystem)
2. NameGenerator → PersonalityGenerator → LifeLog → AgingSystem
3. RobotManager.Initialize(NameGen, PersonalityGen, LifeLog, Aging)
4. DEMS: DedupBuffer → TemplateSelector → MessageBuilder → MessageDispatcher
5. HatManager.Initialize(EventBus, RobotManager)
6. RunManager.Initialise(EventBus, RobotProvider, HatProvider, TierTable, XPTable)
7. SpecTree.Initialize(SpecTreeTable, RobotProvider, EventBus)
8. RunReportEngine.Initialize(EventBus)

## Editor SDK Philosophy

Every gameplay parameter, spawn setting, camera tuning value, and robot configuration must be editable in the Unreal Editor Details panel without recompilation:

- **All tunables are `EditAnywhere, BlueprintReadWrite` UPROPERTYs** with `MX|Module|Subsystem` categories.
- **Generated data is `VisibleAnywhere, BlueprintReadOnly`**.
- **Class references use `TSubclassOf<>` or `TSoftObjectPtr<>`**.
- **Config values have sensible defaults** that produce a working scene without manual setup.
- **Categories are hierarchical:** `MX|Robot|Config`, `MX|Animation|Locomotion`, `MX|RTS|Camera`, etc.

## Critical Conventions

- **GitHub is source of truth.** Sync after every session.
- **Audit before fixing.** Read headers for exact signatures before writing code.
- **Flat includes.** `#include "MXFoo.h"` — no subdirectory prefixes.
- **Blueprint serialisation overrides constructors.** Set transforms in BeginPlay.
- **Forward declarations for circular deps.**
- **Update tracking docs after every session.** All five docs synced to Claude.ai Project knowledge and GitHub.

## Cross-Chat Sync Protocol

Separate specialised Claude chats (PM, dev, bugs/fixes, animation). Changes propagate via tracking docs.

**At session end:** output updated docs, sync summary, reminder to sync.
**At session start:** read latest docs, ask about unsynced changes from other chats.

## Planned Features (Next Up)

- **Phase Anim-B:** Create ABP_MXRobot AnimBP in editor (BlendSpace1D: idle→walk→run). Resolves ISS-001.
- **Phase Anim-C:** Turn-in-place, start/stop blending (AnimBridge already exposes the variables)
- **Phase Anim-D:** Idle variants from personality, fidget system
- **Phase Anim-E:** Traversal actions (vault, climb, mantle)
- **Phase Anim-F:** Wire swarm boid movement, 100-robot perf test
- **Phase 2C-Polish:** Box select HUD, move destination marker
- **Phase 2D:** Name display improvements
- **Phase 2E:** Wire UMXProceduralGen for room-based layouts
- **Phase 2F:** Robot spawn UI
- **Phase 3:** Mannequin materials, hat attachment, evolution visuals
- **Phase 4:** Swarm boid movement driving robot actors
