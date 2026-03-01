// MXReportNarrator.h — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports
// Consumers: MXRunReportEngine
// Change Log:
//   v1.0 — Template-based narrative generation for all report sections

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "MXStatCompiler.h"
#include "MXRunData.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXReportNarrator.generated.h"

// ---------------------------------------------------------------------------
// DataTable Row Structs
// ---------------------------------------------------------------------------

/**
 * FMXOpeningTemplateRow
 * Row structure for DT_ReportOpeningTemplates DataTable.
 */
USTRUCT(BlueprintType)
struct FMXOpeningTemplateRow : public FTableRowBase
{
    GENERATED_BODY()

    /** The narrative template text. Tokens: {run_number}, {deployed}, {survived}, {rescued},
     *  {hat_percent}, {sacrifice_count}, {failure_level}. */
    UPROPERTY(EditAnywhere) FString TemplateText;

    /** Run condition this template matches. Values: high_survival, bloodbath, perfect, failed,
     *  hat_heavy, sacrifice_heavy, any. */
    UPROPERTY(EditAnywhere) FString Condition;

    /** Minimum severity context for template eligibility. */
    UPROPERTY(EditAnywhere) FString MinSeverity;
};

/**
 * FMXHighlightTemplateRow
 * Row structure for DT_ReportHighlightTemplates DataTable.
 */
USTRUCT(BlueprintType)
struct FMXHighlightTemplateRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Template text. Tokens: {name}, {robot_level}, {hat}, {spec}, {verb}, {source},
     *  {level_num}, {near_miss_count}, {death_count}, {spec_action}, {idle_levels},
     *  {record_type}, {record_value}. */
    UPROPERTY(EditAnywhere) FString TemplateText;

    /** Event type this template covers: dramatic_death, near_miss_champion, mass_casualty,
     *  sacrifice, specialist_clutch, hat_comedy, lazy_robot, record_breaker, rescue, near_miss, any. */
    UPROPERTY(EditAnywhere) FString EventType;

    /** Whether this template requires the robot to be wearing a hat. */
    UPROPERTY(EditAnywhere) bool bRequiresHat = false;

    /** Whether this template requires the robot to have a specialization. */
    UPROPERTY(EditAnywhere) bool bRequiresSpec = false;
};

/**
 * FMXAwardTemplateRow
 * Row structure for DT_AwardTemplates DataTable.
 */
USTRUCT(BlueprintType)
struct FMXAwardTemplateRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Template text. Tokens: {name}, {spec}, {stat_summary}, {count}, {idle_time},
     *  {idle_levels}, {hat}, {rarity}, {verb}, {level_num}, {remaining}. */
    UPROPERTY(EditAnywhere) FString TemplateText;

    /** Award category name this template serves. Matches EAwardCategory display name. */
    UPROPERTY(EditAnywhere) FString AwardCategory;
};

/**
 * FMXEulogyTemplateRow
 * Row structure for DT_EulogyTemplates DataTable.
 */
USTRUCT(BlueprintType)
struct FMXEulogyTemplateRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Template text. Tokens: {name}, {spec}, {robot_level}, {runs_survived}, {near_misses},
     *  {active_age}, {hat}, {quirk}, {like_text}. */
    UPROPERTY(EditAnywhere) FString TemplateText;

    /** Level bracket: low_level (<5), mid_level (5-20), veteran (20+), hatted, sacrifice. */
    UPROPERTY(EditAnywhere) FString LevelBracket;
};

/**
 * FMXClosingTemplateRow
 * Row structure for DT_ReportClosingTemplates DataTable.
 */
USTRUCT(BlueprintType)
struct FMXClosingTemplateRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Template text. Tokens: {survived}, {lost}, {deployed}, {levels_completed},
     *  {lost_all}, {next_run}, {surviving}, {new_hats}, {collection_count}. */
    UPROPERTY(EditAnywhere) FString TemplateText;

    /** Run condition: success_low_loss, success_heavy_loss, perfect, failed, first_run, hat_milestone, any. */
    UPROPERTY(EditAnywhere) FString Condition;
};

// ---------------------------------------------------------------------------
// MXReportNarrator
// ---------------------------------------------------------------------------

/**
 * UMXReportNarrator
 * Generates all narrative text for the run report.
 * Uses DataTable templates with token substitution and variety rolls to avoid repetition.
 *
 * Voice: dry, sardonic factory systems AI — stat-nerd precision, affectionate sarcasm,
 * genuine pathos for deaths and sacrifices.
 */
UCLASS()
class UMXReportNarrator : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Load all report template DataTables. Must be called before any generation.
     * Called by MXRunReportEngine::Initialize().
     */
    void Initialize(
        UDataTable* OpeningTemplates,
        UDataTable* HighlightTemplates,
        UDataTable* AwardTemplates,
        UDataTable* EulogyTemplates,
        UDataTable* ClosingTemplates
    );

    // --- Section Generators ---

    /**
     * Generate the opening 2–3 sentence narrative.
     * Selects from opening template pool based on run condition flags in Stats.
     */
    FString GenerateOpeningNarrative(
        const FMXRunData& RunData,
        const FMXCompiledStats& Stats
    );

    /**
     * Expand a scored highlight event into a narrated paragraph.
     * Template chosen based on event_type and robot state.
     */
    FString GenerateHighlightNarrative(
        const FMXHighlight& Highlight,
        const FMXCompiledStats& Stats
    );

    /**
     * Generate an award justification sentence.
     * Template chosen by award category.
     */
    FString GenerateAwardNarrative(
        const FMXAward& Award,
        const FMXRobotProfile& Profile,
        const FMXRobotRunStats& RobotStats
    );

    /**
     * Generate a level-appropriate eulogy for a dead robot.
     * Level bracket: <5 = new, 5-20 = growing, 20+ = veteran. Hatted and sacrifice variants override.
     */
    FString GenerateEulogy(
        const FMXRobotProfile& Profile,
        const FMXEventData& DeathEvent
    );

    /**
     * Generate the extended sacrifice narrative.
     * Longer than a eulogy — lists who was saved and the cost paid.
     */
    FString GenerateSacrificeNarrative(
        const FMXRobotProfile& Profile,
        const FMXEventData& SacrificeEvent,
        const TArray<FString>& SavedNames
    );

    /**
     * Generate hat census commentary lines (hat section of the report).
     * Returns 2–4 short stat lines with dry humor woven in.
     */
    TArray<FString> GenerateHatCensusCommentary(
        const FMXHatCensus& Census,
        const FMXCompiledStats& Stats
    );

    /**
     * Generate the closing 1–2 sentence sign-off.
     */
    FString GenerateClosingNarrative(
        const FMXRunData& RunData,
        const FMXCompiledStats& Stats,
        int32 NextRunNumber,
        int32 HatCollectionCount,
        int32 NewHatsUnlockedThisRun
    );

    /**
     * Generic token replacement. Replaces all {token_name} occurrences with mapped values.
     * Case-insensitive key lookup.
     */
    FString FillTokens(
        const FString& Template,
        const TMap<FString, FString>& TokenMap
    ) const;

private:
    UPROPERTY() UDataTable* OpeningDT   = nullptr;
    UPROPERTY() UDataTable* HighlightDT = nullptr;
    UPROPERTY() UDataTable* AwardDT     = nullptr;
    UPROPERTY() UDataTable* EulogyDT    = nullptr;
    UPROPERTY() UDataTable* ClosingDT   = nullptr;

    /** Used rows tracked per report to avoid repetition within a single session. */
    TSet<FName> UsedOpeningRows;
    TSet<FName> UsedHighlightRows;
    TSet<FName> UsedAwardRows;
    TSet<FName> UsedEulogyRows;
    TSet<FName> UsedClosingRows;

    /**
     * Select a template row by condition string from a given DataTable.
     * Falls back to "any" condition rows if none match. Skips used rows first.
     * Marks selected row as used.
     */
    template<typename TRow>
    const TRow* SelectTemplate(
        UDataTable* DT,
        TSet<FName>& UsedRows,
        const FString& ConditionKey,
        TFunctionRef<bool(const TRow&)> ExtraFilter = [](const TRow&){ return true; }
    );

    /** Determine opening condition key from compiled stats. */
    FString ResolveOpeningCondition(const FMXCompiledStats& Stats) const;

    /** Determine highlight event type key from event data. */
    FString ResolveHighlightType(const FMXEventData& Event) const;

    /** Determine eulogy level bracket from robot profile. */
    FString ResolveEulogyBracket(
        const FMXRobotProfile& Profile,
        const FMXEventData& DeathEvent
    ) const;

    /** Format active_age_seconds into a human-readable string like "4d 2h of active service". */
    FString FormatActiveAge(float Seconds) const;

    /** Format a spec label from profile's role/tier2/tier3. */
    FString FormatSpecLabel(const FMXRobotProfile& Profile) const;
};
