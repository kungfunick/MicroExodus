// MXAwardSelector.h — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports
// Consumers: MXRunReportEngine
// Change Log:
//   v1.0 — Per-category award evaluation with standout threshold enforcement

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXStatCompiler.h"
#include "MXRunData.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXAwardSelector.generated.h"

/**
 * UMXAwardSelector
 * Evaluates all 16 award categories against the compiled run stats.
 * Only includes an award if the winner is clearly a standout (>2× average).
 * Returns up to REPORT_MAX_AWARDS FMXAward entries (narratives filled later by Narrator).
 */
UCLASS()
class UMXAwardSelector : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Evaluate all 16 categories. Awards without a clear winner are skipped.
     *
     * @param Stats     Compiled run statistics.
     * @param Roster    All robot profiles active this run (for personality and hat data).
     * @return          Array of FMXAward (narrative field empty — filled by Narrator).
     */
    TArray<FMXAward> SelectAwards(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster
    );

    /**
     * Returns true if WinnerValue is meaningfully above average.
     * Threshold: winner must be ≥ StandoutMultiplier × average of all robots.
     */
    bool IsStandout(float WinnerValue, float Average) const;

    /** Multiplier above average required to earn an award. Default: 2.0. */
    UPROPERTY(EditAnywhere, Category = "Awards")
    float StandoutMultiplier = 2.0f;

private:
    // --- Per-category evaluators ---
    // Each returns a valid FMXAward if qualified, or an empty TOptional if skipped.

    TOptional<FMXAward> EvaluateMVP(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateNearMissChampion(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateRescueHero(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateIronHide(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateTourist(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateWorstLuck(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateFashionForward(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateTheSacrifice(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateNewcomer(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateVeteranSurvivor(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateBestDressed(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateWorstDressed(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateGuardianAngel(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateTheMechanic(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateHatCasualty(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    TOptional<FMXAward> EvaluateCoward(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& Roster) const;

    // --- Utility ---

    /** Find robot stat with max value of a given accessor. Returns nullptr if no robots. */
    const FMXRobotRunStats* FindMax(
        const FMXCompiledStats& Stats,
        TFunctionRef<float(const FMXRobotRunStats&)> Accessor) const;

    float ComputeAverage(
        const FMXCompiledStats& Stats,
        TFunctionRef<float(const FMXRobotRunStats&)> Accessor) const;

    /** Fetch the matching robot profile, or default. */
    const FMXRobotProfile* FindProfile(
        const FGuid& RobotId,
        const TArray<FMXRobotProfile>& Roster) const;

    /** Build a partial FMXAward from stats. Narrative is left empty. */
    FMXAward MakeAward(
        EAwardCategory Category,
        const FMXRobotRunStats& RS,
        const FString& StatValue) const;
};
