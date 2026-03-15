# MicroExodus — Claude Project Orientation

**Last Updated:** 2026-03-15
**Repo:** github.com/kungfunick/MicroExodus
**Engine:** Unreal Engine 5.7 (C++, DX12/SM6, Lumen, Substrate)

---

## What Is MicroExodus?

A UE5 roguelike where players manage swarms of miniaturised mannequin robots (~36cm tall at 0.20 scale) through hazardous procedurally generated levels. Between levels, a factory lobby lets players recruit, equip hats (run modifiers), and specialise robots. Robots have persistent identities — names, personalities, visual evolution from damage, and emotional narratives via the DEMS (Dynamic Event Messaging System).

## Architecture Philosophy

The C++ engine handles all game logic for speed and determinism. Unreal handles rendering, animation, audio, and UI. All cross-module communication uses events (MXEvents.h delegates), interfaces (MXInterfaces.h), and DataTables. No module directly references another module's concrete class.

## Current State (Phase 2C-Move — Working ✓)

**Phase 2A (Complete):** Blueprint-to-C++ integration ✓
**Phase 2B (Complete):** RTS Camera Controller ✓
**Phase 2C-Move (Working ✓):** Selection + Click-to-Move + Floor + Animation + HUD

Working features as of 2026-03-15:
- Robots spawn on procedural grid floor, animate with BS_Idle_Walk_Run BlendSpace via MXAnimInstance
- LMB click robot = select, LMB click ground = move, LMB drag = box select (green HUD rect)
- RMB drag = tablecloth camera pan, MMB drag = yaw/pitch rotation, scroll = zoom
- Green selection ring + constant-size green name text + "Selected: N" HUD
- Shift+1-9 save control groups, 1-9 recall (HUD shows `[1] (3) [2] (5)`)
- Home = reset view, Ctrl+A = select all, double-click robot = zoom in
- Walk/run distance-based speed scaling (MoveSpeed=400, WalkDistance=150)
- UMXAnimBridge → MXAnimInstance → BS_Idle_Walk_Run pipeline
- All config exposed as EditAnywhere UPROPERTYs (Editor SDK philosophy)

**Open Issues:** ISS-001 (T-pose when no AnimBP set), ISS-003 (SandboxCharacter_CMC), ISS-006 (floor material), ISS-007 (themed names not wired). See ISSUES.md.

## Input Scheme (Current)

| Input | Action |
|-------|--------|
| LMB click robot | Select (Shift = additive) |
| LMB click ground (with selection) | Move selected robots |
| LMB drag | Box select (green rectangle) |
| RMB drag | Tablecloth camera pan |
| MMB drag | Camera yaw (X) + pitch (Y) |
| Scroll wheel | Zoom in/out |
| WASD / Arrows | Keyboard pan (follows camera yaw) |
| Home | Reset view (center, zoom, pitch, yaw) |
| Double-click robot | Center + zoom in |
| Double-click ground | Center camera (no zoom) |
| Shift+1-9 | Save control group |
| 1-9 | Recall control group |
| Ctrl+A | Select all |

## Editor SDK Philosophy

Test levels are the **SDK for the engine**. Every parameter editable in Details panel without recompilation:
- **Tunables:** `EditAnywhere, BlueprintReadWrite` with `MX|Module|Subsystem` categories
- **Runtime data:** `VisibleAnywhere, BlueprintReadOnly`
- **Class refs:** `TSubclassOf<>` or `TSoftObjectPtr<>` for BP overrides
- **Hierarchical categories:** `MX|Robot|Config`, `MX|RTS|Camera`, etc.

## Critical Technical Patterns

| Pattern | Correct | Wrong |
|---------|---------|-------|
| Robot mesh scale | `GetMesh()->SetRelativeScale3D(0.20)` | `SetActorScale3D` (double-mini) |
| Capsule | `InitCapsuleSize(14, 20)` fixed in constructor | Scaling capsule with actor |
| Mesh Z offset | `SetRelativeLocation(0, 0, -20)` | Default -88 from ACharacter |
| Mesh facing | `SetRelativeRotation(0, -90, 0)` | Default 0° (mannequin +Y vs char +X) |
| Unpossessed CMC | `bRunPhysicsWithNoController = true` | Default false (CMC skips tick) |
| Character rotation | `bUseControllerRotationYaw = false` + `bOrientRotationToMovement = true` | bUseControllerRotationYaw true |
| Scroll wheel | `GetInputAnalogKeyState(EKeys::MouseWheelAxis)` | IsInputKeyDown/WasJustPressed |
| Keyboard + cursor | `SetInputMode(FInputModeGameAndUI)` in BeginPlay | Default UI-only mode |
| Camera yaw source | `CameraRig->GetActorRotation().Yaw` | `GetControlRotation()` |
| Number key FKeys | `EKeys::One` through `EKeys::Nine` | `FKey("1")` (silent fail) |
| Interface calls | `IMXFoo::Execute_Method(Object, Args)` | Direct call (crash) |
| UPROPERTY naming | `RobotRole` | `Role` (shadows AActor::Role) |
| BP serialisation | Force values in BeginPlay too | Constructor only (BP overwrites) |

## Critical Conventions

- **GitHub is source of truth.** Sync after every fix.
- **Flat includes.** `#include "MXFoo.h"` — no subdirectory prefixes.
- **Shared enums** in `MXTypes.h`.
- **Update tracking docs** after every session: Claude.md, Agents.md, CHANGE_LOG.md, ISSUES.md, README.md.

## Cross-Chat Sync Protocol

Separate Claude chats (PM, dev, bugs/fixes, animation). At session end: output updated docs + sync summary. At session start: read latest project knowledge.

## Planned Features

- **Phase GASP-B:** Full GASP locomotion (distance matching, traversal, idle variants)
- **Phase 2E:** Wire UMXProceduralGen for room-based layouts
- **Phase 2F:** Robot spawn UI (+ button, type picker, stat viewer)
- **Phase 3:** Materials, hat attachment, evolution visuals
- **Phase 4:** Swarm boid movement, hazard testing
