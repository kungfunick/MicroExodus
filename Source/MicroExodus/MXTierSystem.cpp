// MXTierSystem.cpp — Roguelike Module v1.0
// Agent 6: Roguelike

#include "MXTierSystem.h"
#include "MXConstants.h"

UMXTierSystem::UMXTierSystem()
{
    // Initialise all tiers as locked; Standard is always available.
    UnlockedTiers.SetNum(MXConstants::MAX_TIERS);
    for (bool& Flag : UnlockedTiers)
    {
        Flag = false;
    }
    // Standard tier (index 0) is permanently unlocked from the start.
    UnlockedTiers[0] = true;
}

// ---------------------------------------------------------------------------
// Tier Queries
// ---------------------------------------------------------------------------

bool UMXTierSystem::IsTierUnlocked(ETier Tier) const
{
    const int32 Index = static_cast<int32>(Tier);
    if (!UnlockedTiers.IsValidIndex(Index))
    {
        return false;
    }
    return UnlockedTiers[Index];
}

float UMXTierSystem::GetTierMultiplier(ETier Tier) const
{
    const int32 Index = static_cast<int32>(Tier);
    if (Index >= 0 && Index < MXConstants::MAX_TIERS)
    {
        return MXConstants::TIER_MULTIPLIERS[Index];
    }
    return 1.0f;
}

FMXTierEffects UMXTierSystem::GetTierEffects(ETier Tier) const
{
    return BuildTierEffects(Tier);
}

FString UMXTierSystem::GetTierDisplayName(ETier Tier) const
{
    switch (Tier)
    {
    case ETier::Standard:   return TEXT("Standard");
    case ETier::Hardened:   return TEXT("Hardened");
    case ETier::Brutal:     return TEXT("Brutal");
    case ETier::Nightmare:  return TEXT("Nightmare");
    case ETier::Extinction: return TEXT("Extinction");
    case ETier::Legendary:  return TEXT("Legendary");
    default:                return TEXT("Unknown");
    }
}

// ---------------------------------------------------------------------------
// Tier Progression
// ---------------------------------------------------------------------------

void UMXTierSystem::UnlockTier(ETier Tier)
{
    const int32 Index = static_cast<int32>(Tier);
    if (UnlockedTiers.IsValidIndex(Index))
    {
        UnlockedTiers[Index] = true;
    }
}

void UMXTierSystem::EvaluateAndUnlockNextTier(ETier CompletedTier, int32 SurvivingRobotCount)
{
    // Legendary requires completing Extinction with 50+ surviving robots.
    // All other tiers simply require completing the previous tier.
    const int32 CompletedIndex = static_cast<int32>(CompletedTier);
    const int32 NextIndex      = CompletedIndex + 1;

    if (!UnlockedTiers.IsValidIndex(NextIndex))
    {
        // Already at the highest tier — nothing to unlock.
        return;
    }

    // Special condition: Legendary unlock requires 50+ survivors on Extinction.
    if (CompletedTier == ETier::Extinction)
    {
        if (SurvivingRobotCount >= 50)
        {
            UnlockedTiers[NextIndex] = true;
            UE_LOG(LogTemp, Log, TEXT("MXTierSystem: Legendary tier unlocked (%d survivors)."), SurvivingRobotCount);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("MXTierSystem: Legendary requires 50 survivors; only %d this run."), SurvivingRobotCount);
        }
        return;
    }

    // Standard unlock: completing any lower tier opens the next.
    UnlockedTiers[NextIndex] = true;
    UE_LOG(LogTemp, Log, TEXT("MXTierSystem: Tier %d unlocked after completing tier %d."), NextIndex, CompletedIndex);
}

uint8 UMXTierSystem::GetUnlockedTiersMask() const
{
    uint8 Mask = 0;
    for (int32 i = 0; i < UnlockedTiers.Num() && i < 8; ++i)
    {
        if (UnlockedTiers[i])
        {
            Mask |= (1 << i);
        }
    }
    return Mask;
}

void UMXTierSystem::SetUnlockedTiersMask(uint8 Mask)
{
    for (int32 i = 0; i < UnlockedTiers.Num() && i < 8; ++i)
    {
        UnlockedTiers[i] = (Mask & (1 << i)) != 0;
    }
    // Standard is always unlocked regardless of save data.
    if (UnlockedTiers.IsValidIndex(0))
    {
        UnlockedTiers[0] = true;
    }
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

FMXTierEffects UMXTierSystem::BuildTierEffects(ETier Tier) const
{
    FMXTierEffects Effects;

    switch (Tier)
    {
    case ETier::Standard:
        // Tier 0 — baseline, no modifications.
        Effects.HazardSpeedMultiplier = 1.0f;
        Effects.SpawnReduction        = 0.0f;
        Effects.PathWidthMultiplier   = 1.0f;
        Effects.bFogOfWar             = false;
        Effects.bRandomHazards        = false;
        Effects.bOneHitSubLevel15     = false;
        Effects.bPermadeathOnFirstLoss = false;
        break;

    case ETier::Hardened:
        // Tier 1 — faster hazards, one extra hazard per room (reflected in speed).
        Effects.HazardSpeedMultiplier = 1.3f;
        Effects.SpawnReduction        = 0.0f;
        Effects.PathWidthMultiplier   = 1.0f;
        Effects.bFogOfWar             = false;
        Effects.bRandomHazards        = false;
        Effects.bOneHitSubLevel15     = false;
        Effects.bPermadeathOnFirstLoss = false;
        break;

    case ETier::Brutal:
        // Tier 2 — narrower paths, tighter rescue windows (fewer spawns).
        Effects.HazardSpeedMultiplier = 1.6f;
        Effects.SpawnReduction        = 0.25f;
        Effects.PathWidthMultiplier   = 0.75f;
        Effects.bFogOfWar             = false;
        Effects.bRandomHazards        = false;
        Effects.bOneHitSubLevel15     = false;
        Effects.bPermadeathOnFirstLoss = false;
        break;

    case ETier::Nightmare:
        // Tier 3 — fog of war, random hazard placement.
        Effects.HazardSpeedMultiplier = 1.9f;
        Effects.SpawnReduction        = 0.35f;
        Effects.PathWidthMultiplier   = 0.65f;
        Effects.bFogOfWar             = true;
        Effects.bRandomHazards        = true;
        Effects.bOneHitSubLevel15     = false;
        Effects.bPermadeathOnFirstLoss = false;
        break;

    case ETier::Extinction:
        // Tier 4 — one-hit kill for sub-Level 15 robots, minimal rescue spawns.
        Effects.HazardSpeedMultiplier = 2.2f;
        Effects.SpawnReduction        = 0.75f;
        Effects.PathWidthMultiplier   = 0.55f;
        Effects.bFogOfWar             = true;
        Effects.bRandomHazards        = true;
        Effects.bOneHitSubLevel15     = true;
        Effects.bPermadeathOnFirstLoss = false;
        break;

    case ETier::Legendary:
        // Tier 5 — fully random layouts, permadeath on first loss of any kind.
        Effects.HazardSpeedMultiplier = 2.8f;
        Effects.SpawnReduction        = 0.90f;
        Effects.PathWidthMultiplier   = 0.45f;
        Effects.bFogOfWar             = true;
        Effects.bRandomHazards        = true;
        Effects.bOneHitSubLevel15     = true;
        Effects.bPermadeathOnFirstLoss = true;
        break;

    default:
        // Fallback to Standard values.
        break;
    }

    return Effects;
}
