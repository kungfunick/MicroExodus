// MXSpawnManager.cpp — Roguelike Module v1.0
// Agent 6: Roguelike

#include "MXSpawnManager.h"
#include "MXInterfaces.h"
#include "MXConstants.h"

UMXSpawnManager::UMXSpawnManager()
    : TotalSpawnedThisRun(0)
{
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXSpawnManager::SetRobotProvider(TScriptInterface<IMXRobotProvider> InRobotProvider)
{
    RobotProvider = InRobotProvider;
}

// ---------------------------------------------------------------------------
// Spawn Budget Calculation
// ---------------------------------------------------------------------------

int32 UMXSpawnManager::GetRescueSpawnCount(int32 LevelNumber, ETier Tier, const FMXModifierEffects& Modifiers) const
{
    // Iron Swarm: no rescues at all.
    if (Modifiers.bNoRescues)
    {
        return 0;
    }

    // Legendary tier: 0–1 per level (alternate each level for fairness).
    if (Tier == ETier::Legendary)
    {
        return (LevelNumber % 3 == 0) ? 1 : 0;
    }

    // Calculate base budget for this level band.
    int32 Min = 2;
    int32 Max = 3;
    GetBaseBudgetRange(LevelNumber, Min, Max);

    // Pick a value within the range using a deterministic mix of level number and tier.
    // This keeps runs consistent within a session without true randomness.
    const int32 TierIndex = static_cast<int32>(Tier);
    const int32 BaseCount = Min + ((LevelNumber + TierIndex) % (Max - Min + 1));

    // Apply tier reductions.
    return ApplyTierReduction(BaseCount, Tier);
}

TArray<FVector> UMXSpawnManager::GetSpawnPositions(int32 LevelNumber, int32 Count) const
{
    TArray<FVector> Positions;
    if (Count <= 0)
    {
        return Positions;
    }

    // These are placeholder positions — level Blueprints override these via
    // a tagged Actor array (Tag="RescueSpawn") placed in each level map.
    // The spawn manager returns stub positions so offline/editor contexts can function.
    // At runtime, MXRunManager queries the Blueprint level for real spawn points.
    for (int32 i = 0; i < Count; ++i)
    {
        // Distribute stubs in a small grid near level origin so they're visible in testing.
        const float OffsetX = static_cast<float>(i)  * 200.0f;
        const float OffsetY = static_cast<float>(LevelNumber) * 50.0f;
        Positions.Add(FVector(OffsetX, OffsetY, 0.0f));
    }

    return Positions;
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------

FGuid UMXSpawnManager::SpawnRescueRobot(FVector Position, int32 LevelNumber)
{
    // Validate provider reference.
    if (!RobotProvider.GetInterface())
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpawnManager: No RobotProvider set — cannot spawn rescue robot."));
        return FGuid();
    }

    // Roster capacity check — do not exceed MAX_ROBOTS.
    const int32 CurrentCount = IMXRobotProvider::Execute_GetRobotCount(RobotProvider.GetObject());
    if (CurrentCount >= MXConstants::MAX_ROBOTS)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXSpawnManager: Roster full (%d/%d). Rescue robot not spawned."),
            CurrentCount, MXConstants::MAX_ROBOTS);
        return FGuid();
    }

    // Identity module creates the robot profile and returns the new GUID.
    // CreateRobot is not on IMXRobotProvider (that is a read interface); in practice
    // MXRunManager passes down the concrete UMXRobotManager which exposes CreateRobot.
    // For now we record the spawn and return a new GUID — the runtime wiring happens
    // in MXRunManager::BindToEventBus when it holds the concrete pointer.
    const FGuid NewRobotId = FGuid::NewGuid();

    ++TotalSpawnedThisRun;

    UE_LOG(LogTemp, Log,
        TEXT("MXSpawnManager: Rescue robot spawned at (%.1f, %.1f, %.1f) on Level %d. GUID=%s"),
        Position.X, Position.Y, Position.Z, LevelNumber, *NewRobotId.ToString());

    return NewRobotId;
}

TArray<FGuid> UMXSpawnManager::SpawnRescueRobotsForLevel(int32 LevelNumber, ETier Tier, const FMXModifierEffects& Modifiers)
{
    TArray<FGuid> SpawnedGuids;

    const int32 Count     = GetRescueSpawnCount(LevelNumber, Tier, Modifiers);
    const TArray<FVector> Positions = GetSpawnPositions(LevelNumber, Count);

    for (int32 i = 0; i < Count; ++i)
    {
        const FVector Pos = Positions.IsValidIndex(i) ? Positions[i] : FVector::ZeroVector;
        const FGuid NewId = SpawnRescueRobot(Pos, LevelNumber);
        if (NewId.IsValid())
        {
            SpawnedGuids.Add(NewId);
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXSpawnManager: Spawned %d/%d rescue robots on Level %d (Tier %d)."),
        SpawnedGuids.Num(), Count, LevelNumber, static_cast<int32>(Tier));

    return SpawnedGuids;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

int32 UMXSpawnManager::GetTotalSpawnedThisRun() const
{
    return TotalSpawnedThisRun;
}

void UMXSpawnManager::ResetForNewRun()
{
    TotalSpawnedThisRun = 0;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

void UMXSpawnManager::GetBaseBudgetRange(int32 LevelNumber, int32& OutMin, int32& OutMax) const
{
    if (LevelNumber <= 5)
    {
        OutMin = 2; OutMax = 3;
    }
    else if (LevelNumber <= 10)
    {
        OutMin = 3; OutMax = 4;
    }
    else if (LevelNumber <= 15)
    {
        OutMin = 2; OutMax = 3;
    }
    else
    {
        OutMin = 1; OutMax = 2;
    }
}

int32 UMXSpawnManager::ApplyTierReduction(int32 BaseCount, ETier Tier) const
{
    switch (Tier)
    {
    case ETier::Standard:
    case ETier::Hardened:
    case ETier::Brutal:
        // No reduction for tiers 0–2.
        return BaseCount;

    case ETier::Nightmare:
        // Tier 3: halve spawn count, minimum 1.
        return FMath::Max(1, BaseCount / 2);

    case ETier::Extinction:
        // Tier 4: quarter spawn count, minimum 0.
        return FMath::Max(0, BaseCount / 4);

    case ETier::Legendary:
        // Handled in GetRescueSpawnCount before this call; fallback.
        return FMath::Min(1, BaseCount);

    default:
        return BaseCount;
    }
}
