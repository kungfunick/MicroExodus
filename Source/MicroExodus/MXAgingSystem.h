// MXAgingSystem.h â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity
// Consumers: Identity (internal), Evolution (patina_intensity), UI (roster display)
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXAgingSystem.generated.h"

/**
 * UMXAgingSystem
 * Manages two distinct age concepts for each robot:
 *
 * 1. calendar_age â€” Real-world elapsed time since date_of_birth. Computed on-demand from
 *    the stored FDateTime. Drives patina_intensity in the visual evolution system.
 *    Never stored directly; always derived.
 *
 * 2. active_age_seconds â€” Accumulated in-mission deployment time. Incremented every frame
 *    while a robot is actively deployed in a level. Stored and persisted. Represents
 *    how "experienced" a robot feels in terms of time in the field.
 *
 * The AgingSystem holds active_age_seconds values in a runtime TMap.
 * The Persistence module serializes these values via the RobotManager (which stores
 * active_age_seconds on the FMXRobotProfile directly).
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXAgingSystem : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Calendar Age
    // -------------------------------------------------------------------------

    /**
     * Computes the real-world age of a robot based on its birth timestamp.
     * This is a pure function â€” no state is stored or modified.
     * @param DateOfBirth   The FDateTime when the robot was created.
     * @return              FTimespan representing elapsed real-world time.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    FTimespan ComputeCalendarAge(const FDateTime& DateOfBirth) const;

    /**
     * Returns a human-readable age string for UI display.
     * Examples: "3 hours old", "2 days old", "1 week old", "3 months old".
     * @param DateOfBirth   The robot's birth timestamp.
     * @return              Localized age description string.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    FString GetCalendarAgeDescription(const FDateTime& DateOfBirth) const;

    // -------------------------------------------------------------------------
    // Active Age
    // -------------------------------------------------------------------------

    /**
     * Registers a robot with the aging system so its active_age can be ticked.
     * Called when a robot is added to the active roster.
     * @param RobotId           Robot to register.
     * @param InitialSeconds    Starting active_age_seconds value (loaded from save, or 0 for new robots).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    void RegisterRobot(const FGuid& RobotId, float InitialSeconds = 0.0f);

    /**
     * Removes a robot from the tick registry (called on death or run end).
     * @param RobotId   Robot to deregister.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    void UnregisterRobot(const FGuid& RobotId);

    /**
     * Sets which robots are currently "active" (deployed in a level and being ticked).
     * Robots not in this set accumulate no active_age during that frame.
     * @param ActiveRobotIds    Set of robot GUIDs currently deployed in the level.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    void SetActiveDeployment(const TArray<FGuid>& ActiveRobotIds);

    /**
     * Increments active_age_seconds for all currently deployed robots.
     * Call this from the GameMode or a TickableWorldSubsystem each frame during a level.
     * @param DeltaTime     Frame delta time in seconds.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    void TickActiveAge(float DeltaTime);

    /**
     * Returns the current active_age_seconds for a robot.
     * Returns 0.0 if the robot is not registered.
     * @param RobotId   The robot to query.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    float GetActiveAgeSeconds(const FGuid& RobotId) const;

    /**
     * Returns a human-readable active deployment age string.
     * Examples: "42 seconds in the field", "7 minutes deployed", "2 hours of service".
     * @param RobotId   The robot to query.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    FString GetActiveAgeDescription(const FGuid& RobotId) const;

    /**
     * Flushes all active_age_seconds values back to the caller as a TMap,
     * so the RobotManager can write them to the robot profiles for persistence.
     * Call this at run end before saving.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Aging")
    TMap<FGuid, float> FlushActiveAges() const;

private:

    /** Maps robot GUID â†’ accumulated active_age_seconds. Runtime only; sourced from profiles on load. */
    TMap<FGuid, float> ActiveAgeMap;

    /** Set of robot IDs currently being ticked (deployed in-level). Updated by SetActiveDeployment(). */
    TSet<FGuid> DeployedRobotIds;
};
