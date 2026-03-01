// MXXPDistributor.h — Roguelike Module v1.0
// Agent 6: Roguelike
// Calculates and distributes XP to robots after level completions, run completions,
// and in-level events (near-miss, puzzle, rescue witness, sacrifice witness).
// Loads base XP values from DT_XPCurve.csv.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MXTypes.h"
#include "MXConstants.h"
#include "MXXPDistributor.generated.h"

// Forward declarations
class IMXRobotProvider;

// ---------------------------------------------------------------------------
// FMXXPCurveRow — DataTable row struct for DT_XPCurve.csv
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMXXPCurveRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Event type string matching EEventType (e.g., "LevelComplete", "NearMiss"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString EventType;

    /** Base XP awarded before tier multiplier is applied. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BaseXP = 0;

    /** Human-readable description for tooling and documentation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;
};

// ---------------------------------------------------------------------------
// UMXXPDistributor
// ---------------------------------------------------------------------------

/**
 * UMXXPDistributor
 * Calculates XP amounts from DT_XPCurve and dispatches AddXP calls to the
 * Identity module for each qualifying robot.
 *
 * XP formula: FinalXP = BaseXP × TierMultiplier × SpecialistMultiplier
 * The specialist multiplier (if any) is read from the robot's profile via IMXRobotProvider.
 *
 * This class does NOT directly track level-up thresholds — it calls AddXP on the
 * Identity module which owns leveling logic internally.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXXPDistributor : public UObject
{
    GENERATED_BODY()

public:

    UMXXPDistributor();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Loads XP values from the provided DT_XPCurve DataTable asset.
     * Must be called once at game startup before any XP distribution.
     * @param DataTable  The DT_XPCurve asset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    void InitialiseFromDataTable(UDataTable* DataTable);

    /**
     * Sets the Robot Provider reference used to query robot profiles and call AddXP.
     * @param InRobotProvider  The Identity module's RobotManager.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    void SetRobotProvider(TScriptInterface<IMXRobotProvider> InRobotProvider);

    // -------------------------------------------------------------------------
    // Batch XP Distribution
    // -------------------------------------------------------------------------

    /**
     * Awards level-survival XP to every robot in the surviving list.
     * XP = BaseXP(LevelComplete) × TierMultiplier.
     * @param SurvivingRobots  GUIDs of all robots alive at level end.
     * @param LevelNumber      The level just completed (1–20), for logging.
     * @param Tier             Active difficulty tier for multiplier lookup.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    void DistributeLevelXP(const TArray<FGuid>& SurvivingRobots, int32 LevelNumber, ETier Tier);

    /**
     * Awards run-completion bonus XP to every surviving robot.
     * XP = BaseXP(RunComplete) × TierMultiplier.
     * Called only on successful run completion (all 20 levels cleared).
     * @param SurvivingRobots  GUIDs of robots alive at run end.
     * @param Tier             Active difficulty tier.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    void DistributeRunXP(const TArray<FGuid>& SurvivingRobots, ETier Tier);

    // -------------------------------------------------------------------------
    // Per-Event XP
    // -------------------------------------------------------------------------

    /**
     * Awards XP to a specific robot for a single in-run event.
     * Applies tier multiplier and optional spec multiplier from the robot's profile.
     * @param RobotId    The GUID of the robot receiving XP.
     * @param EventType  The event category (must match a row in DT_XPCurve).
     * @param Tier       Active difficulty tier for multiplier lookup.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    void AwardEventXP(const FGuid& RobotId, EEventType EventType, ETier Tier);

    // -------------------------------------------------------------------------
    // XP Queries
    // -------------------------------------------------------------------------

    /**
     * Returns the final XP amount (after tier multiplier) for a given event type.
     * Does NOT include the specialist multiplier — use AwardEventXP to award with full bonuses.
     * @param EventType  The event category.
     * @param Tier       Active difficulty tier.
     * @return           Calculated XP integer (rounded down).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    int32 GetXPAmount(EEventType EventType, ETier Tier) const;

    /**
     * Returns the base XP for an event type before any multipliers are applied.
     * Returns 0 if the event type is not found in the DataTable.
     * @param EventType  The event category.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    int32 GetBaseXP(EEventType EventType) const;

    /**
     * Returns the total XP distributed by this distributor since the last ResetForNewRun.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    int32 GetTotalXPDistributedThisRun() const;

    /**
     * Resets the accumulated XP counter. Called by MXRunManager at StartRun.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|XP")
    void ResetForNewRun();

private:

    /** XP values keyed by EEventType display name string (e.g., "LevelComplete"). */
    UPROPERTY()
    TMap<FString, int32> XPTable;

    /** Reference to the Identity module for profile queries and AddXP calls. */
    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

    /** Accumulated XP distributed this run for diagnostics. */
    UPROPERTY()
    int32 TotalXPDistributedThisRun = 0;

    /**
     * Converts an EEventType enum value to its string key for DataTable lookup.
     * Matches the EventType column values in DT_XPCurve.csv.
     */
    static FString EventTypeToString(EEventType EventType);

    /**
     * Returns the XP multiplier modifier for the given robot based on its specialization.
     * Reads the robot profile and returns 1.0 if the robot has no relevant spec bonus,
     * or a higher value for Engineer/Mechanic robots that boost XP gain.
     * @param RobotId  The robot to query.
     */
    float GetSpecialistXPMultiplier(const FGuid& RobotId) const;

    /**
     * Internal helper that calls AddXP on the Identity module for a robot.
     * Logs the award and accumulates TotalXPDistributedThisRun.
     * @param RobotId  Target robot.
     * @param XPAmount Final XP amount to add.
     * @param Context  Human-readable label for logging (e.g., "LevelComplete").
     */
    void ApplyXPToRobot(const FGuid& RobotId, int32 XPAmount, const FString& Context);
};
