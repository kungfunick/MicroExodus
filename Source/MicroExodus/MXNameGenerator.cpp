// MXNameGenerator.cpp â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity

#include "MXNameGenerator.h"
#include "Engine/DataTable.h"
#include "Math/UnrealMathUtility.h"

void UMXNameGenerator::LoadFromDataTable(UDataTable* NamesTable)
{
    if (!NamesTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MXNameGenerator: NamesTable is null."));
        return;
    }

    AvailableNames.Empty();
    InUseNames.Empty();
    CooldownNames.Empty();

    // Iterate all rows and extract the Name field
    TArray<FMXNameTableRow*> Rows;
    NamesTable->GetAllRows<FMXNameTableRow>(TEXT("MXNameGenerator"), Rows);

    for (const FMXNameTableRow* Row : Rows)
    {
        if (Row && !Row->Name.IsEmpty())
        {
            AvailableNames.Add(Row->Name);
        }
    }

    TotalPoolSize = AvailableNames.Num();
    UE_LOG(LogTemp, Log, TEXT("MXNameGenerator: Loaded %d names from DataTable."), TotalPoolSize);
}

FString UMXNameGenerator::GenerateName()
{
    if (AvailableNames.IsEmpty())
    {
        // Emergency fallback â€” should rarely/never happen with 230+ names and MAX_ROBOTS=100
        FString Fallback = FString::Printf(TEXT("Unit_%d"), FMath::RandRange(1000, 9999));
        UE_LOG(LogTemp, Warning, TEXT("MXNameGenerator: Name pool exhausted! Using fallback: %s"), *Fallback);
        InUseNames.Add(Fallback);
        return Fallback;
    }

    // Pick a random index from the available pool
    int32 Index = FMath::RandRange(0, AvailableNames.Num() - 1);
    FString ChosenName = AvailableNames[Index];

    // Move from available â†’ in-use
    AvailableNames.RemoveAt(Index);
    InUseNames.Add(ChosenName);

    return ChosenName;
}

void UMXNameGenerator::ReleaseName(const FString& Name, int32 CurrentRunNumber)
{
    if (!InUseNames.Contains(Name))
    {
        UE_LOG(LogTemp, Warning, TEXT("MXNameGenerator::ReleaseName â€” '%s' not found in InUse set."), *Name);
        return;
    }

    InUseNames.Remove(Name);

    // Place in cooldown queue
    FMXNameCooldownEntry Entry;
    Entry.Name        = Name;
    Entry.ReleasedOnRun = CurrentRunNumber;
    Entry.ReleasedAt  = FDateTime::Now();
    CooldownNames.Add(Entry);

    UE_LOG(LogTemp, Log, TEXT("MXNameGenerator: '%s' released, on cooldown until run %d or %d days."),
           *Name, CurrentRunNumber + RunCooldown, DayCooldown);
}

bool UMXNameGenerator::IsNameAvailable(const FString& Name) const
{
    return AvailableNames.Contains(Name);
}

void UMXNameGenerator::TickCooldowns(int32 CurrentRunNumber)
{
    FDateTime Now = FDateTime::Now();

    TArray<FMXNameCooldownEntry> StillOnCooldown;
    int32 Restored = 0;

    for (const FMXNameCooldownEntry& Entry : CooldownNames)
    {
        bool bRunExpired  = (CurrentRunNumber - Entry.ReleasedOnRun) >= RunCooldown;
        bool bDaysExpired = (Now - Entry.ReleasedAt).GetTotalDays() >= static_cast<double>(DayCooldown);

        if (bRunExpired || bDaysExpired)
        {
            AvailableNames.Add(Entry.Name);
            Restored++;
        }
        else
        {
            StillOnCooldown.Add(Entry);
        }
    }

    CooldownNames = StillOnCooldown;

    if (Restored > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("MXNameGenerator: %d name(s) restored from cooldown. Pool: %d available."),
               Restored, AvailableNames.Num());
    }
}

int32 UMXNameGenerator::GetAvailableNameCount() const
{
    return AvailableNames.Num();
}

int32 UMXNameGenerator::GetTotalPoolSize() const
{
    return TotalPoolSize;
}
