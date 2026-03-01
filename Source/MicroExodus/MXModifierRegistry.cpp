// MXModifierRegistry.cpp — Roguelike Module v1.0
// Agent 6: Roguelike

#include "MXModifierRegistry.h"
#include "MXRunData.h"
#include "Engine/DataTable.h"

UMXModifierRegistry::UMXModifierRegistry()
{
    // Overcharge is unlocked by default — the first modifier players can choose.
    UnlockedModifiers.Add(TEXT("Overcharge"));
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXModifierRegistry::InitialiseFromDataTable(UDataTable* DataTable)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXModifierRegistry: Null DataTable passed to InitialiseFromDataTable."));
        return;
    }

    ModifierDefinitions.Empty();

    TArray<FMXModifierRow*> Rows;
    DataTable->GetAllRows<FMXModifierRow>(TEXT("MXModifierRegistry"), Rows);

    for (const FMXModifierRow* Row : Rows)
    {
        if (Row && !Row->ModifierName.IsEmpty())
        {
            ModifierDefinitions.Add(Row->ModifierName, *Row);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXModifierRegistry: Loaded %d modifier definitions."), ModifierDefinitions.Num());
}

// ---------------------------------------------------------------------------
// Modifier Queries
// ---------------------------------------------------------------------------

bool UMXModifierRegistry::IsModifierUnlocked(const FString& ModifierName) const
{
    return UnlockedModifiers.Contains(ModifierName);
}

float UMXModifierRegistry::GetModifierXPBonus(const FString& ModifierName) const
{
    if (const FMXModifierRow* Row = FindModifier(ModifierName))
    {
        return Row->XPBonus;
    }
    return 0.0f;
}

FMXModifierEffects UMXModifierRegistry::GetActiveModifierEffects(const TArray<FString>& ActiveModifiers) const
{
    FMXModifierEffects Effects;

    for (const FString& Name : ActiveModifiers)
    {
        const FMXModifierRow* Row = FindModifier(Name);
        if (!Row)
        {
            UE_LOG(LogTemp, Warning, TEXT("MXModifierRegistry: Unknown modifier '%s' — skipping."), *Name);
            continue;
        }

        // Speed multipliers compound.
        Effects.SpeedMultiplier *= Row->SpeedMult;

        // Most restrictive MaxRobots override wins.
        if (Row->MaxRobots > 0)
        {
            if (Effects.MaxRobotOverride < 0 || Row->MaxRobots < Effects.MaxRobotOverride)
            {
                Effects.MaxRobotOverride = Row->MaxRobots;
            }
        }

        // Boolean flags — any active modifier activating them wins.
        if (Row->OneHit)          Effects.bOneHitKill         = true;
        if (Row->NoRescues)       Effects.bNoRescues          = true;
        if (Row->RandomHazards)   Effects.bRandomHazards      = true;
        if (Row->NoSpecBonuses)   Effects.bNoSpecBonuses      = true;
        if (Row->UnlimitedHats)   Effects.bUnlimitedHatSlots  = true;
        if (Row->HatBonuses)      Effects.bHatStatBonuses     = true;

        // Visibility: multiply together so both Graveyard Shift and tier fog stack.
        Effects.VisibilityMultiplier *= Row->ReducedVisibility;

        // XP bonuses accumulate.
        Effects.CombinedXPBonus += Row->XPBonus;
    }

    return Effects;
}

FString UMXModifierRegistry::GetModifierDescription(const FString& ModifierName) const
{
    if (const FMXModifierRow* Row = FindModifier(ModifierName))
    {
        return Row->Description;
    }
    return FString();
}

TArray<FString> UMXModifierRegistry::GetAllModifierNames() const
{
    TArray<FString> Names;
    ModifierDefinitions.GetKeys(Names);
    return Names;
}

TArray<FString> UMXModifierRegistry::GetUnlockedModifierNames() const
{
    TArray<FString> Result;
    for (const FString& Name : UnlockedModifiers)
    {
        Result.Add(Name);
    }
    return Result;
}

// ---------------------------------------------------------------------------
// Modifier Progression
// ---------------------------------------------------------------------------

void UMXModifierRegistry::UnlockModifier(const FString& ModifierName)
{
    if (ModifierDefinitions.Contains(ModifierName))
    {
        UnlockedModifiers.Add(ModifierName);
        UE_LOG(LogTemp, Log, TEXT("MXModifierRegistry: Unlocked modifier '%s'."), *ModifierName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MXModifierRegistry: Cannot unlock unknown modifier '%s'."), *ModifierName);
    }
}

void UMXModifierRegistry::EvaluateUnlocks(const FMXRunData& CompletedRunData, int32 TotalRunCount)
{
    // --- Unlock progression tied to run count and outcomes ---

    // Skeleton Crew: complete 3 runs on any tier.
    if (TotalRunCount >= 3)
    {
        UnlockModifier(TEXT("SkeletonCrew"));
    }

    // Glass Cannon: complete 5 runs.
    if (TotalRunCount >= 5)
    {
        UnlockModifier(TEXT("GlassCannon"));
    }

    // Chaos Mode: complete 2 runs on Hardened or higher.
    if (static_cast<int32>(CompletedRunData.tier) >= 1 && TotalRunCount >= 2)
    {
        UnlockModifier(TEXT("ChaosMode"));
    }

    // Iron Swarm: complete a run with 0 robots rescued.
    if (CompletedRunData.robots_rescued == 0 &&
        CompletedRunData.outcome == ERunOutcome::Success)
    {
        UnlockModifier(TEXT("IronSwarm"));
    }

    // Graveyard Shift: complete a Brutal or higher run.
    if (static_cast<int32>(CompletedRunData.tier) >= 2 &&
        CompletedRunData.outcome == ERunOutcome::Success)
    {
        UnlockModifier(TEXT("GraveyardShift"));
    }

    // Legendary Escort: complete a Nightmare or higher run.
    if (static_cast<int32>(CompletedRunData.tier) >= 3 &&
        CompletedRunData.outcome == ERunOutcome::Success)
    {
        UnlockModifier(TEXT("LegendaryEscort"));
    }

    // Naked Run: complete 10 runs total.
    if (TotalRunCount >= 10)
    {
        UnlockModifier(TEXT("NakedRun"));
    }

    // Hat Parade: complete any run with at least 20 hatted robots surviving.
    if (CompletedRunData.robots_survived >= 20 &&
        CompletedRunData.outcome == ERunOutcome::Success)
    {
        UnlockModifier(TEXT("HatParade"));
    }

    // Fashionista: unlock after Hat Parade is discovered (check by run count proxy).
    if (TotalRunCount >= 7)
    {
        UnlockModifier(TEXT("Fashionista"));
    }
}

// ---------------------------------------------------------------------------
// Persistence Helpers
// ---------------------------------------------------------------------------

TSet<FString> UMXModifierRegistry::GetUnlockedModifierSet() const
{
    return UnlockedModifiers;
}

void UMXModifierRegistry::SetUnlockedModifierSet(const TSet<FString>& RestoredSet)
{
    UnlockedModifiers = RestoredSet;
    // Overcharge is always unlocked.
    UnlockedModifiers.Add(TEXT("Overcharge"));
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

const FMXModifierRow* UMXModifierRegistry::FindModifier(const FString& ModifierName) const
{
    return ModifierDefinitions.Find(ModifierName);
}
