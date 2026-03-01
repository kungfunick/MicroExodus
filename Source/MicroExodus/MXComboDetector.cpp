// MXComboDetector.cpp — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Change Log:
//   v1.0 — Initial implementation

#include "MXComboDetector.h"

TArray<int32> UMXComboDetector::CheckCombos(
    const TArray<int32>& EquippedHatIds,
    const FMXHatCollection& Collection,
    const TMap<int32, FMXHatDefinition>& HatDefinitions) const
{
    TArray<int32> NewlyDiscovered;

    // Iterate over the known combo hat ID range
    for (int32 ComboHatId = COMBO_HAT_ID_MIN; ComboHatId <= COMBO_HAT_ID_MAX; ++ComboHatId)
    {
        // Skip already-discovered combos
        if (Collection.discovered_combo_ids.Contains(ComboHatId))
        {
            continue;
        }

        // Find the static definition
        const FMXHatDefinition* Def = HatDefinitions.Find(ComboHatId);
        if (!Def || !Def->is_combo)
        {
            continue;
        }

        // Check if the combo requirements are satisfied
        if (IsComboSatisfied(*Def, EquippedHatIds))
        {
            NewlyDiscovered.Add(ComboHatId);
            UE_LOG(LogTemp, Log, TEXT("UMXComboDetector: Combo hat %d ('%s') discovered!"),
                ComboHatId, *Def->name);
        }
    }

    return NewlyDiscovered;
}

bool UMXComboDetector::IsComboSatisfied(
    const FMXHatDefinition& ComboDef,
    const TArray<int32>& EquippedHatIds) const
{
    if (ComboDef.combo_requirements.IsEmpty())
    {
        return false;
    }

    // ALL required hat IDs must appear in EquippedHatIds
    for (int32 RequiredId : ComboDef.combo_requirements)
    {
        if (!EquippedHatIds.Contains(RequiredId))
        {
            return false;
        }
    }

    return true;
}
