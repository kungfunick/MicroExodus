// MXAgingSystem.cpp â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity

#include "MXAgingSystem.h"

FTimespan UMXAgingSystem::ComputeCalendarAge(const FDateTime& DateOfBirth) const
{
    return FDateTime::Now() - DateOfBirth;
}

FString UMXAgingSystem::GetCalendarAgeDescription(const FDateTime& DateOfBirth) const
{
    FTimespan Age = ComputeCalendarAge(DateOfBirth);

    double TotalSeconds = Age.GetTotalSeconds();
    double TotalMinutes = Age.GetTotalMinutes();
    double TotalHours   = Age.GetTotalHours();
    double TotalDays    = Age.GetTotalDays();
    double TotalWeeks   = TotalDays / 7.0;
    double TotalMonths  = TotalDays / 30.44;
    double TotalYears   = TotalDays / 365.25;

    if (TotalSeconds < 60.0)
    {
        return FString::Printf(TEXT("%d seconds old"), FMath::FloorToInt(TotalSeconds));
    }
    else if (TotalMinutes < 60.0)
    {
        int32 Mins = FMath::FloorToInt(TotalMinutes);
        return FString::Printf(TEXT("%d %s old"), Mins, Mins == 1 ? TEXT("minute") : TEXT("minutes"));
    }
    else if (TotalHours < 24.0)
    {
        int32 Hrs = FMath::FloorToInt(TotalHours);
        return FString::Printf(TEXT("%d %s old"), Hrs, Hrs == 1 ? TEXT("hour") : TEXT("hours"));
    }
    else if (TotalDays < 7.0)
    {
        int32 Days = FMath::FloorToInt(TotalDays);
        return FString::Printf(TEXT("%d %s old"), Days, Days == 1 ? TEXT("day") : TEXT("days"));
    }
    else if (TotalWeeks < 4.0)
    {
        int32 Weeks = FMath::FloorToInt(TotalWeeks);
        return FString::Printf(TEXT("%d %s old"), Weeks, Weeks == 1 ? TEXT("week") : TEXT("weeks"));
    }
    else if (TotalMonths < 12.0)
    {
        int32 Months = FMath::FloorToInt(TotalMonths);
        return FString::Printf(TEXT("%d %s old"), Months, Months == 1 ? TEXT("month") : TEXT("months"));
    }
    else
    {
        int32 Years = FMath::FloorToInt(TotalYears);
        return FString::Printf(TEXT("%d %s old"), Years, Years == 1 ? TEXT("year") : TEXT("years"));
    }
}

void UMXAgingSystem::RegisterRobot(const FGuid& RobotId, float InitialSeconds)
{
    if (!RobotId.IsValid()) return;
    ActiveAgeMap.FindOrAdd(RobotId) = InitialSeconds;
}

void UMXAgingSystem::UnregisterRobot(const FGuid& RobotId)
{
    ActiveAgeMap.Remove(RobotId);
    DeployedRobotIds.Remove(RobotId);
}

void UMXAgingSystem::SetActiveDeployment(const TArray<FGuid>& ActiveRobotIds)
{
    DeployedRobotIds.Empty();
    for (const FGuid& Id : ActiveRobotIds)
    {
        if (Id.IsValid())
        {
            DeployedRobotIds.Add(Id);
        }
    }
}

void UMXAgingSystem::TickActiveAge(float DeltaTime)
{
    if (DeltaTime <= 0.0f) return;

    for (const FGuid& Id : DeployedRobotIds)
    {
        float* Age = ActiveAgeMap.Find(Id);
        if (Age)
        {
            *Age += DeltaTime;
        }
        else
        {
            // Robot was deployed but not registered â€” register with 0 baseline
            ActiveAgeMap.Add(Id, DeltaTime);
        }
    }
}

float UMXAgingSystem::GetActiveAgeSeconds(const FGuid& RobotId) const
{
    const float* Age = ActiveAgeMap.Find(RobotId);
    return Age ? *Age : 0.0f;
}

FString UMXAgingSystem::GetActiveAgeDescription(const FGuid& RobotId) const
{
    float TotalSeconds = GetActiveAgeSeconds(RobotId);

    if (TotalSeconds < 60.0f)
    {
        return FString::Printf(TEXT("%.0f seconds in the field"), TotalSeconds);
    }
    else if (TotalSeconds < 3600.0f)
    {
        int32 Mins = FMath::FloorToInt(TotalSeconds / 60.0f);
        return FString::Printf(TEXT("%d %s deployed"), Mins, Mins == 1 ? TEXT("minute") : TEXT("minutes"));
    }
    else if (TotalSeconds < 86400.0f)
    {
        int32 Hrs = FMath::FloorToInt(TotalSeconds / 3600.0f);
        return FString::Printf(TEXT("%d %s of service"), Hrs, Hrs == 1 ? TEXT("hour") : TEXT("hours"));
    }
    else
    {
        int32 Days = FMath::FloorToInt(TotalSeconds / 86400.0f);
        return FString::Printf(TEXT("%d %s in the field"), Days, Days == 1 ? TEXT("day") : TEXT("days"));
    }
}

TMap<FGuid, float> UMXAgingSystem::FlushActiveAges() const
{
    return ActiveAgeMap;
}
