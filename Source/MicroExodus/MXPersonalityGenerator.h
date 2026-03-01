// MXPersonalityGenerator.h â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "MXPersonalityGenerator.generated.h"

/**
 * FMXPersonalityTableRow
 * DataTable row type for DT_Personalities.csv.
 * Each row is one personality archetype with pools of likes, dislikes, and quirks.
 * Comma-separated strings in CSV are parsed into arrays at load time.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXPersonalityTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Short archetype label (e.g., "cautious optimist"). Used as the robot's description. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    /**
     * Comma-separated list of things this archetype "likes".
     * Stored as a single string in CSV; parsed into individual items at load time.
     * Example: "sunlight, routine, warm floors"
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LikesPool;

    /**
     * Comma-separated list of things this archetype "dislikes".
     * Example: "loud noises, surprises"
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DislikesPool;

    /**
     * Comma-separated list of defining quirks for this archetype.
     * One is selected at birth to become the robot's permanent quirk.
     * Example: "always checks behind them, hums while working"
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString QuirksPool;
};

/**
 * FMXGeneratedPersonality
 * Output bundle from a single GeneratePersonality() call.
 * Contains all personality fields ready to be written to FMXRobotProfile.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXGeneratedPersonality
{
    GENERATED_BODY()

    /** Archetype description (e.g., "cautious optimist"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    /** 2â€“3 randomly chosen likes from the archetype's pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Likes;

    /** 1â€“2 randomly chosen dislikes from the archetype's pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Dislikes;

    /** 1 randomly chosen quirk from the archetype's pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Quirk;
};

/**
 * UMXPersonalityGenerator
 * Procedurally generates robot personalities from the DT_Personalities DataTable.
 * Each call to GeneratePersonality() returns a unique combination even within the same archetype.
 * Personalities are immutable after birth.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXPersonalityGenerator : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Loads all personality archetypes from the provided DataTable.
     * Must be called once before GeneratePersonality().
     * @param PersonalitiesTable    Reference to DT_Personalities DataTable asset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Personality")
    void LoadFromDataTable(UDataTable* PersonalitiesTable);

    // -------------------------------------------------------------------------
    // Generation
    // -------------------------------------------------------------------------

    /**
     * Generates a new personality by:
     * 1. Selecting a random archetype from the loaded pool.
     * 2. Picking 2â€“3 likes, 1â€“2 dislikes, and 1 quirk from the archetype's sub-pools.
     * @return  A fully populated FMXGeneratedPersonality ready for profile assignment.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Personality")
    FMXGeneratedPersonality GeneratePersonality();

    /**
     * Returns the number of loaded personality archetypes.
     * Useful for diagnostics.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Personality")
    int32 GetArchetypeCount() const;

private:

    /**
     * Parsed personality data â€” converted from raw DataTable strings at load time.
     * Each entry has pre-split TArrays ready for random selection.
     */
    struct FParsedArchetype
    {
        FString Description;
        TArray<FString> Likes;
        TArray<FString> Dislikes;
        TArray<FString> Quirks;
    };

    /** Parsed archetypes loaded from the DataTable. */
    TArray<FParsedArchetype> Archetypes;

    /**
     * Splits a comma-separated string into a trimmed FString array.
     * @param Input     Raw comma-separated string.
     * @return          Array of trimmed, non-empty strings.
     */
    static TArray<FString> ParsePool(const FString& Input);

    /**
     * Selects N random unique items from a pool, clamped to pool size.
     * @param Pool      Source array to select from.
     * @param Count     Desired number of items.
     * @return          Array of randomly selected items.
     */
    static TArray<FString> SelectRandom(const TArray<FString>& Pool, int32 Count);
};
