# MicroExodus — Issue Tracker

**Last Updated:** 2026-03-09

All known issues, bugs, and improvement requests. Issues are resolved in dev chats and marked with status. Resolved issues are kept for history.

---

## Open Issues

### ISS-001 — Robots in T-pose (no AnimBP)
**Severity:** Visual | **Phase:** 2A | **Since:** 2026-03-08
**Description:** Robots spawn with default T-pose because no Animation Blueprint is assigned to the skeletal mesh. Movement works (CharacterMovementComponent) but robots slide in T-pose.
**Blocked by:** Need to create ABP_MXRobot from Third Person locomotion BlendSpace, wire speed variable. Also need to inspect SandboxCharacter_CMC.uasset for skeleton compatibility.
**Fix plan:** Phase 3 — mannequin mesh, materials, idle/walk animation.

### ISS-002 — Scroll wheel zoom uses IsInputKeyDown polling
**Severity:** Minor UX | **Phase:** 2B | **Since:** 2026-03-08
**Description:** `HandleZoom()` polls `IsInputKeyDown(EKeys::MouseScrollUp/Down)` which may miss fast scroll inputs. UE5's input axis events would be more reliable.
**Fix plan:** Switch to `GetInputAxisKeyValue(EKeys::MouseWheelAxis)` or bind via Enhanced Input.

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

### ISS-008 — Double-click center camera may conflict with double-click select
**Severity:** UX | **Phase:** 2C | **Since:** 2026-03-09
**Description:** `HandleDoubleClickCenter()` triggers on any double-left-click. If the user double-clicks a robot (intending "select all of type" or similar future feature), it also centers the camera. May need to check if a robot was hit first.
**Fix plan:** In HandleDoubleClickCenter, raycast for robots first. If a robot is hit, skip the center and let the selection system handle it.

---

## Resolved Issues

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

---

## Camera Controls Reference (Current)

| Input | Action |
|-------|--------|
| Arrow keys / WASD | Pan camera |
| Middle-click drag | Tablecloth drag pan |
| Right-click drag | Rotate camera yaw |
| Scroll wheel | Zoom in/out |
| Double-click ground | Center camera on click point |
| Left-click | Select robot |
| Left-click drag | Box select |
| Shift+click | Add/remove from selection |
| Right-click (short) | Move selected robots to location |
| Ctrl+1-9 | Save control group |
| 1-9 | Recall control group |
| Ctrl+A | Select all |
