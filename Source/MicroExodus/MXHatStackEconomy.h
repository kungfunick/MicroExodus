// MXHatStackEconomy.h — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Consumers: MXHatManager (internal)
// Change Log:
//   v1.0 — Initial implementation: stack increment/decrement, scarcity queries

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MXHatData.h"
#include "MXTypes.h"
#include "MXHatStackEconomy.generated.h"

/**
 * FMXScarcityRecord
 * Summarizes the current stack state for a single hat — used in scarcity reports.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXScarcityRecord
{
    GENERATED_BODY()

    /** Hat ID this record represents. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Hats|Scarcity")
    int32 hat_id = -1;

    /** Current stack count (copies in collection). */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Hats|Scarcity")
    int32 count = 0;

    /** Rarity tier from the static hat definition. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Hats|Scarcity")
    EHatRarity rarity = EHatRarity::Common;
};

/**
 * UMXHatStackEconomy
 * Handles all stack arithmetic for the hat collection.
 *
 * Rules enforced:
 *   - Stack count is always in [0, MAX_HAT_STACK] (clamped, never negative, never overflows cap)
 *   - DecrementStack returns false without mutating state if count would go below 0
 *   - IncrementStack silently caps at MAX_HAT_STACK and logs a warning on overflow attempt
 *
 * Owned by UMXHatManager. Operates on the FMXHatCollection passed by reference.
 * Not independently persisted — collection is the source of truth.
 */
UCLASS()
class MICROEXODUS_API UMXHatStackEconomy : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Core Stack Operations
    // -------------------------------------------------------------------------

    /**
     * Get the current stack count for a hat. Returns 0 if no entry exists.
     * @param HatId       The hat to query.
     * @param Collection  The collection to read from.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Economy")
    int32 GetStackCount(int32 HatId, const FMXHatCollection& Collection) const;

    /**
     * Add copies to a hat's stack. Caps at MAX_HAT_STACK.
     * Creates a new stack entry if none exists. Safe to call with Amount=0.
     * @param HatId       The hat to increment.
     * @param Amount      Copies to add (default 1).
     * @param Collection  The collection to mutate.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Economy")
    void IncrementStack(int32 HatId, int32 Amount, FMXHatCollection& Collection);

    /**
     * Remove copies from a hat's stack.
     * Returns false without mutating if insufficient copies are available.
     * @param HatId       The hat to decrement.
     * @param Amount      Copies to remove (default 1).
     * @param Collection  The collection to mutate.
     * @return            True if decrement succeeded; false if stack was insufficient.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Economy")
    bool DecrementStack(int32 HatId, int32 Amount, FMXHatCollection& Collection);

    /**
     * Ensure a stack entry exists for HatId with at least count=0.
     * Used when a hat is first unlocked to guarantee the entry appears in iteration.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Economy")
    void EnsureStackEntry(int32 HatId, FMXHatCollection& Collection);

    // -------------------------------------------------------------------------
    // Scarcity Queries
    // -------------------------------------------------------------------------

    /**
     * Among the given hat IDs (e.g., all currently equipped), return the one with the
     * lowest stack count. Useful for UI warnings and the Reports hat census.
     * Returns -1 if EquippedHatIds is empty.
     * @param EquippedHatIds  The set of hat IDs to compare.
     * @param Collection      The collection to read counts from.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Economy")
    int32 GetRarestEquippedHat(const TArray<int32>& EquippedHatIds, const FMXHatCollection& Collection) const;

    /**
     * Returns the sum of all stack counts across the entire collection.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Economy")
    int32 GetTotalHatCopies(const FMXHatCollection& Collection) const;

    /**
     * Builds a scarcity report: one FMXScarcityRecord per unlocked hat.
     * Records are sorted by count ascending (scarcest first).
     * @param Collection      The collection to read from.
     * @param HatDefinitions  The static definition map for rarity lookups.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Economy")
    TArray<FMXScarcityRecord> GetScarcityReport(
        const FMXHatCollection& Collection,
        const TMap<int32, FMXHatDefinition>& HatDefinitions) const;

private:

    /** Locate the FMXHatStack entry in Collection.stacks for HatId. Returns nullptr if absent. */
    static FMXHatStack* FindStack(int32 HatId, FMXHatCollection& Collection);
    static const FMXHatStack* FindStackConst(int32 HatId, const FMXHatCollection& Collection);
};
