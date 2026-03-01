// MXComboDetector.h — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Consumers: MXHatManager (internal — called at run end)
// Change Log:
//   v1.0 — Initial implementation: combo requirement checking, discovery filtering

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MXHatData.h"
#include "MXComboDetector.generated.h"

/**
 * UMXComboDetector
 * Detects hat combination discoveries at run end.
 *
 * Design:
 *   - Combo hats (#51–70) each carry a combo_requirements array in their FMXHatDefinition.
 *   - A combo is "triggered" when ALL required hat IDs appear in the set of hats equipped
 *     during the current run.
 *   - A combo is "discovered" only if it has never appeared in Collection.discovered_combo_ids.
 *   - On discovery: returns the combo hat ID to the caller (MXHatManager fires events).
 *
 * Called once per run (at run end, before DepositRunReward).
 */
UCLASS()
class MICROEXODUS_API UMXComboDetector : public UObject
{
    GENERATED_BODY()

public:

    /**
     * Evaluate all combo hat definitions against the set of hat IDs active during the run.
     * Returns newly discovered combo hat IDs (those whose requirements are satisfied and
     * which have not been previously discovered).
     *
     * @param EquippedHatIds   All hat IDs that were equipped on any robot during the run.
     * @param Collection       Current hat collection (used to filter out already-discovered combos).
     * @param HatDefinitions   Static hat definition map (indexed by hat_id).
     * @return                 Array of combo hat IDs newly discovered this run.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Combos")
    TArray<int32> CheckCombos(
        const TArray<int32>& EquippedHatIds,
        const FMXHatCollection& Collection,
        const TMap<int32, FMXHatDefinition>& HatDefinitions) const;

    /**
     * Returns true if a specific combo's requirements are satisfied by EquippedHatIds.
     * Public for unit-testing purposes.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Combos")
    bool IsComboSatisfied(
        const FMXHatDefinition& ComboDef,
        const TArray<int32>& EquippedHatIds) const;

private:

    // Combo hat_ids occupy range [51, 70] inclusive
    static constexpr int32 COMBO_HAT_ID_MIN = 51;
    static constexpr int32 COMBO_HAT_ID_MAX = 70;
};
