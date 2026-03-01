// MXPersonalityGenerator.cpp â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity

#include "MXPersonalityGenerator.h"
#include "Engine/DataTable.h"
#include "Math/UnrealMathUtility.h"

void UMXPersonalityGenerator::LoadFromDataTable(UDataTable* PersonalitiesTable)
{
    if (!PersonalitiesTable)
    {
        UE_LOG(LogTemp, Error, TEXT("MXPersonalityGenerator: PersonalitiesTable is null."));
        return;
    }

    Archetypes.Empty();

    TArray<FMXPersonalityTableRow*> Rows;
    PersonalitiesTable->GetAllRows<FMXPersonalityTableRow>(TEXT("MXPersonalityGenerator"), Rows);

    for (const FMXPersonalityTableRow* Row : Rows)
    {
        if (!Row || Row->Description.IsEmpty()) continue;

        FParsedArchetype Parsed;
        Parsed.Description = Row->Description;
        Parsed.Likes       = ParsePool(Row->LikesPool);
        Parsed.Dislikes    = ParsePool(Row->DislikesPool);
        Parsed.Quirks      = ParsePool(Row->QuirksPool);

        Archetypes.Add(Parsed);
    }

    UE_LOG(LogTemp, Log, TEXT("MXPersonalityGenerator: Loaded %d archetypes."), Archetypes.Num());
}

FMXGeneratedPersonality UMXPersonalityGenerator::GeneratePersonality()
{
    FMXGeneratedPersonality Out;

    if (Archetypes.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("MXPersonalityGenerator: No archetypes loaded. Returning default personality."));
        Out.Description = TEXT("stoic wanderer");
        Out.Likes       = { TEXT("silence"), TEXT("efficiency") };
        Out.Dislikes    = { TEXT("clutter") };
        Out.Quirk       = TEXT("rarely speaks first");
        return Out;
    }

    // Pick a random archetype
    int32 ArchetypeIndex = FMath::RandRange(0, Archetypes.Num() - 1);
    const FParsedArchetype& Archetype = Archetypes[ArchetypeIndex];

    Out.Description = Archetype.Description;

    // Pick 2â€“3 likes
    int32 LikeCount = FMath::RandRange(2, 3);
    Out.Likes = SelectRandom(Archetype.Likes, LikeCount);

    // Pick 1â€“2 dislikes
    int32 DislikeCount = FMath::RandRange(1, 2);
    Out.Dislikes = SelectRandom(Archetype.Dislikes, DislikeCount);

    // Pick exactly 1 quirk
    TArray<FString> Quirks = SelectRandom(Archetype.Quirks, 1);
    Out.Quirk = Quirks.IsEmpty() ? TEXT("") : Quirks[0];

    return Out;
}

int32 UMXPersonalityGenerator::GetArchetypeCount() const
{
    return Archetypes.Num();
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

TArray<FString> UMXPersonalityGenerator::ParsePool(const FString& Input)
{
    TArray<FString> Out;
    if (Input.IsEmpty()) return Out;

    TArray<FString> Parts;
    Input.ParseIntoArray(Parts, TEXT(","), true);

    for (const FString& Part : Parts)
    {
        FString Trimmed = Part.TrimStartAndEnd();
        if (!Trimmed.IsEmpty())
        {
            Out.Add(Trimmed);
        }
    }

    return Out;
}

TArray<FString> UMXPersonalityGenerator::SelectRandom(const TArray<FString>& Pool, int32 Count)
{
    TArray<FString> Out;
    if (Pool.IsEmpty() || Count <= 0) return Out;

    // Clamp count to pool size
    int32 ActualCount = FMath::Min(Count, Pool.Num());

    // Shuffle a copy of the pool and take the first N
    TArray<FString> Shuffled = Pool;
    for (int32 i = Shuffled.Num() - 1; i > 0; i--)
    {
        int32 j = FMath::RandRange(0, i);
        Shuffled.Swap(i, j);
    }

    for (int32 i = 0; i < ActualCount; i++)
    {
        Out.Add(Shuffled[i]);
    }

    return Out;
}
