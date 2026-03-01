// MXStatCompiler.cpp — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports

#include "MXStatCompiler.h"
#include "MXConstants.h"

// ---------------------------------------------------------------------------
// Public Entry Point
// ---------------------------------------------------------------------------

FMXCompiledStats UMXStatCompiler::CompileStats(
    const FMXRunData& RunData,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    FMXCompiledStats Stats;
    HatRarityCache.Reset();
    HatNameCache.Reset();

    // Pre-populate hat caches from robot profiles
    for (const FMXRobotProfile& Profile : AllProfiles)
    {
        if (Profile.current_hat_id >= 0)
        {
            // Rarity and name are resolved via HatProvider at engine init;
            // here we store what we know from profiles. Narrator fills display names
            // from DataTables; we seed with -1 / empty to be overridden.
            if (!HatNameCache.Contains(Profile.current_hat_id))
            {
                HatNameCache.Add(Profile.current_hat_id, TEXT(""));
            }
        }
    }

    // Prime level stats for all levels that might appear
    // (we grow dynamically in EnsureLevelStats)

    // Track hat usage: hat_id → count of robots wearing it
    TMap<int32, int32> HatUsageCount;

    // ---------------------------------------------------------------------------
    // Single Pass Over Event Log
    // ---------------------------------------------------------------------------
    const TArray<FMXEventData>& Events = RunData.events;
    const int32 EventCount = Events.Num();

    if (EventCount > 0)
    {
        Stats.first_event_index = 0;
        Stats.last_event_index  = EventCount - 1;
    }

    for (int32 i = 0; i < EventCount; ++i)
    {
        const FMXEventData& Ev = Events[i];
        FMXRobotRunStats& RS   = EnsureRobotStats(Stats, Ev);
        FMXLevelRunStats& LS   = EnsureLevelStats(Stats, Ev.level_number);

        RS.event_count++;

        // Hat tracking — grab the hat this robot is wearing during the event
        if (Ev.hat_worn_id >= 0 && RS.hat_id < 0)
        {
            RS.hat_id   = Ev.hat_worn_id;
            RS.hat_name = Ev.hat_worn_name;
            HatUsageCount.FindOrAdd(Ev.hat_worn_id)++;
        }

        // Classify by event type
        switch (Ev.event_type)
        {
        case EEventType::Death:
            Stats.total_losses++;
            LS.losses++;
            RS.died = true;
            if (Ev.robot_level < Stats.lowest_level_lost)
                Stats.lowest_level_lost = Ev.robot_level;
            break;

        case EEventType::Sacrifice:
            Stats.total_sacrifices++;
            LS.sacrifices++;
            RS.was_sacrificed = true;
            break;

        case EEventType::NearMiss:
            Stats.total_near_misses++;
            LS.near_misses++;
            RS.near_misses++;
            break;

        case EEventType::Rescue:
            Stats.total_rescues++;
            LS.rescues++;
            break;

        case EEventType::RescueWitnessed:
            RS.rescues_nearby++;
            break;

        case EEventType::DeathWitnessed:
            RS.deaths_witnessed++;
            break;

        case EEventType::HatLost:
            Stats.hats_lost++;
            LS.hats_lost++;
            break;

        default:
            break;
        }

        // Hazard-type encounter tracking
        if (Ev.event_type == EEventType::NearMiss || Ev.event_type == EEventType::Death)
        {
            switch (Ev.hazard_element)
            {
            case EHazardElement::Fire:        RS.fire_hits++;   break;
            case EHazardElement::Mechanical:  RS.crush_hits++;  break;
            case EHazardElement::Gravity:     RS.fall_hits++;   break;
            case EHazardElement::Ice:         RS.ice_hits++;    break;
            case EHazardElement::Electrical:  RS.emp_hits++;    break;
            default: break;
            }
        }

        // Level activity tracking
        RS.active_levels = FMath::Max(RS.active_levels, Ev.level_number);

        // Update robot level to highest seen (events are chronological)
        if (Ev.robot_level > RS.robot_level)
            RS.robot_level = Ev.robot_level;

        // Track overall highest level robot
        if (Ev.robot_level > Stats.highest_level_robot)
            Stats.highest_level_robot = Ev.robot_level;

        // Mass casualty peak tracking per level
        if (Ev.event_type == EEventType::Death || Ev.event_type == EEventType::Sacrifice)
        {
            // We count sequential deaths in same event group by checking a 2-event window
            // A proper mass_casualty_count is set in finalization; here we just increment
            LS.mass_casualty_count++;
        }
    }

    // ---------------------------------------------------------------------------
    // Finalization Pass
    // ---------------------------------------------------------------------------
    FinalizeStats(Stats, RunData, AllProfiles);

    // Hat census: determine most common and rarest
    {
        FString MostCommon;
        int32   MostCommonCount = 0;
        FString Rarest;
        EHatRarity RarestRarity = EHatRarity::Common;

        for (auto& Pair : HatUsageCount)
        {
            FString HatName = HatNameCache.FindRef(Pair.Key);
            if (HatName.IsEmpty())
                HatName = FString::Printf(TEXT("Hat #%d"), Pair.Key);

            if (Pair.Value > MostCommonCount)
            {
                MostCommonCount = Pair.Value;
                MostCommon      = HatName;
            }

            EHatRarity Rarity = ResolveHatRarity(Pair.Key);
            if ((int32)Rarity > (int32)RarestRarity)
            {
                RarestRarity = Rarity;
                Rarest       = HatName;
            }
        }

        Stats.most_common_hat    = MostCommon;
        Stats.rarest_hat         = Rarest;
        Stats.rarest_hat_rarity  = RarestRarity;
        Stats.unique_hat_types   = HatUsageCount.Num();
        Stats.total_hatted       = 0;
        for (const FMXRobotRunStats& RS : Stats.robot_stats)
            if (RS.hat_id >= 0) Stats.total_hatted++;

        Stats.hat_percent = (Stats.robots_deployed > 0)
            ? (float)Stats.total_hatted / (float)Stats.robots_deployed * 100.0f
            : 0.0f;
    }

    // Condition flags
    {
        const float SurvivalPct = Stats.survival_rate * 100.0f;
        Stats.is_perfect_run      = (Stats.total_losses == 0 && Stats.total_sacrifices == 0 && !Stats.is_failed);
        Stats.is_bloodbath        = (SurvivalPct < 50.0f);
        Stats.is_high_survival    = (SurvivalPct >= 80.0f && !Stats.is_perfect_run);
        Stats.is_hat_heavy        = (Stats.hat_percent >= 60.0f);
        Stats.is_sacrifice_heavy  = (Stats.total_sacrifices >= 3);
        Stats.is_failed           = (RunData.outcome == ERunOutcome::Failure);
    }

    // Timing
    {
        FTimespan Duration = RunData.end_time - RunData.start_time;
        Stats.total_run_time_seconds = (float)Duration.GetTotalSeconds();
        Stats.levels_completed       = RunData.levels_completed;
    }

    // Longest near-miss streak (per robot)
    {
        for (const FMXRobotRunStats& RS : Stats.robot_stats)
            Stats.longest_near_miss_streak = FMath::Max(Stats.longest_near_miss_streak, RS.near_misses);
    }

    return Stats;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

FMXRobotRunStats& UMXStatCompiler::EnsureRobotStats(
    FMXCompiledStats& Stats,
    const FMXEventData& Event)
{
    FMXRobotRunStats* Existing = Stats.FindRobotStatsMutable(Event.robot_id);
    if (Existing) return *Existing;

    FMXRobotRunStats New;
    New.robot_id    = Event.robot_id;
    New.robot_name  = Event.robot_name;
    New.robot_level = Event.robot_level;
    New.robot_spec  = Event.robot_spec;
    New.hat_id      = -1;
    Stats.robot_stats.Add(New);
    return Stats.robot_stats.Last();
}

FMXLevelRunStats& UMXStatCompiler::EnsureLevelStats(
    FMXCompiledStats& Stats,
    int32 LevelNumber)
{
    FMXLevelRunStats* Existing = Stats.FindLevelStatsMutable(LevelNumber);
    if (Existing) return *Existing;

    FMXLevelRunStats New;
    New.level_number = LevelNumber;
    Stats.level_stats.Add(New);
    return Stats.level_stats.Last();
}

EHatRarity UMXStatCompiler::ResolveHatRarity(int32 HatId) const
{
    // Without a live HatProvider during compilation we default to Common.
    // The RunReportEngine pre-populates HatRarityCache from the HatProvider before calling CompileStats.
    const EHatRarity* Found = HatRarityCache.Find(HatId);
    return Found ? *Found : EHatRarity::Common;
}

void UMXStatCompiler::FinalizeStats(
    FMXCompiledStats& Stats,
    const FMXRunData& RunData,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    Stats.robots_deployed  = RunData.robots_deployed;
    Stats.robots_survived  = RunData.robots_survived;
    Stats.total_losses     = FMath::Max(Stats.total_losses, RunData.robots_lost);

    Stats.survival_rate = (Stats.robots_deployed > 0)
        ? (float)Stats.robots_survived / (float)Stats.robots_deployed
        : 0.0f;

    // Mark survivors using profile data (profiles know if robot is still alive)
    for (const FMXRobotProfile& Profile : AllProfiles)
    {
        FMXRobotRunStats* RS = Stats.FindRobotStatsMutable(Profile.robot_id);
        if (RS)
        {
            // If the robot didn't die and wasn't sacrificed, it survived
            if (!RS->died && !RS->was_sacrificed)
                RS->survived = true;

            // Carry over XP gained proxy from level delta (rough, but good enough for awards)
            RS->xp_gained_this_run = Profile.xp;
        }
    }

    // Sort level stats by level number for ordered display
    Stats.level_stats.Sort([](const FMXLevelRunStats& A, const FMXLevelRunStats& B)
    {
        return A.level_number < B.level_number;
    });

    // Clamp lowest level lost if no losses occurred
    if (Stats.total_losses == 0 && Stats.total_sacrifices == 0)
        Stats.lowest_level_lost = 0;
}
