// MXRoleComponent.cpp — Specialization Module Version 1.0
// Created: 2026-02-22
// Agent 5: Specialization
// Change Log:
//   v1.0 — Initial implementation

#include "MXRoleComponent.h"
#include "MXSpecBonusCalculator.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXRoleComponent::UMXRoleComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void UMXRoleComponent::BeginPlay()
{
    Super::BeginPlay();
    // Profile initialization is deferred to InitializeFromProfile(), called by the
    // robot actor after it receives its FMXRobotProfile from the Identity module.
}

// ---------------------------------------------------------------------------
// InitializeFromProfile
// ---------------------------------------------------------------------------

void UMXRoleComponent::InitializeFromProfile(const FMXRobotProfile& Robot)
{
    // Cache the spec path
    CachedRole    = Robot.role;
    CachedTier2   = Robot.tier2_spec;
    CachedTier3   = Robot.tier3_spec;
    CachedMastery = Robot.mastery_title;

    // Compute and cache the bonus struct
    CachedBonus = UMXSpecBonusCalculator::CalculateBonus(
        CachedRole, CachedTier2, CachedTier3, CachedMastery);

    // Initialize per-run charges from the computed bonus
    LethalHitsRemaining = CachedBonus.lethal_hits_per_run;
    RevivesRemaining    = CachedBonus.revives_per_run;

    UE_LOG(LogTemp, Log, TEXT("MXRoleComponent: Initialized '%s' | Role=%d Tier2=%d Tier3=%d | "
        "Speed=%.2f Weight=%.2f XP=%.2f LethalHits=%d Revives=%d"),
        *Robot.name,
        (int32)CachedRole, (int32)CachedTier2, (int32)CachedTier3,
        CachedBonus.speed_modifier, CachedBonus.weight_modifier, CachedBonus.xp_modifier,
        LethalHitsRemaining, RevivesRemaining);
}

// ---------------------------------------------------------------------------
// Bonus Accessors
// ---------------------------------------------------------------------------

FMXSpecBonus UMXRoleComponent::GetSpecBonus() const
{
    if (!bBonusesActive)
    {
        // Return a neutral/baseline bonus (Naked Run — all bonuses suppressed)
        return FMXSpecBonus();
    }
    return CachedBonus;
}

float UMXRoleComponent::GetSpeedModifier() const
{
    return bBonusesActive ? CachedBonus.speed_modifier : 1.0f;
}

float UMXRoleComponent::GetWeightModifier() const
{
    return bBonusesActive ? CachedBonus.weight_modifier : 1.0f;
}

float UMXRoleComponent::GetXPModifier() const
{
    return bBonusesActive ? CachedBonus.xp_modifier : 1.0f;
}

float UMXRoleComponent::GetHazardRevealRadius() const
{
    return bBonusesActive ? CachedBonus.hazard_reveal_radius : 0.0f;
}

float UMXRoleComponent::GetAutoDodgeChance() const
{
    return bBonusesActive ? CachedBonus.auto_dodge_chance : 0.0f;
}

// ---------------------------------------------------------------------------
// Per-Run Charge Management — Lethal Hits
// ---------------------------------------------------------------------------

int32 UMXRoleComponent::GetLethalHitsRemaining() const
{
    return bBonusesActive ? LethalHitsRemaining : 0;
}

bool UMXRoleComponent::AbsorbLethalHit()
{
    if (!bBonusesActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("MXRoleComponent: AbsorbLethalHit — bonuses inactive (Naked Run)."));
        return false;
    }

    if (LethalHitsRemaining <= 0)
    {
        return false;
    }

    LethalHitsRemaining--;

    UE_LOG(LogTemp, Log, TEXT("MXRoleComponent: Lethal hit absorbed. %d charges remaining."),
        LethalHitsRemaining);

    return true;
}

// ---------------------------------------------------------------------------
// Per-Run Charge Management — Revives
// ---------------------------------------------------------------------------

int32 UMXRoleComponent::GetRevivesRemaining() const
{
    return bBonusesActive ? RevivesRemaining : 0;
}

bool UMXRoleComponent::UseRevive()
{
    if (!bBonusesActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("MXRoleComponent: UseRevive — bonuses inactive (Naked Run)."));
        return false;
    }

    if (RevivesRemaining <= 0)
    {
        return false;
    }

    RevivesRemaining--;

    UE_LOG(LogTemp, Log, TEXT("MXRoleComponent: Revive used. %d revives remaining."),
        RevivesRemaining);

    return true;
}

// ---------------------------------------------------------------------------
// Run Boundary Management
// ---------------------------------------------------------------------------

void UMXRoleComponent::ResetPerRunCharges()
{
    // Reload from the cached bonus (which was computed at init from the profile)
    LethalHitsRemaining = CachedBonus.lethal_hits_per_run;
    RevivesRemaining    = CachedBonus.revives_per_run;

    UE_LOG(LogTemp, Log, TEXT("MXRoleComponent: Per-run charges reset — LethalHits=%d, Revives=%d."),
        LethalHitsRemaining, RevivesRemaining);
}

// ---------------------------------------------------------------------------
// Naked Run Modifier
// ---------------------------------------------------------------------------

void UMXRoleComponent::SetBonusesActive(bool bActive)
{
    if (bBonusesActive == bActive) return;

    bBonusesActive = bActive;

    UE_LOG(LogTemp, Log, TEXT("MXRoleComponent: Spec bonuses %s."),
        bActive ? TEXT("ENABLED") : TEXT("DISABLED (Naked Run)"));
}

// ---------------------------------------------------------------------------
// IMXPersistable — Serializes per-run charges only (spec path lives in RobotProfile)
// ---------------------------------------------------------------------------

void UMXRoleComponent::MXSerialize(FArchive& Ar)
{
    Ar << LethalHitsRemaining;
    Ar << RevivesRemaining;
    Ar << bBonusesActive;
}

void UMXRoleComponent::MXDeserialize(FArchive& Ar)
{
    Ar << LethalHitsRemaining;
    Ar << RevivesRemaining;
    Ar << bBonusesActive;
}
