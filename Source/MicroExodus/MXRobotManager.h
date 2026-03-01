// MXRobotManager.h â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity
// Consumers: All modules (via IMXRobotProvider interface)
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXEventData.h"
#include "MXInterfaces.h"
#include "MXRobotManager.generated.h"

class UMXNameGenerator;
class UMXPersonalityGenerator;
class UMXLifeLog;
class UMXAgingSystem;

/**
 * UMXRobotManager
 * The authoritative owner of all robot profiles on the active roster.
 * Implements IMXRobotProvider so any module can query robot data without coupling to this class.
 * Implements IMXPersistable so the Persistence module can serialize/deserialize the full roster.
 * Registers with the event bus at startup to receive all relevant game events.
 *
 * Access pattern: Get this via the GameInstance subsystem, then cast to IMXRobotProvider.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXRobotManager : public UObject,
                                         public IMXRobotProvider,
                                         public IMXEventListener,
                                         public IMXPersistable
{
    GENERATED_BODY()

public:

    UMXRobotManager();

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Must be called once after construction (e.g., from GameInstance::Init).
     * Binds all event bus delegates and initializes sub-systems.
     * @param InNameGenerator       Shared name generator instance.
     * @param InPersonalityGen      Shared personality generator instance.
     * @param InLifeLog             Shared life log instance.
     * @param InAgingSystem         Shared aging system instance.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void Initialize(UMXNameGenerator* InNameGenerator,
                    UMXPersonalityGenerator* InPersonalityGen,
                    UMXLifeLog* InLifeLog,
                    UMXAgingSystem* InAgingSystem);

    // -------------------------------------------------------------------------
    // Robot Creation / Removal
    // -------------------------------------------------------------------------

    /**
     * Creates a new robot and adds it to the active roster.
     * Generates name, personality, appearance, birth timestamp.
     * Does nothing and returns an invalid GUID if roster is full.
     * @param LevelNumber   The level on which this robot was rescued (1â€“20).
     * @param RunNumber     The current run number (for life log context).
     * @return              The new robot's unique ID, or FGuid() if roster is full.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    FGuid CreateRobot(int32 LevelNumber, int32 RunNumber);

    /**
     * Removes a robot from the active roster (called on death or sacrifice).
     * Snapshots the profile to the dead robot archive before removal.
     * Releases the robot's name back to the name pool with a cooldown.
     * @param RobotId       GUID of the robot to remove.
     * @param CurrentRunNumber  Used for name cooldown tracking.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void RemoveRobot(const FGuid& RobotId, int32 CurrentRunNumber);

    // -------------------------------------------------------------------------
    // Stat Updates
    // -------------------------------------------------------------------------

    /**
     * Increments the element-specific encounter stat and near_misses on a robot's profile.
     * @param RobotId       Robot that had the near miss.
     * @param Element       Hazard element involved.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void RecordNearMiss(const FGuid& RobotId, EHazardElement Element);

    /**
     * Increments a social witness stat on all robots within RESCUE_WITNESS_RADIUS of an event location.
     * @param EventLocation     World location of the event.
     * @param EventType         The type of event witnessed (RescueWitnessed, DeathWitnessed, SacrificeWitnessed).
     * @param ExcludeId         Robot ID to exclude from witnessing (the primary robot of the event).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void RecordWitnessEvent(const FVector& EventLocation, EEventType EventType, const FGuid& ExcludeId);

    /**
     * Adds XP to a robot. If the total crosses an XP_THRESHOLDS boundary, triggers a level-up.
     * Fires OnLevelUp on the event bus when a level-up occurs.
     * @param RobotId   Target robot.
     * @param Amount    XP to add (before spec xp_modifier is applied).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void AddXP(const FGuid& RobotId, int32 Amount);

    /**
     * Updates the specialization fields on a robot's profile after a spec choice.
     * @param RobotId   Target robot.
     * @param Role      Chosen role.
     * @param Tier2     Chosen Tier 2 spec.
     * @param Tier3     Chosen Tier 3 spec.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void ApplySpecChoice(const FGuid& RobotId, ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3);

    /**
     * Evaluates all title unlock conditions for a robot and appends any newly earned titles.
     * Should be called after each run completes.
     * @param RobotId   Target robot.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void CheckTitles(const FGuid& RobotId);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /**
     * Returns true if the active roster has reached MAX_ROBOTS.
     * The Swarm module should check this before attempting to rescue a robot.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    bool IsRosterFull() const;

    /**
     * Returns the death/sacrifice snapshot of a robot for the Memorial Wall and Run Reports.
     * Returns a default-initialized struct if no snapshot exists.
     * @param RobotId   The dead robot's GUID.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    FMXRobotProfile GetDeadRobotSnapshot(const FGuid& RobotId) const;

    /**
     * Returns all death snapshots accumulated since the last save-load cycle.
     * Used by the Reports module to build the Death Roll and Memorial Wall.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    TArray<FMXRobotProfile> GetAllDeadRobotSnapshots() const;

    /**
     * Returns all robot profiles that were lost during the current run (for the run report).
     * Cleared when a new run starts.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    TArray<FMXRobotProfile> GetCurrentRunDeaths() const;

    /**
     * Clears the current-run death cache. Called by the Roguelike module at run start.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Identity")
    void ClearCurrentRunDeaths();

    // -------------------------------------------------------------------------
    // IMXRobotProvider Implementation
    // -------------------------------------------------------------------------

    /** Returns a copy of the profile for the given robot ID. Returns default struct if not found. */
    virtual FMXRobotProfile GetRobotProfile_Implementation(const FGuid& RobotId) const override;

    /** Returns copies of all active robot profiles on the roster. */
    virtual TArray<FMXRobotProfile> GetAllRobotProfiles_Implementation() const override;

    /** Returns the current number of active robots on the roster (0â€“MAX_ROBOTS). */
    virtual int32 GetRobotCount_Implementation() const override;

    // -------------------------------------------------------------------------
    // IMXEventListener Implementation
    // -------------------------------------------------------------------------

    /**
     * Receives all DEMS events. Routes them to the life log and updates stats accordingly.
     * @param Event     The fully resolved DEMS event.
     */
    virtual void OnDEMSEvent_Implementation(const FMXEventData& Event) override;

    // -------------------------------------------------------------------------
    // IMXPersistable Implementation
    // -------------------------------------------------------------------------

    /** Serializes the full roster and dead robot archive to the archive stream. */
    virtual void MXSerialize(FArchive& Ar) override;

    /** Deserializes the roster and dead robot archive from the archive stream. */
    virtual void MXDeserialize(FArchive& Ar) override;

private:

    // -------------------------------------------------------------------------
    // Internal Data
    // -------------------------------------------------------------------------

    /** Active roster: maps robot GUID â†’ full persistent profile. Never exceeds MAX_ROBOTS entries. */
    TMap<FGuid, FMXRobotProfile> ActiveRoster;

    /** Permanent archive of all robots that have died, keyed by their GUID. Used for the Memorial Wall. */
    TMap<FGuid, FMXRobotProfile> DeadRobotArchive;

    /** Snapshot cache of robots that died during the current run specifically. Cleared on run start. */
    TArray<FMXRobotProfile> CurrentRunDeaths;

    // -------------------------------------------------------------------------
    // Sub-System References
    // -------------------------------------------------------------------------

    /** Name generator â€” assigned by Initialize(). */
    UPROPERTY()
    UMXNameGenerator* NameGenerator = nullptr;

    /** Personality generator â€” assigned by Initialize(). */
    UPROPERTY()
    UMXPersonalityGenerator* PersonalityGenerator = nullptr;

    /** Life log manager â€” assigned by Initialize(). */
    UPROPERTY()
    UMXLifeLog* LifeLog = nullptr;

    /** Aging system â€” assigned by Initialize(). */
    UPROPERTY()
    UMXAgingSystem* AgingSystem = nullptr;

    // -------------------------------------------------------------------------
    // Internal Helpers
    // -------------------------------------------------------------------------

    /** Computes and returns the XP threshold for a given level (1-indexed). */
    int32 GetXPThresholdForLevel(int32 Level) const;

    /** Generates a random appearance for a new robot (chassis_color, eye_color, antenna_style, paint_job). */
    void GenerateAppearance(FMXRobotProfile& OutProfile) const;

    /** Increments the appropriate element-specific encounter counter on a profile. */
    void IncrementElementEncounter(FMXRobotProfile& Profile, EHazardElement Element) const;

    /**
     * Evaluates a single title condition for the given profile.
     * Adds the title string to earned_titles[] if the condition is met and not already earned.
     * @param Profile       Profile to evaluate.
     * @param Title         Title string to potentially award.
     * @param bCondition    Whether the condition for this title is currently met.
     */
    void TryAwardTitle(FMXRobotProfile& Profile, const FString& Title, bool bCondition) const;

    /** Event bus handlers â€” bound during Initialize(). */
    UFUNCTION()
    void HandleRobotRescued(FGuid RobotId, int32 LevelNumber);

    UFUNCTION()
    void HandleRobotDied(FGuid RobotId, FMXEventData DeathEvent);

    UFUNCTION()
    void HandleRobotSacrificed(FGuid RobotId, FMXEventData SacrificeEvent);

    UFUNCTION()
    void HandleNearMiss(FGuid RobotId, FMXObjectEventFields HazardFields);

    UFUNCTION()
    void HandleLevelComplete(int32 LevelNumber, const TArray<FGuid>& SurvivingRobots);

    UFUNCTION()
    void HandleRunComplete(FMXRunData RunData);

    UFUNCTION()
    void HandleRunFailed(FMXRunData RunData, int32 FailureLevel);

    UFUNCTION()
    void HandleHatEquipped(FGuid RobotId, int32 HatId);

    UFUNCTION()
    void HandleHatLost(FGuid RobotId, int32 HatId, FMXEventData DeathEvent);

    UFUNCTION()
    void HandleSpecChosen(FGuid RobotId, ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3);
};
