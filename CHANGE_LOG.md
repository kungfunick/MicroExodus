# MicroExodus — Change Log

All notable changes to this project are documented here. Entries are reverse-chronological (newest first).

---

## [2026-03-13] — Phase Anim-A: AnimBridge + GASP Removal

**Chat:** Animation
**Phase:** Anim-A (Animation Foundation)

### Changes
- Created `MXAnimBridge.h/.cpp` — UActorComponent that reads CMC state each tick and exposes animation-relevant UPROPERTYs. Zero GASP references — works with any AnimBP.
- Added three enums to `MXTypes.h`: `EMXLocomotionState`, `ETraversalType`, `EMXActionMontage`
- Patched `MXRobotActor.h/.cpp` to add UMXAnimBridge as a default subobject
- **Removed GASP dependency** — SandboxCharacter_CMC.uasset deleted, GASP integration prompt retired
- Created `GASP_Removal_And_BasicLocomotion_Guide.md` — cleanup checklist + step-by-step Blueprint instructions for basic Manny locomotion AnimBP

### AnimBridge Variables (for AnimBP)
- Locomotion: Speed, SpeedNormalized, Direction, bIsMoving, bIsFalling, bHasMoveTarget, LocomotionState
- Turn: TurnAngle, bShouldTurnInPlace
- Acceleration: Acceleration, LeanAngle
- Traversal: bIsTraversing, ActiveTraversalType
- Idle: IdleVariant, IdleTime
- Config: MovingThreshold(5), TurnInPlaceThreshold(45°), LeanInterpSpeed(8), MaxLeanAngle(15°), IdleFidgetDelay(5s), StopToIdleDelay(0.2s)

### AnimBridge Action API
- PlayTraversalAction(), EndTraversal(), PlayActionMontage(), StopActionMontage(), SetIdleVariant()

### Decisions
- **GASP removed** — too many errors. Using built-in UE5 Manny animations (MM_Idle, MM_Walk_Fwd, MM_Run_Fwd) instead. GASP can be re-integrated later through the AnimBridge without C++ changes.
- AnimBridge is engine-agnostic: any AnimBP works, GASP or otherwise
- Basic locomotion plan: BlendSpace1D driven by Speed (0=idle, 150=walk, 300=run)
- Phase naming changed from "GASP-A/B/C" to "Anim-A/B/C"

### Issues
- ISS-003 resolved (SandboxCharacter_CMC deleted — no longer needs inspection)
- ISS-001 partially addressed (bridge exists, AnimBP creation is next — Blueprint work in editor)

### Files Delivered
- MXAnimBridge.h, MXAnimBridge.cpp (new C++ files)
- MXTypes_GASP_Additions.h (enum additions for MXTypes.h — rename is cosmetic, content is engine-agnostic)
- MXRobotActor_GASP_Patch.txt (patch instructions for adding AnimBridge component)
- GASP_Removal_And_BasicLocomotion_Guide.md (removal checklist + AnimBP setup steps)

### Files to Delete
- Content/Blueprints/SandboxCharacter_CMC.uasset
- MicroExodus_GASP_Integration_Prompt.md
- Any GASP-migrated content folders (Content/GAS/, Content/GASP/, etc.)

---

## [2026-03-10] — Animation Integration Prompt + Cross-Chat Sync Protocol

**Chat:** PM chat
**Phase:** Planning

### Changes
- Created `MicroExodus_GASP_Integration_Prompt.md` — **now retired** (GASP removed 2026-03-13)
- Designed UMXAnimBridge architecture: C++ component decouples engine logic from animation display
- Added Cross-Chat Sync Protocol to Claude.md
- Reconciled all tracking docs with debug chat fixes (ISS-R05 through ISS-R15)

---

## [2026-03-10] — Bugs & Fixes Session: Camera, Scaling, Input, Editor SDK

**Chat:** Bugs & Fixes dev chat
**Phase:** 2C-Move (polish)

### Changes
- Fixed double-miniaturization (ISS-R05): mesh-only `SetRelativeScale3D`, never `SetActorScale3D`
- Fixed scroll zoom (ISS-R12): `GetInputAnalogKeyState(EKeys::MouseWheelAxis)`
- Fixed WASD pan direction (ISS-R07): reads `CameraRig->GetActorRotation().Yaw`
- Fixed tablecloth drag (ISS-R07): removed extra `* DeltaTime`
- Implemented double-click behavior (ISS-R08): robot=center+zoom, ground=center
- Fixed name text position (ISS-R09): Z=26, WorldSize=2.0
- Removed edge pan (ISS-R10)
- Fixed keyboard input (ISS-R11): `SetInputMode(FInputModeGameAndUI)`
- Fixed spawn area (ISS-R13): SpawnRadius circle formation
- Added Home key reset (ISS-R14)
- Added BindToFullProfile (ISS-R15): editor-visible profile data
- Fixed `Role` shadowing `AActor::Role` → renamed to `RobotRole`

### Key Patterns Established
- Miniaturized ACharacter: fixed capsule + mesh-only SetRelativeScale3D
- Scroll wheel: GetInputAnalogKeyState(EKeys::MouseWheelAxis)
- Keyboard with mouse cursor: SetInputMode(FInputModeGameAndUI) in BeginPlay
- Camera rig rotation: CameraRig->GetActorRotation().Yaw, not GetControlRotation()
- UPROPERTY naming: don't shadow inherited AActor properties

---

## [2026-03-09] — Camera Fix + Issue Tracker + Bug Fix Prompt

**Chat:** Dev chat
**Phase:** 2C-Move (polish)

### Changes
- Removed HandleEdgePan, created ISSUES.md, created bug fix prompt

---

## [2026-03-09] — Themed Name Evolution: Per-Robot Flavour Packs

**Chat:** Dev chat
**Phase:** Identity Module Enhancement

### Changes
- Created MXNameTheme.h/.cpp (6 themes, ~420 names)
- Created MXNameEvolution.h/.cpp (theme-aware name composer)
- Requires 3 patches to wire (ISS-007)

---

## [2026-03-09] — Phase 2C-Move: Selection + Click-to-Move + Procedural Floor

**Chat:** Dev chat
**Phase:** 2C-Move

### Changes
- MXSelectionManager, MXTestFloorGenerator (new)
- MXRobotActor, MXRTSPlayerController, MXSpawnTestGameMode (modified)

---

## [2026-03-09] — PM Audit: Tracking Document Refresh

**Phase:** Project Management

---

## [2026-03-09] — Phase 2 Design & Project Management Setup

**Phase:** Planning

---

## [2026-03-08] — Phase 2B: RTS Camera Controller

**Phase:** 2B

---

## [2026-03-08] — Phase 2A: Blueprint-to-C++ Integration

**Phase:** 2A

---

## [2026-03-02] — Agent 14: Robot Factory Module

**Phase:** 1

---

## [2026-03-02] — Agent 13: Robot Assembly Module

**Phase:** 1

---

## [2026-03-01] — Procedural Generation Module

**Phase:** 1

---

## [2026-03-01] — Compilation Audit & Fix Session

**Phase:** 1

---

## [Pre-2026-03-01] — Initial Module Generation

**Phase:** 1 — 12 modules, ~94 files
