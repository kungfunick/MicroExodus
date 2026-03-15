# MicroExodus — Claude Project Orientation

**Last Updated:** 2026-03-10
**Repo:** github.com/kungfunick/MicroExodus
**Engine:** Unreal Engine 5.7 (C++, DX12/SM6, Lumen, Substrate)

---

## What Is MicroExodus?

A UE5 roguelike where players manage swarms of miniaturised mannequin robots (~36cm tall at 0.20 scale) through hazardous procedurally generated levels. Between levels, a factory lobby lets players recruit, equip hats (run modifiers), and specialise robots. Robots have persistent identities — names, personalities, visual evolution from damage, and emotional narratives via the DEMS (Dynamic Event Messaging System).

## Architecture Philosophy

The C++ engine handles all game logic — calculations, profiles, events, procedural generation — for speed and determinism. Unreal handles rendering, animation, audio, and UI. This separation means the entire game logic layer is pure code, while visual presentation is handled through Blueprints, materials, and UMG widgets.

All cross-module communication uses: events (MXEvents.h delegates), interfaces (MXInterfaces.h), and DataTables. No module directly references another module's concrete class — they communicate through the EventBus and provider interfaces.

## Current State (Phase 2C-Move)

**Compiling:** All 120+ source files compile. Phase 2C-Move adds 4 new files and modifies 6 — see Phase2C_Move_SetupGuide.md for potential compilation notes.

**Phase 2A (Complete):** Blueprint-to-C++ integration ✓
**Phase 2B (Complete):** RTS Camera Controller ✓

**Phase 2C-Move (In Progress):** Selection + Click-to-Move + Procedural Floor:
- UMXSelectionManager: click-select, box-select, Shift+multi, Ctrl+1-9 groups, Ctrl+A select all
- AMXRobotActor updated: selection/hover state, MoveToLocation with CMC, conditional name display, full profile binding with VisibleAnywhere identity/personality/role fields
- AMXTestFloorGenerator: procedural grid floor with collision (replaces manual Plane mesh)
- AMXRTSPlayerController: WASD/arrow pan (follows camera yaw), middle-click tablecloth drag, right-click yaw rotate, scroll zoom (analog axis), double-click (robot=zoom, ground=center), Home=reset view
- AMXSpawnTestGameMode: spawns floor → robots → camera, configurable SpawnRadius for tight central spawning
- All config exposed as EditAnywhere UPROPERTYs (Editor SDK philosophy)

**Themed Name Evolution (New):** Per-robot naming themes stamped at birth from run selection:
- 6 themes: Robot, Wizard, Pirate, Samurai, SciFi, Mythic (~420 names)
- Theme is per-RUN → mixed roster: "Barnacle Planksworth Captain Scourge" next to "Bolt Sprocket The Fireproof"
- Requires 3 patches: surname + name_theme on FMXRobotProfile, GameInstance wiring, RobotManager stamping
- See ThemedNameEvolution_SetupGuide.md for details

**Known Issues:** See `ISSUES.md` for full tracker (14 open, 15 resolved). Critical blockers from 2026-03-10 playtest:
1. ISS-019: Robot movement broken (right-click move not working)
2. ISS-020: Selection broken (not all robots selectable, box select unreliable)
3. ISS-021: Some robots embedded in floor after Manny mesh revert
4. ISS-022: Selection ring hard to see, name text scales with zoom (should be constant, green)
5. ISS-023: No camera pitch control
6. ISS-024: No control group HUD indicators

## File Structure

All C++ source is in `Source/MicroExodus/` — **flat structure, no subdirectories**.

```
Source/MicroExodus/           ← All .h/.cpp files (flat, ~130 files)
Content/Blueprints/           ← BP_MXGameInstance, BP_MXRobot, BP_MXSpawnTestGameMode, SandboxCharacter_CMC
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

The test levels (L_SpawnTest and future test maps) are not throwaway scaffolding — they are the **SDK for the engine**. Every gameplay parameter, spawn setting, camera tuning value, and robot configuration must be editable in the Unreal Editor Details panel without recompilation. This applies to all current and future C++ classes:

- **All tunables are `EditAnywhere, BlueprintReadWrite` UPROPERTYs** with descriptive `Category` tags following the `MX|Module|Subsystem` naming convention.
- **Generated data is `VisibleAnywhere, BlueprintReadOnly`** so developers can inspect runtime state (robot profiles, personalities, roles) directly in the editor Outliner.
- **Class references use `TSubclassOf<>` or `TSoftObjectPtr<>`** so Blueprints can override C++ defaults without code changes.
- **Config values should have sensible defaults** that produce a working scene without any manual setup.
- **Categories are hierarchical:** `MX|Robot|Config`, `MX|Robot|Profile`, `MX|RTS|Camera`, `MX|SpawnTest|Floor`, etc. This groups related properties in the Details panel.

When adding any new system, ask: "Can a designer tweak every parameter from the editor?" If not, wrap it in a UPROPERTY.

## Critical Conventions

- **GitHub is source of truth.** After any fix, sync to both local project AND push to repo. Stale files cause previously resolved errors to reappear.
- **Audit before fixing.** Read headers for exact signatures and wiring before writing new code. The modules were AI-generated by separate agents — assume nothing.
- **Flat includes.** All `#include` paths are bare filenames (e.g., `#include "MXRobotManager.h"`) — no subdirectory prefixes.
- **Blueprint serialisation overrides constructors.** Component transforms set in a C++ constructor get overwritten by Blueprint child class serialised values. Set transforms in BeginPlay instead.
- **Forward declarations for circular deps.** Use `class UMXFoo;` after `.generated.h` include when headers would create circular dependencies.
- **Update tracking docs after every session.** README.md, CHANGE_LOG.md, Claude.md, Agents.md, and ISSUES.md must all be synced to Claude.ai Project knowledge and GitHub after each phase.

## Cross-Chat Sync Protocol

MicroExodus uses **separate specialised Claude chats** (PM, dev, bugs/fixes, animation). Changes made in one chat must propagate to the others via these tracking docs. Every chat must follow this protocol:

**At session end**, every chat outputs:
1. Updated versions of any tracking docs it changed (Claude.md, Agents.md, CHANGE_LOG.md, ISSUES.md, README.md)
2. A **sync summary** listing: files created/modified, issues resolved/opened, decisions made, key patterns discovered
3. A reminder for the developer to sync all updated docs to Claude.ai Project knowledge AND GitHub

**At session start**, every chat should:
1. Read the latest tracking docs from project knowledge (they may have been updated by another chat)
2. Ask the developer if there are changes from other chats not yet reflected in project knowledge

**CHANGE_LOG.md** entries must include which chat produced them (e.g., "Chat: Bugs & Fixes", "Chat: PM", "Chat: GASP Animation").

**ISSUES.md** is the single source of truth for bugs. All chats read and write it.

## Planned Features (Next Up)

- **Phase GASP:** Integrate GASP locomotion via UMXAnimBridge (resolves ISS-001 T-pose). Distance matching, turn-in-place, traversal, idle variants. Prompt ready: `MicroExodus_GASP_Integration_Prompt.md`
- **Phase 2C-Polish:** Box select HUD drawing, move destination marker
- **Phase 2D:** Name display improvements (font, scale, occlusion)
- **Phase 2E:** Wire UMXProceduralGen for room-based layouts (replace simple grid)
- **Phase 2F:** Robot spawn UI (+ button, type picker, stat viewer)
- **Phase 3:** Mannequin materials, hat attachment, evolution visuals
- **Phase 4:** Swarm boid movement driving robot actors, hazard testing
