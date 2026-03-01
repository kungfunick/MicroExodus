// MXBoidAI.h — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm
// Per-robot boid movement calculation component.
// Computes the combined force vector for each robot each tick using classic
// boid rules (separation, alignment, cohesion) plus player input and personality offsets.
//
// This component lives on the same actor as MXSwarmController and is called by it each tick.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXBoidAI.generated.h"

// ---------------------------------------------------------------------------
// FMXBoidConfig
// ---------------------------------------------------------------------------

/**
 * FMXBoidConfig
 * Configurable weights for the boid simulation.
 * Adjusted per swarm mode by MXSwarmController.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXBoidConfig
{
    GENERATED_BODY()

    /** How strongly robots steer away from nearby neighbors to avoid crowding. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SeparationWeight = 1.5f;

    /** How strongly robots align their velocity direction to neighbors. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AlignmentWeight = 1.0f;

    /** How strongly robots steer toward the group center of mass. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CohesionWeight = 1.2f;

    /** How strongly the player input vector is applied relative to boid forces. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PlayerInputWeight = 3.0f;

    /** Distance below which separation kicks in (Unreal units / cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SeparationRadius = 80.0f;

    /** Maximum distance at which neighbors influence alignment/cohesion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float NeighborRadius = 300.0f;

    /** Maximum speed the robot can reach (cm/s). Overridden per mode by SwarmController. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxSpeed = 400.0f;

    /** Max angular turn rate per second (degrees). Prevents robots from instantly reversing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxTurnRateDegrees = 180.0f;
};

// ---------------------------------------------------------------------------
// UMXBoidAI
// ---------------------------------------------------------------------------

/**
 * UMXBoidAI
 * ActorComponent that provides the boid force calculation for individual robots.
 * Called from MXSwarmController::TickGroup() for each active robot.
 *
 * This component does NOT move actors directly — it returns force vectors
 * that the SwarmController integrates into position each tick.
 */
UCLASS(ClassGroup=(MicroExodus), BlueprintType, Blueprintable,
       meta=(BlueprintSpawnableComponent))
class MICROEXODUS_API UMXBoidAI : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXBoidAI();

    // -------------------------------------------------------------------------
    // Core Boid Calculation
    // -------------------------------------------------------------------------

    /**
     * Calculate the combined boid force vector for a single robot this tick.
     * Combines separation, alignment, cohesion, player input, and personality offset.
     *
     * @param RobotId           The GUID of the robot being evaluated.
     * @param AllGroupPositions All neighbor positions in the same boid group (including self).
     * @param PlayerInput       Normalized 2D player input (X = right, Y = forward).
     * @return                  The combined force vector to apply this frame (not yet scaled by DeltaTime).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Boid")
    FVector CalculateBoidForces(const FGuid& RobotId,
                                const TArray<FVector>& AllGroupPositions,
                                FVector2D PlayerInput) const;

    // -------------------------------------------------------------------------
    // Personality Offsets
    // -------------------------------------------------------------------------

    /**
     * Register a quirk-to-movement mapping for a specific robot.
     * Called by SwarmController during InitializeSwarm and AddRobotToSwarm.
     * @param RobotId  The robot to configure.
     * @param Quirk    The personality quirk string from FMXRobotProfile.quirk.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Boid")
    void SetPersonalityOffset(const FGuid& RobotId, const FString& Quirk);

    /**
     * Return the cached personality offset for a robot.
     * Returns ZeroVector if no offset has been registered.
     * @param RobotId  The robot to query.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Boid")
    FVector GetPersonalityOffset(const FGuid& RobotId) const;

    /**
     * Clear all cached personality offsets. Called on level end.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Boid")
    void ClearAllOffsets();

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    /**
     * Apply a new boid configuration (called by SwarmController when mode changes).
     * @param NewConfig  The boid weights and limits to apply going forward.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Boid")
    void ApplyConfig(const FMXBoidConfig& NewConfig);

    /** Current boid configuration. Editable in editor for per-level tuning. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Boid")
    FMXBoidConfig Config;

    /**
     * Preset configs for each swarm mode. Applied automatically by SwarmController
     * when SetSwarmMode is called. Editable in editor for designer tuning.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Boid")
    FMXBoidConfig NormalConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Boid")
    FMXBoidConfig CarefulConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Boid")
    FMXBoidConfig SprintConfig;

private:

    // -------------------------------------------------------------------------
    // Per-Force Helpers
    // -------------------------------------------------------------------------

    /**
     * Separation force: steer away from neighbors within SeparationRadius.
     * Returns a repulsion vector pointing away from the crowded direction.
     */
    FVector CalculateSeparation(const FVector& MyPosition,
                                const TArray<FVector>& Neighbors) const;

    /**
     * Alignment force: steer to match the average heading of nearby neighbors.
     * Simplified: steers toward the average neighbor position relative to center.
     */
    FVector CalculateAlignment(const FVector& MyPosition,
                               const TArray<FVector>& Neighbors) const;

    /**
     * Cohesion force: steer toward the center of mass of nearby neighbors.
     */
    FVector CalculateCohesion(const FVector& MyPosition,
                              const FVector& GroupCenter) const;

    /** Map of RobotId -> personality movement offset. */
    TMap<FGuid, FVector> PersonalityOffsets;

    /**
     * Parse a quirk string and return the corresponding movement bias.
     * Positive X = forward bias, negative X = backward bias.
     */
    FVector QuirkToOffset(const FString& Quirk) const;
};
