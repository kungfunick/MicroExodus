// MXHatUnlockChecker.h — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Consumers: MXHatManager (internal — called at run end and level end)
// Change Log:
//   v1.0 — Initial implementation: Standard, Hidden, Level-Specific, Legendary unlock evaluation

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MXHatData.h"
#include "MXRunData.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXHatUnlockChecker.generated.h"

/**
 * UMXHatUnlockChecker
 * Evaluates unlock conditions for all four hat category types.
 *
 * Called by MXHatManager at specific game boundary events:
 *   - CheckStandardUnlocks   : at run end (Success or Failure)
 *   - CheckHiddenUnlocks     : at run end (Success or Failure)
 *   - CheckLevelSpecificUnlocks : at level completion
 *   - CheckLegendaryUnlocks  : at run end (Success only, typically)
 *
 * Returns arrays of hat IDs that have newly satisfied their unlock conditions.
 * The caller (HatManager) is responsible for calling UnlockHat on each result.
 *
 * Hat ID ranges:
 *   Standard      #1–30
 *   Hidden        #31–50
 *   Level-Specific #71–90
 *   Legendary     #91–100
 */
UCLASS()
class MICROEXODUS_API UMXHatUnlockChecker : public UObject
{
    GENERATED_BODY()

public:

    /**
     * Initialize with a robot provider for hidden unlock condition checking.
     * @param InRobotProviderObject  UObject implementing IMXRobotProvider.
     */
    void Initialize(UObject* InRobotProviderObject);

    // -------------------------------------------------------------------------
    // Standard Unlocks (#1–30)
    // -------------------------------------------------------------------------

    /**
     * Check visible milestone-based unlock conditions.
     * Conditions are evaluated from run statistics and robot profiles.
     * Returns hat IDs newly satisfying their conditions.
     *
     * Example conditions:
     *   #1  Cap — Starter hat, always unlocked at first run.
     *   #2  Top Hat — Complete 5 runs.
     *   #3  Hard Hat — Reach Level 10 in any run.
     *   #4  Cowboy Hat — Deploy 50 total robots across all runs.
     *   #5  Beret — Have a robot survive 10 consecutive runs.
     *   ...etc.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Unlocks")
    TArray<int32> CheckStandardUnlocks(
        const FMXRunData& RunData,
        const TArray<FMXRobotProfile>& AllProfiles);

    // -------------------------------------------------------------------------
    // Hidden Unlocks (#31–50)
    // -------------------------------------------------------------------------

    /**
     * Check secret unlock conditions (not displayed to player until discovered).
     * Evaluates conditions against run stats, robot profiles, and internal counters.
     * Returns hat IDs newly satisfying their conditions.
     *
     * Example conditions:
     *   #31 Tinfoil Hat — A robot witnesses 25+ deaths without dying.
     *   #32 Traffic Cone — Complete a run in under 30 minutes.
     *   #33 Dunce Cap — Lose 50+ robots in a single run.
     *   #34 Crown of Thorns — Have a robot survive while in a team of 3 or fewer.
     *   #35 Propeller Hat — A robot survives 5 near-misses in one run.
     *   ...etc.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Unlocks")
    TArray<int32> CheckHiddenUnlocks(
        const FMXRunData& RunData,
        const TArray<FMXRobotProfile>& AllProfiles);

    // -------------------------------------------------------------------------
    // Level-Specific Unlocks (#71–90)
    // -------------------------------------------------------------------------

    /**
     * Check level-specific hat conditions at level completion.
     * For each level-specific hat in #71–90, checks if any surviving robot
     * wore the prerequisite hat on the required level.
     *
     * @param LevelNumber     The level just completed (1–20).
     * @param SurvivingRobots GUIDs of robots that survived this level.
     * @param AllProfiles     Current robot profiles (for current_hat_id lookup).
     * @param HatDefinitions  Static hat definition map.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Unlocks")
    TArray<int32> CheckLevelSpecificUnlocks(
        int32 LevelNumber,
        const TArray<FGuid>& SurvivingRobots,
        const TArray<FMXRobotProfile>& AllProfiles,
        const TMap<int32, FMXHatDefinition>& HatDefinitions);

    // -------------------------------------------------------------------------
    // Legendary Unlocks (#91–100)
    // -------------------------------------------------------------------------

    /**
     * Check long-term legendary hat unlock conditions.
     * These require major cumulative achievements across many runs.
     * Returns hat IDs newly satisfying their conditions.
     *
     * Example conditions:
     *   #91 Halo — Have 10 robots each reach Level 50.
     *   #92 Crown of All — Unlock all 90 non-legendary hats.
     *   #93 Iron Helmet — Complete a run losing 0 robots on Nightmare or higher.
     *   #94 Jester Crown — Reach Level 20 on Standard tier with 100 robots deployed.
     *   #95 Angel Wings Hat — Have a robot witness 100 sacrifices lifetime.
     *   #96 Ghost Hood — Complete a run with no hats equipped at all.
     *   #97 Champion's Laurel — Win 50 total runs.
     *   #98 Mourning Veil — Lose 1000 total robots across all runs.
     *   #99 Infinity Circlet — Have all 100 robots wearing different hats in one run.
     *   #100 The Exodus Crown — Complete every other hat unlock (100% collection minus this).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Unlocks")
    TArray<int32> CheckLegendaryUnlocks(
        const FMXRunData& RunData,
        const FMXHatCollection& Collection);

private:

    // -------------------------------------------------------------------------
    // Internal state counters (accumulated across calls — session-persistent)
    // -------------------------------------------------------------------------

    /** Total robots deployed across all runs (session counter for standard unlocks). */
    int32 CumulativeRobotsDeployed = 0;

    /** Total runs completed (success). */
    int32 TotalRunsCompleted = 0;

    /** Total runs attempted (success + failure). */
    int32 TotalRunsAttempted = 0;

    /** Total robots lost across all runs. */
    int32 TotalRobotsLost = 0;

    /** Set of hats already checked/awarded (prevents duplicate fires across calls). */
    TSet<int32> AlreadyUnlocked;

    /** Cached robot provider for hidden unlock checks. */
    UPROPERTY()
    TObjectPtr<UObject> RobotProviderObject;

    // -------------------------------------------------------------------------
    // Helper Utilities
    // -------------------------------------------------------------------------

    /** Returns true if HatId is not yet in AlreadyUnlocked. Adds it if checking. */
    bool CanUnlock(int32 HatId) const;

    /** Mark HatId as unlocked in internal state. */
    void MarkUnlocked(int32 HatId);

    /** Get the maximum consecutive run streak across all profiles. */
    static int32 GetMaxRunStreak(const TArray<FMXRobotProfile>& AllProfiles);

    /** Get the max deaths_witnessed value across all profiles. */
    static int32 GetMaxDeathsWitnessed(const TArray<FMXRobotProfile>& AllProfiles);

    /** Count how many profiles have level >= MinLevel. */
    static int32 CountProfilesAtLevel(const TArray<FMXRobotProfile>& AllProfiles, int32 MinLevel);

    /** Count distinct hat IDs across all profiles' hat_history. */
    static int32 CountDistinctHatsEverWorn(const TArray<FMXRobotProfile>& AllProfiles);

    // Level-specific hat ranges
    static constexpr int32 LEVEL_SPECIFIC_HAT_MIN = 71;
    static constexpr int32 LEVEL_SPECIFIC_HAT_MAX = 90;
};
