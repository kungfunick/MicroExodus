# MicroExodus — Issue Tracker

**Last Updated:** 2026-03-10

All known issues, bugs, and improvement requests. Issues are resolved in dev chats and marked with status. Resolved issues are kept for history.

---

## Open Issues

### ISS-001 — Robots in T-pose (no AnimBP)
**Severity:** Visual | **Phase:** 2A | **Since:** 2026-03-08
**Description:** Robots spawn with default T-pose because no Animation Blueprint is assigned to the skeletal mesh. Movement works (CharacterMovementComponent) but robots slide in T-pose.
**Blocked by:** Need to create ABP_MXRobot from Third Person locomotion BlendSpace, wire speed variable. Also need to inspect SandboxCharacter_CMC.uasset for skeleton compatibility.
**Fix plan:** Phase 3 — mannequin mesh, materials, idle/walk animation.

### ISS-002 — ~~Scroll wheel zoom uses IsInputKeyDown polling~~
**Severity:** Minor UX | **Phase:** 2B | **Since:** 2026-03-08
**Status:** RESOLVED 2026-03-10 — Replaced with `WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)`. See ISS-R06.

### ISS-003 — SandboxCharacter_CMC.uasset not inspected
**Severity:** Planning | **Phase:** 2-Next | **Since:** 2026-03-08
**Description:** The migrated Blueprint from Game Animation Sample hasn't been opened to verify skeleton compatibility, animation retarget setup, or CharacterMovementComponent configuration.
**Fix plan:** Open in editor, document skeleton type, socket setup, AnimBP references.

### ISS-004 — Box select rectangle not drawn on screen
**Severity:** Visual/UX | **Phase:** 2C | **Since:** 2026-03-09
**Description:** Box selection works functionally (robots get selected on release) but the selection rectangle is not drawn on screen during drag. Needs a HUD class with Canvas drawing or UMG overlay.
**Fix plan:** Create `AMXRTSHUD` with `DrawHUD()` override, draw box rect from SelectionManager's BoxStart/BoxEnd.

### ISS-005 — Right-click move vs rotate threshold may need tuning
**Severity:** UX | **Phase:** 2C | **Since:** 2026-03-09
**Description:** Right-click distinguishes move command from camera rotation by click duration (<0.25s, <10px drag = move). May feel unresponsive or trigger unintended moves. Needs playtesting.
**Fix plan:** Playtest and adjust thresholds. Consider: right-click always = move when selection exists, Alt+right-click = rotate.

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

---

## Camera Controls Reference (Current)

| Input | Action |
|-------|--------|
| Arrow keys / WASD | Pan camera (follows camera yaw) |
| Middle-click drag | Tablecloth drag pan |
| Right-click drag | Rotate camera yaw |
| Scroll wheel | Zoom in/out (analog axis) |
| Home | Reset view (center, default zoom, 0° yaw) |
| Double-click ground | Center camera on click point (no zoom) |
| Double-click robot | Center + zoom in to robot |
| Left-click | Select robot |
| Left-click drag | Box select |
| Shift+click | Add/remove from selection |
| Right-click (short) | Move selected robots to location |
| Ctrl+1-9 | Save control group |
| 1-9 | Recall control group |
| Ctrl+A | Select all |
