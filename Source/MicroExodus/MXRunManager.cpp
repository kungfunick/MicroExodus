// MXRunManager.cpp — Roguelike Module v1.0
// Agent 6: Roguelike

#include "MXRunManager.h"
#include "MXEventData.h"
#include "MXEventFields.h"

// Target time per level: 5 minutes (300 seconds), per GDD.
static constexpr float LEVEL_TARGET_SECONDS = 300.0f;

// Minimum level required for post-failure hat rewards.
static constexpr int32 HAT_REWARD_MIN_LEVEL = 5;

// Scoring constants (per GDD).
static constexpr int32 SCORE_RESCUE_BONUS    =  500;
static constexpr int32 SCORE_SURVIVAL_BONUS  =  200;
static constexpr int32 SCORE_LOSS_PENALTY    = -300;
static constexpr int32 SCORE_TIME_BONUS      =  100;

UMXRunManager::UMXRunManager()
    : bRunActive(false)
    , CurrentLevel(1)
    , LevelsUnderTargetTime(0)
    , GlobalRunCount(0)
{
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXRunManager::Initialise(
    UMXEventBus*                       InEventBus,
    TScriptInterface<IMXRobotProvider>  InRobotProvider,
    TScriptInterface<IMXHatProvider>    InHatProvider,
    UDataTable*                         InTierDataTable,
    UDataTable*                         InXPDataTable)
{
    EventBus      = InEventBus;
    RobotProvider = InRobotProvider;
    HatProvider   = InHatProvider;

    // Construct and initialise subsystems.
    TierSystem = NewObject<UMXTierSystem>(this);

    ModifierRegistry = NewObject<UMXModifierRegistry>(this);
    if (InTierDataTable)
    {
        ModifierRegistry->InitialiseFromDataTable(InTierDataTable);
    }

    SpawnManager = NewObject<UMXSpawnManager>(this);
    SpawnManager->SetRobotProvider(RobotProvider);

    XPDistributor = NewObject<UMXXPDistributor>(this);
    XPDistributor->SetRobotProvider(RobotProvider);
    if (InXPDataTable)
    {
        XPDistributor->InitialiseFromDataTable(InXPDataTable);
    }

    // Wire event bus callbacks.
    BindToEventBus();

    UE_LOG(LogTemp, Log, TEXT("MXRunManager: Initialised. Global run count so far: %d."), GlobalRunCount);
}

// ---------------------------------------------------------------------------
// Run Lifecycle
// ---------------------------------------------------------------------------

bool UMXRunManager::StartRun(ETier Tier, const TArray<FString>& Modifiers, const TArray<FGuid>& SelectedRobots)
{
    if (bRunActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRunManager::StartRun — A run is already active. Ignoring."));
        return false;
    }

    // Validate tier unlock.
    if (!TierSystem->IsTierUnlocked(Tier))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXRunManager::StartRun — Tier %d is not unlocked. Cannot start run."),
            static_cast<int32>(Tier));
        return false;
    }

    // Validate and enforce robot count limits.
    CurrentModifierEffects = ModifierRegistry->GetActiveModifierEffects(Modifiers);
    const int32 MaxRobots  = (CurrentModifierEffects.MaxRobotOverride > 0)
                                ? CurrentModifierEffects.MaxRobotOverride
                                : MXConstants::MAX_ROBOTS;

    if (SelectedRobots.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRunManager::StartRun — No robots selected. Aborting."));
        return false;
    }

    if (SelectedRobots.Num() > MaxRobots)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXRunManager::StartRun — %d robots selected but cap is %d (modifier or MAX_ROBOTS). Aborting."),
            SelectedRobots.Num(), MaxRobots);
        return false;
    }

    // Cache tier effects for use during the run.
    CurrentTierEffects = TierSystem->GetTierEffects(Tier);

    // Increment global run counter.
    ++GlobalRunCount;

    // Initialise run data.
    CurrentRunData = FMXRunData();
    CurrentRunData.run_number       = GlobalRunCount;
    CurrentRunData.tier             = Tier;
    CurrentRunData.active_modifiers = Modifiers;
    CurrentRunData.start_time       = FDateTime::UtcNow();
    CurrentRunData.robots_deployed  = SelectedRobots.Num();
    CurrentRunData.outcome          = ERunOutcome::Failure; // Default; overridden on success.

    // Initialise surviving robot tracking.
    SurvivingRobotIds = SelectedRobots;

    // Reset subsystems.
    SpawnManager->ResetForNewRun();
    XPDistributor->ResetForNewRun();
    LevelsUnderTargetTime = 0;

    // Begin at Level 1.
    CurrentLevel         = 1;
    CurrentLevelStartTime = FDateTime::UtcNow();
    bRunActive           = true;

    // Spawn rescue robots for Level 1.
    SpawnManager->SpawnRescueRobotsForLevel(CurrentLevel, Tier, CurrentModifierEffects);

    UE_LOG(LogTemp, Log,
        TEXT("MXRunManager: Run #%d started. Tier=%d, Modifiers=%d, Robots=%d."),
        GlobalRunCount, static_cast<int32>(Tier), Modifiers.Num(), SelectedRobots.Num());

    return true;
}

void UMXRunManager::EndRun(ERunOutcome Outcome)
{
    if (!bRunActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRunManager::EndRun called but no run is active."));
        return;
    }

    bRunActive = false;
    UnbindFromEventBus();

    // Populate survivors count from the final surviving list.
    CurrentRunData.robots_survived = SurvivingRobotIds.Num();
    CurrentRunData.outcome         = Outcome;
    CurrentRunData.end_time        = FDateTime::UtcNow();
    CurrentRunData.levels_completed = CurrentLevel - 1; // Level we just *completed* before ending.

    // If the run succeeded, the player cleared all levels — cap at MAX_LEVELS.
    if (Outcome == ERunOutcome::Success)
    {
        CurrentRunData.levels_completed = MXConstants::MAX_LEVELS;
    }

    // Calculate final score.
    const FMXScoreBreakdown Breakdown = CalculateScore(Outcome);
    CurrentRunData.score_breakdown    = Breakdown;
    CurrentRunData.score              = Breakdown.final_score;

    // Award run-completion XP to survivors on success.
    if (Outcome == ERunOutcome::Success)
    {
        XPDistributor->DistributeRunXP(SurvivingRobotIds, CurrentRunData.tier);

        // Evaluate tier progression.
        TierSystem->EvaluateAndUnlockNextTier(CurrentRunData.tier, SurvivingRobotIds.Num());
    }

    // Evaluate modifier unlocks regardless of outcome.
    ModifierRegistry->EvaluateUnlocks(CurrentRunData, GlobalRunCount);

    // Distribute hat rewards for eligible runs.
    const bool bEligibleForHats = (Outcome == ERunOutcome::Success) ||
                                   (CurrentRunData.levels_completed >= HAT_REWARD_MIN_LEVEL);
    if (bEligibleForHats)
    {
        DistributeHatRewards();
    }

    // Fire the appropriate event bus broadcast.
    if (EventBus)
    {
        if (Outcome == ERunOutcome::Success)
        {
            EventBus->OnRunComplete.Broadcast(CurrentRunData);
        }
        else
        {
            EventBus->OnRunFailed.Broadcast(CurrentRunData, CurrentLevel);
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXRunManager: Run #%d ended. Outcome=%s, Score=%d, Levels=%d, Survived=%d, Lost=%d."),
        CurrentRunData.run_number,
        (Outcome == ERunOutcome::Success) ? TEXT("SUCCESS") : TEXT("FAILURE"),
        CurrentRunData.score,
        CurrentRunData.levels_completed,
        CurrentRunData.robots_survived,
        CurrentRunData.robots_lost);
}

void UMXRunManager::AdvanceLevel()
{
    if (!bRunActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRunManager::AdvanceLevel called but no run is active."));
        return;
    }

    // Check time bonus for the level just completed.
    if (IsCurrentLevelUnderTargetTime())
    {
        ++LevelsUnderTargetTime;
    }

    // Award level-completion XP to all surviving robots.
    XPDistributor->DistributeLevelXP(SurvivingRobotIds, CurrentLevel, CurrentRunData.tier);

    // Fire the OnLevelComplete event so Camera, DEMS, and UI can react.
    if (EventBus)
    {
        EventBus->OnLevelComplete.Broadcast(CurrentLevel, SurvivingRobotIds);
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXRunManager: Level %d complete. %d survivors. Time bonus: %s."),
        CurrentLevel,
        SurvivingRobotIds.Num(),
        IsCurrentLevelUnderTargetTime() ? TEXT("YES") : TEXT("NO"));

    // Check for run completion.
    if (CurrentLevel >= MXConstants::MAX_LEVELS)
    {
        EndRun(ERunOutcome::Success);
        return;
    }

    // Advance to next level.
    ++CurrentLevel;
    CurrentLevelStartTime = FDateTime::UtcNow();

    // Spawn rescue robots for the new level.
    SpawnManager->SpawnRescueRobotsForLevel(CurrentLevel, CurrentRunData.tier, CurrentModifierEffects);
}

void UMXRunManager::AbortRun()
{
    if (!bRunActive)
    {
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("MXRunManager: Run #%d aborted by player at Level %d."), GlobalRunCount, CurrentLevel);
    EndRun(ERunOutcome::Failure);
}

// ---------------------------------------------------------------------------
// In-Run Event Recording
// ---------------------------------------------------------------------------

void UMXRunManager::RecordEvent(const FMXEventData& Event)
{
    if (bRunActive)
    {
        CurrentRunData.events.Add(Event);
    }
}

void UMXRunManager::IncrementRobotsLost()
{
    ++CurrentRunData.robots_lost;
}

void UMXRunManager::IncrementRobotsRescued()
{
    ++CurrentRunData.robots_rescued;
}

void UMXRunManager::IncrementRobotsSurvived()
{
    ++CurrentRunData.robots_survived;
}

// ---------------------------------------------------------------------------
// State Queries
// ---------------------------------------------------------------------------

bool UMXRunManager::IsRunActive() const
{
    return bRunActive;
}

int32 UMXRunManager::GetCurrentLevel() const
{
    return CurrentLevel;
}

int32 UMXRunManager::GetGlobalRunCount() const
{
    return GlobalRunCount;
}

FMXModifierEffects UMXRunManager::GetCurrentModifierEffects() const
{
    return CurrentModifierEffects;
}

FMXTierEffects UMXRunManager::GetCurrentTierEffects() const
{
    return CurrentTierEffects;
}

// ---------------------------------------------------------------------------
// IMXRunProvider Implementation
// ---------------------------------------------------------------------------

FMXRunData UMXRunManager::GetCurrentRunData_Implementation() const
{
    return CurrentRunData;
}

int32 UMXRunManager::GetRunNumber_Implementation() const
{
    return CurrentRunData.run_number;
}

ETier UMXRunManager::GetCurrentTier_Implementation() const
{
    return CurrentRunData.tier;
}

// ---------------------------------------------------------------------------
// IMXPersistable Implementation
// ---------------------------------------------------------------------------

void UMXRunManager::MXSerialize(FArchive& Ar)
{
    Ar << GlobalRunCount;

    uint8 TierMask = TierSystem ? TierSystem->GetUnlockedTiersMask() : 1;
    Ar << TierMask;

    TArray<FString> UnlockedMods;
    if (ModifierRegistry)
    {
        UnlockedMods = ModifierRegistry->GetUnlockedModifierNames();
    }
    Ar << UnlockedMods;
}

void UMXRunManager::MXDeserialize(FArchive& Ar)
{
    Ar << GlobalRunCount;

    uint8 TierMask = 1;
    Ar << TierMask;
    if (TierSystem)
    {
        TierSystem->SetUnlockedTiersMask(TierMask);
    }

    TArray<FString> UnlockedMods;
    Ar << UnlockedMods;
    if (ModifierRegistry)
    {
        TSet<FString> ModSet(UnlockedMods);
        ModifierRegistry->SetUnlockedModifierSet(ModSet);
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXRunManager: Deserialised. GlobalRunCount=%d, TierMask=%d, Mods=%d."),
        GlobalRunCount, TierMask, UnlockedMods.Num());
}

// ---------------------------------------------------------------------------
// Scoring
// ---------------------------------------------------------------------------

FMXScoreBreakdown UMXRunManager::CalculateScore(ERunOutcome Outcome) const
{
    FMXScoreBreakdown Breakdown;

    // Component values.
    Breakdown.rescue_bonus   = CurrentRunData.robots_rescued * SCORE_RESCUE_BONUS;
    Breakdown.survival_bonus = SurvivingRobotIds.Num()       * SCORE_SURVIVAL_BONUS;
    Breakdown.loss_penalty   = CurrentRunData.robots_lost    * SCORE_LOSS_PENALTY;
    Breakdown.time_bonus     = LevelsUnderTargetTime         * SCORE_TIME_BONUS;

    // Multiplier = TierMultiplier × (1 + sum of active modifier XP bonuses).
    const float TierMult     = MXConstants::TIER_MULTIPLIERS[static_cast<int32>(CurrentRunData.tier)];
    const float ModBonusSum  = CurrentModifierEffects.CombinedXPBonus;
    Breakdown.multiplier     = TierMult * (1.0f + ModBonusSum);

    // Final score — clamp to 0 so a catastrophic run cannot go deeply negative.
    const int32 RawSum = Breakdown.rescue_bonus
                       + Breakdown.survival_bonus
                       + Breakdown.loss_penalty
                       + Breakdown.time_bonus;

    Breakdown.final_score = FMath::Max(0,
        FMath::FloorToInt(static_cast<float>(RawSum) * Breakdown.multiplier));

    return Breakdown;
}

// ---------------------------------------------------------------------------
// Hat Reward Distribution
// ---------------------------------------------------------------------------

void UMXRunManager::DistributeHatRewards()
{
    if (!HatProvider.GetInterface())
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRunManager::DistributeHatRewards — No HatProvider set."));
        return;
    }

    // Deposit one copy of every hat currently unlocked in the collection.
    // Combo and hidden unlock checks are delegated to the Hats module.
    // We call ReturnHat for each hat worn by a surviving robot, and the
    // Hats module handles combo detection internally via OnHatEquipped events
    // that were already fired during the run.
    //
    // The full hat-reward deposit flow (depositing one copy of each unlocked hat
    // for the collection) requires iteration over the hat catalogue — this is
    // done by calling GetHatCollection() and iterating the unlocked IDs.
    const FMXHatCollection Collection = IMXHatProvider::Execute_GetHatCollection(HatProvider.GetObject());

    // For each hat that was unlocked before this run started, deposit one copy.
    for (const int32 HatId : Collection.unlocked_hat_ids)
    {
        IMXHatProvider::Execute_ReturnHat(HatProvider.GetObject(), HatId);
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXRunManager: Deposited post-run hat rewards (%d hat types)."),
        Collection.unlocked_hat_ids.Num());
}

// ---------------------------------------------------------------------------
// Event Bus Handlers
// ---------------------------------------------------------------------------

void UMXRunManager::HandleRobotDied(FGuid RobotId, FMXEventData DeathEvent)
{
    if (!bRunActive) return;

    RecordEvent(DeathEvent);
    IncrementRobotsLost();

    // Remove from surviving list.
    SurvivingRobotIds.Remove(RobotId);

    // Check Legendary permadeath condition.
    CheckPermadeathCondition();

    // Check if the escort robot was the one that died.
    if (CurrentModifierEffects.EscortTargetRobot.IsValid())
    {
        CheckEscortCondition(RobotId);
    }

    // Check for total wipeout.
    if (SurvivingRobotIds.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("MXRunManager: Total wipeout on Level %d. Ending run."), CurrentLevel);
        EndRun(ERunOutcome::Failure);
    }
}

void UMXRunManager::HandleRobotSacrificed(FGuid RobotId, FMXEventData SacrificeEvent)
{
    if (!bRunActive) return;

    RecordEvent(SacrificeEvent);

    // Sacrifices are tracked separately — they are NOT losses for scoring purposes.
    // Remove from surviving list so they don't count as survivors either.
    SurvivingRobotIds.Remove(RobotId);

    // Award witness XP to the related robot if present (FMXEventData has related_robot_id, not a list).
    // Full witness broadcast is handled by the DEMS/EventBus layer when the event fires.
    if (SacrificeEvent.related_robot_id.IsValid() && SacrificeEvent.related_robot_id != RobotId)
    {
        XPDistributor->AwardEventXP(SacrificeEvent.related_robot_id, EEventType::SacrificeWitnessed, CurrentRunData.tier);
    }

    // Check for total wipeout after sacrifice.
    if (SurvivingRobotIds.Num() == 0 && bRunActive)
    {
        EndRun(ERunOutcome::Failure);
    }
}

void UMXRunManager::HandleRobotRescued(FGuid RobotId, int32 LevelNumber)
{
    if (!bRunActive) return;

    IncrementRobotsRescued();

    // Add rescued robot to the surviving list.
    SurvivingRobotIds.AddUnique(RobotId);

    // Award witness XP to nearby robots. The SpawnManager already created the robot;
    // we distribute witness XP to all current survivors within RESCUE_WITNESS_RADIUS.
    // Proximity filtering is expected to happen at the gameplay layer — here we award
    // a flat witness bonus to all current survivors as a simplified approach.
    for (const FGuid& SurvivorId : SurvivingRobotIds)
    {
        if (SurvivorId != RobotId)
        {
            XPDistributor->AwardEventXP(SurvivorId, EEventType::RescueWitnessed, CurrentRunData.tier);
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXRunManager: Robot %s rescued on Level %d. Total rescued=%d."),
        *RobotId.ToString(), LevelNumber, CurrentRunData.robots_rescued);
}

void UMXRunManager::HandleNearMiss(FGuid RobotId, FMXObjectEventFields HazardFields)
{
    if (!bRunActive) return;

    XPDistributor->AwardEventXP(RobotId, EEventType::NearMiss, CurrentRunData.tier);
}

void UMXRunManager::HandleHatEquipped(FGuid RobotId, int32 HatId)
{
    // Hat tracking is handled by the Hats module; RunManager just logs for run data.
    UE_LOG(LogTemp, Verbose,
        TEXT("MXRunManager: Hat %d equipped on robot %s."), HatId, *RobotId.ToString());
}

void UMXRunManager::HandleHatLost(FGuid RobotId, int32 HatId, FMXEventData DeathEvent)
{
    // Hat loss is handled by the Hats module. RunManager notes it for report generation.
    UE_LOG(LogTemp, Verbose,
        TEXT("MXRunManager: Hat %d lost with robot %s."), HatId, *RobotId.ToString());
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

void UMXRunManager::BindToEventBus()
{
    if (!EventBus) return;

    EventBus->OnRobotDied.AddDynamic(this,         &UMXRunManager::HandleRobotDied);
    EventBus->OnRobotSacrificed.AddDynamic(this,   &UMXRunManager::HandleRobotSacrificed);
    EventBus->OnRobotRescued.AddDynamic(this,      &UMXRunManager::HandleRobotRescued);
    EventBus->OnNearMiss.AddDynamic(this,          &UMXRunManager::HandleNearMiss);
    EventBus->OnHatEquipped.AddDynamic(this,       &UMXRunManager::HandleHatEquipped);
    EventBus->OnHatLost.AddDynamic(this,           &UMXRunManager::HandleHatLost);
}

void UMXRunManager::UnbindFromEventBus()
{
    if (!EventBus) return;

    EventBus->OnRobotDied.RemoveDynamic(this,       &UMXRunManager::HandleRobotDied);
    EventBus->OnRobotSacrificed.RemoveDynamic(this, &UMXRunManager::HandleRobotSacrificed);
    EventBus->OnRobotRescued.RemoveDynamic(this,    &UMXRunManager::HandleRobotRescued);
    EventBus->OnNearMiss.RemoveDynamic(this,        &UMXRunManager::HandleNearMiss);
    EventBus->OnHatEquipped.RemoveDynamic(this,     &UMXRunManager::HandleHatEquipped);
    EventBus->OnHatLost.RemoveDynamic(this,         &UMXRunManager::HandleHatLost);
}

void UMXRunManager::CheckEscortCondition(const FGuid& DiedRobotId)
{
    if (!bRunActive) return;

    if (CurrentModifierEffects.EscortTargetRobot == DiedRobotId)
    {
        UE_LOG(LogTemp, Log,
            TEXT("MXRunManager: Escort robot %s has died. Run failed (Legendary Escort)."),
            *DiedRobotId.ToString());
        EndRun(ERunOutcome::Failure);
    }
}

void UMXRunManager::CheckPermadeathCondition()
{
    if (!bRunActive) return;

    if (CurrentTierEffects.bPermadeathOnFirstLoss)
    {
        UE_LOG(LogTemp, Log,
            TEXT("MXRunManager: Permadeath triggered (Legendary tier). Run ended on first loss."));
        EndRun(ERunOutcome::Failure);
    }
}

bool UMXRunManager::IsCurrentLevelUnderTargetTime() const
{
    const FTimespan Elapsed = FDateTime::UtcNow() - CurrentLevelStartTime;
    return Elapsed.GetTotalSeconds() < LEVEL_TARGET_SECONDS;
}
