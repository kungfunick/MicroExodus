// MXAwardSelector.cpp — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports

#include "MXAwardSelector.h"
#include "MXConstants.h"

// ---------------------------------------------------------------------------
// SelectAwards
// ---------------------------------------------------------------------------

TArray<FMXAward> UMXAwardSelector::SelectAwards(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster)
{
    TArray<FMXAward> Awards;
    Awards.Reserve(MXConstants::REPORT_MAX_AWARDS);

    // Evaluate each category in display order; skipped if no standout found
    auto Try = [&](TOptional<FMXAward> Opt)
    {
        if (Opt.IsSet())
            Awards.Add(Opt.GetValue());
    };

    Try(EvaluateMVP(Stats, Roster));
    Try(EvaluateNearMissChampion(Stats, Roster));
    Try(EvaluateRescueHero(Stats, Roster));
    Try(EvaluateIronHide(Stats, Roster));
    Try(EvaluateTourist(Stats, Roster));
    Try(EvaluateWorstLuck(Stats, Roster));
    Try(EvaluateFashionForward(Stats, Roster));
    Try(EvaluateTheSacrifice(Stats, Roster));
    Try(EvaluateNewcomer(Stats, Roster));
    Try(EvaluateVeteranSurvivor(Stats, Roster));
    Try(EvaluateBestDressed(Stats, Roster));
    Try(EvaluateWorstDressed(Stats, Roster));
    Try(EvaluateGuardianAngel(Stats, Roster));
    Try(EvaluateTheMechanic(Stats, Roster));
    Try(EvaluateHatCasualty(Stats, Roster));
    Try(EvaluateCoward(Stats, Roster));

    return Awards;
}

// ---------------------------------------------------------------------------
// IsStandout
// ---------------------------------------------------------------------------

bool UMXAwardSelector::IsStandout(float WinnerValue, float Average) const
{
    if (Average <= 0.0f) return WinnerValue > 0.0f;
    return WinnerValue >= Average * StandoutMultiplier;
}

// ---------------------------------------------------------------------------
// Per-Category Evaluators
// ---------------------------------------------------------------------------

TOptional<FMXAward> UMXAwardSelector::EvaluateMVP(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // MVP: highest event_count (proxy for score contribution and activity)
    auto Acc = [](const FMXRobotRunStats& RS) { return (float)RS.event_count; };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout((float)Winner->event_count, Avg)) return {};
    return MakeAward(EAwardCategory::MVP, *Winner,
        FString::Printf(TEXT("%d events"), Winner->event_count));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateNearMissChampion(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    auto Acc = [](const FMXRobotRunStats& RS) { return (float)RS.near_misses; };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner || Winner->near_misses < 3) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout((float)Winner->near_misses, Avg)) return {};
    return MakeAward(EAwardCategory::NearMissChampion, *Winner,
        FString::Printf(TEXT("%d near-misses"), Winner->near_misses));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateRescueHero(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    auto Acc = [](const FMXRobotRunStats& RS) { return (float)RS.rescues_nearby; };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner || Winner->rescues_nearby < 2) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout((float)Winner->rescues_nearby, Avg)) return {};
    return MakeAward(EAwardCategory::RescueHero, *Winner,
        FString::Printf(TEXT("near %d rescues"), Winner->rescues_nearby));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateIronHide(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // IronHide: most total hazard hits survived (sum of all encounter types)
    auto Acc = [](const FMXRobotRunStats& RS) {
        return (float)(RS.fire_hits + RS.crush_hits + RS.fall_hits + RS.ice_hits + RS.emp_hits);
    };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner || Acc(*Winner) < 3.0f) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout(Acc(*Winner), Avg)) return {};
    return MakeAward(EAwardCategory::IronHide, *Winner,
        FString::Printf(TEXT("%d hazard encounters"), (int32)Acc(*Winner)));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateTourist(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // Tourist: lowest event_count among survivors (least participation)
    const FMXRobotRunStats* Laziest = nullptr;
    int32 LowestCount = INT_MAX;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        if (RS.survived && RS.event_count < LowestCount)
        {
            LowestCount = RS.event_count;
            Laziest     = &RS;
        }
    }
    if (!Laziest) return {};
    float Avg = ComputeAverage(Stats, [](const FMXRobotRunStats& RS) { return (float)RS.event_count; });
    // Tourist wins if they had significantly fewer events than average
    if (Avg <= 0.0f || (float)LowestCount >= Avg * 0.4f) return {};
    return MakeAward(EAwardCategory::Tourist, *Laziest,
        FString::Printf(TEXT("%d events logged"), LowestCount));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateWorstLuck(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // WorstLuck: most deaths witnessed + own near-misses
    auto Acc = [](const FMXRobotRunStats& RS) {
        return (float)(RS.deaths_witnessed + RS.near_misses);
    };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner || Acc(*Winner) < 4.0f) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout(Acc(*Winner), Avg)) return {};
    return MakeAward(EAwardCategory::WorstLuck, *Winner,
        FString::Printf(TEXT("%d combined misfortune events"), (int32)Acc(*Winner)));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateFashionForward(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // FashionForward: surviving robot wearing the rarest hat
    const FMXRobotRunStats* Best = nullptr;
    EHatRarity BestRarity = EHatRarity::Common;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        if (RS.survived && RS.hat_id >= 0)
        {
            // Best approximation: if this robot's hat matches the run's rarest hat
            if (!Stats.rarest_hat.IsEmpty() && RS.hat_name == Stats.rarest_hat)
            {
                Best = &RS;
                BestRarity = Stats.rarest_hat_rarity;
            }
        }
    }
    if (!Best) return {};
    if ((int32)BestRarity < (int32)EHatRarity::Rare) return {}; // Only Rare+ warrants the award
    return MakeAward(EAwardCategory::FashionForward, *Best,
        FString::Printf(TEXT("%s (survived)"), *Best->hat_name));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateTheSacrifice(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // TheSacrifice: highest-level robot that was sacrificed
    const FMXRobotRunStats* Best = nullptr;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        if (RS.was_sacrificed)
        {
            if (!Best || RS.robot_level > Best->robot_level)
                Best = &RS;
        }
    }
    if (!Best) return {};
    return MakeAward(EAwardCategory::TheSacrifice, *Best,
        FString::Printf(TEXT("Lvl %d"), Best->robot_level));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateNewcomer(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // Newcomer: recently rescued robot with the highest XP gained this run
    auto Acc = [](const FMXRobotRunStats& RS) { return (float)RS.xp_gained_this_run; };
    const FMXRobotRunStats* Winner = nullptr;
    float BestXP = 0.0f;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        // Proxy for "recently rescued": low level robot
        if (RS.robot_level <= 5 && RS.xp_gained_this_run > BestXP)
        {
            BestXP  = (float)RS.xp_gained_this_run;
            Winner  = &RS;
        }
    }
    if (!Winner || BestXP <= 0.0f) return {};
    return MakeAward(EAwardCategory::NewcomerOfTheRun, *Winner,
        FString::Printf(TEXT("%d XP gained"), (int32)BestXP));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateVeteranSurvivor(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // VeteranSurvivor: highest-level robot that survived
    const FMXRobotRunStats* Best = nullptr;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        if (RS.survived && (!Best || RS.robot_level > Best->robot_level))
            Best = &RS;
    }
    if (!Best || Best->robot_level < 10) return {};
    return MakeAward(EAwardCategory::VeteranSurvivor, *Best,
        FString::Printf(TEXT("Lvl %d"), Best->robot_level));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateBestDressed(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // BestDressed: hatted robot with the most events (most visible, most active)
    auto Acc = [](const FMXRobotRunStats& RS) {
        return (RS.hat_id >= 0) ? (float)RS.event_count : 0.0f;
    };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner || Winner->hat_id < 0 || Winner->event_count < 3) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout(Acc(*Winner), Avg)) return {};
    return MakeAward(EAwardCategory::BestDressed, *Winner,
        FString::Printf(TEXT("%s, %d events"), *Winner->hat_name, Winner->event_count));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateWorstDressed(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // WorstDressed: hatless robot with the most near-misses
    const FMXRobotRunStats* Best = nullptr;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        if (RS.hat_id < 0 && RS.near_misses > 0)
        {
            if (!Best || RS.near_misses > Best->near_misses)
                Best = &RS;
        }
    }
    if (!Best || Best->near_misses < 3) return {};
    return MakeAward(EAwardCategory::WorstDressed, *Best,
        FString::Printf(TEXT("%d hatless near-misses"), Best->near_misses));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateGuardianAngel(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    auto Acc = [](const FMXRobotRunStats& RS) { return (float)RS.lethal_hits_absorbed; };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner || Winner->lethal_hits_absorbed < 2) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout((float)Winner->lethal_hits_absorbed, Avg)) return {};
    return MakeAward(EAwardCategory::GuardianAngel, *Winner,
        FString::Printf(TEXT("%d hits absorbed"), Winner->lethal_hits_absorbed));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateTheMechanic(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    auto Acc = [](const FMXRobotRunStats& RS) { return (float)RS.revives_performed; };
    const FMXRobotRunStats* Winner = FindMax(Stats, Acc);
    if (!Winner || Winner->revives_performed < 2) return {};
    float Avg = ComputeAverage(Stats, Acc);
    if (!IsStandout((float)Winner->revives_performed, Avg)) return {};
    return MakeAward(EAwardCategory::TheMechanic, *Winner,
        FString::Printf(TEXT("%d revives"), Winner->revives_performed));
}

TOptional<FMXAward> UMXAwardSelector::EvaluateHatCasualty(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // HatCasualty: any robot that died wearing the rarest hat lost this run
    if (Stats.hats_lost == 0) return {};
    // Find a dead hatted robot with a notable hat
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        if (RS.died && RS.hat_id >= 0)
        {
            // Best candidate: if their hat name matches the rarest lost hat
            return MakeAward(EAwardCategory::HatCasualty, RS,
                FString::Printf(TEXT("%s lost forever"), *RS.hat_name));
        }
    }
    return {};
}

TOptional<FMXAward> UMXAwardSelector::EvaluateCoward(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& Roster) const
{
    // Coward: surviving robot with the lowest event count — i.e., stayed out of trouble
    TOptional<FMXAward> Tourist = EvaluateTourist(Stats, Roster);
    if (!Tourist.IsSet())
        return TOptional<FMXAward>{};
    FMXAward A = Tourist.GetValue();
    A.category = EAwardCategory::CowardOfTheRun;
    return A;
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

const FMXRobotRunStats* UMXAwardSelector::FindMax(
    const FMXCompiledStats& Stats,
    TFunctionRef<float(const FMXRobotRunStats&)> Accessor) const
{
    const FMXRobotRunStats* Best = nullptr;
    float BestVal = -FLT_MAX;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
    {
        float Val = Accessor(RS);
        if (Val > BestVal)
        {
            BestVal = Val;
            Best    = &RS;
        }
    }
    return Best;
}

float UMXAwardSelector::ComputeAverage(
    const FMXCompiledStats& Stats,
    TFunctionRef<float(const FMXRobotRunStats&)> Accessor) const
{
    if (Stats.robot_stats.IsEmpty()) return 0.0f;
    float Sum = 0.0f;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
        Sum += Accessor(RS);
    return Sum / (float)Stats.robot_stats.Num();
}

const FMXRobotProfile* UMXAwardSelector::FindProfile(
    const FGuid& RobotId,
    const TArray<FMXRobotProfile>& Roster) const
{
    for (const FMXRobotProfile& P : Roster)
        if (P.robot_id == RobotId) return &P;
    return nullptr;
}

FMXAward UMXAwardSelector::MakeAward(
    EAwardCategory Category,
    const FMXRobotRunStats& RS,
    const FString& StatValue) const
{
    FMXAward A;
    A.category    = Category;
    A.robot_id    = RS.robot_id;
    A.robot_name  = RS.robot_name;
    A.stat_value  = StatValue;
    A.narrative   = TEXT(""); // filled by ReportNarrator
    return A;
}
