// MXSwarmController.h — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm
// Central swarm movement controller. Manages all active robot positions/velocities,
// processes player input, applies boid rules via MXBoidAI, and tracks swarm state.
//
// This class is the top-level gameplay controller — the player interacts with the swarm
// through this class. It delegates per-robot boid calculations to MXBoidAI.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MXConstants.h"
#include "MXSpecData.h"
#include "MXInterfaces.h"
#include "MXSwarmController.generated.h"

// ---------------------------------------------------------------------------
// ESwarmMode
// ---------------------------------------------------------------------------

/** The current movement mode applied to the entire swarm. */
UENUM(BlueprintType)
enum class ESwarmMode : uint8
{
    Normal      UMETA(DisplayName = "Normal"),   // Standard speed, default formation
    Careful     UMETA(DisplayName = "Careful"),  // Slower, tighter cluster, less spread
    Sprint      UMETA(DisplayName = "Sprint"),   // Faster, looser formation
    Halt        UMETA(DisplayName = "Halt"),     // All robots stop in place
};

// ---------------------------------------------------------------------------
// FMXSwarmRobotState
// ---------------------------------------------------------------------------

/**
 * FMXSwarmRobotState
 * Per-robot runtime state tracked by the SwarmController each tick.
 * Not persisted — rebuilt each level from RobotProfiles.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXSwarmRobotState
{
    GENERATED_BODY()

    /** Unique robot identifier, matching the persistent FMXRobotProfile.robot_id. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RobotId;

    /** Current world-space position of this robot. Updated each tick. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position = FVector::ZeroVector;

    /** Current velocity vector (world-space, cm/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Velocity = FVector::ZeroVector;

    /** Index of the sub-group this robot belongs to after a Split. 0 = main group. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GroupIndex = 0;

    /** Whether this robot is currently selected by the Lasso tool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bLassoSelected = false;

    /** Whether this robot is active this level (not stranded, not dead). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bActive = true;

    /** Cached spec speed modifier from FMXSpecBonus. Refreshed at level start. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpecSpeedModifier = 1.0f;

    /** Personality-driven movement offset applied on top of boid forces (forward/backward bias). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector PersonalityOffset = FVector::ZeroVector;
};

// ---------------------------------------------------------------------------
// UMXSwarmController
// ---------------------------------------------------------------------------

/**
 * UMXSwarmController
 * Actor component (or standalone Actor) that owns all swarm movement logic.
 * Typically attached to the player pawn or a dedicated swarm manager actor.
 *
 * Workflow each tick:
 *   1. Receive player input via MoveSwarm().
 *   2. Delegate per-robot force calculation to MXBoidAI.
 *   3. Integrate velocities and update FMXSwarmRobotState.Position.
 *   4. Notify any registered robot Blueprint actors of their new target positions.
 */
UCLASS(ClassGroup=(MicroExodus), BlueprintType, Blueprintable)
class MICROEXODUS_API UMXSwarmController : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXSwarmController();

protected:

    virtual void BeginPlay() override;

public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // -------------------------------------------------------------------------
    // Player Input
    // -------------------------------------------------------------------------

    /**
     * Apply player directional input to the swarm this frame.
     * Combined with boid forces in CalculateBoidForces(). Called from the player pawn.
     * @param InputDirection  Normalized 2D input vector (X = right, Y = forward).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void MoveSwarm(FVector2D InputDirection);

    /**
     * Change the swarm's movement mode (Normal/Careful/Sprint/Halt).
     * Updates speed limits, cohesion weights, and formation tightness.
     * @param Mode  The new mode to apply to all active robots.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void SetSwarmMode(ESwarmMode Mode);

    /**
     * Recall all stranded or lagging robots back toward the swarm center.
     * Temporarily overrides their boid forces with a strong cohesion pull.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void RecallSwarm();

    /**
     * Split the swarm into two groups for puzzle solving.
     * GroupA and GroupB must be disjoint subsets of GetActiveRobots().
     * Any robot not in either list remains in group 0.
     * @param GroupA  First sub-group (assigned GroupIndex = 1).
     * @param GroupB  Second sub-group (assigned GroupIndex = 2).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void SplitSwarm(const TArray<FGuid>& GroupA, const TArray<FGuid>& GroupB);

    /**
     * Merge all split groups back into one cohesive swarm.
     * Resets all GroupIndex values to 0.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void MergeSwarm();

    /**
     * Select a subset of robots using a screen-space marquee (lasso) rectangle.
     * Converts 2D screen coords to world rays and collects robots within the frustum.
     * @param ScreenStart  Top-left corner of the lasso rectangle in screen pixels.
     * @param ScreenEnd    Bottom-right corner of the lasso rectangle in screen pixels.
     * @return             Array of robot GUIDs whose positions fall within the lasso.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    TArray<FGuid> LassoSelect(FVector2D ScreenStart, FVector2D ScreenEnd);

    // -------------------------------------------------------------------------
    // Swarm State Queries
    // -------------------------------------------------------------------------

    /**
     * Compute and return the average world-space position of all active robots.
     * @return  Swarm centroid. Returns ZeroVector if no active robots.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Swarm")
    FVector GetSwarmCenter() const;

    /**
     * Compute and return the maximum distance from the swarm center to the furthest active robot.
     * Used by the camera system and hazard exposure calculations.
     * @return  Swarm radius in Unreal units (cm). Returns 0 if fewer than 2 active robots.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Swarm")
    float GetSwarmRadius() const;

    /**
     * Return the count of currently active (alive, deployed) robots.
     * @return  Integer in range [0, MXConstants::MAX_ROBOTS].
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Swarm")
    int32 GetSwarmCount() const;

    /**
     * Return the GUIDs of all active robots in the swarm this level.
     * @return  Array of FGuid. Order is not guaranteed.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Swarm")
    TArray<FGuid> GetActiveRobots() const;

    /**
     * Return the full runtime state record for a specific robot.
     * @param RobotId  The GUID to look up.
     * @param OutState Out parameter populated with the found state.
     * @return         True if the robot was found in the active roster.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    bool GetRobotState(const FGuid& RobotId, FMXSwarmRobotState& OutState) const;

    /**
     * Return all robot state records (active and inactive).
     * Useful for hazard systems iterating over nearby robots.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Swarm")
    const TArray<FMXSwarmRobotState>& GetAllRobotStates() const { return RobotStates; }

    // -------------------------------------------------------------------------
    // Swarm Lifecycle
    // -------------------------------------------------------------------------

    /**
     * Initialize the swarm roster at the start of a level.
     * Queries the RobotProvider for the active roster and caches spec bonuses.
     * @param InitialPositions  Map of RobotId -> world spawn position for this level.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void InitializeSwarm(const TMap<FGuid, FVector>& InitialPositions);

    /**
     * Mark a robot as dead/removed. It is removed from the active set immediately.
     * Called by hazard collision code after firing OnRobotDied.
     * @param RobotId  The GUID of the robot to remove from the swarm.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void RemoveRobotFromSwarm(const FGuid& RobotId);

    /**
     * Add a newly rescued robot to the active swarm.
     * Called by MXRescueMechanic after completing a rescue.
     * @param RobotId        The GUID of the robot to add.
     * @param SpawnPosition  World position to place the robot at entry.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Swarm")
    void AddRobotToSwarm(const FGuid& RobotId, const FVector& SpawnPosition);

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** Base movement speed of the swarm in Normal mode (cm/s). Modified by tier and specs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Swarm|Config")
    float BaseMovementSpeed = 300.0f;

    /** Speed multiplier applied in Careful mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Swarm|Config")
    float CarefulSpeedMultiplier = 0.5f;

    /** Speed multiplier applied in Sprint mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Swarm|Config")
    float SprintSpeedMultiplier = 1.75f;

    /** In Careful mode, robots try to stay within this radius of the swarm center (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Swarm|Config")
    float CarefulFormationRadius = 150.0f;

    /** Distance threshold below which a robot is considered a "straggler" and recalled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Swarm|Config")
    float StraggleDistanceThreshold = 600.0f;

    /** Strength of the recall force applied to stragglers during RecallSwarm(). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Swarm|Config")
    float RecallForceStrength = 5.0f;

    /** Pointer to the RobotProvider interface for querying robot profiles. Resolved at BeginPlay. */
    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

private:

    /** Runtime state for all robots, indexed by position in the array. */
    UPROPERTY()
    TArray<FMXSwarmRobotState> RobotStates;

    /** Map from RobotId to index in RobotStates for O(1) lookups. */
    TMap<FGuid, int32> RobotIndexMap;

    /** Current swarm movement mode. */
    ESwarmMode CurrentMode = ESwarmMode::Normal;

    /** Last player input direction, cached for boid calculations this tick. */
    FVector2D CurrentInputDirection = FVector2D::ZeroVector;

    /** Whether RecallSwarm() is currently active. Cleared after all robots are near center. */
    bool bRecallActive = false;

    /** Effective base speed for the current mode (BaseMovementSpeed * mode multiplier). */
    float EffectiveSpeed = 300.0f;

    /** Compute the effective speed for a given mode. */
    float GetSpeedForMode(ESwarmMode Mode) const;

    /** Resolve and cache the RobotProvider from GameInstance at BeginPlay. */
    void CacheRobotProvider();

    /** Refresh SpecSpeedModifier and PersonalityOffset for all robots. */
    void RefreshSpecAndPersonalityOffsets();

    /** Tick update for a single sub-group. GroupIndex 0 = all robots if not split. */
    void TickGroup(int32 GroupIndex, float DeltaTime);
};
