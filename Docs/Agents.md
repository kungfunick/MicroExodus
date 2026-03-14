# MicroExodus — Module Registry (Agents.md)

**Last Updated:** 2026-03-10

> **Editor SDK Rule:** All gameplay parameters must be `EditAnywhere, BlueprintReadWrite` UPROPERTYs with `MX|Module|Subsystem` categories. Generated runtime data must be `VisibleAnywhere, BlueprintReadOnly`. Test levels are the SDK — every value must be tweakable from the editor Details panel without recompilation.

---

## Agent 0: Shared Contracts

**Status:** Compiling ✓
**Files:** MXTypes.h, MXRobotProfile.h, MXHatData.h, MXEventData.h, MXRunData.h, MXEventFields.h, MXSpecData.h, MXInterfaces.h, MXEvents.h, MXConstants.h, MXEvolutionData.h

**Purpose:** All cross-module contracts: enums, structs, interfaces, delegates, constants. Read by every module. Written only by Agent 0.

**Key Types:**
- `FMXRobotProfile` — full persistent robot data (name, stats, spec, hat, appearance, visual evolution)
- `FMXEventData` — DEMS event payload (robot_id, event_type, message_text, severity, camera_behavior)
- `FMXRunData` — complete run record (events, scores, modifiers, tier)
- `FMXHatData` / `FMXHatDefinition` — hat definitions and collection
- `FMXSpecData` / `FMXSpecDefinition` — specialisation tree definitions

**Key Interfaces:**
- `IMXRobotProvider` — GetRobotProfile(FGuid), GetAllRobotProfiles(), GetRobotCount()
- `IMXHatProvider` — GetHatData(int32), GetCollection(), ConsumeHat(), ReturnHat()
- `IMXRunProvider` — GetCurrentRunData(), GetRunNumber(), GetCurrentTier()
- `IMXEventListener` — OnDEMSEvent(FMXEventData)
- `IMXEvolutionTarget` — ApplyVisualMark(FString, float)
- `IMXPersistable` — MXSerialize(FArchive&), MXDeserialize(FArchive&)

**Key Constants (MXConstants.h):**
- MAX_ROBOTS = 100, MAX_HATS = 100, MAX_LEVELS = 20, MAX_TIERS = 6
- XP_THRESHOLDS[] array (12 entries)

**EventBus Delegates (MXEvents.h):**
- OnRobotRescued(FGuid, int32 LevelNumber)
- OnRobotDied(FGuid, FMXEventData)
- OnRobotSacrificed(FGuid, FMXEventData)
- OnNearMiss(FGuid, FMXObjectEventFields)
- OnLevelComplete(int32, TArray\<FGuid\>)
- OnRunComplete(FMXRunData), OnRunFailed(FMXRunData, int32)
- OnHatEquipped(FGuid, int32), OnHatLost(FGuid, int32, FMXEventData)
- OnLevelUp(FGuid, int32)
- OnSpecChosen(FGuid, ERobotRole, ETier2Spec, ETier3Spec)
- OnComboDiscovered(TArray\<int32\>, int32)
- OnHatUnlocked(int32)

**EventBus Access Pattern:**
```cpp
UMXEventBusSubsystem* BusSub = GetGameInstance()->GetSubsystem<UMXEventBusSubsystem>();
UMXEventBus* Bus = BusSub->EventBus;
Bus->OnRobotDied.AddDynamic(this, &UMyClass::HandleRobotDied);
```

---

## Agent 1: DEMS (Dynamic Event Messaging System)

**Status:** Compiling ✓
**Files:** MXEventMessageComponent.h/cpp, MXMessageBuilder.h/cpp, MXMessageDispatcher.h/cpp, MXTemplateSelector.h/cpp, MXDeduplicationBuffer.h/cpp

**Key Functions:**
- `UMXEventMessageComponent::TriggerDeathEvent(FGuid)` — builds and dispatches death message
- `UMXEventMessageComponent::TriggerRescueEvent(FGuid, int32)` — rescue message
- `UMXEventMessageComponent::TriggerSacrificeEvent(FGuid, int32)` — sacrifice message
- `UMXEventMessageComponent::TriggerNearMissEvent(FGuid)` — near-miss message
- `UMXMessageBuilder::Initialize(UMXTemplateSelector*, UMXDeduplicationBuffer*)`
- `UMXTemplateSelector::LoadTemplates()` — loads from DataTables or fallback

**Wiring (from GameInstance::Init):**
- TemplateSelector->LoadTemplates()
- MessageBuilder->Initialize(TemplateSelector, DedupBuffer)
- MessageDispatcher->RegisterListener(RobotManager)
- MessageDispatcher->RegisterListener(LifeLog)

**Depends On:** IMXRobotProvider, IMXHatProvider
**Listens To:** OnRobotDied, OnRobotRescued, OnRobotSacrificed, OnNearMiss, OnLevelComplete, OnRunComplete
**DataTables:** DT_DeathTemplates, DT_RescueTemplates, DT_SacrificeTemplates, DT_NearMissTemplates

---

## Agent 2: Identity

**Status:** Compiling ✓
**Files:** MXRobotManager.h/cpp, MXNameGenerator.h/cpp, MXNameEvolution.h/cpp, MXNameTheme.h/cpp, MXPersonalityGenerator.h/cpp, MXLifeLog.h/cpp, MXAgingSystem.h/cpp

**Key Functions:**
- `UMXRobotManager::Initialize(UMXNameGenerator*, UMXPersonalityGenerator*, UMXLifeLog*, UMXAgingSystem*)` — **requires all four pointers**
- `UMXRobotManager::CreateRobot(int32 LevelNumber, int32 RunNumber)` → FGuid (invalid if roster full)
- `UMXRobotManager::RemoveRobot(FGuid, int32 RunNumber)` — archives to DeadRobotArchive
- `UMXRobotManager::GetRobotProfile_Implementation(FGuid)` → FMXRobotProfile
- `UMXRobotManager::GetAllRobotProfiles_Implementation()` → TArray\<FMXRobotProfile\>
- `UMXRobotManager::AddXP(FGuid, int32)` — with level-up check
- `UMXRobotManager::CheckTitles(FGuid)` — awards titles based on stat thresholds (Veteran, Firewalker, etc.)
- `UMXNameGenerator::GenerateName()` → FString (fallback names if no DataTable)
- `UMXNameGenerator::ReleaseName(FString, int32)` — returns name to pool with cooldown
- `UMXNameEvolution::EvaluateNameEvolution(FMXRobotProfile&)` → bool (rolls themed surname at 5 runs survived)
- `UMXNameEvolution::GetDisplayName(FMXRobotProfile)` → FString ("[Name] [Surname] [Prefix Title]")
- `UMXNameEvolution::GetShortName(FMXRobotProfile)` → FString ("[Name] [Surname]")
- `UMXNameEvolution::RollSurname(FGuid, ERobotRole, ENameTheme)` → FString (deterministic from GUID, theme-aware)
- `UMXNameThemeManager::GetFirstNames(ENameTheme)` → themed first name pool
- `UMXNameThemeManager::GetSurnames(ENameTheme, ERobotRole)` → themed surname pool
- `UMXNameThemeManager::GetThemedTitle(ENameTheme, FString)` → title alias lookup

**Themed Name Evolution (new):**
- Theme is per-RUN, stamped onto every robot born/rescued during that run via profile.name_theme
- Robots keep their theme forever → mixed roster from different themed runs
- 6 built-in themes: Robot, Wizard, Pirate, Samurai, SciFi, Mythic (~420 names total)
- Each theme: first name pool, per-role surnames, title aliases, title prefix
- Pirate: "Barnacle Planksworth Captain Scourge" / Wizard: "Ember Hexfire The Great Pyromancer"
- FMXRobotProfile needs: surname (FString), name_theme (ENameTheme) fields
- Custom themes loadable from DataTables via RegisterCustomTheme()

**Wiring (from GameInstance::Init):**
- NameGenerator = NewObject → optional LoadFromDataTable(NamesTable)
- PersonalityGenerator = NewObject → optional LoadFromDataTable(PersonalitiesTable)
- LifeLog = NewObject, AgingSystem = NewObject
- NameEvolution = NewObject → Initialize(NameThemeManager)
- NameThemeManager = NewObject → Initialize()
- RobotManager->Initialize(NameGen, PersonalityGen, LifeLog, AgingSystem)

**Implements:** IMXRobotProvider, IMXEventListener, IMXPersistable
**Listens To:** OnRobotDied, OnRobotRescued, OnRobotSacrificed, OnNearMiss, OnLevelComplete, OnRunComplete, OnRunFailed, OnHatEquipped, OnHatLost, OnSpecChosen
**DataTables:** DT_Names (optional), DT_Personalities (optional), DT_NameTheme_* (optional, for custom themes)

---

## Agent 3: Hats

**Status:** Compiling ✓
**Files:** MXHatManager.h/cpp, MXHatStackEconomy.h/cpp, MXComboDetector.h/cpp, MXHatPhysics.h/cpp, MXHatUnlockChecker.h/cpp

**Key Functions:**
- `UMXHatManager::Initialize(UMXEventBus*, UObject* RobotProviderObject)` — creates sub-components, binds events
- `UMXHatManager::EquipHat(FGuid RobotId, int32 HatId)` → bool
- `UMXHatManager::GetHatData_Implementation(int32 HatId)` → FMXHatDefinition
- `UMXHatManager::GetCollection_Implementation()` → FMXHatCollection
- `UMXComboDetector::CheckCombos(TArray\<int32\>, FMXHatCollection, TMap)` → TArray\<int32\>
- `UMXHatUnlockChecker::CheckStandardUnlocks(FMXRunData, TArray\<FMXRobotProfile\>)` → TArray\<int32\>

**Wiring (from GameInstance::Init):**
- HatManager->Initialize(EventBus, RobotManager.Get())

**Implements:** IMXHatProvider
**Listens To:** OnRobotDied, OnRunComplete, OnRunFailed, OnLevelComplete
**DataTables:** DT_HatDefinitions

---

## Agent 4: Evolution (Visual)

**Status:** Compiling ✓
**Files:** MXEvolutionLayerSystem.h/cpp, MXDecalManager.h/cpp, MXWearShader.h/cpp, MXEvolutionCalculator.h/cpp, MXEvolutionData.h

**Key Functions:**
- `UMXEvolutionLayerSystem::RecalculateEvolution()` — full pipeline: Calculator → WearShader → DecalManager
- `UMXEvolutionLayerSystem::ApplyVisualMark_Implementation(FString MarkType, float Intensity)` — DEMS-driven marks
- `UMXEvolutionCalculator::ComputeFullState(FMXRobotProfile)` → FMXVisualEvolutionState (pure, static)
- `UMXWearShader::SetWearParameters(UMaterialInstanceDynamic*, FMXVisualEvolutionState)` — push to material
- `UMXDecalManager::GenerateDecalPlacements(int32 Seed, FMXVisualEvolutionState)` → TArray\<FMXDecalPlacement\>

**Wiring Required:**
- RobotId (FGuid) — set on the component before BeginPlay
- BaseMaterial (UMaterialInterface*) — reference to M_RobotMaster
- TargetMesh — the mesh component that receives the MID
- Currently `GetRobotProvider()` returns nullptr — needs wiring to GameInstance

**Implements:** IMXEvolutionTarget
**Listens To:** OnLevelUp, OnSpecChosen, OnRunComplete, OnNearMiss

---

## Agent 5: Specialisation

**Status:** Compiling ✓
**Files:** MXSpecTree.h/cpp, MXRoleComponent.h/cpp

**Key Functions:**
- `UMXSpecTree::Initialize(UDataTable*, TScriptInterface<IMXRobotProvider>, UMXEventBus*)`
- `UMXSpecTree::GetAvailableChoices(FMXRobotProfile)` → TArray\<FMXSpecChoice\>
- `UMXSpecTree::ApplyRoleChoice(FGuid, ERobotRole)` — broadcasts OnSpecChosen
- `UMXSpecTree::ApplyTier2Choice(FGuid, ETier2Spec)`, `ApplyTier3Choice(FGuid, ETier3Spec)`
- Role unlock at Lv5, Tier2 at Lv10, Tier3 at Lv20, Mastery at Lv35
- Roles: Scout, Guardian, Engineer → 2 Tier2 each → 2 Tier3 each = 12 total paths

**Wiring (from GameInstance::Init):**
- SpecTree->Initialize(SpecTreeTable, RobotProviderSI, EventBus)

**Implements:** IMXPersistable
**Listens To:** OnLevelUp, OnRunComplete, OnRunFailed
**DataTables:** DT_SpecTree

---

## Agent 6: Roguelike

**Status:** Compiling ✓
**Files:** MXRunManager.h/cpp, MXSpawnManager.h/cpp, MXRescueMechanic.h/cpp, MXSwarmGate.h/cpp (+ gate subclasses), MXHazardBase.h/cpp (+ 10 hazard subclasses), MXEventBusSubsystem.h/cpp

**Key Functions:**
- `UMXRunManager::Initialise(UMXEventBus*, TScriptInterface<IMXRobotProvider>, TScriptInterface<IMXHatProvider>, UDataTable* TierTable, UDataTable* XPTable)`
- `UMXRunManager::StartRun(ETier, TArray<FGuid> SelectedRobots, TArray<int32> EquippedHats, TArray<FString> Modifiers)`
- `UMXRunManager::AdvanceLevel()`, `EndRun(ERunOutcome)`
- `UMXRunManager::RecordEvent(FMXEventData)`, `GetCurrentRunData()`
- `UMXSpawnManager::SpawnRescueRobotsForLevel(int32, ETier, FMXModifierEffects)` → TArray\<FGuid\>
- `UMXEventBusSubsystem::Initialize()` — creates the shared UMXEventBus

**Wiring (from GameInstance::Init):**
- EventBus obtained via GetSubsystem\<UMXEventBusSubsystem\>()->EventBus
- RunManager->Initialise(EventBus, RobotProviderSI, HatProviderSI, TierTable, XPTable)

**Implements:** IMXRunProvider, IMXPersistable
**Listens To:** OnRobotDied, OnRobotSacrificed, OnRobotRescued, OnNearMiss, OnHatEquipped, OnHatLost
**Hazard Subclasses:** Furnace, Crusher, SpikeTrap, Pit, LavaPool, Conveyor, BuzzSaw, IceFloor, EMPField, FallingDebris
**Gate Subclasses:** WeightPlate, SwarmDoor, ChainStation, Sacrifice

---

## Agent 7: Camera

**Status:** Compiling ✓ | Patched Phase 2A
**Files:** MXSwarmCamera.h/cpp, MXTiltShiftEffect.h/cpp, MXTimeDilation.h/cpp, MXCameraBehaviors.h/cpp, MXCameraPersonality.h/cpp

**Key Functions:**
- `UMXSwarmCamera::SetSwarmTarget(TArray<FGuid>)` — set tracked robots
- `UMXSwarmCamera::UpdateRobotPosition(FGuid, FVector)` — **Phase 2A addition**, feeds positions
- `UMXSwarmCamera::UpdateRobotPositions(TMap<FGuid, FVector>)` — batch version
- `UMXSwarmCamera::UpdateZoom(int32 SwarmCount, float DeltaTime)` — drive zoom from count
- `UMXSwarmCamera::EnterInspectMode(FGuid)` / `ExitInspectMode()`
- `UMXSwarmCamera::QueueCameraEvent(FMXCameraEvent)` — priority queue
- `UMXSwarmCamera::SetCameraMode(ECameraPersonalityMode)` — Director/Operator/Locked/Replay
- `UMXTiltShiftEffect::SetPreset(ETiltShiftPreset, float TransitionTime)` — Diorama/Cinematic/Wide/Report/Off
- `UMXTiltShiftEffect::ScaleToZoomLevel(float CameraHeight)`
- `UMXTimeDilation::TriggerAutoDilation(EMXDilationMode, float Duration)`

**Wiring Required (on host actor / camera rig):**
- `SwarmCamera->SpringArm` (USpringArmComponent*) — **must be set by host actor constructor**
- `SwarmCamera->CameraActor` (ACameraActor*) — optional, unused if using UCameraComponent
- `TiltShiftEffect->PostProcessComponent` — **auto-discovers** via FindComponentByClass in BeginPlay
- `TimeDilation->TiltShiftEffect` (UMXTiltShiftEffect*) — **must be set explicitly**

**Zoom Table (GDD):**
| Robots | Height (cm) |
|--------|-------------|
| 1–5 | 300 |
| 6–15 | 600 |
| 16–30 | 1000 |
| 31–50 | 1400 |
| 51–75 | 1700 |
| 76–100 | 2000 |

**Camera Modes:** Director (all events), Operator (Epic+Cinematic only), Locked (no events), Replay (all, extended)

**Depends On:** MXConstants
**Depended On By:** MXCameraBehaviors, MXCameraRig (Phase 2A)

---

## Agent 8: Swarm (Logic Only — No Files Yet)

**Status:** Not implemented as separate module. Swarm logic is distributed across RunManager, SpawnManager, and the planned AMXRobotActor tick.

**Intended Responsibility:** Robot AI movement, flocking behaviors, collision avoidance, position reporting to camera, swarm splitting/merging.

---

## Agent 9: Reports

**Status:** Compiling ✓
**Files:** MXRunReportEngine.h/cpp, MXStatCompiler.h/cpp, MXHighlightScorer.h/cpp, MXAwardSelector.h/cpp, MXReportNarrator.h/cpp, MXRunReportUI.h/cpp

**Key Functions:**
- `UMXRunReportEngine::Initialize(UMXEventBus*)`
- `UMXRunReportEngine::GenerateReport(FMXRunData, TArray<FMXRobotProfile>)` → FMXRunReport
- `UMXStatCompiler::CompileStats(FMXRunData, TArray<FMXRobotProfile>)` → FMXCompiledStats
- `UMXHighlightScorer::ScoreHighlights(FMXCompiledStats)` → TArray\<FMXHighlight\>
- `UMXAwardSelector::SelectAwards(FMXCompiledStats, TArray<FMXRobotProfile>)` → TArray\<FMXAward\>

**Listens To:** OnRunComplete, OnRunFailed

---

## Agent 10: Persistence

**Status:** Compiling ✓
**Files:** MXSaveManager.h/cpp

**Key Functions:**
- `UMXSaveManager::Initialize(UMXEventBus*)`
- `UMXSaveManager::RegisterPersistable(TScriptInterface<IMXPersistable>)` — call for each module
- `UMXSaveManager::SaveGame()` — serialises all registered modules, CRC32 validation, 3-slot rotation
- `UMXSaveManager::LoadGame()` → bool
- `UMXSaveManager::GetSaveMetadata()` → FMXSaveMetadata (lightweight header read)

**Save Slot Strategy:** save_latest, save_backup1, save_backup2 (rolling)
**Serialisation Order:** Identity → Hats → Roguelike → Reports → Specialization

**Listens To:** OnRunComplete, OnRunFailed, OnHatUnlocked, OnComboDiscovered

---

## Agent 11: UI / HUD

**Status:** Compiling ✓
**Files:** MXHUDWidget.h/cpp, MXRunReportUI.h/cpp

**Key Functions:**
- `UMXHUDWidget::UpdateSwarmCounter(int32)`, `UpdateLevelIndicator(int32)`, `UpdateTimer(float)`
- `UMXHUDWidget::ShowToast(FMXEventData)` — event notification popup
- `UMXHUDWidget::OnMinimapDataUpdated(TArray<FVector>, TArray<FVector>, TArray<FVector>)`
- `UMXRunReportUI::DisplayReport(FMXRunReport)` — full run report widget

**Wiring:** Widget Blueprints (WBP_HUD, WBP_RunReport) bind to C++ via meta=(BindWidget)

---

## Agent 12: Procedural Generation

**Status:** Compiling ✓
**Files:** MXProceduralGen.h/cpp, MXRoomGenerator.h/cpp, MXHazardPlacer.h/cpp, MXProceduralData.h

**Key Functions:**
- `UMXProceduralGen::Initialize(UDataTable* HazardDataTable)`
- `UMXProceduralGen::GenerateRunLayout(int64 MasterSeed, ETier, TArray<FString> Modifiers)` → FMXRunLayout
- `UMXProceduralGen::GetLevelLayout(int32 LevelNumber)` → FMXLevelLayout
- `UMXRoomGenerator::GenerateRooms(int64 LevelSeed, int32 Level, ETier)` → TArray\<FMXRoomDef\>
- `UMXRoomGenerator::ComputeCriticalPath(TArray<FMXRoomDef>)` → TArray\<int32\>
- `UMXHazardPlacer::PlaceHazards(TArray<FMXRoomDef>, FMXLevelConditions, int64)` → TArray\<FMXHazardPlacement\>

**Pacing Curve:** Levels 1–3 intro, 4–7 rising, 8–12 crisis, 13–16 escalation, 17–19 climax, 20 finale

**Key Data Structures:**
- `FMXRunLayout` — 20 FMXLevelLayouts, master seed, tier, active modifiers
- `FMXLevelLayout` — rooms, hazard placements, conditions, rescue count
- `FMXRoomDef` — room type, size, connections, element theme
- `FMXHazardPlacement` — hazard type, position, element, speed multiplier

---

## Agent 13: Robot Assembly

**Status:** Compiling ✓
**Files:** MXRobotAssembler.h/cpp, MXRobotMeshComponent.h/cpp, MXPartDestructionManager.h/cpp, MXRobotAssemblyData.h

**Key Functions:**
- `UMXRobotAssembler::InitialiseFromDataTable(UDataTable*)` — loads part definitions
- `UMXRobotAssembler::GeneratePlaceholderParts(int32 VariantsPerSlot)` — for testing
- `UMXRobotAssembler::BuildRecipeFromProfile(FMXRobotProfile)` → FMXAssemblyRecipe
- `UMXRobotMeshComponent::AssembleFromRecipe(FMXAssemblyRecipe, UMXRobotAssembler*)`
- `UMXPartDestructionManager::ApplyDamage(FGuid, EPartSlot, int32)` → FMXDamageResult

**Part Slots:** Locomotion, Body, ArmLeft, ArmRight, Head (5 modular sockets)
**LOD Strategy:** Full (5-part) < 2000cm, Simplified (3-part) 2000–5000cm, Impostor > 5000cm
**Socket Names:** socket_locomotion, socket_body, socket_arm_l, socket_arm_r, socket_head

---

## Agent 14: Robot Factory

**Status:** Compiling ✓
**Files:** MXRobotFactory.h/cpp, MXRobotCodec.h/cpp, MXRobotFactoryData.h

**Key Functions:**
- `UMXRobotFactory::Initialize(UMXRobotManager*, UMXRobotAssembler*, UMXNameGenerator*, UMXPersonalityGenerator*)`
- `UMXRobotFactory::BuildLobbyPool(TArray<FGuid> Survivors, TArray<FGuid> Rescued, int32 Level, ETier, TArray<FMXSalvageDrop>)`
- `UMXRobotFactory::ToggleCandidateSelection(FGuid)` → bool
- `UMXRobotFactory::GetSelectedRobots()` → TArray\<FGuid\> (max 5)
- `UMXRobotFactory::RedeemCode(FString)` → bool
- `UMXRobotFactory::GetRobotCode(FGuid)` → FString (Base32 Crockford from GUID)
- `UMXRobotFactory::PurchaseUpgrade(EFactoryUpgrade)` → bool

**Delegates:**
- OnLobbyPoolReady(FMXLobbyPool)
- OnCodeRedeemed(FString, FMXRecruitCandidate)
- OnCodeRejected(FString, FString Reason)
- OnUpgradePurchased(EFactoryUpgrade, int32 NewLevel)

**Upgrade Types:** BatchSize, CodeTerminal, RecruitDiversity, SalvageEfficiency, CodeSlots

---

## Phase 2A Additions

**Status:** Working ✓ (updated in Phase 2C-Move)
**Files:** MXRobotActor.h/cpp, MXCameraRig.h/cpp, MXSpawnTestGameMode.h/cpp

### AMXRobotActor (ACharacter subclass)
- `BindToProfile(FGuid, FString)` — simple binding: sets RobotId, RobotName, updates text
- `BindToFullProfile(FMXRobotProfile)` — rich binding: populates all editor-visible profile fields
- `SetSelected(bool)`, `SetHovered(bool)` — selection/hover state (Phase 2C)
- `MoveToLocation(FVector)`, `StopMoving()` — click-to-move with CMC (Phase 2C)
- **Config UPROPERTYs (EditAnywhere):** RobotScale(0.20), MoveSpeed(150), StopDistance(20), RotationInterpSpeed(8), SelectionColor, SelectedNameColor, HoveredNameColor, SkeletalMeshAsset
- **Profile UPROPERTYs (VisibleAnywhere):** RobotId, RobotName, PersonalityDescription, Quirk, Likes, Dislikes, RobotRole, Level, DisplayedTitle, ChassisColor, EyeColor
- `NameTextComponent` (UTextRenderComponent) — hidden by default, shown on hover/select
- `SelectionRingComponent` (UStaticMeshComponent) — cylinder at feet, visible when selected
- Capsule sized at (8, 18) in constructor. Mesh-only scale in BeginPlay (no actor scale).
- GravityScale = 1.0 (floor has collision)

### AMXCameraRig (AActor)
- Constructor creates and wires: SpringArm → Camera, PostProcess, SwarmCamera, TiltShiftEffect, TimeDilation
- `SwarmCamera->SpringArm` wired in constructor
- `TimeDilation->TiltShiftEffect` wired in constructor
- TiltShiftEffect auto-discovers PostProcess in BeginPlay

### AMXSpawnTestGameMode (AGameModeBase)
- Gets UMXGameInstance → UMXRobotManager
- **Spawn order:** Floor → Robots → Camera (Phase 2C)
- Robots spawn in a circle within SpawnRadius of floor center (not full floor area)
- Uses `BindToFullProfile()` when Identity module is available for rich editor data
- **Config UPROPERTYs (EditAnywhere):** NumRobots(8), FloorGridX(10), FloorGridY(10), FloorTileSize(200), SpawnRadius(300), RobotActorClass, CameraRigClass, FloorGeneratorClass

---

## Phase 2B: RTS Camera Controller (Complete)

**Status:** Complete ✓ (updated in Phase 2C-Move)
**Files:** MXRTSPlayerController.h/cpp

**AMXRTSPlayerController (APlayerController subclass)**
- `bShowMouseCursor = true`, `bEnableClickEvents = true`, `bEnableMouseOverEvents = true`
- `SetInputMode(FInputModeGameAndUI)` in BeginPlay — enables keyboard input with visible cursor
- Finds AMXCameraRig via TActorIterator in BeginPlay, calls SetViewTargetWithBlend
- **Camera:** HandleZoom (analog axis), HandleRotation (right-drag), HandleKeyboardPan (WASD/arrows, follows camera yaw), HandleDragPan (middle-click tablecloth), HandleResetView (Home key)
- **Selection (Phase 2C):** HandleLeftMouseInput (click + box drag + double-click), HandleRightClickMove, HandleControlGroups, HandleSelectAll
- **Double-click:** Robot = center + zoom in; Ground = center only (no zoom)
- `UMXSelectionManager` component — created as default subobject (Phase 2C)
- `IssueMoveCommand(FVector)` — sends MoveToLocation to all selected robots with circle formation
- `GetGroundHitUnderCursor()` — raycasts ECC_WorldStatic for move targets
- `GetRobotUnderCursor()` — raycasts ECC_Pawn for robot detection
- All input via raw polling in PlayerTick (no Enhanced Input dependency)
- Right-click: short click (<0.25s, <10px drag) = move command; longer = camera rotation

**Config UPROPERTYs (EditAnywhere):** ZoomSpeed(50), MinZoom(100), MaxZoom(3000), ZoomInterpSpeed(6), DefaultZoom(800), PanSpeed(800), RotateSpeed(0.3), DragPanSpeed(2), FormationSpacing(40), DoubleClickTime(0.3), DoubleClickRadius(20), DoubleClickRobotZoom(150), BoxSelectColor, BoxSelectBorderColor

**Depends On:** AMXCameraRig (Phase 2A)

---

## Phase 2C-Move: Selection + Click-to-Move + Procedural Floor

**Status:** Code delivered, pending compilation ⏳
**Files:** MXSelectionManager.h/cpp, MXTestFloorGenerator.h/cpp (+ modified files above)

### UMXSelectionManager (ActorComponent on PlayerController)
- `TrySelectAtCursor(bool bAdditive)` — raycast select, Shift toggles additive mode
- `BeginBoxSelect / UpdateBoxSelect / EndBoxSelect` — screen-space rectangle selection
- `ClearSelection()`, `SelectAll()` — bulk operations
- `SaveControlGroup(int32)`, `RecallControlGroup(int32)` — Ctrl+1-9 save, 1-9 recall
- `UpdateHover()` — per-tick raycast for hover state (drives name visibility)
- `OnSelectionChanged` delegate — broadcasts TArray<AMXRobotActor*>
- Selection stored as TArray<TWeakObjectPtr<AMXRobotActor>> (safe against destruction)
- `BoxSelectThreshold` = 5px (distinguishes click from drag)
- `SelectionTraceChannel` = ECC_Pawn

### AMXTestFloorGenerator (AActor)
- `GenerateFloor()` — spawns GridSizeX × GridSizeY tiles using /Engine/BasicShapes/Cube
- Tiles have collision (ECollisionEnabled::QueryAndPhysics, ECR_Block, ECC_WorldStatic)
- `GetFloorCenter()`, `GetFloorBounds()`, `GetRandomFloorPosition(float Margin)`
- Config: GridSizeX(10), GridSizeY(10), TileSize(200), TileThickness(10), TileGap(2), FloorZ(0)
- Checkerboard pattern with configurable colors
- bAutoGenerate = true (generates on BeginPlay)

**Depends On:** AMXRTSPlayerController (Phase 2B), AMXRobotActor (Phase 2A)

---

## Phase Anim-A: Animation Bridge Foundation (Complete)

**Status:** Code delivered, pending compilation ⏳
**Files:** MXAnimBridge.h/cpp (new), MXTypes.h (3 enums added), MXRobotActor.h/cpp (patched)

### UMXAnimBridge (ActorComponent on AMXRobotActor)

**Purpose:** Pure C++ decoupling layer between game logic and Animation Blueprint. Reads CMC state each tick, exposes animation-relevant UPROPERTYs. Zero external animation system dependencies — works with any AnimBP (built-in Manny, custom, or GASP if re-integrated later).

**Exposed State (VisibleAnywhere, BlueprintReadOnly):**
- `Speed` (float), `SpeedNormalized` (float), `Direction` (float), `bIsMoving` (bool), `bIsFalling` (bool), `bHasMoveTarget` (bool), `LocomotionState` (EMXLocomotionState)
- `TurnAngle` (float), `bShouldTurnInPlace` (bool)
- `Acceleration` (float), `LeanAngle` (float)
- `bIsTraversing` (bool), `ActiveTraversalType` (ETraversalType)
- `IdleVariant` (int32), `IdleTime` (float)

**Config (EditAnywhere):** MovingThreshold(5), TurnInPlaceThreshold(45°), LeanInterpSpeed(8), MaxLeanAngle(15°), IdleFidgetDelay(5s), StopToIdleDelay(0.2s)

**Action API (BlueprintCallable):** PlayTraversalAction(), EndTraversal(), PlayActionMontage()→bool, StopActionMontage(), SetIdleVariant()

**Montage Config:** `MontageMap` TMap<EMXActionMontage, TSoftObjectPtr<UAnimMontage>> — EditAnywhere

**Events:** OnLocomotionStateChanged, OnTraversalStarted, OnActionMontageEnded

**Shared Enums (MXTypes.h):** EMXLocomotionState, ETraversalType, EMXActionMontage

**Depends On:** AMXRobotActor (Phase 2A), UCharacterMovementComponent (engine)

---

## Update "Planned Modules" — Replace GASP section with:

### Phase Anim-B: Basic Locomotion AnimBP (Blueprint Work)
**Purpose:** Create ABP_MXRobot and BS_MXRobot_Locomotion in Unreal Editor. BlendSpace1D driven by AnimBridge.Speed (0=idle, 150=walk, 300=run). Uses built-in UE5 Manny animations (MM_Idle, MM_Walk_Fwd, MM_Run_Fwd). Resolves ISS-001 (T-pose).
**Type:** Blueprint-only work in editor (no C++ changes needed)
**Depends On:** UMXAnimBridge (Anim-A ✓), Manny content plugin
**Guide:** GASP_Removal_And_BasicLocomotion_Guide.md

### GASP Status: Removed (2026-03-13)
SandboxCharacter_CMC.uasset deleted. GASP integration prompt retired. AnimBridge designed to be animation-system-agnostic — GASP can be re-integrated later by swapping the AnimBP internals without any C++ changes.

## Planned Modules (Not Started)

### Phase GASP: Animation & Locomotion Integration (Prompt Ready)
**Purpose:** Integrate GASP locomotion for fluid robot movement. Resolves ISS-001 (T-pose) and ISS-003 (SandboxCharacter_CMC inspection).
**Planned Classes:**
- `UMXAnimBridge` (ActorComponent on AMXRobotActor) — reads CMC velocity/state, exposes UPROPERTY variables to AnimBP. Triggers traversal actions and montages from C++. Zero GASP headers in C++.
- `ABP_MXRobot` (Animation Blueprint) — GASP locomotion state machine (idle/walk/run/start/stop/pivot), traversal actions (vault/climb/mantle), action montage slots (rescue/flinch/death/sacrifice), idle personality variants.
**Architecture:** C++ engine computes positions → CMC moves actor → AnimBridge reads CMC state → AnimBP plays animations. Animation layer fully swappable.
**Phased:** GASP-A (inspect SandboxCharacter_CMC), GASP-B (locomotion), GASP-C (traversal), GASP-D (idle/personality), GASP-E (swarm integration + 100-robot perf test)
**Depends On:** AMXRobotActor (Phase 2A), UMXSwarmController (Agent 8), SandboxCharacter_CMC.uasset
**Prompt:** `MicroExodus_GASP_Integration_Prompt.md`

### Phase 2C-Polish: Selection Visual Feedback
**Purpose:** Draw box select rectangle on screen, move destination marker.
**Approach:** HUD class with Canvas drawing, or UMG overlay widget.

### Phase 2E-Advanced: Procedural Room Layouts
**Purpose:** Replace simple grid floor with UMXProceduralGen room-based layouts.
**Approach:** Use UMXProceduralGen::GenerateRunLayout() to get FMXLevelLayout, spawn floor geometry from FMXRoomDef array.
**Depends On:** UMXProceduralGen (Agent 12 — already exists), AMXTestFloorGenerator

### Phase 2F: Robot Spawn UI
**Purpose:** In-game UI for adding robots with + button, selecting type, viewing/editing stats.
**Planned Class:** `UMXSpawnTestWidget` (UUserWidget)
**Features:** + button spawns a new robot via RobotManager::CreateRobot(). Type picker (role assignment). Stat panel reads FMXRobotProfile. Edit capabilities for testing.
**Depends On:** UMXRobotManager (Agent 2), UMXSpecTree (Agent 5), AMXRobotActor

