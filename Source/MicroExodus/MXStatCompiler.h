// MXStatCompiler.h — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports
// Consumers: MXRunReportEngine, MXHighlightScorer, MXAwardSelector, MXReportNarrator
// Change Log:
//   v1.0 — Initial single-pass stat aggregation over frozen event log

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXRunData.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXStatCompiler.generated.h"

// ---------------------------------------------------------------------------
// Per-Robot Stats (collected during single pass)
// ---------------------------------------------------------------------------

/**
 * FMXRobotRunStats
 * Per-robot statistics accumulated during the stat compilation pass.
 * Exists only within the Reports module for award selection and narrative generation.
 */
USTRUCT()
struct FMXRobotRunStats
{
    GENERATED_BODY()

    UPROPERTY() FGuid  robot_id;
    UPROPERTY() FString robot_name;
    UPROPERTY() int32  robot_level        = 0;
    UPROPERTY() FString robot_spec;

    /** Hat worn during this run (-1 = none). */
    UPROPERTY() int32  hat_id             = -1;
    UPROPERTY() FString hat_name;

    /** Number of near-miss events this robot personally survived. */
    UPROPERTY() int32  near_misses        = 0;

    /** Deaths this robot witnessed. */
    UPROPERTY() int32  deaths_witnessed   = 0;

    /** Rescues that occurred near this robot. */
    UPROPERTY() int32  rescues_nearby     = 0;

    /** Approximate seconds the robot spent stationary (idle proxy). */
    UPROPERTY() float  idle_time_seconds  = 0.0f;

    /** Levels where this robot had at least one event. Used for idle tracking. */
    UPROPERTY() int32  active_levels      = 0;

    /** Hazard encounters by element type this run. */
    UPROPERTY() int32  fire_hits          = 0;
    UPROPERTY() int32  crush_hits         = 0;
    UPROPERTY() int32  fall_hits          = 0;
    UPROPERTY() int32  ice_hits           = 0;
    UPROPERTY() int32  emp_hits           = 0;

    /** XP gained this run (inferred from level delta if profile available). */
    UPROPERTY() int32  xp_gained_this_run = 0;

    /** Score contribution (events where this robot was primary). */
    UPROPERTY() int32  score_contribution = 0;

    /** Lethal hits absorbed (Guardian/Bulwark role tracking). */
    UPROPERTY() int32  lethal_hits_absorbed = 0;

    /** Number of revives performed (Mechanic/Engineer role tracking). */
    UPROPERTY() int32  revives_performed  = 0;

    /** Number of events where this robot was primary — proxy for activity. */
    UPROPERTY() int32  event_count        = 0;

    /** True if robot survived to run end. */
    UPROPERTY() bool   survived           = false;

    /** True if this robot was sacrificed this run. */
    UPROPERTY() bool   was_sacrificed     = false;

    /** True if this robot died (non-sacrifice) this run. */
    UPROPERTY() bool   died               = false;
};

// ---------------------------------------------------------------------------
// Per-Level Stats
// ---------------------------------------------------------------------------

/**
 * FMXLevelRunStats
 * Per-level summary statistics aggregated during the compilation pass.
 */
USTRUCT()
struct FMXLevelRunStats
{
    GENERATED_BODY()

    UPROPERTY() int32  level_number   = 0;
    UPROPERTY() int32  swarm_in       = 0;   // robots entering this level
    UPROPERTY() int32  swarm_out      = 0;   // robots exiting alive
    UPROPERTY() int32  rescues        = 0;
    UPROPERTY() int32  losses         = 0;
    UPROPERTY() int32  near_misses    = 0;
    UPROPERTY() int32  hats_lost      = 0;
    UPROPERTY() int32  sacrifices     = 0;
    UPROPERTY() float  time_seconds   = 0.0f;
    UPROPERTY() int32  score_gained   = 0;
    UPROPERTY() int32  mass_casualty_count = 0; // single-event death count peak
};

// ---------------------------------------------------------------------------
// Full Compiled Stats
// ---------------------------------------------------------------------------

/**
 * FMXCompiledStats
 * The complete aggregated stat record for a single run.
 * Produced by MXStatCompiler in a single pass over the event log.
 * Consumed by HighlightScorer, AwardSelector, and ReportNarrator.
 * This struct lives only within the Reports module.
 */
USTRUCT()
struct FMXCompiledStats
{
    GENERATED_BODY()

    // --- Run-wide totals ---
    UPROPERTY() int32  total_near_misses      = 0;
    UPROPERTY() int32  total_rescues          = 0;
    UPROPERTY() int32  total_losses           = 0;
    UPROPERTY() int32  total_sacrifices       = 0;
    UPROPERTY() int32  robots_deployed        = 0;
    UPROPERTY() int32  robots_survived        = 0;
    UPROPERTY() float  survival_rate          = 0.0f;  // 0.0–1.0

    // --- Hat census ---
    UPROPERTY() int32  total_hatted           = 0;
    UPROPERTY() int32  unique_hat_types       = 0;
    UPROPERTY() int32  hats_lost              = 0;
    UPROPERTY() float  hat_percent            = 0.0f;  // % of deployed robots that wore a hat
    UPROPERTY() FString most_common_hat;
    UPROPERTY() FString rarest_hat;
    UPROPERTY() EHatRarity rarest_hat_rarity  = EHatRarity::Common;

    // --- Extremes ---
    UPROPERTY() int32  highest_level_robot    = 0;     // highest level of any robot this run
    UPROPERTY() int32  lowest_level_lost      = 999;   // lowest level of any robot that died
    UPROPERTY() int32  longest_near_miss_streak = 0;   // most consecutive near-misses by one robot

    // --- Timing ---
    UPROPERTY() float  total_run_time_seconds = 0.0f;
    UPROPERTY() int32  levels_completed       = 0;

    // --- First/last event refs ---
    UPROPERTY() int32  first_event_index      = -1;
    UPROPERTY() int32  last_event_index       = -1;

    // --- Per-robot lookup ---
    UPROPERTY() TArray<FMXRobotRunStats> robot_stats;

    // --- Per-level lookup ---
    UPROPERTY() TArray<FMXLevelRunStats> level_stats;

    // --- Outcome condition flags (for template selection) ---
    UPROPERTY() bool   is_perfect_run         = false;  // zero losses
    UPROPERTY() bool   is_bloodbath           = false;  // >50% casualties
    UPROPERTY() bool   is_hat_heavy           = false;  // >60% of swarm hatted
    UPROPERTY() bool   is_sacrifice_heavy     = false;  // ≥3 sacrifices
    UPROPERTY() bool   is_high_survival       = false;  // >80% survival, not perfect
    UPROPERTY() bool   is_failed              = false;  // run failed

    // Helper accessors (not UPROPERTY — inline logic)
    const FMXRobotRunStats* FindRobotStats(const FGuid& RobotId) const
    {
        for (const FMXRobotRunStats& S : robot_stats)
        {
            if (S.robot_id == RobotId) return &S;
        }
        return nullptr;
    }

    FMXRobotRunStats* FindRobotStatsMutable(const FGuid& RobotId)
    {
        for (FMXRobotRunStats& S : robot_stats)
        {
            if (S.robot_id == RobotId) return &S;
        }
        return nullptr;
    }

    FMXLevelRunStats* FindLevelStatsMutable(int32 LevelNum)
    {
        for (FMXLevelRunStats& S : level_stats)
        {
            if (S.level_number == LevelNum) return &S;
        }
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// MXStatCompiler
// ---------------------------------------------------------------------------

/**
 * UMXStatCompiler
 * Performs a single-pass aggregation over the frozen event log to produce FMXCompiledStats.
 * Called once at report generation time. Results are immutable after compilation.
 */
UCLASS()
class UMXStatCompiler : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Main entry point. Iterates all events in RunData exactly once.
     * Also accepts optional robot profiles for richer per-robot data.
     * @param RunData       Finalized run record (frozen at run end).
     * @param AllProfiles   Robot profiles fetched from IMXRobotProvider.
     * @return              Fully populated FMXCompiledStats.
     */
    FMXCompiledStats CompileStats(
        const FMXRunData& RunData,
        const TArray<FMXRobotProfile>& AllProfiles
    );

private:
    /** Ensure a per-robot entry exists and return a mutable pointer. */
    FMXRobotRunStats& EnsureRobotStats(
        FMXCompiledStats& Stats,
        const FMXEventData& Event
    );

    /** Ensure a per-level entry exists and return a mutable pointer. */
    FMXLevelRunStats& EnsureLevelStats(
        FMXCompiledStats& Stats,
        int32 LevelNumber
    );

    /** Resolve hat rarity from a hat ID (uses cached hat rarity map built from profiles). */
    EHatRarity ResolveHatRarity(int32 HatId) const;

    /** Post-pass calculations: survival rate, condition flags, hat census. */
    void FinalizeStats(
        FMXCompiledStats& Stats,
        const FMXRunData& RunData,
        const TArray<FMXRobotProfile>& AllProfiles
    );

    /** Map from hat_id → rarity, populated from AllProfiles during first pass. */
    TMap<int32, EHatRarity> HatRarityCache;

    /** Map from hat_id → display name. */
    TMap<int32, FString> HatNameCache;
};
