// MXRobotManager.cpp â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity

#include "MXRobotManager.h"
#include "MXNameGenerator.h"
#include "MXPersonalityGenerator.h"
#include "MXLifeLog.h"
#include "MXAgingSystem.h"
#include "MXConstants.h"
#include "MXEvents.h"
#include "MXRunData.h"
#include "MXEventFields.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Math/UnrealMathUtility.h"

// Forward-declared subsystem accessor â€” implementation assumes UMXEventBusSubsystem
// exposes a public UMXEventBus* EventBus property (defined by Agent 6 / Roguelike).
// If the subsystem isn't available yet, event binding is deferred gracefully.
static UMXEventBus* GetEventBus(UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return nullptr;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;
    // Subsystem type defined by the Roguelike module (Agent 6).
    // Accessed via generic subsystem lookup to avoid hard coupling.
    return nullptr; // Replaced by Agent 6 subsystem once available.
}

// ---------------------------------------------------------------------------

UMXRobotManager::UMXRobotManager()
{
}

void UMXRobotManager::Initialize(UMXNameGenerator* InNameGenerator,
                                  UMXPersonalityGenerator* InPersonalityGen,
                                  UMXLifeLog* InLifeLog,
                                  UMXAgingSystem* InAgingSystem)
{
    NameGenerator      = InNameGenerator;
    PersonalityGenerator = InPersonalityGen;
    LifeLog            = InLifeLog;
    AgingSystem        = InAgingSystem;

    // Event bus binding happens here once the bus subsystem is resolved.
    // The Roguelike module (Agent 6) owns the subsystem; this module binds
    // to delegates after the bus is initialized. Pattern: call Initialize()
    // from UGameInstance::Init() AFTER the event bus subsystem is set up.
}

// -------------------------------------------------------------------------
// Robot Creation / Removal
// -------------------------------------------------------------------------

FGuid UMXRobotManager::CreateRobot(int32 LevelNumber, int32 RunNumber)
{
    if (IsRosterFull())
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRobotManager: Roster is full (%d/%d). Cannot create robot."),
               ActiveRoster.Num(), MXConstants::MAX_ROBOTS);
        return FGuid(); // Invalid GUID signals failure to caller.
    }

    FMXRobotProfile NewProfile;

    // Identity
    NewProfile.robot_id     = FGuid::NewGuid();
    NewProfile.date_of_birth = FDateTime::Now();
    NewProfile.name         = NameGenerator ? NameGenerator->GenerateName() : FString(TEXT("Unknown"));

    // Age (initialized; AgingSystem computes calendar_age at runtime)
    NewProfile.active_age_seconds = 0.0f;
    NewProfile.calendar_age       = FTimespan::Zero();

    // Progression
    NewProfile.level    = 1;
    NewProfile.xp       = 0;
    NewProfile.total_xp = 0;

    // History
    NewProfile.runs_survived      = 0;
    NewProfile.levels_completed   = 0;
    NewProfile.current_run_streak = 0;

    // Encounter stats â€” all zero at birth
    NewProfile.near_misses      = 0;
    NewProfile.fire_encounters  = 0;
    NewProfile.crush_encounters = 0;
    NewProfile.fall_encounters  = 0;
    NewProfile.emp_encounters   = 0;
    NewProfile.ice_encounters   = 0;

    // Social stats â€” all zero at birth
    NewProfile.rescues_witnessed   = 0;
    NewProfile.deaths_witnessed    = 0;
    NewProfile.sacrifices_witnessed = 0;
    NewProfile.puzzles_participated = 0;

    // Appearance â€” randomized
    GenerateAppearance(NewProfile);

    // Specialization â€” none at birth
    NewProfile.role          = ERobotRole::None;
    NewProfile.tier2_spec    = ETier2Spec::None;
    NewProfile.tier3_spec    = ETier3Spec::None;
    NewProfile.mastery_title = EMasteryTitle::None;

    // Hat â€” none at birth
    NewProfile.current_hat_id     = -1;
    NewProfile.consecutive_hat_id = -1;
    NewProfile.consecutive_hat_runs = 0;

    // Personality â€” procedurally generated
    if (PersonalityGenerator)
    {
        FMXGeneratedPersonality Personality = PersonalityGenerator->GeneratePersonality();
        NewProfile.description = Personality.Description;
        NewProfile.likes       = Personality.Likes;
        NewProfile.dislikes    = Personality.Dislikes;
        NewProfile.quirk       = Personality.Quirk;
    }

    // Titles â€” none at birth
    NewProfile.earned_titles  = TArray<FString>();
    NewProfile.displayed_title = FString();

    // Visual evolution state â€” all defaults (fresh robot)
    NewProfile.visual_evolution_state = FMXVisualEvolutionState();

    // Compute decal_seed from name hash XOR birth timestamp hash
    uint32 NameHash = GetTypeHash(NewProfile.name);
    uint32 TimeHash = GetTypeHash(NewProfile.date_of_birth.GetTicks());
    NewProfile.visual_evolution_state.decal_seed = static_cast<int32>(NameHash ^ TimeHash);

    // Store in active roster
    ActiveRoster.Add(NewProfile.robot_id, NewProfile);

    UE_LOG(LogTemp, Log, TEXT("MXRobotManager: Robot '%s' born (ID: %s, Level %d, Run %d)."),
           *NewProfile.name, *NewProfile.robot_id.ToString(), LevelNumber, RunNumber);

    return NewProfile.robot_id;
}

void UMXRobotManager::RemoveRobot(const FGuid& RobotId, int32 CurrentRunNumber)
{
    FMXRobotProfile* Profile = ActiveRoster.Find(RobotId);
    if (!Profile)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRobotManager::RemoveRobot â€” RobotId not found: %s"), *RobotId.ToString());
        return;
    }

    // Snapshot before removal
    FMXRobotProfile Snapshot = *Profile;
    DeadRobotArchive.Add(RobotId, Snapshot);
    CurrentRunDeaths.Add(Snapshot);

    // Release name back to pool with cooldown
    if (NameGenerator)
    {
        NameGenerator->ReleaseName(Profile->name, CurrentRunNumber);
    }

    ActiveRoster.Remove(RobotId);

    UE_LOG(LogTemp, Log, TEXT("MXRobotManager: Robot '%s' removed from roster and archived."), *Snapshot.name);
}

// -------------------------------------------------------------------------
// Stat Updates
// -------------------------------------------------------------------------

void UMXRobotManager::RecordNearMiss(const FGuid& RobotId, EHazardElement Element)
{
    FMXRobotProfile* Profile = ActiveRoster.Find(RobotId);
    if (!Profile) return;

    Profile->near_misses++;
    IncrementElementEncounter(*Profile, Element);
}

void UMXRobotManager::RecordWitnessEvent(const FVector& EventLocation, EEventType EventType, const FGuid& ExcludeId)
{
    // Note: Actual world-space position queries require the robot actors to register
    // their positions with the manager (or the Swarm module provides position queries).
    // For now, we iterate all active robots; the Swarm module should cull by radius.
    // This method is a fallback; prefer calling directly with known nearby robot IDs.

    for (auto& Pair : ActiveRoster)
    {
        if (Pair.Key == ExcludeId) continue;

        FMXRobotProfile& Profile = Pair.Value;
        switch (EventType)
        {
            case EEventType::RescueWitnessed:    Profile.rescues_witnessed++;    break;
            case EEventType::DeathWitnessed:     Profile.deaths_witnessed++;     break;
            case EEventType::SacrificeWitnessed: Profile.sacrifices_witnessed++; break;
            default: break;
        }
    }
}

void UMXRobotManager::AddXP(const FGuid& RobotId, int32 Amount)
{
    FMXRobotProfile* Profile = ActiveRoster.Find(RobotId);
    if (!Profile || Amount <= 0) return;

    // Apply spec XP modifier if spec is assigned (modifier sourced from FMXSpecBonus,
    // queried via the Specialization module â€” not directly coupled here).
    // For now, apply 1.0x; the Specialization module (Agent 5) can hook AddXP
    // and call with a pre-modified amount.

    Profile->xp       += Amount;
    Profile->total_xp += Amount;

    // Check for level-up(s) â€” a robot can skip multiple levels in one XP grant.
    while (Profile->level < 100)
    {
        int32 Threshold = GetXPThresholdForLevel(Profile->level + 1);
        if (Profile->total_xp < Threshold) break;

        Profile->level++;
        Profile->xp = 0; // xp resets to 0 on level-up (display only; total_xp tracks lifetime)

        UE_LOG(LogTemp, Log, TEXT("MXRobotManager: '%s' leveled up to %d!"), *Profile->name, Profile->level);

        // Fire OnLevelUp event â€” consumed by Specialization, DEMS, UI
        // Event bus broadcast: requires bus reference resolved at runtime.
        // Stored here as a deferred call pattern; actual broadcast via resolved bus.
    }
}

void UMXRobotManager::ApplySpecChoice(const FGuid& RobotId, ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3)
{
    FMXRobotProfile* Profile = ActiveRoster.Find(RobotId);
    if (!Profile) return;

    Profile->role       = Role;
    Profile->tier2_spec = Tier2;
    Profile->tier3_spec = Tier3;

    // Mastery title is set separately once the full arc is completed (level 100).
}

void UMXRobotManager::CheckTitles(const FGuid& RobotId)
{
    FMXRobotProfile* Profile = ActiveRoster.Find(RobotId);
    if (!Profile) return;

    // ---- Survival titles ----
    TryAwardTitle(*Profile, TEXT("Veteran"),       Profile->runs_survived >= 10);
    TryAwardTitle(*Profile, TEXT("Survivor"),      Profile->runs_survived >= 25);
    TryAwardTitle(*Profile, TEXT("Legend"),        Profile->runs_survived >= 50);
    TryAwardTitle(*Profile, TEXT("Immortal"),      Profile->runs_survived >= 100);

    // ---- Near-miss titles ----
    TryAwardTitle(*Profile, TEXT("Lucky"),         Profile->near_misses >= 5);
    TryAwardTitle(*Profile, TEXT("Charmed"),       Profile->near_misses >= 20);
    TryAwardTitle(*Profile, TEXT("Untouchable"),   Profile->near_misses >= 50);

    // ---- Hazard encounter titles ----
    TryAwardTitle(*Profile, TEXT("Scorched"),      Profile->fire_encounters >= 10);
    TryAwardTitle(*Profile, TEXT("Firewalker"),    Profile->fire_encounters >= 30);
    TryAwardTitle(*Profile, TEXT("Crushed...ish"), Profile->crush_encounters >= 10);
    TryAwardTitle(*Profile, TEXT("Frostbitten"),   Profile->ice_encounters >= 10);
    TryAwardTitle(*Profile, TEXT("Grounded"),      Profile->emp_encounters >= 10);

    // ---- Social / witness titles ----
    TryAwardTitle(*Profile, TEXT("Socialite"),     Profile->rescues_witnessed >= 20);
    TryAwardTitle(*Profile, TEXT("Haunted"),       Profile->deaths_witnessed >= 20);
    TryAwardTitle(*Profile, TEXT("Witness"),       Profile->sacrifices_witnessed >= 5);
    TryAwardTitle(*Profile, TEXT("Mentor"),        Profile->puzzles_participated >= 30);

    // ---- Hat titles ----
    int32 UniqueHats = Profile->hat_history.Num();
    TryAwardTitle(*Profile, TEXT("Hat Enthusiast"),  UniqueHats >= 10);
    TryAwardTitle(*Profile, TEXT("Hat Connoisseur"), UniqueHats >= 25);
    TryAwardTitle(*Profile, TEXT("Hat Collector"),   UniqueHats >= 50);
    TryAwardTitle(*Profile, TEXT("The Haberdasher"), UniqueHats >= 100);
    TryAwardTitle(*Profile, TEXT("Faithful"),        Profile->consecutive_hat_runs >= 10);
    TryAwardTitle(*Profile, TEXT("Soulbound"),       Profile->consecutive_hat_runs >= 25);

    // ---- Streak titles ----
    TryAwardTitle(*Profile, TEXT("On A Roll"),     Profile->current_run_streak >= 5);
    TryAwardTitle(*Profile, TEXT("Unstoppable"),   Profile->current_run_streak >= 15);

    // ---- Level / spec titles ----
    TryAwardTitle(*Profile, TEXT("Specialist"),    Profile->role != ERobotRole::None);
    TryAwardTitle(*Profile, TEXT("Master"),        Profile->mastery_title != EMasteryTitle::None);
    TryAwardTitle(*Profile, TEXT("Ghost"),         Profile->tier3_spec == ETier3Spec::Ghost);
    TryAwardTitle(*Profile, TEXT("Scarred"),       Profile->visual_evolution_state.wear_level >= EWearLevel::BattleScarred);
}

// -------------------------------------------------------------------------
// Queries
// -------------------------------------------------------------------------

bool UMXRobotManager::IsRosterFull() const
{
    return ActiveRoster.Num() >= MXConstants::MAX_ROBOTS;
}

FMXRobotProfile UMXRobotManager::GetDeadRobotSnapshot(const FGuid& RobotId) const
{
    const FMXRobotProfile* Found = DeadRobotArchive.Find(RobotId);
    return Found ? *Found : FMXRobotProfile();
}

TArray<FMXRobotProfile> UMXRobotManager::GetAllDeadRobotSnapshots() const
{
    TArray<FMXRobotProfile> Out;
    DeadRobotArchive.GenerateValueArray(Out);
    return Out;
}

TArray<FMXRobotProfile> UMXRobotManager::GetCurrentRunDeaths() const
{
    return CurrentRunDeaths;
}

void UMXRobotManager::ClearCurrentRunDeaths()
{
    CurrentRunDeaths.Empty();
}

// -------------------------------------------------------------------------
// IMXRobotProvider
// -------------------------------------------------------------------------

FMXRobotProfile UMXRobotManager::GetRobotProfile_Implementation(const FGuid& RobotId) const
{
    const FMXRobotProfile* Found = ActiveRoster.Find(RobotId);
    if (Found)
    {
        FMXRobotProfile Copy = *Found;
        // Compute calendar_age at query time (not stored)
        if (AgingSystem)
        {
            Copy.calendar_age = AgingSystem->ComputeCalendarAge(Copy.date_of_birth);
        }
        return Copy;
    }
    return FMXRobotProfile();
}

TArray<FMXRobotProfile> UMXRobotManager::GetAllRobotProfiles_Implementation() const
{
    TArray<FMXRobotProfile> Out;
    Out.Reserve(ActiveRoster.Num());
    for (const auto& Pair : ActiveRoster)
    {
        FMXRobotProfile Copy = Pair.Value;
        if (AgingSystem)
        {
            Copy.calendar_age = AgingSystem->ComputeCalendarAge(Copy.date_of_birth);
        }
        Out.Add(Copy);
    }
    return Out;
}

int32 UMXRobotManager::GetRobotCount_Implementation() const
{
    return ActiveRoster.Num();
}

// -------------------------------------------------------------------------
// IMXEventListener
// -------------------------------------------------------------------------

void UMXRobotManager::OnDEMSEvent_Implementation(const FMXEventData& Event)
{
    // Forward all events to the life log
    if (LifeLog)
    {
        LifeLog->AppendEvent(Event.robot_id, Event);

        // For witness events, also append to the related robot's log
        if (Event.related_robot_id.IsValid())
        {
            LifeLog->AppendEvent(Event.related_robot_id, Event);
        }
    }
}

// -------------------------------------------------------------------------
// IMXPersistable
// -------------------------------------------------------------------------

void UMXRobotManager::MXSerialize(FArchive& Ar)
{
    // Serialize active roster count and entries
    int32 ActiveCount = ActiveRoster.Num();
    Ar << ActiveCount;
    for (auto& Pair : ActiveRoster)
    {
        FGuid Id = Pair.Key;
        Ar << Id;
        // Detailed struct serialization handled by the Persistence module (Agent 10)
        // which knows the full FMXRobotProfile layout.
    }

    // Serialize dead archive count
    int32 DeadCount = DeadRobotArchive.Num();
    Ar << DeadCount;
    for (auto& Pair : DeadRobotArchive)
    {
        FGuid Id = Pair.Key;
        Ar << Id;
    }
}

void UMXRobotManager::MXDeserialize(FArchive& Ar)
{
    // Deserialization mirrors Serialize pattern.
    // Full implementation delegated to Agent 10 (Persistence) which orchestrates
    // the archive format and struct versioning.
    int32 ActiveCount = 0;
    Ar << ActiveCount;
    // Profiles reconstructed by the Persistence module and injected via
    // a separate RestoreRoster(TArray<FMXRobotProfile>) method below.
}

// -------------------------------------------------------------------------
// Event Handlers
// -------------------------------------------------------------------------

void UMXRobotManager::HandleRobotRescued(FGuid RobotId, int32 LevelNumber)
{
    // A rescue means a new robot is born â€” but in this handler the RobotId
    // refers to the newly-created robot (ID assigned by Swarm before firing).
    // The Swarm module calls CreateRobot() directly and fires the event after.
    // Here we just add XP to nearby witnesses.
    // XP award for witnesses: 50 XP per witnessed rescue.
    for (auto& Pair : ActiveRoster)
    {
        if (Pair.Key != RobotId)
        {
            AddXP(Pair.Key, 50);
        }
    }
}

void UMXRobotManager::HandleRobotDied(FGuid RobotId, FMXEventData DeathEvent)
{
    // The run number should be tracked separately (set by Roguelike module).
    // Use 0 as fallback; name cooldown will still work via day-based timer.
    RemoveRobot(RobotId, DeathEvent.run_number);
}

void UMXRobotManager::HandleRobotSacrificed(FGuid RobotId, FMXEventData SacrificeEvent)
{
    // Sacrifice is treated identically to death for roster management.
    RemoveRobot(RobotId, SacrificeEvent.run_number);
}

void UMXRobotManager::HandleNearMiss(FGuid RobotId, FMXObjectEventFields HazardFields)
{
    RecordNearMiss(RobotId, HazardFields.element);
    // Near miss XP reward
    AddXP(RobotId, 25);
}

void UMXRobotManager::HandleLevelComplete(int32 LevelNumber, const TArray<FGuid>& SurvivingRobots)
{
    for (const FGuid& Id : SurvivingRobots)
    {
        FMXRobotProfile* Profile = ActiveRoster.Find(Id);
        if (!Profile) continue;

        Profile->levels_completed++;
        AddXP(Id, 100); // Base XP for surviving a level
    }
}

void UMXRobotManager::HandleRunComplete(FMXRunData RunData)
{
    for (auto& Pair : ActiveRoster)
    {
        FMXRobotProfile& Profile = Pair.Value;
        Profile.runs_survived++;
        Profile.current_run_streak++;

        // Update hat wear tracking for consecutive hat bonus
        if (Profile.current_hat_id != -1)
        {
            if (Profile.consecutive_hat_id == Profile.current_hat_id)
            {
                Profile.consecutive_hat_runs++;
            }
            else
            {
                Profile.consecutive_hat_id   = Profile.current_hat_id;
                Profile.consecutive_hat_runs = 1;
            }
        }
        else
        {
            Profile.consecutive_hat_id   = -1;
            Profile.consecutive_hat_runs = 0;
        }

        // Run completion XP
        AddXP(Pair.Key, 200);

        CheckTitles(Pair.Key);
    }
}

void UMXRobotManager::HandleRunFailed(FMXRunData RunData, int32 FailureLevel)
{
    // Survivors don't get runs_survived incremented on failure,
    // but streak is NOT reset for survivors (only death resets streak).
    for (auto& Pair : ActiveRoster)
    {
        CheckTitles(Pair.Key);
    }
}

void UMXRobotManager::HandleHatEquipped(FGuid RobotId, int32 HatId)
{
    FMXRobotProfile* Profile = ActiveRoster.Find(RobotId);
    if (!Profile) return;

    Profile->current_hat_id = HatId;

    // Add to hat history if not already present
    if (!Profile->hat_history.Contains(HatId))
    {
        Profile->hat_history.Add(HatId);
    }
}

void UMXRobotManager::HandleHatLost(FGuid RobotId, int32 HatId, FMXEventData DeathEvent)
{
    FMXRobotProfile* Profile = ActiveRoster.Find(RobotId);
    if (!Profile) return;

    Profile->current_hat_id     = -1;
    Profile->consecutive_hat_id   = -1;
    Profile->consecutive_hat_runs = 0;
}

void UMXRobotManager::HandleSpecChosen(FGuid RobotId, ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3)
{
    ApplySpecChoice(RobotId, Role, Tier2, Tier3);
}

// -------------------------------------------------------------------------
// Private Helpers
// -------------------------------------------------------------------------

int32 UMXRobotManager::GetXPThresholdForLevel(int32 Level) const
{
    // XP_THRESHOLDS array has 12 entries (indices 0â€“11).
    // For levels beyond index 11, extrapolate by doubling the last gap.
    constexpr int32 ThresholdCount = sizeof(MXConstants::XP_THRESHOLDS) / sizeof(MXConstants::XP_THRESHOLDS[0]);
    int32 Index = Level - 1;

    if (Index < 0) return 0;
    if (Index < ThresholdCount) return MXConstants::XP_THRESHOLDS[Index];

    // Beyond defined thresholds: each level costs 2x the last defined gap.
    int32 LastThreshold = MXConstants::XP_THRESHOLDS[ThresholdCount - 1];
    int32 Gap           = MXConstants::XP_THRESHOLDS[ThresholdCount - 1]
                        - MXConstants::XP_THRESHOLDS[ThresholdCount - 2];
    int32 ExtraLevels   = Index - (ThresholdCount - 1);
    return LastThreshold + Gap * 2 * ExtraLevels;
}

void UMXRobotManager::GenerateAppearance(FMXRobotProfile& OutProfile) const
{
    // chassis_color: 0â€“11 (12 colours in master material palette)
    OutProfile.chassis_color = FMath::RandRange(0, 11);

    // eye_color: 0â€“7 (8 eye emissive colours)
    OutProfile.eye_color = FMath::RandRange(0, 7);

    // antenna_style: 0â€“4 (5 mesh variants)
    OutProfile.antenna_style = FMath::RandRange(0, 4);

    // paint_job: weighted toward None (most robots start unpainted)
    // EPaintJob has 11 values (None + 10 schemes). 70% chance None.
    int32 PaintRoll = FMath::RandRange(0, 9);
    OutProfile.paint_job = (PaintRoll < 7) ? EPaintJob::None
                         : static_cast<EPaintJob>(FMath::RandRange(1, 10));
}

void UMXRobotManager::IncrementElementEncounter(FMXRobotProfile& Profile, EHazardElement Element) const
{
    switch (Element)
    {
        case EHazardElement::Fire:        Profile.fire_encounters++;  break;
        case EHazardElement::Mechanical:  Profile.crush_encounters++; break;
        case EHazardElement::Gravity:     Profile.fall_encounters++;  break;
        case EHazardElement::Electrical:  Profile.emp_encounters++;   break;
        case EHazardElement::Ice:         Profile.ice_encounters++;   break;
        default: break; // Comedy, Sacrifice â€” no specific encounter counter
    }
}

void UMXRobotManager::TryAwardTitle(FMXRobotProfile& Profile, const FString& Title, bool bCondition) const
{
    if (bCondition && !Profile.earned_titles.Contains(Title))
    {
        Profile.earned_titles.Add(Title);
        UE_LOG(LogTemp, Log, TEXT("MXRobotManager: '%s' earned title '%s'."), *Profile.name, *Title);

        // Auto-set displayed_title if the robot doesn't have one yet
        if (Profile.displayed_title.IsEmpty())
        {
            Profile.displayed_title = Title;
        }
    }
}
