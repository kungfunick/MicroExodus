# MicroExodus — Issue Tracker

**Last Updated:** 2026-03-15 (comprehensive sync — all sessions consolidated)

All known issues, bugs, and improvement requests. Issues are resolved in dev chats and marked with status. Resolved issues are kept for history.

---

## Open Issues

### ISS-001 — Robots in T-pose (no AnimBP)
**Severity:** Visual | **Phase:** 2A | **Since:** 2026-03-08
**Status:** PARTIALLY RESOLVED 2026-03-15 — Animation pipeline working (MXAnimInstance + BS_Idle_Walk_Run). Requires manual editor setup: reparent ABP_Unarmed to MXAnimInstance, wire BlendSpace Speed/Direction, set AnimBlueprintClass on BP_MXRobot. Full GASP integration (distance matching, traversal, idle variants) deferred to Phase GASP-B.

### ISS-002 — ~~Scroll wheel zoom uses IsInputKeyDown polling~~
**Severity:** Minor UX | **Phase:** 2B | **Since:** 2026-03-08
**Status:** RESOLVED 2026-03-10 — Replaced with `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)`. See ISS-R06.

### ISS-003 — SandboxCharacter_CMC.uasset not inspected
**Severity:** Planning | **Phase:** 2-Next | **Since:** 2026-03-08
**Description:** The migrated Blueprint from Game Animation Sample hasn't been opened to verify skeleton compatibility, animation retarget setup, or CharacterMovementComponent configuration.
**Fix plan:** Phase GASP-A — first action in the GASP integration chat. See `MicroExodus_GASP_Integration_Prompt.md`.

### ISS-004 — ~~Box select rectangle not drawn on screen~~
**Severity:** Visual/UX | **Phase:** 2C | **Since:** 2026-03-09
**Status:** RESOLVED 2026-03-10 — Created AMXRTSHUD class. See ISS-R23.

### ISS-005 — ~~Right-click move vs rotate threshold may need tuning~~
**Severity:** UX | **Phase:** 2C | **Since:** 2026-03-09
**Status:** RESOLVED 2026-03-10 — No longer applicable. Input scheme remapped: RMB = camera only, LMB = select + move. See ISS-R21.

### ISS-006 — Floor tile material may not accept "Color" parameter
**Severity:** Cosmetic | **Phase:** 2C | **Since:** 2026-03-09
**Description:** `AMXTestFloorGenerator` creates `UMaterialInstanceDynamic` from BasicShapeMaterial and calls `SetVectorParameterValue("Color", ...)`. BasicShapeMaterial may not expose a "Color" parameter — tiles may appear default grey/white instead of dark checkerboard.
**Fix plan:** Check parameter name in editor. May need to use a custom material or different parameter name (e.g., "BaseColor").

### ISS-007 — Themed name system not yet wired
**Severity:** Feature incomplete | **Phase:** 2C | **Since:** 2026-03-09
**Description:** MXNameTheme and MXNameEvolution files are compiled but not wired into GameInstance, RobotManager, or RunManager. Robots still use plain first names only. Three patches documented in ThemedNameEvolution_SetupGuide.md.
**Fix plan:** Apply Patches 1-3 from the setup guide: profile fields, GameInstance wiring, RobotManager stamping.

### ISS-008 — ~~Double-click center camera may conflict with double-click select~~
**Severity:** UX | **Phase:** 2C | **Since:** 2026-03-09
**Status:** RESOLVED 2026-03-10 — Replaced with proper HandleDoubleClick: raycast for robot first (zoom in), ground hit (center only, no zoom). See ISS-R08.

### ISS-019 — ~~Robots not moving when selected and right-click issued~~
**Severity:** Gameplay (broken) | **Phase:** 2C | **Since:** 2026-03-10
**Status:** RESOLVED 2026-03-10 — See ISS-R16.

### ISS-020 — ~~Selection broken: not all robots selected, box select unreliable~~
**Severity:** Gameplay (broken) | **Phase:** 2C | **Since:** 2026-03-10
**Status:** RESOLVED 2026-03-10 — See ISS-R17.

### ISS-021 — ~~Some robots appear halfway embedded in the floor~~
**Severity:** Visual | **Phase:** 2C | **Since:** 2026-03-10
**Status:** RESOLVED 2026-03-10 — See ISS-R18.

### ISS-022 — ~~Selection indicator hard to see, name text scales with zoom~~
**Severity:** UX | **Phase:** 2C | **Since:** 2026-03-10
**Status:** RESOLVED 2026-03-10 — See ISS-R19.

### ISS-023 — ~~No camera pitch control (RMB should pitch and yaw)~~
**Severity:** Feature request | **Phase:** 2C | **Since:** 2026-03-10
**Status:** RESOLVED 2026-03-10 — See ISS-R20.

### ISS-024 — ~~No visual indicator for control groups~~
**Severity:** Feature request | **Phase:** 2C | **Since:** 2026-03-10
**Status:** RESOLVED 2026-03-10 — Created AMXRTSHUD with group display. See ISS-R23.

---

## Resolved Issues

### ~~ISS-R05~~ — Robots hover above floor plane (ISS-009)
**Severity:** Visual | **Phase:** 2C | **Resolved:** 2026-03-09
**Root cause:** Double-miniaturization. Constructor set mini capsule `InitCapsuleSize(8, 18)`, then BeginPlay called `SetActorScale3D(0.2)` on the entire actor, shrinking the capsule to an effective 3.6cm half-height. Spawn collision adjustment positioned the capsule for the pre-scale size, then the scale change left it floating. Additionally, the mesh relative Z was still at the ACharacter default (−88) instead of matching our mini capsule (−18).
**Fix:** (1) Constructor: added `GetMesh()->SetRelativeLocation(FVector(0, 0, -18))` after `InitCapsuleSize`. (2) BeginPlay: replaced `SetActorScale3D(RobotScale)` with `GetMesh()->SetRelativeScale3D(RobotScale)` — scales only the mesh, not the actor/capsule. (3) Reduced spawn Z offset from 20 to 5. Files: MXRobotActor.cpp, MXSpawnTestGameMode.cpp.

### ~~ISS-R06~~ — Scroll zoom not working (ISS-002)
**Severity:** UX | **Phase:** 2B → Fixed 2C | **Resolved:** 2026-03-10
**Root cause:** `IsInputKeyDown(EKeys::MouseScrollUp/Down)` unreliable — scroll fires as single-frame press/release events that `IsInputKeyDown` misses at high framerates.
**Fix:** Replaced with `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)` which catches the frame transition. File: MXRTSPlayerController.cpp.

### ~~ISS-R07~~ — WASD pan direction doesn't follow camera yaw + tablecloth drag broken (ISS-010)
**Severity:** UX | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** Two bugs. (1) `GetPlanarDirections()` used `GetControlRotation()` which never updates when we rotate the camera rig actor directly — pan always went in the initial world direction. (2) `HandleDragPan()` multiplied the per-frame mouse delta by `DeltaTime` a second time, effectively zeroing the movement (~0.016 at 60fps).
**Fix:** (1) `GetPlanarDirections()` now reads `CameraRig->GetActorRotation().Yaw` so WASD/drag follow camera orientation. (2) Removed extra `* DeltaTime` from drag pan, added zoom-proportional scaling. File: MXRTSPlayerController.cpp.

### ~~ISS-R08~~ — Double-click behavior implemented (ISS-008 + ISS-011)
**Severity:** Feature | **Phase:** 2C | **Resolved:** 2026-03-10
**Description:** Double-click on robot: centers camera and zooms in to `DoubleClickRobotZoom` (150cm arm length). Double-click on ground: centers camera on that point, no zoom change. Uses 0.3s/20px thresholds (configurable UPROPERTYs).
**Files:** MXRTSPlayerController.h/cpp.

### ~~ISS-R09~~ — Name text appeared in centre of mesh (ISS-012)
**Severity:** Visual | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** NameTextComponent relative Z=45 and WorldSize=8 were designed for actor-scale 0.2 (effective Z=9, size=1.6). After ISS-009 fix (mesh-only scale), actor is unscaled so Z=45 was above the robot and WorldSize=8 was giant. SelectionRing Z=-17 was also slightly off.
**Fix:** NameText relative Z → 26 (8cm above head), WorldSize → 2.0. SelectionRing relative Z → -18 (exact feet). File: MXRobotActor.cpp.

### ~~ISS-R01~~ — Name text display always visible
**Severity:** Visual | **Phase:** 2A → Fixed 2C | **Resolved:** 2026-03-09
**Fix:** Names hidden by default, shown on hover/selection only. `SetSelected()`/`SetHovered()` on AMXRobotActor.

### ~~ISS-R02~~ — Manual Plane mesh, no collision, GravityScale=0 hack
**Severity:** Gameplay | **Phase:** 2A → Fixed 2C | **Resolved:** 2026-03-09
**Fix:** `AMXTestFloorGenerator` spawns collision-enabled cube tiles. GravityScale restored to 1.0.

### ~~ISS-R03~~ — Edge-of-screen panning causes camera drift
**Severity:** UX | **Phase:** 2B → Fixed 2C | **Resolved:** 2026-03-09
**Fix:** Removed `HandleEdgePan()` entirely. Camera controls: arrow keys, middle-click tablecloth drag, double-click center, right-click rotate, scroll zoom.

### ~~ISS-R04~~ — GetRobotProfile called directly on interface (assertion crash)
**Severity:** Crash | **Phase:** 2C | **Resolved:** 2026-03-09
**Fix:** Changed to `IMXRobotProvider::Execute_GetRobotProfile()` pattern. Added `#include "MXInterfaces.h"`.

### ~~ISS-R10~~ — Edge-of-screen pan still active (ISS-013)
**Severity:** UX | **Phase:** 2C | **Resolved:** 2026-03-10
**Description:** `HandleEdgePan()` was still called in PlayerTick and function body still present despite ISS-R03 noting it was removed. Edge pan was causing unwanted camera drift when mousing to screen edges.
**Fix:** Removed `HandleEdgePan()` call from PlayerTick, deleted entire function body, removed `EdgePanSpeed` and `EdgePanThreshold` UPROPERTYs and declaration from header. Files: MXRTSPlayerController.h/cpp.

### ~~ISS-R11~~ — WASD / arrow keys not working (ISS-014)
**Severity:** UX | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** With `bShowMouseCursor = true` and `DefaultPawnClass = nullptr`, UE5 defaults to UI-only input mode. Mouse events work through the separate click/hover pipeline (`bEnableClickEvents`), but keyboard events don't reach `IsInputKeyDown` without explicit game input mode.
**Fix:** Added `SetInputMode(FInputModeGameAndUI())` in BeginPlay with `DoNotLock` mouse and `HideCursorDuringCapture(false)`. File: MXRTSPlayerController.cpp.

### ~~ISS-R12~~ — Scroll wheel zoom not working (ISS-015)
**Severity:** UX | **Phase:** 2B | **Resolved:** 2026-03-10
**Root cause:** Both `IsInputKeyDown` and `WasInputKeyJustPressed` are unreliable for scroll wheel in UE5. The scroll wheel fires as a sub-frame analog impulse, not a sustained digital key state, so digital key detection misses it.
**Fix:** Replaced with `GetInputAnalogKeyState(EKeys::MouseWheelAxis)` which reads the raw analog scroll value each tick. This is the standard UE5 approach. File: MXRTSPlayerController.cpp.

### ~~ISS-R13~~ — Robots spawn across entire floor, fall off edges (ISS-016)
**Severity:** Gameplay | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** `GetRandomFloorPosition(100)` returns positions across the entire 2020cm floor. 8 tiny 36cm robots scattered across 4 million cm² is far too sparse, and edge positions risk falling off.
**Fix:** Added `SpawnRadius` UPROPERTY (default 300cm) to AMXSpawnTestGameMode. Robots now spawn in a circle formation around floor center instead of random full-floor positions. File: MXSpawnTestGameMode.h/cpp.

### ~~ISS-R14~~ — No reset view key (ISS-017)
**Severity:** Feature | **Phase:** 2C | **Resolved:** 2026-03-10
**Fix:** Home key resets camera to initial position (floor center), default zoom (800cm), and 0° yaw. Added `DefaultZoom` UPROPERTY (800) and `InitialCameraPos` tracking. File: MXRTSPlayerController.h/cpp.

### ~~ISS-R15~~ — Robot actors lack editor-visible profile data (ISS-018)
**Severity:** Feature | **Phase:** 2C | **Resolved:** 2026-03-10
**Fix:** Added `BindToFullProfile(FMXRobotProfile)` to AMXRobotActor. Populates VisibleAnywhere UPROPERTYs: PersonalityDescription, Quirk, Likes, Dislikes, Role, Level, DisplayedTitle, ChassisColor, EyeColor. SpawnTestGameMode calls full binding when Identity module is available. Click a robot in the editor Outliner → Details panel shows all generated data. Files: MXRobotActor.h/cpp, MXSpawnTestGameMode.cpp.

### ~~ISS-R16~~ — Robots not moving (ISS-019)
**Severity:** Gameplay blocker | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** Robots are unpossessed ACharacters (no PlayerController attached). `UCharacterMovementComponent::bRunPhysicsWithNoController` defaults to `false`, which causes the CMC to skip its entire tick. `AddMovementInput()` accumulated pending input but CMC never consumed it.
**Fix:** Set `CMC->bRunPhysicsWithNoController = true` in AMXRobotActor constructor. File: MXRobotActor.cpp.

### ~~ISS-R17~~ — Selection broken (ISS-020)
**Severity:** Gameplay blocker | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** Capsule was 16cm diameter (radius 8) — very small click target. The Pawn collision profile wasn't explicitly set, so depending on Blueprint serialisation the capsule might not block ECC_Pawn traces used by `RaycastForRobot()`.
**Fix:** (1) Increased capsule to `InitCapsuleSize(14, 20)` — 28cm diameter, easier to click. (2) Added explicit `SetCollisionProfileName("Pawn")` to ensure ECC_Pawn traces always hit. Files: MXRobotActor.cpp.

### ~~ISS-R18~~ — Robots embedded in floor (ISS-021)
**Severity:** Visual | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** Spawn Z was 5cm, but capsule half-height was 18→20cm. Capsule bottom sat at Z=5-20=-15, well below the floor surface at Z=0.
**Fix:** (1) Capsule half-height now 20. Spawn Z set to 20 so capsule bottom sits exactly at floor Z=0. (2) Mesh relative Z offset updated to -20 to match new half-height. Files: MXRobotActor.cpp, MXSpawnTestGameMode.cpp.

### ~~ISS-R19~~ — Selection ring too small, name not constant size, wrong colours (ISS-022)
**Severity:** UX | **Phase:** 2C | **Resolved:** 2026-03-10
**Fix:** (a) Selection ring scale 0.25→0.40 for visibility. (b) Name text WorldSize now scales inversely with camera distance each tick (`Dist * 0.03f`, clamped 5–50) for constant screen size. (c) All colours changed to green: SelectionColor `(0, 0.85, 0.25, 1)`, SelectedNameColor `(0, 220, 60)`, HoveredNameColor `(100, 200, 100)`. Files: MXRobotActor.h/cpp.

### ~~ISS-R20~~ — Camera pitch control (ISS-023)
**Severity:** Feature | **Phase:** 2C | **Resolved:** 2026-03-10
**Fix:** `HandleRotation()` now on Middle Mouse (was RMB). Reads DeltaY for pitch on SpringArm, clamped PitchMin(-80°) to PitchMax(-10°). Reset view resets pitch to DefaultPitch(-45°). Files: MXRTSPlayerController.h/cpp.

### ~~ISS-R21~~ — Input scheme remap (ISS-025)
**Severity:** UX overhaul | **Phase:** 2C | **Resolved:** 2026-03-10
**Description:** Fundamental input remap: LMB does both selection AND movement. RMB = camera pan only. MMB = camera rotation. Old scheme had RMB split between camera and move commands via fragile timing thresholds.
**Fix:** (1) LMB click robot = select. LMB click ground with selection = move command. LMB drag = box select. (2) RMB = tablecloth drag pan (was middle mouse). (3) MMB = yaw+pitch rotation (was RMB). (4) Deleted `HandleRightClickMove()` entirely. (5) DragPanSpeed 2→8. Files: MXRTSPlayerController.h/cpp.

### ~~ISS-R22~~ — Control groups not working (ISS-026)
**Severity:** Gameplay | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** `FKey("1")` ≠ `EKeys::One`. UE5 key names are "One","Two",etc. `WasInputKeyJustPressed(FKey("1"))` silently never matched. Also used Ctrl instead of Shift for save.
**Fix:** Replaced with proper `EKeys::One` through `EKeys::Nine` array. Save modifier changed from Ctrl to Shift. Files: MXRTSPlayerController.cpp.

### ~~ISS-R23~~ — Box select rectangle not visible + control group HUD (ISS-004 + ISS-024)
**Severity:** UX | **Phase:** 2C | **Resolved:** 2026-03-10
**Fix:** Created `AMXRTSHUD` class (MXRTSHUD.h/cpp). DrawBoxSelectRect() draws green filled rectangle with border during LMB drag. DrawControlGroups() shows "Selected: N" count, assigned groups as `[1] (3) [2] (5)`, and hint text. Wired to GameMode via `HUDClass`. Added `GetControlGroupCount()` query to SelectionManager. Files: MXRTSHUD.h/cpp, MXSelectionManager.h/cpp, MXSpawnTestGameMode.cpp.

### ~~ISS-R24~~ — Name text too small and scaling wrong (ISS-027)
**Severity:** UX | **Phase:** 2C | **Resolved:** 2026-03-10
**Root cause:** `Dist * 0.008f` at 500cm = 4cm character height — unreadable.
**Fix:** Changed to `Dist * 0.03f`, clamped 5–50. At 500cm → 15cm characters — clearly readable at default zoom. File: MXRobotActor.cpp.

### ~~ISS-R25~~ — Mannequin mesh faces wrong direction (ISS-028)
**Severity:** Visual | **Phase:** 2C | **Resolved:** 2026-03-15
**Root cause:** UE5 mannequin mesh faces +Y, but ACharacter's forward axis is +X. Without a -90° yaw correction, robots visually face sideways while moving correctly.
**Fix:** Added `GetMesh()->SetRelativeRotation(FRotator(0, -90, 0))` in constructor. File: MXRobotActor.cpp.

### ~~ISS-R26~~ — Robots don't turn to face movement direction (ISS-029)
**Severity:** Gameplay | **Phase:** 2C | **Resolved:** 2026-03-15
**Root cause:** Three compounding issues: (1) `bUseControllerRotationYaw = true` (default or BP-serialised) overrides CMC rotation — unpossessed robots have no controller so rotation target is always 0°. (2) Manual `SetActorRotation()` in TickMovement fought CMC's `bOrientRotationToMovement`. (3) Blueprint serialisation overwrote constructor values.
**Fix:** (1) Set `bUseControllerRotationYaw/Pitch/Roll = false` in both constructor AND BeginPlay. (2) Removed manual `SetActorRotation` from TickMovement. (3) Force `bOrientRotationToMovement = true` in BeginPlay. (4) Bumped RotationRate to 720°/s. File: MXRobotActor.cpp.

### ~~ISS-R27~~ — Animation pipeline (MXAnimInstance + walk/run blending) (ISS-030)
**Severity:** Feature | **Phase:** 2C | **Resolved:** 2026-03-15
**Description:** Created MXAnimInstance C++ AnimInstance base class that auto-reads Speed, Direction, bIsMoving, bIsFalling, LeanAngle from UMXAnimBridge every tick via NativeUpdateAnimation. Properties are directly usable as BlendSpace axes. MoveSpeed bumped 150→400, added WalkDistance=150 for distance-based speed scaling (walk close, run far). MaxAcceleration 800→1600.
**Editor setup:** Reparent ABP_Unarmed to MXAnimInstance, wire BS_Idle_Walk_Run with Speed/Direction, set AnimBlueprintClass on BP_MXRobot.
**Files:** MXAnimInstance.h/cpp (new), MXRobotActor.h/cpp.

### ~~ISS-R28~~ — DragPanSpeed tuned (ISS-031)
**Severity:** UX | **Phase:** 2C | **Resolved:** 2026-03-15
**Fix:** DragPanSpeed default adjusted to 3.0 (was 2.0 invisible, 8.0 too fast). Property on PlayerController under MX|RTS|Camera. File: MXRTSPlayerController.h.

---

## Camera Controls Reference (Current)

| Input | Action |
|-------|--------|
| Arrow keys / WASD | Pan camera (follows camera yaw) |
| **LMB click robot** | Select (Shift = additive) |
| **LMB click ground** (with selection) | Move selected robots to location |
| **LMB drag** | Box select (green rectangle drawn) |
| **RMB drag** | Tablecloth drag pan (camera movement) |
| **MMB drag** | Camera yaw (mouse X) + pitch (mouse Y) |
| Scroll wheel | Zoom in/out (analog axis) |
| Home | Reset view (center, default zoom/pitch/yaw) |
| Double-click ground | Center camera on click point (no zoom) |
| Double-click robot | Center + zoom in to robot |
| Shift+1-9 | Save control group |
| 1-9 | Recall control group |
| Ctrl+A | Select all |
