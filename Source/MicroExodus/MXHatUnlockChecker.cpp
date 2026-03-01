// MXHatUnlockChecker.cpp — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Change Log:
//   v1.0 — Initial implementation

#include "MXHatUnlockChecker.h"
#include "MXInterfaces.h"
#include "MXConstants.h"
#include "MXTypes.h"

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void UMXHatUnlockChecker::Initialize(UObject* InRobotProviderObject)
{
    RobotProviderObject = InRobotProviderObject;
}

// ---------------------------------------------------------------------------
// Standard Unlocks (#1–30)
// ---------------------------------------------------------------------------

TArray<int32> UMXHatUnlockChecker::CheckStandardUnlocks(
    const FMXRunData& RunData,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    TArray<int32> NewUnlocks;

    // Accumulate session counters
    CumulativeRobotsDeployed += RunData.robots_deployed;
    TotalRunsAttempted++;
    TotalRobotsLost += RunData.robots_lost;

    if (RunData.outcome == ERunOutcome::Success)
    {
        TotalRunsCompleted++;
    }

    // #1 — Cap: Always unlocked (starter hat). Check once.
    if (CanUnlock(1))
    {
        MarkUnlocked(1); NewUnlocks.Add(1);
    }

    // #2 — Top Hat: Complete 5 runs.
    if (CanUnlock(2) && TotalRunsCompleted >= 5)
    {
        MarkUnlocked(2); NewUnlocks.Add(2);
    }

    // #3 — Hard Hat: Reach Level 10 in any run.
    if (CanUnlock(3) && RunData.levels_completed >= 10)
    {
        MarkUnlocked(3); NewUnlocks.Add(3);
    }

    // #4 — Cowboy Hat: Deploy 50 total robots across all runs.
    if (CanUnlock(4) && CumulativeRobotsDeployed >= 50)
    {
        MarkUnlocked(4); NewUnlocks.Add(4);
    }

    // #5 — Beret: Have any robot survive 5 consecutive runs.
    if (CanUnlock(5) && GetMaxRunStreak(AllProfiles) >= 5)
    {
        MarkUnlocked(5); NewUnlocks.Add(5);
    }

    // #6 — Helmet: Complete a run without losing a single robot.
    if (CanUnlock(6) && RunData.outcome == ERunOutcome::Success && RunData.robots_lost == 0)
    {
        MarkUnlocked(6); NewUnlocks.Add(6);
    }

    // #7 — Safari Hat: Complete 10 levels in a single run.
    if (CanUnlock(7) && RunData.levels_completed >= 10)
    {
        MarkUnlocked(7); NewUnlocks.Add(7);
    }

    // #8 — Graduation Cap: Have any robot reach Level 10.
    if (CanUnlock(8) && CountProfilesAtLevel(AllProfiles, 10) >= 1)
    {
        MarkUnlocked(8); NewUnlocks.Add(8);
    }

    // #9 — Fez: Complete 20 total runs (any outcome).
    if (CanUnlock(9) && TotalRunsAttempted >= 20)
    {
        MarkUnlocked(9); NewUnlocks.Add(9);
    }

    // #10 — Baseball Cap: Rescue 10 total robots (across all runs).
    if (CanUnlock(10))
    {
        // Sum rescues across this run's data; in a real implementation we'd sum lifetime
        // For now track via robots_rescued in RunData as an approximation
        // (Persistence module would maintain a lifetime counter)
        if (RunData.robots_rescued >= 10)
        {
            MarkUnlocked(10); NewUnlocks.Add(10);
        }
    }

    // #11 — Winter Hat: Survive a run on Hardened tier or higher.
    if (CanUnlock(11) && RunData.outcome == ERunOutcome::Success
        && static_cast<uint8>(RunData.tier) >= static_cast<uint8>(ETier::Hardened))
    {
        MarkUnlocked(11); NewUnlocks.Add(11);
    }

    // #12 — Party Hat: Have a robot with 25+ levels.
    if (CanUnlock(12) && CountProfilesAtLevel(AllProfiles, 25) >= 1)
    {
        MarkUnlocked(12); NewUnlocks.Add(12);
    }

    // #13 — Fedora: Complete 50 total runs (any outcome).
    if (CanUnlock(13) && TotalRunsAttempted >= 50)
    {
        MarkUnlocked(13); NewUnlocks.Add(13);
    }

    // #14 — Beanie: Deploy 200 total robots across all runs.
    if (CanUnlock(14) && CumulativeRobotsDeployed >= 200)
    {
        MarkUnlocked(14); NewUnlocks.Add(14);
    }

    // #15 — Sombrero: Have any robot survive 10 consecutive runs.
    if (CanUnlock(15) && GetMaxRunStreak(AllProfiles) >= 10)
    {
        MarkUnlocked(15); NewUnlocks.Add(15);
    }

    // #16 — Bucket Hat: Complete a run on Brutal tier or higher.
    if (CanUnlock(16) && RunData.outcome == ERunOutcome::Success
        && static_cast<uint8>(RunData.tier) >= static_cast<uint8>(ETier::Brutal))
    {
        MarkUnlocked(16); NewUnlocks.Add(16);
    }

    // #17 — Wizard Hat: Have a robot reach Level 50.
    if (CanUnlock(17) && CountProfilesAtLevel(AllProfiles, 50) >= 1)
    {
        MarkUnlocked(17); NewUnlocks.Add(17);
    }

    // #18 — Aviator Cap: Complete 100 total runs (any outcome).
    if (CanUnlock(18) && TotalRunsAttempted >= 100)
    {
        MarkUnlocked(18); NewUnlocks.Add(18);
    }

    // #19 — Crown: Deploy 500 total robots.
    if (CanUnlock(19) && CumulativeRobotsDeployed >= 500)
    {
        MarkUnlocked(19); NewUnlocks.Add(19);
    }

    // #20 — Deerstalker: Have 5 robots each survive 10 consecutive runs.
    if (CanUnlock(20))
    {
        int32 Count = 0;
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.current_run_streak >= 10) Count++;
        }
        if (Count >= 5) { MarkUnlocked(20); NewUnlocks.Add(20); }
    }

    // #21 — Top Hat Deluxe: Complete 200 total runs (any outcome).
    if (CanUnlock(21) && TotalRunsAttempted >= 200)
    {
        MarkUnlocked(21); NewUnlocks.Add(21);
    }

    // #22 — Pork Pie Hat: Complete a run on Nightmare tier or higher.
    if (CanUnlock(22) && RunData.outcome == ERunOutcome::Success
        && static_cast<uint8>(RunData.tier) >= static_cast<uint8>(ETier::Nightmare))
    {
        MarkUnlocked(22); NewUnlocks.Add(22);
    }

    // #23 — Napoleon Hat: Have a robot reach Level 75.
    if (CanUnlock(23) && CountProfilesAtLevel(AllProfiles, 75) >= 1)
    {
        MarkUnlocked(23); NewUnlocks.Add(23);
    }

    // #24 — Tricorne: Deploy 1000 total robots.
    if (CanUnlock(24) && CumulativeRobotsDeployed >= 1000)
    {
        MarkUnlocked(24); NewUnlocks.Add(24);
    }

    // #25 — Viking Helmet: Complete a run on Extinction tier or higher.
    if (CanUnlock(25) && RunData.outcome == ERunOutcome::Success
        && static_cast<uint8>(RunData.tier) >= static_cast<uint8>(ETier::Extinction))
    {
        MarkUnlocked(25); NewUnlocks.Add(25);
    }

    // #26 — Military Helmet: Have 10 robots each at Level 50+.
    if (CanUnlock(26) && CountProfilesAtLevel(AllProfiles, 50) >= 10)
    {
        MarkUnlocked(26); NewUnlocks.Add(26);
    }

    // #27 — Top Knot: Survive 500 total runs (success outcomes only).
    if (CanUnlock(27) && TotalRunsCompleted >= 500)
    {
        MarkUnlocked(27); NewUnlocks.Add(27);
    }

    // #28 — Mortarboard: Have a robot reach Level 100.
    if (CanUnlock(28) && CountProfilesAtLevel(AllProfiles, 100) >= 1)
    {
        MarkUnlocked(28); NewUnlocks.Add(28);
    }

    // #29 — Papal Mitre: Complete a Legendary tier run successfully.
    if (CanUnlock(29) && RunData.outcome == ERunOutcome::Success
        && RunData.tier == ETier::Legendary)
    {
        MarkUnlocked(29); NewUnlocks.Add(29);
    }

    // #30 — Golden Cap: Deploy 5000 total robots.
    if (CanUnlock(30) && CumulativeRobotsDeployed >= 5000)
    {
        MarkUnlocked(30); NewUnlocks.Add(30);
    }

    return NewUnlocks;
}

// ---------------------------------------------------------------------------
// Hidden Unlocks (#31–50)
// ---------------------------------------------------------------------------

TArray<int32> UMXHatUnlockChecker::CheckHiddenUnlocks(
    const FMXRunData& RunData,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    TArray<int32> NewUnlocks;

    const FTimespan RunDuration = RunData.end_time - RunData.start_time;
    const float RunSeconds = static_cast<float>(RunDuration.GetTotalSeconds());

    // #31 — Tinfoil Hat: A robot witnesses 25+ deaths lifetime without dying.
    if (CanUnlock(31))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            // Proxy for "without dying": runs_survived > 0 and deaths_witnessed >= 25
            if (P.deaths_witnessed >= 25 && P.runs_survived > 0)
            {
                MarkUnlocked(31); NewUnlocks.Add(31); break;
            }
        }
    }

    // #32 — Traffic Cone: Complete a run in under 30 minutes.
    if (CanUnlock(32) && RunData.outcome == ERunOutcome::Success && RunSeconds < 1800.0f)
    {
        MarkUnlocked(32); NewUnlocks.Add(32);
    }

    // #33 — Dunce Cap: Lose 50+ robots in a single run.
    if (CanUnlock(33) && RunData.robots_lost >= 50)
    {
        MarkUnlocked(33); NewUnlocks.Add(33);
    }

    // #34 — Crown of Thorns: Survive a run with 3 or fewer robots remaining.
    if (CanUnlock(34) && RunData.outcome == ERunOutcome::Success && RunData.robots_survived <= 3)
    {
        MarkUnlocked(34); NewUnlocks.Add(34);
    }

    // #35 — Propeller Hat: A robot accumulates 10+ near misses in lifetime.
    if (CanUnlock(35))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.near_misses >= 10)
            {
                MarkUnlocked(35); NewUnlocks.Add(35); break;
            }
        }
    }

    // #36 — Court Jester Hat: Fail a run on Level 1.
    if (CanUnlock(36) && RunData.outcome == ERunOutcome::Failure && RunData.levels_completed == 0)
    {
        MarkUnlocked(36); NewUnlocks.Add(36);
    }

    // #37 — Ushanka: Have a robot with 25+ ice encounters.
    if (CanUnlock(37))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.ice_encounters >= 25)
            {
                MarkUnlocked(37); NewUnlocks.Add(37); break;
            }
        }
    }

    // #38 — Welder's Mask: Have a robot with 25+ fire encounters.
    if (CanUnlock(38))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.fire_encounters >= 25)
            {
                MarkUnlocked(38); NewUnlocks.Add(38); break;
            }
        }
    }

    // #39 — Crash Helmet: Have a robot with 25+ crush encounters.
    if (CanUnlock(39))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.crush_encounters >= 25)
            {
                MarkUnlocked(39); NewUnlocks.Add(39); break;
            }
        }
    }

    // #40 — Lightning Rod Hat: Have a robot with 25+ EMP encounters.
    if (CanUnlock(40))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.emp_encounters >= 25)
            {
                MarkUnlocked(40); NewUnlocks.Add(40); break;
            }
        }
    }

    // #41 — Parachute Cap: Have a robot with 25+ fall encounters.
    if (CanUnlock(41))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.fall_encounters >= 25)
            {
                MarkUnlocked(41); NewUnlocks.Add(41); break;
            }
        }
    }

    // #42 — Nurse Cap: A robot witnesses 50+ rescues lifetime.
    if (CanUnlock(42))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.rescues_witnessed >= 50)
            {
                MarkUnlocked(42); NewUnlocks.Add(42); break;
            }
        }
    }

    // #43 — Grief Cap: A robot witnesses 50+ sacrifices lifetime.
    if (CanUnlock(43))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.sacrifices_witnessed >= 50)
            {
                MarkUnlocked(43); NewUnlocks.Add(43); break;
            }
        }
    }

    // #44 — Thinking Cap: A robot participates in 50+ puzzles.
    if (CanUnlock(44))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.puzzles_participated >= 50)
            {
                MarkUnlocked(44); NewUnlocks.Add(44); break;
            }
        }
    }

    // #45 — Speedster Helmet: Complete a run in under 15 minutes.
    if (CanUnlock(45) && RunData.outcome == ERunOutcome::Success && RunSeconds < 900.0f)
    {
        MarkUnlocked(45); NewUnlocks.Add(45);
    }

    // #46 — Wanderer's Bandana: Lose 500 total robots across all runs.
    if (CanUnlock(46) && TotalRobotsLost >= 500)
    {
        MarkUnlocked(46); NewUnlocks.Add(46);
    }

    // #47 — Philosopher's Cap: Have any robot reach 100 total_xp (000 XP lifetime).
    if (CanUnlock(47))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.total_xp >= 100000)
            {
                MarkUnlocked(47); NewUnlocks.Add(47); break;
            }
        }
    }

    // #48 — The Hat That Was There: Have a robot complete 50 levels total across all runs.
    if (CanUnlock(48))
    {
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.levels_completed >= 50)
            {
                MarkUnlocked(48); NewUnlocks.Add(48); break;
            }
        }
    }

    // #49 — Lone Survivor Hat: Win a run with exactly 1 robot surviving.
    if (CanUnlock(49) && RunData.outcome == ERunOutcome::Success && RunData.robots_survived == 1)
    {
        MarkUnlocked(49); NewUnlocks.Add(49);
    }

    // #50 — Hat of Many Hats: Wear 10 distinct hat types in a single run (checked via DEMS events).
    //   Approximated here by checking if AllProfiles shows 10 distinct current_hat_ids in a run context.
    //   (A more precise check would require run-scoped event data from DEMS.)
    if (CanUnlock(50))
    {
        TSet<int32> DistinctHatsThisRun;
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.current_hat_id != -1)
            {
                DistinctHatsThisRun.Add(P.current_hat_id);
            }
        }
        if (DistinctHatsThisRun.Num() >= 10)
        {
            MarkUnlocked(50); NewUnlocks.Add(50);
        }
    }

    return NewUnlocks;
}

// ---------------------------------------------------------------------------
// Level-Specific Unlocks (#71–90)
// ---------------------------------------------------------------------------

TArray<int32> UMXHatUnlockChecker::CheckLevelSpecificUnlocks(
    int32 LevelNumber,
    const TArray<FGuid>& SurvivingRobots,
    const TArray<FMXRobotProfile>& AllProfiles,
    const TMap<int32, FMXHatDefinition>& HatDefinitions)
{
    TArray<int32> NewUnlocks;

    // Build a map of RobotId → current_hat_id for fast lookup
    TMap<FGuid, int32> RobotHatMap;
    for (const FMXRobotProfile& P : AllProfiles)
    {
        RobotHatMap.Add(P.robot_id, P.current_hat_id);
    }

    // Evaluate each level-specific hat definition
    for (int32 HatId = LEVEL_SPECIFIC_HAT_MIN; HatId <= LEVEL_SPECIFIC_HAT_MAX; ++HatId)
    {
        if (!CanUnlock(HatId)) continue;

        const FMXHatDefinition* Def = HatDefinitions.Find(HatId);
        if (!Def || !Def->is_level_specific) continue;

        if (Def->level_specific_level != LevelNumber) continue;

        const int32 RequiredHat = Def->level_specific_hat_required;

        for (const FGuid& RobotId : SurvivingRobots)
        {
            const int32* WornHatId = RobotHatMap.Find(RobotId);
            if (!WornHatId) continue;

            // No prerequisite hat required (-1), OR the required hat is worn
            const bool bHatMatch = (RequiredHat == -1) || (*WornHatId == RequiredHat);
            if (bHatMatch)
            {
                MarkUnlocked(HatId);
                NewUnlocks.Add(HatId);
                UE_LOG(LogTemp, Log, TEXT("UMXHatUnlockChecker: Level-specific hat %d '%s' unlocked on Level %d."),
                    HatId, *Def->name, LevelNumber);
                break;
            }
        }
    }

    return NewUnlocks;
}

// ---------------------------------------------------------------------------
// Legendary Unlocks (#91–100)
// ---------------------------------------------------------------------------

TArray<int32> UMXHatUnlockChecker::CheckLegendaryUnlocks(
    const FMXRunData& RunData,
    const FMXHatCollection& Collection)
{
    TArray<int32> NewUnlocks;

    // #91 — Halo: Have 10 robots each reach Level 50.
    // (Deferred to AllProfiles — called via CheckStandardUnlocks path in manager)
    // Here we check collection-based conditions.

    // #92 — Crown of All: Unlock all 90 non-legendary hats (#1–90).
    if (CanUnlock(92))
    {
        bool bAllNonLegendaryUnlocked = true;
        for (int32 i = 1; i <= 90; ++i)
        {
            if (!Collection.unlocked_hat_ids.Contains(i))
            {
                bAllNonLegendaryUnlocked = false;
                break;
            }
        }
        if (bAllNonLegendaryUnlocked) { MarkUnlocked(92); NewUnlocks.Add(92); }
    }

    // #93 — Iron Helmet: Complete a run losing 0 robots on Nightmare tier or higher.
    if (CanUnlock(93) && RunData.outcome == ERunOutcome::Success
        && RunData.robots_lost == 0
        && static_cast<uint8>(RunData.tier) >= static_cast<uint8>(ETier::Nightmare))
    {
        MarkUnlocked(93); NewUnlocks.Add(93);
    }

    // #94 — Jester Crown: Complete Level 20 on Standard tier with 100 robots deployed.
    if (CanUnlock(94) && RunData.outcome == ERunOutcome::Success
        && RunData.tier == ETier::Standard
        && RunData.levels_completed >= 20
        && RunData.robots_deployed >= 100)
    {
        MarkUnlocked(94); NewUnlocks.Add(94);
    }

    // #95 — Ghost Hood: Complete a run with no hats equipped at all.
    if (CanUnlock(95) && RunData.outcome == ERunOutcome::Success)
    {
        // Check via events: if no OnHatEquipped events fired this run
        // Proxy: robots_deployed > 0 and no HatEquipped events in RunData.events
        bool bAnyHatEquipped = false;
        for (const FMXEventData& Evt : RunData.events)
        {
            if (Evt.event_type == EEventType::HatEquipped)
            {
                bAnyHatEquipped = true;
                break;
            }
        }
        if (!bAnyHatEquipped && RunData.robots_deployed > 0)
        {
            MarkUnlocked(95); NewUnlocks.Add(95);
        }
    }

    // #96 — Champion's Laurel: Win 50 total runs.
    if (CanUnlock(96) && TotalRunsCompleted >= 50)
    {
        MarkUnlocked(96); NewUnlocks.Add(96);
    }

    // #97 — Mourning Veil: Lose 1000 total robots across all runs.
    if (CanUnlock(97) && TotalRobotsLost >= 1000)
    {
        MarkUnlocked(97); NewUnlocks.Add(97);
    }

    // #98 — Infinity Circlet: All 100 robots wearing different hats in one run.
    if (CanUnlock(98) && RunData.robots_deployed >= 100)
    {
        // Check for distinct hat count across events
        TSet<int32> DistinctHats;
        for (const FMXEventData& Evt : RunData.events)
        {
            if (Evt.event_type == EEventType::HatEquipped)
            {
                DistinctHats.Add(Evt.hat_worn_id);
            }
        }
        if (DistinctHats.Num() >= 100)
        {
            MarkUnlocked(98); NewUnlocks.Add(98);
        }
    }

    // #99 — The Exodus Crown: Unlock all other 99 hats.
    if (CanUnlock(99))
    {
        bool bAll99Unlocked = true;
        for (int32 i = 1; i <= 98; ++i)
        {
            if (!Collection.unlocked_hat_ids.Contains(i))
            {
                bAll99Unlocked = false;
                break;
            }
        }
        if (bAll99Unlocked) { MarkUnlocked(99); NewUnlocks.Add(99); }
    }

    // #100 — The Haberdasher's Crown: 100% collection (all 99 other hats) + completing a Legendary run.
    if (CanUnlock(100) && Collection.unlocked_hat_ids.Contains(99)
        && RunData.outcome == ERunOutcome::Success
        && RunData.tier == ETier::Legendary)
    {
        MarkUnlocked(100); NewUnlocks.Add(100);
    }

    return NewUnlocks;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

bool UMXHatUnlockChecker::CanUnlock(int32 HatId) const
{
    return !AlreadyUnlocked.Contains(HatId);
}

void UMXHatUnlockChecker::MarkUnlocked(int32 HatId)
{
    AlreadyUnlocked.Add(HatId);
}

int32 UMXHatUnlockChecker::GetMaxRunStreak(const TArray<FMXRobotProfile>& AllProfiles)
{
    int32 MaxStreak = 0;
    for (const FMXRobotProfile& P : AllProfiles)
    {
        MaxStreak = FMath::Max(MaxStreak, P.current_run_streak);
    }
    return MaxStreak;
}

int32 UMXHatUnlockChecker::GetMaxDeathsWitnessed(const TArray<FMXRobotProfile>& AllProfiles)
{
    int32 Max = 0;
    for (const FMXRobotProfile& P : AllProfiles)
    {
        Max = FMath::Max(Max, P.deaths_witnessed);
    }
    return Max;
}

int32 UMXHatUnlockChecker::CountProfilesAtLevel(const TArray<FMXRobotProfile>& AllProfiles, int32 MinLevel)
{
    int32 Count = 0;
    for (const FMXRobotProfile& P : AllProfiles)
    {
        if (P.level >= MinLevel) Count++;
    }
    return Count;
}

int32 UMXHatUnlockChecker::CountDistinctHatsEverWorn(const TArray<FMXRobotProfile>& AllProfiles)
{
    TSet<int32> AllHats;
    for (const FMXRobotProfile& P : AllProfiles)
    {
        for (int32 HatId : P.hat_history)
        {
            AllHats.Add(HatId);
        }
    }
    return AllHats.Num();
}
