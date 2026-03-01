// MXRescueMechanic.h — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm
// Manages the rescue interaction system for caged robots scattered through levels.
// Player holds the Rescue input near a cage for RESCUE_HOLD_TIME to free the robot.
// Rescued robots join the swarm immediately and trigger DEMS rescue events.
// Nearby robots witness the rescue and have their social stats incremented.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "MXEventMessageComponent.h"
#include "MXInterfaces.h"
#include "MXEvents.h"
#include "MXConstants.h"
#include "MXRescueMechanic.generated.h"

// ---------------------------------------------------------------------------
// FMXRescueCage
// ---------------------------------------------------------------------------

/**
 * FMXRescueCage
 * Runtime record for a single rescue cage in the current level.
 * Populated by MXRescueMechanic when the level loads (data comes from SpawnManager).
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRescueCage
{
    GENERATED_BODY()

    /** Unique identifier for this cage (not the same as the robot inside). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid CageId;

    /** GUID of the robot waiting inside this cage. Assigned by SpawnManager. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid CaptiveRobotId;

    /** World-space position of the cage. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector WorldPosition = FVector::ZeroVector;

    /** Whether this cage has already been rescued (robot freed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRescued = false;

    /**
     * Whether this cage has permanently locked (timed out in higher tiers).
     * Once locked, rescue is no longer possible.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bLocked = false;

    /**
     * Absolute game time (seconds) at which this cage locks forever.
     * 0 = no timeout (only applies on tiers with rescue timers).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LockTime = 0.0f;

    /** The actor reference for this cage in the level (used to call TriggerRescueEvent). */
    UPROPERTY()
    AActor* CageActor = nullptr;
};

// ---------------------------------------------------------------------------
// UMXRescueMechanic
// ---------------------------------------------------------------------------

/**
 * UMXRescueMechanic
 * ActorComponent attached to the player pawn or a dedicated RescueManager actor.
 * Manages all active cages in the current level, the player's rescue hold progress,
 * and witness event distribution after a successful rescue.
 *
 * Usage flow:
 *   1. Level loads → SpawnManager creates cages, registers them via RegisterCage().
 *   2. Player moves swarm near cage → swarm overlap triggers ShowRescuePrompt(CageId).
 *   3. Player holds Rescue input → each tick calls UpdateRescueHold(DeltaTime).
 *   4. Progress reaches 1.0 → CompleteRescue() fires, robot joins swarm.
 *   5. Player releases early → CancelRescue() resets progress.
 */
UCLASS(ClassGroup=(MicroExodus), BlueprintType, Blueprintable,
       meta=(BlueprintSpawnableComponent))
class MICROEXODUS_API UMXRescueMechanic : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXRescueMechanic();

protected:

    virtual void BeginPlay() override;

public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // -------------------------------------------------------------------------
    // Cage Registration (called by SpawnManager at level load)
    // -------------------------------------------------------------------------

    /**
     * Register a rescue cage in the current level.
     * Called by the Roguelike SpawnManager when cages are placed.
     * @param Cage  The populated cage record including position and captive robot.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    void RegisterCage(const FMXRescueCage& Cage);

    /**
     * Remove all cage registrations. Called at level end or run end.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    void ClearAllCages();

    /**
     * Return all currently registered cages (includes rescued and locked).
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Rescue")
    const TArray<FMXRescueCage>& GetAllCages() const { return RegisteredCages; }

    // -------------------------------------------------------------------------
    // Rescue Interaction
    // -------------------------------------------------------------------------

    /**
     * Begin a rescue hold on the specified cage.
     * Called when the player is within range of the cage and presses the Rescue input.
     * Does nothing if a rescue is already in progress or the cage is locked/rescued.
     * @param CageId  The GUID of the cage to rescue from.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    void BeginRescue(const FGuid& CageId);

    /**
     * Cancel the current rescue (player released input early or moved out of range).
     * Resets hold progress but does NOT reset the lock timer.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    void CancelRescue();

    /**
     * Advance the rescue hold progress for the current in-progress rescue.
     * Called from player input handling (not TickComponent — input binding driven).
     * Automatically calls CompleteRescue() when progress hits 1.0.
     * @param DeltaTime  Time since last frame (seconds).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    void UpdateRescueHold(float DeltaTime);

    /**
     * Force-complete the active rescue (called internally when hold timer finishes).
     * Returns the GUID of the rescued robot. Fires DEMS + EventBus events.
     * @param CageId  The cage being completed (must match ActiveCageId).
     * @return        The GUID of the newly freed robot. Invalid if rescue failed.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    FGuid CompleteRescue(const FGuid& CageId);

    // -------------------------------------------------------------------------
    // State Queries
    // -------------------------------------------------------------------------

    /**
     * Return the hold progress for the current active rescue (0.0–1.0).
     * Returns 0.0 if no rescue is in progress.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Rescue")
    float GetRescueProgress() const { return RescueProgress; }

    /**
     * Is a rescue currently in progress?
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Rescue")
    bool IsRescueInProgress() const { return bRescueInProgress; }

    /**
     * Return the CageId of the currently active rescue. Invalid if none in progress.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Rescue")
    FGuid GetActiveCageId() const { return ActiveCageId; }

    /**
     * Can the specified cage still be rescued? (Not already rescued, not locked.)
     * @param CageId  The cage to check.
     * @return        True if the cage is still available for rescue.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    bool IsRescueAvailable(const FGuid& CageId) const;

    /**
     * Return the remaining time before a timed cage locks forever (seconds).
     * Returns -1.0 if no timeout applies. Returns 0.0 if already locked.
     * @param CageId  The cage to query.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    float GetTimeUntilLock(const FGuid& CageId) const;

    /**
     * Return all cage IDs within rescue range of the given world position.
     * Used by the player pawn to determine which cages to show rescue UI for.
     * @param Position     World position to check from.
     * @param SearchRadius Maximum distance to search (cm).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Rescue")
    TArray<FGuid> GetCagesInRange(const FVector& Position, float SearchRadius) const;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * Radius around the rescue cage within which robots witness the rescue event.
     * Robots within this sphere have their rescues_witnessed stat incremented.
     * Default: MXConstants::RESCUE_WITNESS_RADIUS * 100 (100 = cm per m conversion).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Rescue")
    float WitnessRadius = 500.0f; // 5 meters in cm

    /**
     * Time in seconds the player must hold the rescue input to free a robot.
     * Matches MXConstants::RESCUE_HOLD_TIME.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Rescue")
    float HoldTimeRequired = MXConstants::RESCUE_HOLD_TIME;

    /**
     * Whether cage lock timers are active this level.
     * Set by RunManager based on current tier settings.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Rescue")
    bool bLockTimersActive = false;

    /**
     * Duration (seconds) before a cage locks permanently when bLockTimersActive is true.
     * Set per-level by RunManager.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Rescue")
    float CageLockDuration = 30.0f;

    /** Cached pointer to the SwarmController for adding rescued robots to the swarm. */
    UPROPERTY()
    class UMXSwarmController* SwarmController;

    /** Cached RobotProvider for looking up profiles during witness events. */
    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

private:

    /** All cages registered for the current level. */
    UPROPERTY()
    TArray<FMXRescueCage> RegisteredCages;

    /** Map from CageId to index in RegisteredCages for O(1) lookup. */
    TMap<FGuid, int32> CageIndexMap;

    /** Whether a rescue hold is currently in progress. */
    bool bRescueInProgress = false;

    /** The CageId being rescued. Invalid if not in progress. */
    FGuid ActiveCageId;

    /** Hold progress toward completion [0.0–1.0]. */
    float RescueProgress = 0.0f;

    /** Resolve and cache SwarmController and RobotProvider at BeginPlay. */
    void CacheDependencies();

    /**
     * Distribute RescueWitnessed events to all robots within WitnessRadius of the rescue cage.
     * Called immediately after CompleteRescue.
     * @param CagePosition  World position of the rescue event.
     * @param RescuedId     The robot that was just rescued.
     */
    void DistributeWitnessEvents(const FVector& CagePosition, const FGuid& RescuedId);

    /** Find a cage record by its GUID. Returns nullptr if not found. */
    FMXRescueCage* FindCage(const FGuid& CageId);
    const FMXRescueCage* FindCage(const FGuid& CageId) const;

    /** Get the EventBus from GameInstance subsystem. */
    UMXEventBus* GetEventBus() const;

    /** Current level number (cached from RunManager at BeginPlay). */
    int32 CachedLevelNumber = 1;
};
