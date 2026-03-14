# MicroExodus — Issue Tracker

**Last Updated:** 2026-03-13

All known issues, bugs, and improvement requests. Issues are resolved in dev chats and marked with status. Resolved issues are kept for history.

---

## Open Issues

### ISS-001 — Robots in T-pose (no AnimBP)
**Severity:** Visual | **Phase:** 2A | **Since:** 2026-03-08
**Description:** Robots spawn with default T-pose because no Animation Blueprint is assigned to the skeletal mesh. Movement works (CharacterMovementComponent) but robots slide in T-pose.
**Progress:** UMXAnimBridge component delivered (2026-03-13). Three animation enums in MXTypes.h. AnimBridge auto-attached to AMXRobotActor. GASP removed — using built-in UE5 Manny animations instead.
**Fix plan:** Create ABP_MXRobot AnimBP in editor with BS_MXRobot_Locomotion BlendSpace1D (Idle→Walk→Run driven by AnimBridge.Speed). Assign to SkeletalMeshComponent. See GASP_Removal_And_BasicLocomotion_Guide.md Steps 2–4.

### ISS-004 — Box select rectangle not drawn on screen
**Severity:** Visual/UX | **Phase:** 2C | **Since:** 2026-03-09
**Description:** Box selection works functionally but no rectangle drawn during drag.
**Fix plan:** Create `AMXRTSHUD` with `DrawHUD()` override.

### ISS-005 — Right-click move vs rotate threshold may need tuning
**Severity:** UX | **Phase:** 2C | **Since:** 2026-03-09
**Fix plan:** Playtest and adjust thresholds.

### ISS-006 — Floor tile material may not accept "Color" parameter
**Severity:** Cosmetic | **Phase:** 2C | **Since:** 2026-03-09
**Fix plan:** Check parameter name in editor. May need "BaseColor".

### ISS-007 — Themed name system not yet wired
**Severity:** Feature incomplete | **Phase:** 2C | **Since:** 2026-03-09
**Fix plan:** Apply Patches 1-3 from ThemedNameEvolution_SetupGuide.md.

---

## Resolved Issues

### ~~ISS-R17~~ — GASP removed, SandboxCharacter_CMC deleted (ISS-003)
**Severity:** Planning | **Phase:** Anim-A | **Resolved:** 2026-03-13
**Description:** GASP was producing errors. Decision: remove all GASP content, use built-in UE5 Manny animations instead. SandboxCharacter_CMC.uasset deleted. GASP integration prompt retired. AnimBridge (zero GASP deps) retained.
**Files removed:** Content/Blueprints/SandboxCharacter_CMC.uasset, MicroExodus_GASP_Integration_Prompt.md

### ~~ISS-R16~~ — UMXAnimBridge component created (ISS-001 partial)
**Severity:** Feature | **Phase:** Anim-A | **Resolved:** 2026-03-13
**Description:** UMXAnimBridge ActorComponent delivered. Reads CMC state, exposes animation variables. Zero GASP dependencies.
**Files:** MXAnimBridge.h/.cpp, three enums in MXTypes.h, AnimBridge on AMXRobotActor.

### ~~ISS-R05~~ — Robots hover above floor plane (ISS-009)
**Resolved:** 2026-03-09. Mesh-only scale, fixed capsule, corrected Z offset.

### ~~ISS-R06~~ — Scroll zoom not working (ISS-002)
**Resolved:** 2026-03-10. `GetInputAnalogKeyState(EKeys::MouseWheelAxis)`.

### ~~ISS-R07~~ — WASD pan + tablecloth drag broken (ISS-010)
**Resolved:** 2026-03-10. Read CameraRig yaw, remove extra DeltaTime.

### ~~ISS-R08~~ — Double-click behavior (ISS-008 + ISS-011)
**Resolved:** 2026-03-10. Robot=center+zoom, ground=center only.

### ~~ISS-R09~~ — Name text position (ISS-012)
**Resolved:** 2026-03-10. Z=26, WorldSize=2.0.

### ~~ISS-R01~~ — Name text always visible
**Resolved:** 2026-03-09. Hidden by default, shown on hover/select.

### ~~ISS-R02~~ — Manual Plane, no collision, GravityScale=0
**Resolved:** 2026-03-09. Procedural floor with collision.

### ~~ISS-R03~~ — Edge pan drift
**Resolved:** 2026-03-09. HandleEdgePan removed.

### ~~ISS-R04~~ — Interface assertion crash
**Resolved:** 2026-03-09. Execute_ pattern.

### ~~ISS-R10~~ — Edge pan still present (ISS-013)
**Resolved:** 2026-03-10. Fully purged.

### ~~ISS-R11~~ — Keyboard not working (ISS-014)
**Resolved:** 2026-03-10. SetInputMode(FInputModeGameAndUI).

### ~~ISS-R12~~ — Scroll wheel (ISS-015)
**Resolved:** 2026-03-10. Analog key state.

### ~~ISS-R13~~ — Spawn area too large (ISS-016)
**Resolved:** 2026-03-10. SpawnRadius + circle formation.

### ~~ISS-R14~~ — No reset view (ISS-017)
**Resolved:** 2026-03-10. Home key.

### ~~ISS-R15~~ — No editor-visible profile data (ISS-018)
**Resolved:** 2026-03-10. BindToFullProfile.

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
