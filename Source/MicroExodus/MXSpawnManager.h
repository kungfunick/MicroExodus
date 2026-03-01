// MXSpawnManager.h — Roguelike Module v1.0
// Agent 6: Roguelike
// Calculates and manages rescue robot spawn budgets per level.
// Spawns rescue robots via the Identity module's CreateRobot interface.

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXModifierRegistry.h"
#include "MXSpawnManager.generated.h"

// Forward declarations
class IMXRobotProvider;

// ---------------------------------------------------------------------------
// UMXSpawnManager
// ---------------------------------------------------------------------------

/**
 * UMXSpawnManager
 * Determines how many rescue robots spawn on each level based on tier and modifiers,
 * then places them in the world by calling into the Identity module.
 *
 * Spawn budget rules (from GDD):
 *   Levels  1–5  : 2–3 rescues
 *   Levels  6–10 : 3–4 rescues
 *   Levels 11–15 : 2–3 rescues
 *   Levels 16–20 : 1–2 rescues
 *
 * Higher tiers reduce counts:
 *   Tier 3 (Nightmare)  : halve spawn count
 *   Tier 4 (Extinction) : quarter spawn count
 *   Tier 5 (Legendary)  : 0–1 per level
 *
 * Iron Swarm modifier forces spawn count to 0.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXSpawnManager : public UObject
{
    GENERATED_BODY()

public:

    UMXSpawnManager();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Supplies the Robot Provider reference used to call CreateRobot on the Identity module.
     * Must be called before SpawnRescueRobots.
     * @param InRobotProvider  The Identity module's RobotManager (implements IMXRobotProvider).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Spawning")
    void SetRobotProvider(TScriptInterface<IMXRobotProvider> InRobotProvider);

    // -------------------------------------------------------------------------
    // Spawn Budget Calculation
    // -------------------------------------------------------------------------

    /**
     * Calculates how many rescue robots should spawn on the given level.
     * Returns 0 if Iron Swarm is active.
     * @param LevelNumber  1-indexed level in the run (1–20).
     * @param Tier         Current run difficulty tier.
     * @param Modifiers    Combined modifier effects for this run.
     * @return             Number of rescue robots to place.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Spawning")
    int32 GetRescueSpawnCount(int32 LevelNumber, ETier Tier, const FMXModifierEffects& Modifiers) const;

    /**
     * Returns candidate world positions for rescue robot placement on a given level.
     * Positions are defined per-level and capped to the requested Count.
     * The actual positions are populated by level Blueprints — this returns a stub
     * set of placeholder vectors that Blueprints override at runtime.
     * @param LevelNumber  1-indexed level number.
     * @param Count        Number of positions requested.
     * @return             Array of world-space FVector positions.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Spawning")
    TArray<FVector> GetSpawnPositions(int32 LevelNumber, int32 Count) const;

    // -------------------------------------------------------------------------
    // Spawning
    // -------------------------------------------------------------------------

    /**
     * Spawns a single rescue robot at the given world position.
     * Calls into the Identity module to create the robot profile, then notifies
     * the event bus with OnRobotRescued so DEMS and camera react.
     * @param Position     World-space location to place the rescued robot actor.
     * @param LevelNumber  Current level, forwarded to the rescue event.
     * @return             The GUID of the newly created robot, or an invalid GUID on failure.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Spawning")
    FGuid SpawnRescueRobot(FVector Position, int32 LevelNumber);

    /**
     * Runs the full spawn sequence for a level: calculates budget, fetches positions,
     * and spawns the appropriate number of rescue robots.
     * @param LevelNumber  Current level number.
     * @param Tier         Current run difficulty tier.
     * @param Modifiers    Combined modifier effects for this run.
     * @return             Array of GUIDs for all robots spawned.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Spawning")
    TArray<FGuid> SpawnRescueRobotsForLevel(int32 LevelNumber, ETier Tier, const FMXModifierEffects& Modifiers);

    // -------------------------------------------------------------------------
    // Diagnostics
    // -------------------------------------------------------------------------

    /**
     * Returns the total number of rescue robots spawned during the current run.
     * Resets to 0 when MXRunManager calls ResetForNewRun.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Spawning")
    int32 GetTotalSpawnedThisRun() const;

    /**
     * Resets the spawn counter for a new run. Called by MXRunManager at StartRun.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Spawning")
    void ResetForNewRun();

private:

    /** Reference to the Identity module's robot manager for robot creation. */
    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

    /** Total rescue robots spawned since the last ResetForNewRun call. */
    UPROPERTY()
    int32 TotalSpawnedThisRun = 0;

    /**
     * Returns the base (pre-tier-reduction) spawn range [Min, Max] for a given level band.
     * @param LevelNumber  1-indexed level.
     * @param OutMin       Minimum rescues for this band.
     * @param OutMax       Maximum rescues for this band.
     */
    void GetBaseBudgetRange(int32 LevelNumber, int32& OutMin, int32& OutMax) const;

    /**
     * Applies tier-based spawn reduction to a raw count.
     * @param BaseCount  Pre-reduction count.
     * @param Tier       Active difficulty tier.
     * @return           Reduced spawn count.
     */
    int32 ApplyTierReduction(int32 BaseCount, ETier Tier) const;
};
