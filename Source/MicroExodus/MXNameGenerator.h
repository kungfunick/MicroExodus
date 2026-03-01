// MXNameGenerator.h â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "MXNameGenerator.generated.h"

/**
 * FMXNameTableRow
 * DataTable row type for DT_Names.csv.
 * One row per candidate robot name.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXNameTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** The robot name string. Must be unique within the DataTable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;
};

/**
 * FMXNameCooldownEntry
 * Tracks a released name and when it becomes available again.
 * A name is re-eligible once either 10 runs have passed OR 7 real-world days have elapsed.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXNameCooldownEntry
{
    GENERATED_BODY()

    /** The name on cooldown. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    /** The run number when the owning robot died. Used for run-count cooldown. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ReleasedOnRun = 0;

    /** Real-world timestamp when the owning robot died. Used for day-count cooldown. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime ReleasedAt;
};

/**
 * UMXNameGenerator
 * Manages the pool of 230+ robot names loaded from DT_Names.csv.
 * Guarantees no two living roster robots share a name.
 * Names released on death enter a cooldown before returning to the available pool.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXNameGenerator : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** Number of runs that must pass before a released name is eligible again. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Names")
    int32 RunCooldown = 10;

    /** Number of real-world days before a released name is eligible again (whichever comes first). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Names")
    int32 DayCooldown = 7;

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Loads the name pool from the provided DataTable asset.
     * Must be called once after construction before any GenerateName() calls.
     * @param NamesTable    Reference to DT_Names DataTable asset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names")
    void LoadFromDataTable(UDataTable* NamesTable);

    // -------------------------------------------------------------------------
    // Name Management
    // -------------------------------------------------------------------------

    /**
     * Selects and returns a random available name, marking it as in-use.
     * Returns "Unit_[GUID]" as an emergency fallback if the pool is exhausted.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names")
    FString GenerateName();

    /**
     * Releases a name from active use and places it in the cooldown queue.
     * Called when a robot dies.
     * @param Name              The name to release.
     * @param CurrentRunNumber  The current run number, for cooldown tracking.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names")
    void ReleaseName(const FString& Name, int32 CurrentRunNumber);

    /**
     * Returns true if the given name is in the available pool (not in-use and not on cooldown).
     * @param Name  Name to check.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names")
    bool IsNameAvailable(const FString& Name) const;

    /**
     * Processes the cooldown queue, moving names whose cooldown has expired back to the available pool.
     * Call once per run start.
     * @param CurrentRunNumber  The run number just started.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names")
    void TickCooldowns(int32 CurrentRunNumber);

    /**
     * Returns the count of currently available names in the pool.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names")
    int32 GetAvailableNameCount() const;

    /**
     * Returns the total pool size (including in-use and cooldown names).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names")
    int32 GetTotalPoolSize() const;

private:

    /** Names currently available for assignment. */
    TArray<FString> AvailableNames;

    /** Names currently assigned to living robots. */
    TSet<FString> InUseNames;

    /** Names released on robot death, pending cooldown expiry. */
    TArray<FMXNameCooldownEntry> CooldownNames;

    /** Total pool (for reporting). Immutable after LoadFromDataTable(). */
    int32 TotalPoolSize = 0;
};
