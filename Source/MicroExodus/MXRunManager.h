// MXRunManager.h — Roguelike Module v1.0
// Agent 6: Roguelike
// Central manager for the entire run lifecycle in Micro Exodus.
// Implements IMXRunProvider (read access for Reports/UI) and IMXPersistable (save/load).
// Orchestrates MXTierSystem, MXModifierRegistry, MXSpawnManager, and MXXPDistributor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MXInterfaces.h"
#include "MXTypes.h"
#include "MXRunData.h"
#include "MXEvents.h"
#include "MXConstants.h"
#include "MXTierSystem.h"
#include "MXModifierRegistry.h"
#include "MXSpawnManager.h"
#include "MXXPDistributor.h"
#include "MXRunManager.generated.h"

// Forward declarations
class UMXEventBus;

// ---------------------------------------------------------------------------
// UMXRunManager
// ---------------------------------------------------------------------------

/**
 * UMXRunManager
 * The authoritative run lifecycle controller for Micro Exodus.
 *
 * Responsibilities:
 *  - Validates and starts runs (StartRun)
 *  - Manages level transitions (AdvanceLevel)
 *  - Tracks all in-run counters (robots survived, lost, rescued)
 *  - Calculates and finalises score on run end
 *  - Fires OnRunComplete / OnRunFailed via the event bus
 *  - Triggers hat rewards through IMXHatProvider
 *  - Manages global run counter persistence via IMXPersistable
 *
 * Singleton-like usage: one instance lives on the GameMode or GameInstance.
 * External systems reach it through its IMXRunProvider interface.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXRunManager : public UObject, public IMXRunProvider, public IMXPersistable
{
    GENERATED_BODY()

public:

    UMXRunManager();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Binds all subsystems and wires event bus handlers.
     * Must be called once before any run can be started.
     * @param InEventBus       The game's central event bus instance.
     * @param InRobotProvider  The Identity module's RobotManager.
     * @param InHatProvider    The Hats module's HatManager.
     * @param InTierDataTable      DataTable asset DT_TierModifiers.
     * @param InXPDataTable        DataTable asset DT_XPCurve.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void Initialise(
        UMXEventBus*                      InEventBus,
        TScriptInterface<IMXRobotProvider> InRobotProvider,
        TScriptInterface<IMXHatProvider>   InHatProvider,
        UDataTable*                        InTierDataTable,
        UDataTable*                        InXPDataTable
    );

    // -------------------------------------------------------------------------
    // Run Lifecycle
    // -------------------------------------------------------------------------

    /**
     * Starts a new run. Validates tier unlock, applies modifiers, initialises FMXRunData,
     * resets subsystem state, and prepares Level 1.
     * @param Tier            The difficulty tier requested.
     * @param Modifiers       Array of active modifier name strings.
     * @param SelectedRobots  GUIDs of the robots the player is deploying (max 100, or modifier cap).
     * @return                True if the run started successfully; false if validation failed.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    bool StartRun(ETier Tier, const TArray<FString>& Modifiers, const TArray<FGuid>& SelectedRobots);

    /**
     * Ends the current run with the given outcome.
     * Finalises FMXRunData (end_time, score, outcome), fires the appropriate event,
     * triggers hat reward distribution, evaluates tier and modifier unlocks.
     * @param Outcome  ERunOutcome::Success or ERunOutcome::Failure.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void EndRun(ERunOutcome Outcome);

    /**
     * Transitions from the current level to the next.
     * Tallies surviving robots, awards level XP, fires OnLevelComplete, advances CurrentLevel.
     * Called automatically when the level-end condition is met (all robots clear the exit zone).
     * If CurrentLevel == MAX_LEVELS after advancing, calls EndRun(Success).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void AdvanceLevel();

    /**
     * Aborts the current run immediately (player-initiated quit).
     * Treated as a failure. Eligible for post-run hat rewards if past Level 5.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void AbortRun();

    // -------------------------------------------------------------------------
    // In-Run Event Recording
    // -------------------------------------------------------------------------

    /**
     * Appends a DEMS event to the current run's event log.
     * All systems that produce events call this so the full log is available for Reports.
     * @param Event  The fully constructed FMXEventData.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void RecordEvent(const FMXEventData& Event);

    /**
     * Increments the robots_lost counter in the current run data.
     * Called automatically from the OnRobotDied handler.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void IncrementRobotsLost();

    /**
     * Increments the robots_rescued counter in the current run data.
     * Called automatically from the OnRobotRescued handler.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void IncrementRobotsRescued();

    /**
     * Increments the robots_survived counter in the current run data.
     * Called during AdvanceLevel for each robot that cleared the level.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    void IncrementRobotsSurvived();

    // -------------------------------------------------------------------------
    // State Queries
    // -------------------------------------------------------------------------

    /**
     * Returns true if a run is currently active (between StartRun and EndRun).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    bool IsRunActive() const;

    /**
     * Returns the 1-indexed number of the level currently being played.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    int32 GetCurrentLevel() const;

    /**
     * Returns the all-time total run count including the current run if active.
     * Persisted across sessions.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    int32 GetGlobalRunCount() const;

    /**
     * Returns the combined modifier effects active in the current run.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    FMXModifierEffects GetCurrentModifierEffects() const;

    /**
     * Returns the tier effects active in the current run.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    FMXTierEffects GetCurrentTierEffects() const;

    // -------------------------------------------------------------------------
    // IMXRunProvider Implementation
    // -------------------------------------------------------------------------

    /** Returns a snapshot of the current in-progress run data. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    virtual FMXRunData GetCurrentRunData_Implementation() const override;

    /** Returns the sequential run number for the current run. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    virtual int32 GetRunNumber_Implementation() const override;

    /** Returns the active difficulty tier. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    virtual ETier GetCurrentTier_Implementation() const override;

    // -------------------------------------------------------------------------
    // IMXPersistable Implementation
    // -------------------------------------------------------------------------

    /** Serialises GlobalRunCount, tier unlock mask, and unlocked modifier set. */
    virtual void MXSerialize(FArchive& Ar) override;

    /** Deserialises GlobalRunCount, tier unlock mask, and unlocked modifier set. */
    virtual void MXDeserialize(FArchive& Ar) override;

    // -------------------------------------------------------------------------
    // Subsystem Access (read-only for other Roguelike classes)
    // -------------------------------------------------------------------------

    /** Returns the TierSystem subsystem for external Roguelike queries. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    UMXTierSystem* GetTierSystem() const { return TierSystem; }

    /** Returns the ModifierRegistry subsystem. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    UMXModifierRegistry* GetModifierRegistry() const { return ModifierRegistry; }

    /** Returns the SpawnManager subsystem. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    UMXSpawnManager* GetSpawnManager() const { return SpawnManager; }

    /** Returns the XPDistributor subsystem. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunManager")
    UMXXPDistributor* GetXPDistributor() const { return XPDistributor; }

private:

    // -------------------------------------------------------------------------
    // Subsystems
    // -------------------------------------------------------------------------

    /** Owns tier unlock state and gameplay effect definitions. */
    UPROPERTY()
    UMXTierSystem* TierSystem = nullptr;

    /** Owns modifier definitions and unlock progression. */
    UPROPERTY()
    UMXModifierRegistry* ModifierRegistry = nullptr;

    /** Manages rescue robot spawn budgets and placement. */
    UPROPERTY()
    UMXSpawnManager* SpawnManager = nullptr;

    /** Calculates and dispatches XP to robots after events. */
    UPROPERTY()
    UMXXPDistributor* XPDistributor = nullptr;

    // -------------------------------------------------------------------------
    // External References
    // -------------------------------------------------------------------------

    /** Central event bus for firing and listening to game events. */
    UPROPERTY()
    UMXEventBus* EventBus = nullptr;

    /** Identity module robot provider — used for roster queries and XP calls. */
    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

    /** Hats module provider — used for post-run hat reward deposit. */
    UPROPERTY()
    TScriptInterface<IMXHatProvider> HatProvider;

    // -------------------------------------------------------------------------
    // Run State
    // -------------------------------------------------------------------------

    /** The in-progress run data record. Owned exclusively by this manager. */
    UPROPERTY()
    FMXRunData CurrentRunData;

    /** True while a run is active between StartRun and EndRun. */
    UPROPERTY()
    bool bRunActive = false;

    /** 1-indexed number of the level currently in play. */
    UPROPERTY()
    int32 CurrentLevel = 1;

    /** Active modifier effects for the current run — cached at StartRun. */
    UPROPERTY()
    FMXModifierEffects CurrentModifierEffects;

    /** Active tier effects for the current run — cached at StartRun. */
    UPROPERTY()
    FMXTierEffects CurrentTierEffects;

    /** GUIDs of all robots alive as of the last level tally. */
    UPROPERTY()
    TArray<FGuid> SurvivingRobotIds;

    /** Time at which the current level started (used for time bonus calculation). */
    UPROPERTY()
    FDateTime CurrentLevelStartTime;

    /** Number of levels completed within target time this run. */
    UPROPERTY()
    int32 LevelsUnderTargetTime = 0;

    // -------------------------------------------------------------------------
    // Persistence State
    // -------------------------------------------------------------------------

    /** Monotonically increasing total run count. Persisted across sessions. */
    UPROPERTY()
    int32 GlobalRunCount = 0;

    // -------------------------------------------------------------------------
    // Scoring
    // -------------------------------------------------------------------------

    /**
     * Calculates the final FMXScoreBreakdown and total score from accumulated run data.
     * Called during EndRun before the run data is finalised.
     * @param Outcome  The run outcome, used to determine which bonuses apply.
     * @return         Completed score breakdown struct.
     */
    FMXScoreBreakdown CalculateScore(ERunOutcome Outcome) const;

    // -------------------------------------------------------------------------
    // Hat Reward Distribution
    // -------------------------------------------------------------------------

    /**
     * Deposits hat rewards into the player's collection after a run ends.
     * Eligible if the run succeeded OR if the run failed after Level 5.
     * Delegates combo and unlock checks to the Hats module.
     */
    void DistributeHatRewards();

    // -------------------------------------------------------------------------
    // Event Bus Handlers
    // -------------------------------------------------------------------------

    /** Bound to EventBus->OnRobotDied. Increments loss counter and records event. */
    UFUNCTION()
    void HandleRobotDied(FGuid RobotId, FMXEventData DeathEvent);

    /** Bound to EventBus->OnRobotSacrificed. Records sacrifice (does NOT count as a loss). */
    UFUNCTION()
    void HandleRobotSacrificed(FGuid RobotId, FMXEventData SacrificeEvent);

    /** Bound to EventBus->OnRobotRescued. Increments rescued counter, awards witness XP. */
    UFUNCTION()
    void HandleRobotRescued(FGuid RobotId, int32 LevelNumber);

    /** Bound to EventBus->OnNearMiss. Awards near-miss XP to the affected robot. */
    UFUNCTION()
    void HandleNearMiss(FGuid RobotId, FMXObjectEventFields HazardFields);

    /** Bound to EventBus->OnHatEquipped. Tracks hatted robot count in run data. */
    UFUNCTION()
    void HandleHatEquipped(FGuid RobotId, int32 HatId);

    /** Bound to EventBus->OnHatLost. Tracks hats permanently lost in run data. */
    UFUNCTION()
    void HandleHatLost(FGuid RobotId, int32 HatId, FMXEventData DeathEvent);

    // -------------------------------------------------------------------------
    // Internal Helpers
    // -------------------------------------------------------------------------

    /** Wires all dynamic event bus delegates. Called at the end of Initialise. */
    void BindToEventBus();

    /** Unregisters all event bus delegates. Called at the start of EndRun. */
    void UnbindFromEventBus();

    /**
     * Checks whether the Legendary Escort modifier condition is still valid.
     * If the escort robot has died, calls EndRun(Failure).
     */
    void CheckEscortCondition(const FGuid& DiedRobotId);

    /**
     * Checks whether a Legendary permadeath condition has been triggered.
     * If so, calls EndRun(Failure) immediately.
     */
    void CheckPermadeathCondition();

    /**
     * Returns true if the elapsed time for the current level is under the per-level target.
     * Target = 5 minutes per level as per GDD.
     */
    bool IsCurrentLevelUnderTargetTime() const;
};
