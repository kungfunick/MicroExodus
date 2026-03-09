# MicroExodus — Change Log

All notable changes to this project are documented here. Entries are reverse-chronological (newest first).

---

## [2026-03-09] — Phase 2 Design & Project Management Setup

**Phase:** Planning

### Changes
- Created `MicroExodus_Phase2_EditorPlan.docx` — comprehensive Editor-side development plan covering mannequin robots (0.20 scale Manny/Quinn), factory lobby (7 stations), Fab framework integration strategy, and 6-week roadmap
- Created `MicroExodus_Phase2A_ContinuationPrompt.md` — detailed handoff prompt for Blueprint-to-C++ integration and spawn test
- Created `MicroExodus_ProjectManagement_Prompt.md` — PM chat prompt for tracking status across all development chats
- Created `CHANGE_LOG.md` (this file) — project history
- Updated memory: all chats now remind to update README.md, CHANGE_LOG.md, Claude.md, and Agents.md after each phase

### Decisions
- Robots will use UE5 Manny/Quinn mannequin at 0.20 scale (~36cm tall) on SK_Mannequin (89 bones)
- Factory lobby will be a 3D environment (not a flat menu) using Derelict Corridor Megascans + Factory Modular Pack (both free on Fab)
- Lyra patterns cherry-picked (Common UI, mannequin AnimBP) but NOT used as project base — too invasive for existing module architecture
- Steve's UE Helpers adopted as C++ plugin for UI navigation and actor pooling
- Need to inspect SandboxCharacter_CMC.uasset before deciding skeleton compatibility approach

### New Issues
- README.md outdated (references subdirectories, old 12-agent structure)

---

## [2026-03-08] — Phase 2B: RTS Camera Controller

**Phase:** 2B

### Changes
- Created `AMXRTSPlayerController` — RTS-style camera with scroll zoom, right-click rotate, WASD pan, edge pan, middle-drag pan
- Modified `AMXSpawnTestGameMode` — sets PlayerControllerClass, DefaultPawnClass = nullptr
- Disabled SwarmCamera tick during RTS mode to prevent fighting pan inputs

### Known Issues Post-Session
- No robot selection system yet
- Name text always visible (needs hover/selection only)
- Robots in T-pose (no AnimBP)

---

## [2026-03-08] — Phase 2A: Blueprint-to-C++ Integration

**Phase:** 2A

### Changes
- Created `AMXRobotActor` (ACharacter subclass) — spawnable robot with SkeletalMesh, TextRenderComponent for name display
- Created `AMXCameraRig` (AActor) — hosts SpringArm + Camera + PostProcess + SwarmCamera + TiltShiftEffect + TimeDilation components
- Created `AMXSpawnTestGameMode` — spawns 5-10 robots via UMXRobotManager, feeds GUIDs to camera
- Created `L_SpawnTest` level — flat test floor with basic lighting
- Created `BP_MXSpawnTestGameMode` Blueprint
- Patched `UMXSwarmCamera` with `UpdateRobotPosition()` for position tracking

### Decisions
- C++ actor classes preferred over pure Blueprint for long-term module integration
- GravityScale=0 as temporary workaround for floor plane lacking collision
- Blueprint serialisation overrides C++ constructor transforms — set transforms in BeginPlay instead

---

## [2026-03-02] — Agent 14: Robot Factory Module

**Phase:** 1 (Engine Logic)

### Changes
- Created `MXRobotFactoryData.h` (203 lines) — EFactoryUpgrade, ERecruitSource, FMXFactoryUpgradeLevel, FMXFactoryState, FMXRecruitCandidate, FMXSalvageDrop, FMXLobbyPool
- Created `MXRobotCodec.h` (193 lines) — GUID-to-shareable-code conversion (Base32 Crockford, XXXX-XXXX-XXXX-XXXX format)
- Created `MXRobotCodec.cpp` (261 lines) — encode/decode, seed derivation, assembly GUID derivation
- Created `MXRobotFactory.h` (340 lines) — lobby manager: BuildLobbyPool, ToggleCandidateSelection, RedeemCode, PurchaseUpgrade
- Created `MXRobotFactory.cpp` (609 lines) — full implementation with exponential cost scaling, tier-based recruit counts

### Decisions
- Robot codes are generation seeds (not compressed GUIDs) — same code produces identical robot on any machine
- CodeTerminal upgrade costs 50 salvage parts to unlock code redemption
- 8 factory upgrades with 1.5× exponential cost scaling

---

## [2026-03-02] — Agent 13: Robot Assembly Module

**Phase:** 1 (Engine Logic)

### Changes
- Created `MXRobotAssemblyData.h` — EPartSlot (5 slots), FMXPartDefinition, FMXAssemblyRecipe, FMXPartDamageState
- Created `MXRobotAssembler.h/.cpp` — modular mesh system with per-part LOD for 100 robots
- Created `MXPartDestructionManager.h/.cpp` — per-part damage tracking, destruction thresholds
- Created `MXPartDestructionData.h` — damage state data structures
- Created `MXPartDestructionVisuals.cpp` — visual feedback for part damage

---

## [2026-03-01] — Procedural Generation Module

**Phase:** 1 (Engine Logic)

### Changes
- Created procedural level generation system (20-level layouts)
- Room types: corridors, arenas, gauntlets, puzzle rooms
- Hazard placement with element theming per level
- Rescue robot spawn point generation

---

## [2026-03-01] — Compilation Audit & Fix Session

**Phase:** 1 (Engine Logic)

### Changes
- Audited entire codebase for compilation errors
- Catalogued 13 recurring error patterns across all agent-generated modules
- Fixed: subdirectory prefix issues, wrong UE5 module paths, circular includes, UFUNCTION on FArchive methods, namespace qualification (MX:: vs bare), field name mismatches, delegate signatures
- Established flat include structure as project convention
- All 94+ files (Phases 1-9) confirmed compiling

### Decisions
- Flat source structure mandatory — no `#include "Camera/MXSwarmCamera.h"`, only `#include "MXSwarmCamera.h"`
- Forward declarations preferred over full includes in headers to avoid circular deps
- EventBus lookup pattern standardised across all modules
- For systematic errors, full audit first — not reactive one-at-a-time patching

---

## [Pre-2026-03-01] — Initial Module Generation

**Phase:** 1 (Engine Logic)

### Changes
- 12 original modules generated by specialised AI agents (Agents 0-11)
- Each agent built independently without awareness of others
- Agent 0 (Shared) defined all cross-module contracts
- Agents 1-11 implemented their respective modules against contracts
- ~94 initial source files created

### Known Issues at Generation
- Systematic cross-cutting integration errors (13 patterns catalogued in audit)
- Each agent made independent assumptions about include paths, API names, enum values
- No end-to-end compilation test performed during generation
