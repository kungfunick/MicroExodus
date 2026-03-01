// MXHighlightScorer.cpp — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports

#include "MXHighlightScorer.h"
#include "MXConstants.h"

// ---------------------------------------------------------------------------
// ScoreEvents — batch scoring and top-N extraction
// ---------------------------------------------------------------------------

TArray<FMXHighlight> UMXHighlightScorer::ScoreEvents(
    const TArray<FMXEventData>& Events,
    const FMXCompiledStats& Stats)
{
    const int32 Total = Events.Num();
    TArray<FMXHighlight> Scored;
    Scored.Reserve(Total);

    for (int32 i = 0; i < Total; ++i)
    {
        // Skip bookkeeping events that don't make interesting highlights
        const EEventType ET = Events[i].event_type;
        if (ET == EEventType::LevelComplete    ||
            ET == EEventType::RunComplete      ||
            ET == EEventType::RunFailed        ||
            ET == EEventType::HatEquipped      ||
            ET == EEventType::ComboDiscovered  ||
            ET == EEventType::HatUnlocked)
        {
            continue;
        }

        FMXHighlight H;
        H.event     = Events[i];
        H.score     = ScoreEvent(Events[i], i, Total, Stats);
        H.narrative = TEXT(""); // filled later by ReportNarrator
        Scored.Add(H);
    }

    // Sort descending by score
    Scored.Sort([](const FMXHighlight& A, const FMXHighlight& B)
    {
        return A.score > B.score;
    });

    // Clamp to REPORT_MAX_HIGHLIGHTS
    const int32 Cap = FMath::Min(Scored.Num(), MXConstants::REPORT_MAX_HIGHLIGHTS);
    Scored.SetNum(Cap);

    return Scored;
}

// ---------------------------------------------------------------------------
// ScoreEvent — weighted criteria evaluation for one event
// ---------------------------------------------------------------------------

float UMXHighlightScorer::ScoreEvent(
    const FMXEventData& Event,
    int32 EventIdx,
    int32 TotalEvents,
    const FMXCompiledStats& Stats)
{
    float Score = 0.0f;

    // Base score by severity
    switch (Event.severity)
    {
    case ESeverity::Dramatic: Score += Base_Dramatic; break;
    case ESeverity::Major:    Score += Base_Major;    break;
    default: break;
    }

    // High-level robot (+50 per 10 levels above 20)
    if (Event.robot_level > 20)
    {
        const int32 ExtraLevels = Event.robot_level - 20;
        Score += Weight_HighLevel * FMath::CeilToFloat((float)ExtraLevels / 10.0f);
    }

    // Hatted robot
    if (Event.hat_worn_id >= 0)
    {
        Score += Weight_Hatted;

        // Rare hat bonus
        const FMXRobotRunStats* RS = Stats.FindRobotStats(Event.robot_id);
        if (RS)
        {
            // Infer rarity from the compiled stats' rarest hat tracking
            // If this event's hat matches the run's rarest hat, grant bonus
            if (!Stats.rarest_hat.IsEmpty() && Event.hat_worn_name == Stats.rarest_hat)
                Score += Weight_RareHat;
        }
    }

    // Sacrifice event
    if (Event.event_type == EEventType::Sacrifice)
        Score += Weight_Sacrifice;

    // First or last event of the run
    if (EventIdx == 0 || EventIdx == TotalEvents - 1)
        Score += Weight_FirstLast;

    // Streak breaker — robot with many runs survived just died
    if ((Event.event_type == EEventType::Death || Event.event_type == EEventType::Sacrifice)
        && HadLongStreak(Event, Stats))
    {
        Score += Weight_StreakBreaker;
    }

    // Specialist ability implied (any event where robot has a spec and it's dramatic)
    if (!Event.robot_spec.IsEmpty()
        && Event.robot_spec != TEXT("None")
        && Event.severity == ESeverity::Dramatic)
    {
        Score += Weight_SpecialistAbility;
    }

    // Comedy element
    if (Event.hazard_element == EHazardElement::Comedy)
        Score += Weight_Comedy;

    // Record-breaking
    if (IsRecordBreaking(Event, Stats))
        Score += Weight_RecordBreaking;

    // Multi-robot mass casualty proxy: add per-additional estimated robot
    const int32 ExtraRobots = FMath::Max(0, EstimateRobotsInEvent(Event, Stats) - 1);
    Score += Weight_MultiRobot * (float)ExtraRobots;

    return Score;
}

// ---------------------------------------------------------------------------
// IsRecordBreaking
// ---------------------------------------------------------------------------

bool UMXHighlightScorer::IsRecordBreaking(
    const FMXEventData& Event,
    const FMXCompiledStats& Stats) const
{
    // Death of the highest-level robot in the run
    if ((Event.event_type == EEventType::Death || Event.event_type == EEventType::Sacrifice)
        && Event.robot_level == Stats.highest_level_robot
        && Event.robot_level >= 20)
    {
        return true;
    }

    // Robot that has the most near-misses this run just died
    if (Event.event_type == EEventType::Death)
    {
        const FMXRobotRunStats* RS = Stats.FindRobotStats(Event.robot_id);
        if (RS && RS->near_misses == Stats.longest_near_miss_streak && RS->near_misses >= 5)
            return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

int32 UMXHighlightScorer::EstimateRobotsInEvent(
    const FMXEventData& Event,
    const FMXCompiledStats& Stats) const
{
    // Without a richer event payload we can't know the exact room count.
    // We use the level's loss/near-miss count as a proxy for mass events.
    for (const FMXLevelRunStats& LS : Stats.level_stats)
    {
        if (LS.level_number == Event.level_number)
        {
            // If this level had many deaths and this is a death event, hint mass casualty
            if ((Event.event_type == EEventType::Death) && LS.losses >= 3)
                return FMath::Clamp(LS.losses, 1, 5);
        }
    }
    return 1;
}

bool UMXHighlightScorer::HadLongStreak(
    const FMXEventData& Event,
    const FMXCompiledStats& Stats) const
{
    // A robot is a "streak breaker" if its current_run_streak was >= 5.
    // We can't read profile directly here, but the robot_level is a good proxy:
    // high-level robots are typically veterans. Level 20+ with at least a few near-misses.
    if (Event.robot_level >= 20)
        return true;

    // Also check if the robot had survived many near-misses this run (implies long tenure)
    const FMXRobotRunStats* RS = Stats.FindRobotStats(Event.robot_id);
    return RS && RS->near_misses >= 3;
}
