// MXRunData.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: Roguelike, Reports, Persistence, UI
// Change Log:
//   v1.0 â€” Initial definition of all run and report structs

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXEventData.h"
#include "MXRobotProfile.h"
#include "MXRunData.generated.h"

// ---------------------------------------------------------------------------
// Score
// ---------------------------------------------------------------------------

/**
 * FMXScoreBreakdown
 * Itemized scoring components for a single run. Assembled by the Roguelike module, read by Reports and UI.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXScoreBreakdown
{
    GENERATED_BODY()

    /** Bonus points for each robot rescued from mid-level captivity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 rescue_bonus = 0;

    /** Bonus points for each robot that survived to the end of the run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 survival_bonus = 0;

    /** Point deduction for each robot lost during the run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 loss_penalty = 0;

    /** Bonus points awarded for completing levels within target time thresholds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 time_bonus = 0;

    /** Tier multiplier applied to the sum of all other components. Sourced from MXConstants tier_multipliers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float multiplier = 1.0f;

    /** Final computed score after all components and the multiplier are applied. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 final_score = 0;
};

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------

/**
 * FMXRunData
 * The complete data record for a single run from start to finish.
 * Written by the Roguelike module during play, finalized on run end, then handed to Reports and Persistence.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRunData
{
    GENERATED_BODY()

    /** Sequential run number (1-indexed) across the player's entire save history. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 run_number = 0;

    /** Difficulty tier this run was played on. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier tier = ETier::Standard;

    /** Human-readable modifier names active during this run (e.g., "Double Hazards", "No Rescues"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> active_modifiers;

    /** Wall-clock time when the run began. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime start_time;

    /** Wall-clock time when the run ended (success or failure). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime end_time;

    /** Whether the run ended in success or failure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERunOutcome outcome = ERunOutcome::Failure;

    /** Number of levels completed before the run ended. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 levels_completed = 0;

    /** Total robots deployed across all levels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 robots_deployed = 0;

    /** Robots that reached the end of the final level alive. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 robots_survived = 0;

    /** Robots that died during the run (not counting sacrifices separately). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 robots_lost = 0;

    /** Robots rescued from captivity within levels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 robots_rescued = 0;

    /** Final score before the run report is compiled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 score = 0;

    /** Itemized score components. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXScoreBreakdown score_breakdown;

    /** All DEMS events generated during this run, in chronological order. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXEventData> events;
};

// ---------------------------------------------------------------------------
// Report Sub-Structures
// ---------------------------------------------------------------------------

/**
 * FMXHatCensus
 * Aggregated hat statistics for the run. Compiled by the Reports module from run events and robot profiles.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXHatCensus
{
    GENERATED_BODY()

    /** Number of robots that entered any level wearing a hat during this run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 total_hatted = 0;

    /** Number of distinct hat types worn across the run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 unique_hats = 0;

    /** Display name of the most commonly worn hat this run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString most_common_hat;

    /** Display name of the rarest hat worn this run (by EHatRarity). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString rarest_hat;

    /** Number of hats permanently lost to robot deaths this run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 hats_lost = 0;

    /** Ratio of robots that entered with a hat and survived with it still on (0.0â€“1.0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float hat_survival_rate = 0.0f;
};

/**
 * FMXHighlight
 * A single notable event excerpt for the run report's Highlight Reel section.
 * Selected by HighlightScorer based on severity, uniqueness, and hat involvement.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXHighlight
{
    GENERATED_BODY()

    /** Numeric score used to rank this highlight against others. Higher = more likely to be featured. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float score = 0.0f;

    /** The underlying DEMS event this highlight is based on. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXEventData event;

    /** Expanded narrative context around the event, generated by ReportNarrator. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString narrative;
};

/**
 * FMXAward
 * A single award entry in the run report's Awards Ceremony section.
 * One robot per EAwardCategory. Some categories may go unawarded if no robot qualifies.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXAward
{
    GENERATED_BODY()

    /** The award category this entry represents. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EAwardCategory category = EAwardCategory::MVP;

    /** The robot receiving this award. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid robot_id;

    /** Cached display name at time of award. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString robot_name;

    /** Narrative sentence explaining why this robot won the award. Generated by ReportNarrator. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString narrative;

    /** The specific stat value that clinched this award (e.g., "7 near misses", "Level 42"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString stat_value;
};

/**
 * FMXDeathEntry
 * A single entry in the run report's Death Roll section.
 * One per robot that died during the run. Robots that were sacrificed appear in sacrifice_honor_roll instead.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXDeathEntry
{
    GENERATED_BODY()

    /** Full snapshot of the robot's profile at time of death. Used for the memorial wall. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXRobotProfile robot_profile_snapshot;

    /** The DEMS event that killed this robot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXEventData death_event;

    /** Short eulogy generated by ReportNarrator, referencing the robot's history and personality. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString eulogy;

    /** If the robot was wearing a hat when it died, a note about the hat's fate (e.g., "The Cowboy hat was lost forever"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString hat_lost_note;
};

/**
 * FMXSacrificeEntry
 * A single entry in the run report's Sacrifice Honor Roll section.
 * One per robot that chose to sacrifice itself to save others.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXSacrificeEntry
{
    GENERATED_BODY()

    /** Full snapshot of the robot's profile at the moment of sacrifice. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXRobotProfile robot_profile_snapshot;

    /** The DEMS event generated for the sacrifice moment. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXEventData sacrifice_event;

    /** Extended narrative honoring the sacrifice, longer than a standard eulogy. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString extended_narrative;

    /** Display names of all robots that survived because of this sacrifice. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> robots_saved;
};

// ---------------------------------------------------------------------------
// Full Run Report
// ---------------------------------------------------------------------------

/**
 * FMXRunReport
 * The complete post-run report package handed from the Reports module to Persistence (save) and UI (display).
 * Contains all sections of the cinematic run summary screen.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRunReport
{
    GENERATED_BODY()

    /** The underlying run data this report is built from. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXRunData run_data;

    /** Opening narrative paragraph generated by ReportNarrator. Sets the tone for the run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString opening_narrative;

    /** Hat census statistics for the report's hat section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXHatCensus hat_census;

    /** Top N highlights from the run, capped at REPORT_MAX_HIGHLIGHTS. Sorted by score descending. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXHighlight> highlights;

    /** Awards distributed to standout robots. Up to REPORT_MAX_AWARDS entries. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXAward> awards;

    /** Entries for each robot that died (non-sacrifice) during the run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXDeathEntry> death_roll;

    /** Entries for each robot that sacrificed itself during the run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXSacrificeEntry> sacrifice_honor_roll;

    /** Closing narrative paragraph. Reflects on the run outcome and teases the next run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString closing_narrative;
};
