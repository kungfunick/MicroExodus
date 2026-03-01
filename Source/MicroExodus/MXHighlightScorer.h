// MXHighlightScorer.h — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports
// Consumers: MXRunReportEngine
// Change Log:
//   v1.0 — Weighted event scoring, top-N highlight extraction

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXStatCompiler.h"
#include "MXRunData.h"
#include "MXEventData.h"
#include "MXHighlightScorer.generated.h"

/**
 * UMXHighlightScorer
 * Scores every event in the frozen event log using configurable weighted criteria,
 * then returns the top REPORT_MAX_HIGHLIGHTS events for the Highlight Reel.
 *
 * Criteria weights are exposed as UPROPERTY floats so QA can tune them without recompiling.
 */
UCLASS()
class UMXHighlightScorer : public UObject
{
    GENERATED_BODY()

public:
    // ---------------------------------------------------------------------------
    // Tunable Weights (QA-adjustable via CDO or config)
    // ---------------------------------------------------------------------------

    /** Bonus per 10 levels above level 20 (high-level robot involvement). */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_HighLevel      = 50.0f;

    /** Flat bonus for any hatted robot being the event subject. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_Hatted         = 30.0f;

    /** Per-additional-robot bonus for mass events. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_MultiRobot     = 20.0f;

    /** Bonus for Legendary or Epic hat involvement. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_RareHat        = 40.0f;

    /** Bonus for sacrifice events (highest single bonus). */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_Sacrifice      = 100.0f;

    /** Bonus for the very first or very last event of the run. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_FirstLast      = 60.0f;

    /** Bonus when a robot with a long survival streak dies. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_StreakBreaker  = 75.0f;

    /** Bonus for any event where a specialist ability was triggered (inferred from robot_spec != "None"). */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_SpecialistAbility = 25.0f;

    /** Bonus for Comedy-element events. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_Comedy         = 35.0f;

    /** Bonus if this event represents a run-wide record. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Weight_RecordBreaking = 50.0f;

    /** Base score for a Dramatic severity event. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Base_Dramatic         = 30.0f;

    /** Base score for a Major severity event. */
    UPROPERTY(EditAnywhere, Category = "Scoring")
    float Base_Major            = 10.0f;

    // ---------------------------------------------------------------------------
    // Public Interface
    // ---------------------------------------------------------------------------

    /**
     * Score every event, then return the top REPORT_MAX_HIGHLIGHTS sorted descending.
     * Narratives are left empty — filled by MXReportNarrator after selection.
     *
     * @param Events    Frozen event log from RunData.
     * @param Stats     Compiled run statistics (used for record checks and robot context).
     * @return          Array of FMXHighlight, sorted by score descending, capped at REPORT_MAX_HIGHLIGHTS.
     */
    TArray<FMXHighlight> ScoreEvents(
        const TArray<FMXEventData>& Events,
        const FMXCompiledStats& Stats
    );

    /**
     * Score a single event. Public for unit testing.
     *
     * @param Event     The event to score.
     * @param EventIdx  Index within the run's event array (for first/last detection).
     * @param TotalEvents Total number of events (for last-event detection).
     * @param Stats     Compiled run statistics.
     * @return          Float score — higher = more highlight-worthy.
     */
    float ScoreEvent(
        const FMXEventData& Event,
        int32 EventIdx,
        int32 TotalEvents,
        const FMXCompiledStats& Stats
    );

    /**
     * Determine if an event represents a run-wide or career record.
     * Currently checks: highest-level robot death, most near-misses on one robot.
     */
    bool IsRecordBreaking(
        const FMXEventData& Event,
        const FMXCompiledStats& Stats
    ) const;

private:
    /** Estimate robots simultaneously affected (rough — checks for mass casualty events). */
    int32 EstimateRobotsInEvent(
        const FMXEventData& Event,
        const FMXCompiledStats& Stats
    ) const;

    /** Check if event robot had a long survival streak (>3 runs survived). */
    bool HadLongStreak(
        const FMXEventData& Event,
        const FMXCompiledStats& Stats
    ) const;
};
