# MicroExodus — Change Log

All notable changes to this project are documented here. Entries are reverse-chronological (newest first).

---

## [2026-03-15] — Bug Fix Marathon: Input Remap + Movement + Animation + HUD

**Chat:** Bugs & Fixes dev chat
**Phase:** 2C-Move (bug fixes + polish + animation)

### Input Scheme Remap (ISS-R21)
- **LMB** = select + move (click robot = select, click ground with selection = move, drag = box select)
- **RMB** = tablecloth camera pan (was MMB). DragPanSpeed tuned to 3.0
- **MMB** = yaw + pitch rotation (was RMB)
- Deleted `HandleRightClickMove()` entirely — no more fragile timing thresholds
- Control groups: Shift+1-9 save, 1-9 recall (fixed FKey("1") → EKeys::One)

### Movement Fixes (ISS-R16, R26, R27)
- `bRunPhysicsWithNoController = true` — CMC was skipping tick for unpossessed robots
- `bUseControllerRotationYaw = false` in constructor AND BeginPlay — BP serialisation was overriding
- Removed manual `SetActorRotation` from TickMovement — was fighting CMC's `bOrientRotationToMovement`
- Mesh rotation `FRotator(0, -90, 0)` — mannequin faces +Y, character forward is +X
- MoveSpeed 150→400, WalkDistance=150 for distance-based walk/run scaling

### Selection + Visual Fixes (ISS-R17, R18, R19, R22, R23, R24)
- Capsule `InitCapsuleSize(14, 20)` + explicit Pawn collision profile
- Spawn Z = capsule half-height (20) so robots sit on floor
- Green selection ring (0.40 scale), green name text, constant screen size (`Dist * 0.03f`)
- Box select green rectangle drawn via AMXRTSHUD
- Control group HUD: "Selected: N" + `[1] (3) [2] (5)` indicators

### Animation (ISS-R27)
- Created `MXAnimInstance` C++ base class — auto-reads Speed/Direction/bIsMoving from UMXAnimBridge
- Pipeline: CMC → AnimBridge (tick) → MXAnimInstance (NativeUpdateAnimation) → BS_Idle_Walk_Run
- Editor setup: reparent ABP_Unarmed to MXAnimInstance, wire BlendSpace, set AnimBlueprintClass

### New Files
- MXAnimInstance.h/cpp — C++ AnimInstance base
- MXRTSHUD.h/cpp — box select rectangle + control group HUD

### Issues Resolved
ISS-R16 through ISS-R28 (13 issues). See ISSUES.md.

### Key Patterns Discovered
- Mannequin mesh needs -90° yaw rotation (faces +Y, character forward is +X)
- `bUseControllerRotationYaw` must be false for unpossessed characters
- `FKey("1")` silently fails — must use `EKeys::One`
- Blueprint serialisation overwrites constructor values — force in BeginPlay too
- `bRunPhysicsWithNoController` required for unpossessed ACharacter movement

---

## [2026-03-10] — Playtest Issue Logging (6 new issues)

**Chat:** PM chat
**Phase:** 2C-Move (playtest)

### New Issues Logged
- ISS-019: Robots not moving on right-click move command (BROKEN)
- ISS-020: Selection broken — not all robots selectable, box select unreliable (BROKEN)
- ISS-021: Some robots embedded halfway in floor after Manny mesh revert
- ISS-022: Selection ring hard to see, name text scales with zoom (should be constant size, green)
- ISS-023: No camera pitch control — RMB should yaw AND pitch
- ISS-024: No visual indicator for control groups (HUD bar with group numbers + unit counts)

### Context
Nick reverted to standard Manny mesh in Unreal Editor. Issues 019-021 may be related to mesh/collision changes. Issues 022-024 are polish/feature requests from playtesting.

### Files Updated
- ISSUES.md — 6 new open issues (ISS-019 through ISS-024)
- MicroExodus_BugFix_Prompt.md — updated with priority issue list

---

## [2026-03-10] — GASP Integration Prompt + Cross-Chat Sync Protocol

**Chat:** PM chat
**Phase:** Planning

### Changes
- Created `MicroExodus_GASP_Integration_Prompt.md` — continuation prompt for GASP animation integration chat
- Designed UMXAnimBridge architecture: C++ component decouples engine logic from animation display
- Added Cross-Chat Sync Protocol to Claude.md — ensures changes from debug/dev/animation chats propagate via tracking docs
- Reconciled all tracking docs with debug chat fixes (ISS-R05 through ISS-R15)

### Decisions
- Cherry-pick GASP locomotion/traversal only — NOT its input, camera, or controller
- UMXAnimBridge reads CMC state, exposes UPROPERTYs to AnimBP — zero GASP headers in C++
- Animation layer fully swappable: different AnimBP + Bridge = different visual style, same engine
- Phased: GASP-A (inspection), GASP-B (locomotion), GASP-C (traversal), GASP-D (idle), GASP-E (swarm perf)

---

## [2026-03-10] — Bugs & Fixes Session: Camera, Scaling, Input, Editor SDK

**Chat:** Bugs & Fixes dev chat
**Phase:** 2C-Move (polish)

### Changes
- Fixed double-miniaturization (ISS-R05): capsule sized in constructor, mesh-only scale via `SetRelativeScale3D` — never `SetActorScale3D` on ACharacter with custom capsule
- Fixed scroll zoom (ISS-R06 → ISS-R12): went through 3 iterations, landed on `GetInputAnalogKeyState(EKeys::MouseWheelAxis)` — the only reliable UE5 approach for scroll wheel
- Fixed WASD pan direction (ISS-R07): `GetPlanarDirections()` now reads from `CameraRig->GetActorRotation().Yaw`, not `GetControlRotation()` which never updates when rotating the rig directly
- Fixed tablecloth drag (ISS-R07): removed extra `* DeltaTime` that was zeroing movement
- Implemented double-click behavior (ISS-R08): robot = center + zoom in, ground = center only
- Fixed name text position (ISS-R09): Z=26, WorldSize=2.0 for unscaled actor
- Removed edge pan again (ISS-R10): was still present despite earlier removal attempt
- Fixed keyboard input (ISS-R11): `SetInputMode(FInputModeGameAndUI)` in BeginPlay — required when `bShowMouseCursor=true` with no possessed pawn
- Fixed spawn area (ISS-R13): `SpawnRadius` UPROPERTY, circle formation around floor center
- Added Home key reset view (ISS-R14): resets position, zoom, and yaw
- Added `BindToFullProfile()` (ISS-R15): editor-visible personality, role, title, colours
- Fixed `Role` UPROPERTY shadowing `AActor::Role` — renamed to `RobotRole`
- Established Editor SDK Philosophy: all tunables = EditAnywhere, all runtime data = VisibleAnywhere

### Key Patterns Established
- **Miniaturized ACharacter scaling:** Fixed capsule + mesh-only `SetRelativeScale3D`, never `SetActorScale3D`
- **Scroll wheel input:** Must use `GetInputAnalogKeyState(EKeys::MouseWheelAxis)`
- **Keyboard with mouse cursor:** Requires `SetInputMode(FInputModeGameAndUI)` in BeginPlay
- **Camera rig rotation:** Read `CameraRig->GetActorRotation().Yaw`, not `GetControlRotation()`
- **UPROPERTY shadowing:** Custom props must not shadow inherited AActor properties

### Issues Resolved
ISS-R05 through ISS-R15 (11 issues). See ISSUES.md for full details.

---

## [2026-03-09] — Camera Fix + Issue Tracker + Bug Fix Prompt

**Chat:** Dev chat (selection & movement)
**Phase:** 2C-Move (polish)

### Changes
- Modified `MXRTSPlayerController.h/.cpp` — removed HandleEdgePan entirely (caused camera drift), removed EdgePanSpeed/EdgePanThreshold config. Added HandleDoubleClickCenter (double-left-click ground to snap camera position).
- Created `ISSUES.md` — living issue tracker with 8 open issues, 4 resolved, camera controls reference
- Created `MicroExodus_BugFix_Prompt.md` — continuation prompt for bugs & fixes dev chat

### Issues Resolved
- ISS-R03: Edge-of-screen panning removed. Camera now uses: arrows, middle-drag, scroll zoom, right-click rotate, double-click center.
- ISS-R04: Execute_GetRobotProfile crash (fixed in prior hotfix)

### New Issues
- ISS-008: Double-click center may conflict with future double-click-to-select-all-of-type

### Camera Controls (Current)
Arrow keys/WASD=pan, middle-drag=tablecloth pan, right-drag=rotate, scroll=zoom, double-click=center, left-click=select, left-drag=box select, short right-click=move command

---

## [2026-03-09] — Themed Name Evolution: Per-Robot Flavour Packs

**Chat:** Dev chat (DEMS themed naming)
**Phase:** Identity Module Enhancement

### Changes
- Created `MXNameTheme.h/.cpp` — UMXNameThemeManager registry with 6 built-in themes (Robot, Wizard, Pirate, Samurai, SciFi, Mythic). Per-robot themes stamped at birth from the run's selection.
- Created `MXNameEvolution.h/.cpp` — theme-aware name composer. Reads profile.name_theme to select correct surname pool + title aliases.
- Created `ThemedNameEvolution_SetupGuide.md` — deployment instructions

### Design
- Theme is per-RUN, not global. Player picks theme at run launch. All robots born/rescued inherit it.
- Robots keep their theme forever → mixed roster from different themed runs
- Name formula: `[FirstName] [Surname] [Prefix Title]`
  - Pirate: "Barnacle Planksworth Captain Scourge"
  - Wizard: "Ember Hexfire The Great Pyromancer"
  - Samurai: "Kumo Hayabusa Honourable Flame Dancer"
- Each theme provides: first name pool, per-role surnames, title aliases, title prefix
- 6 themes × ~30 first names + ~26 surnames + ~14 title aliases each = ~420 names total
- Themes unlockable via achievements (Robot always available)
- DEMS templates unaffected — they consume {name}/{title} which NameEvolution composes

### Required Patches (3 existing files)
1. `MXRobotProfile.h` — add `FString surname` + `ENameTheme name_theme` fields
2. `MXGameInstance.h/.cpp` — create and wire ThemeManager + NameEvolution in Init()
3. `MXRobotManager.cpp` — stamp run theme on CreateRobot, call EvaluateNameEvolution post-run

### Files Delivered
- MXNameTheme.h, MXNameTheme.cpp
- MXNameEvolution.h, MXNameEvolution.cpp
- ThemedNameEvolution_SetupGuide.md

---

## [2026-03-09] — Phase 2C-Move: Selection + Click-to-Move + Procedural Floor

**Chat:** Dev chat (selection & movement objective)
**Phase:** 2C-Move

### Changes
- Created `MXSelectionManager.h/.cpp` — selection component: left-click single select, box-drag select, Shift+click additive, Ctrl+1-9 control groups, Ctrl+A select all, hover tracking
- Created `MXTestFloorGenerator.h/.cpp` — procedural grid floor (10x10 tiles, 200cm each) with collision via engine Cube mesh. Checkerboard pattern. Replaces manual Plane mesh.
- Modified `MXRobotActor.h/.cpp` — added SetSelected/SetHovered, MoveToLocation/StopMoving with CharacterMovementComponent, selection ring (cylinder mesh), conditional name display (hidden by default, shown on hover or select). Removed GravityScale=0 hack.
- Modified `MXRTSPlayerController.h/.cpp` — integrated UMXSelectionManager as component, added HandleLeftMouseInput (click + box drag), HandleRightClickMove (short right-click = move command), HandleControlGroups (Ctrl+number save, number recall), HandleSelectAll (Ctrl+A). Right-click distinguishes move vs rotate by click duration (<0.25s).
- Modified `MXSpawnTestGameMode.h/.cpp` — rewired spawn order: floor → robots → camera. Robots spawn at random floor positions. FloorGeneratorClass configurable. Removed manual floor plane spawning.
- Created `Phase2C_Move_SetupGuide.md` — deployment instructions, compilation notes, test checklist

### Decisions
- Selection state stored as TWeakObjectPtr<AMXRobotActor> for safety (handles actor destruction)
- Movement uses ACharacter::AddMovementInput — CharacterMovementComponent handles physics/gravity
- Multi-robot move command uses circular formation (spacing = 40cm * sqrt(N))
- Right-click move vs rotate distinguished by click duration threshold (0.25s) and drag distance (10px)
- Floor uses /Engine/BasicShapes/Cube scaled flat rather than ProceduralMeshComponent for simplicity
- Box select threshold 5px to distinguish click from drag

### Issues Resolved
- Issue #2: Name text now hidden by default, shown on hover/selection only
- Issue #3: Procedural floor with collision replaces manual Plane. GravityScale=0 removed.

### New Issues
- Issue #6: Box select rectangle not drawn on screen (needs HUD class — selection still works)
- Issue #7: Right-click move/rotate time threshold may need tuning
- Issue #8: Floor tile BasicShapeMaterial may not accept "Color" vector param (cosmetic)

### Compilation Notes
- May need `GetRobotManager()` accessor on UMXGameInstance
- May need to adjust `GetRobotProfile()` call to use interface Execute_ pattern
- See Phase2C_Move_SetupGuide.md for full details

### Files Delivered
- MXSelectionManager.h, MXSelectionManager.cpp
- MXTestFloorGenerator.h, MXTestFloorGenerator.cpp
- MXRobotActor.h, MXRobotActor.cpp (modified)
- MXRTSPlayerController.h, MXRTSPlayerController.cpp (modified)
- MXSpawnTestGameMode.h, MXSpawnTestGameMode.cpp (modified)
- Phase2C_Move_SetupGuide.md

---

## [2026-03-09] — PM Audit: Tracking Document Refresh

**Phase:** Project Management

### Changes
- Updated `Claude.md` — fixed Phase 2B status from "In Progress" to "Complete", added Phase 2-Next to roadmap, added doc-sync convention to Critical Conventions
- Updated `Agents.md` — bumped date to 2026-03-09
- Verified `README.md` — already reflects current state. No changes needed.
- Verified `CHANGE_LOG.md` — entries accurate and chronological.

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
- Created `MXRobotFactoryData.h` — EFactoryUpgrade, ERecruitSource, FMXFactoryState, FMXRecruitCandidate, FMXSalvageDrop, FMXLobbyPool
- Created `MXRobotCodec.h/.cpp` — GUID-to-shareable-code conversion (Base32 Crockford)
- Created `MXRobotFactory.h/.cpp` — lobby manager: BuildLobbyPool, ToggleCandidateSelection, RedeemCode, PurchaseUpgrade

### Decisions
- Robot codes are generation seeds (not compressed GUIDs)
- CodeTerminal upgrade costs 50 salvage parts
- 8 factory upgrades with 1.5× exponential cost scaling

---

## [2026-03-02] — Agent 13: Robot Assembly Module

**Phase:** 1 (Engine Logic)

### Changes
- Created `MXRobotAssemblyData.h` — EPartSlot (5 slots), FMXPartDefinition, FMXAssemblyRecipe
- Created `MXRobotAssembler.h/.cpp` — modular mesh system with per-part LOD
- Created `MXPartDestructionManager.h/.cpp` — per-part damage tracking

---

## [2026-03-01] — Procedural Generation Module

**Phase:** 1 (Engine Logic)

### Changes
- Created procedural level generation system (20-level layouts)
- Room types: corridors, arenas, gauntlets, puzzle rooms
- Hazard placement with element theming per level

---

## [2026-03-01] — Compilation Audit & Fix Session

**Phase:** 1 (Engine Logic)

### Changes
- Audited entire codebase for compilation errors
- Catalogued 13 recurring error patterns across all agent-generated modules
- Established flat include structure as project convention
- All 94+ files (Phases 1-9) confirmed compiling

### Decisions
- Flat source structure mandatory
- Forward declarations preferred over full includes in headers
- EventBus lookup pattern standardised
- For systematic errors, full audit first — not reactive one-at-a-time patching

---

## [Pre-2026-03-01] — Initial Module Generation

**Phase:** 1 (Engine Logic)

### Changes
- 12 original modules generated by specialised AI agents (Agents 0-11)
- Each agent built independently without awareness of others
- ~94 initial source files created
