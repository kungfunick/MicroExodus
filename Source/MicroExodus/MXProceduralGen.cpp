// MXProceduralGen.cpp — Procedural Generation Module v1.0
// Agent 12: Procedural Generation

#include "MXProceduralGen.h"
#include "MXConstants.h"

UMXProceduralGen::UMXProceduralGen()
{
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXProceduralGen::Initialize(UDataTable* HazardDataTable)
{
    RoomGenerator = NewObject<UMXRoomGenerator>(this);

    HazardPlacer = NewObject<UMXHazardPlacer>(this);
    HazardPlacer->InitialiseFromDataTable(HazardDataTable);

    bInitialized = true;

    UE_LOG(LogTemp, Log, TEXT("MXProceduralGen: Initialized."));
}

// ---------------------------------------------------------------------------
// Public: GenerateRunLayout
// ---------------------------------------------------------------------------

FMXRunLayout UMXProceduralGen::GenerateRunLayout(int64 MasterSeed, ETier Tier, const TArray<FString>& ActiveModifiers)
{
    if (!bInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("MXProceduralGen: GenerateRunLayout called before Initialize!"));
        return FMXRunLayout();
    }

    UE_LOG(LogTemp, Log,
        TEXT("=========================================="));
    UE_LOG(LogTemp, Log,
        TEXT("MXProceduralGen: Generating run layout..."));
    UE_LOG(LogTemp, Log,
        TEXT("  Master Seed : %lld"), MasterSeed);
    UE_LOG(LogTemp, Log,
        TEXT("  Tier        : %d"), static_cast<int32>(Tier));
    UE_LOG(LogTemp, Log,
        TEXT("  Modifiers   : %d active"), ActiveModifiers.Num());
    UE_LOG(LogTemp, Log,
        TEXT("=========================================="));

    FMXRunLayout Layout;
    Layout.MasterSeed      = MasterSeed;
    Layout.Tier            = Tier;
    Layout.ActiveModifiers = ActiveModifiers;
    Layout.Levels.Reserve(MXConstants::MAX_LEVELS);

    // Check for modifier flags.
    const bool bRandomHazards     = HasModifier(ActiveModifiers, TEXT("Random Hazards"));
    const bool bReducedVisibility = HasModifier(ActiveModifiers, TEXT("Reduced Visibility"));
    const bool bNoRescues         = HasModifier(ActiveModifiers, TEXT("Iron Swarm"));

    // Tier-derived hazard speed multiplier.
    // Standard=1.0, Hardened=1.1, Brutal=1.2, Nightmare=1.4, Extinction=1.6, Legendary=1.8
    const float HazardSpeedMult = 1.0f + static_cast<float>(Tier) * 0.15f
                                  + ((Tier >= ETier::Nightmare) ? 0.1f : 0.0f);

    int32 TotalHazards = 0;
    int32 TotalRescues = 0;
    int32 TotalRooms   = 0;

    for (int32 Level = 1; Level <= MXConstants::MAX_LEVELS; ++Level)
    {
        const int64 LevelSeed = DeriveLevelSeed(MasterSeed, Level);
        FRandomStream ConditionStream(static_cast<int32>((LevelSeed ^ 0x434F4E44) & 0x7FFFFFFF)); // "COND"

        FMXLevelLayout LevelLayout;
        LevelLayout.LevelNumber = Level;
        LevelLayout.LevelSeed   = LevelSeed;

        // 1. Generate conditions (element theme, pacing).
        LevelLayout.Conditions = GenerateConditions(Level, Tier, ActiveModifiers, ConditionStream);

        // Apply modifier overrides.
        if (bReducedVisibility && Level >= 8) // Visibility mod kicks in mid-run.
        {
            LevelLayout.Conditions.bReducedVisibility = true;
        }

        // 2. Generate rooms.
        LevelLayout.Rooms = RoomGenerator->GenerateRooms(LevelSeed, Level, Tier);

        // 3. Compute critical path.
        LevelLayout.CriticalPath = RoomGenerator->ComputeCriticalPath(LevelLayout.Rooms);

        // Mark rooms on the critical path.
        for (const int32 PathIdx : LevelLayout.CriticalPath)
        {
            if (PathIdx >= 0 && PathIdx < LevelLayout.Rooms.Num())
            {
                LevelLayout.Rooms[PathIdx].bOnCriticalPath = true;
            }
        }

        // 4. Place sacrifice gate (levels 5, 10, 15, 20).
        LevelLayout.GateRoomIndex = RoomGenerator->PickGateRoom(LevelLayout.Rooms, Level, LevelSeed);
        if (LevelLayout.GateRoomIndex >= 0 && LevelLayout.GateRoomIndex < LevelLayout.Rooms.Num())
        {
            LevelLayout.Rooms[LevelLayout.GateRoomIndex].RoomType = ERoomType::GateRoom;
        }

        // 5. Place hazards.
        LevelLayout.Hazards = HazardPlacer->PlaceHazards(
            LevelLayout.Rooms,
            LevelLayout.Conditions,
            LevelSeed,
            Level,
            Tier,
            HazardSpeedMult,
            bRandomHazards
        );

        // 6. Place rescue spawns.
        const int32 RescueCount = bNoRescues ? 0 : ComputeRescueCount(Level, Tier, ActiveModifiers);
        LevelLayout.RescueSpawns = HazardPlacer->PlaceRescueSpawns(
            LevelLayout.Rooms,
            LevelLayout.Hazards,
            RescueCount,
            LevelSeed
        );

        // Populate convenience counts.
        LevelLayout.TotalHazardCount = LevelLayout.Hazards.Num();
        LevelLayout.TotalRescueCount = LevelLayout.RescueSpawns.Num();

        TotalHazards += LevelLayout.TotalHazardCount;
        TotalRescues += LevelLayout.TotalRescueCount;
        TotalRooms   += LevelLayout.Rooms.Num();

        Layout.Levels.Add(LevelLayout);
    }

    Layout.TotalHazards = TotalHazards;
    Layout.TotalRescues = TotalRescues;
    Layout.TotalRooms   = TotalRooms;

    // Store for subsequent queries.
    CurrentRunLayout = Layout;

    // Log the summary.
    LogRunSummary();

    return Layout;
}

// ---------------------------------------------------------------------------
// Public: GetLevelLayout
// ---------------------------------------------------------------------------

FMXLevelLayout UMXProceduralGen::GetLevelLayout(int32 LevelNumber) const
{
    const int32 Index = LevelNumber - 1;
    if (Index >= 0 && Index < CurrentRunLayout.Levels.Num())
    {
        return CurrentRunLayout.Levels[Index];
    }

    UE_LOG(LogTemp, Warning,
        TEXT("MXProceduralGen: GetLevelLayout(%d) out of range. Returning default."), LevelNumber);
    return FMXLevelLayout();
}

// ---------------------------------------------------------------------------
// Public: GenerateSeedFromTime
// ---------------------------------------------------------------------------

int64 UMXProceduralGen::GenerateSeedFromTime()
{
    // Combine system time components for a reasonably unique seed.
    const FDateTime Now = FDateTime::UtcNow();
    int64 Seed = Now.GetTicks();
    // Mix in some bits for better distribution.
    Seed ^= (Seed >> 16);
    Seed *= 0x45d9f3b;
    Seed ^= (Seed >> 16);
    return Seed;
}

// ---------------------------------------------------------------------------
// Public: LogRunSummary
// ---------------------------------------------------------------------------

void UMXProceduralGen::LogRunSummary() const
{
    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("╔══════════════════════════════════════════════════════╗"));
    UE_LOG(LogTemp, Log, TEXT("║            PROCEDURAL RUN LAYOUT SUMMARY            ║"));
    UE_LOG(LogTemp, Log, TEXT("╠══════════════════════════════════════════════════════╣"));
    UE_LOG(LogTemp, Log, TEXT("║  Master Seed  : %-36lld  ║"), CurrentRunLayout.MasterSeed);
    UE_LOG(LogTemp, Log, TEXT("║  Tier         : %-36d  ║"), static_cast<int32>(CurrentRunLayout.Tier));
    UE_LOG(LogTemp, Log, TEXT("║  Total Rooms  : %-36d  ║"), CurrentRunLayout.TotalRooms);
    UE_LOG(LogTemp, Log, TEXT("║  Total Hazards: %-36d  ║"), CurrentRunLayout.TotalHazards);
    UE_LOG(LogTemp, Log, TEXT("║  Total Rescues: %-36d  ║"), CurrentRunLayout.TotalRescues);
    UE_LOG(LogTemp, Log, TEXT("╠══════════════════════════════════════════════════════╣"));

    static const TCHAR* ElementNames[] = {
        TEXT("Fire"), TEXT("Mech"), TEXT("Grav"), TEXT("Ice"), TEXT("Elec"), TEXT("Comedy"), TEXT("Sacr")
    };

    for (const FMXLevelLayout& L : CurrentRunLayout.Levels)
    {
        const int32 PriIdx = FMath::Clamp(static_cast<int32>(L.Conditions.PrimaryElement), 0, 6);
        const int32 SecIdx = FMath::Clamp(static_cast<int32>(L.Conditions.SecondaryElement), 0, 6);

        UE_LOG(LogTemp, Log,
            TEXT("║  Lvl %2d │ %2d rooms │ %2d hazards │ %d rescues │ %s/%s │ threat %.1f%s ║"),
            L.LevelNumber,
            L.Rooms.Num(),
            L.TotalHazardCount,
            L.TotalRescueCount,
            ElementNames[PriIdx],
            ElementNames[SecIdx],
            L.Conditions.AmbientThreatLevel,
            L.GateRoomIndex >= 0 ? TEXT(" [GATE]") : TEXT("       "));
    }

    UE_LOG(LogTemp, Log, TEXT("╚══════════════════════════════════════════════════════╝"));
    UE_LOG(LogTemp, Log, TEXT(""));
}

// ---------------------------------------------------------------------------
// Public: LogLevelDetail
// ---------------------------------------------------------------------------

void UMXProceduralGen::LogLevelDetail(int32 LevelNumber) const
{
    const FMXLevelLayout Level = GetLevelLayout(LevelNumber);
    if (Level.Rooms.Num() == 0) return;

    static const TCHAR* RoomTypeNames[] = {
        TEXT("Corridor"), TEXT("Arena"), TEXT("Bottleneck"), TEXT("Sanctuary"),
        TEXT("Gauntlet"), TEXT("Junction"), TEXT("GateRoom"), TEXT("Entrance"), TEXT("Exit")
    };

    UE_LOG(LogTemp, Log, TEXT("--- Level %d Detail (seed=%lld) ---"), LevelNumber, Level.LevelSeed);

    for (const FMXRoomDef& R : Level.Rooms)
    {
        const int32 TypeIdx = FMath::Clamp(static_cast<int32>(R.RoomType), 0, 8);
        UE_LOG(LogTemp, Log,
            TEXT("  Room[%d] %-12s  %4.0f x %4.0f  slots=%d  connections=%d  %s"),
            R.RoomIndex, RoomTypeNames[TypeIdx],
            R.Size.X, R.Size.Y,
            R.MaxHazardSlots,
            R.ConnectedRoomIndices.Num(),
            R.bOnCriticalPath ? TEXT("[CRIT]") : TEXT(""));
    }

    UE_LOG(LogTemp, Log, TEXT("  Critical path: %d rooms"), Level.CriticalPath.Num());

    for (const FMXHazardPlacement& H : Level.Hazards)
    {
        UE_LOG(LogTemp, Log,
            TEXT("  Hazard: %-15s in Room[%d] at (%.0f, %.0f) speed=%.2f"),
            *H.HazardType, H.RoomIndex,
            H.LocalPosition.X, H.LocalPosition.Y, H.SpeedMultiplier);
    }

    for (const FMXRescueSpawn& S : Level.RescueSpawns)
    {
        UE_LOG(LogTemp, Log,
            TEXT("  Rescue: Room[%d] at (%.0f, %.0f) %s"),
            S.RoomIndex, S.LocalPosition.X, S.LocalPosition.Y,
            S.bGuarded ? TEXT("[GUARDED]") : TEXT(""));
    }

    if (Level.GateRoomIndex >= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("  Sacrifice Gate: Room[%d]"), Level.GateRoomIndex);
    }
}

// ---------------------------------------------------------------------------
// Private: DeriveLevelSeed
// ---------------------------------------------------------------------------

int64 UMXProceduralGen::DeriveLevelSeed(int64 MasterSeed, int32 LevelNumber)
{
    // Simple but effective seed derivation: multiply by a prime and XOR with level.
    int64 Seed = MasterSeed * 6364136223846793005LL + static_cast<int64>(LevelNumber) * 1442695040888963407LL;
    Seed ^= (Seed >> 17);
    return Seed;
}

// ---------------------------------------------------------------------------
// Private: GenerateConditions
// ---------------------------------------------------------------------------

FMXLevelConditions UMXProceduralGen::GenerateConditions(
    int32                   LevelNumber,
    ETier                   Tier,
    const TArray<FString>&  Modifiers,
    FRandomStream&          Stream) const
{
    FMXLevelConditions Cond;

    // --- Element theme ---
    // The available element pool (excluding Sacrifice and Comedy from normal rotation).
    const TArray<EHazardElement> ElementPool = {
        EHazardElement::Fire,
        EHazardElement::Mechanical,
        EHazardElement::Gravity,
        EHazardElement::Ice,
        EHazardElement::Electrical,
    };

    // Primary element selection based on level "arc":
    //   Levels 1-4:   Mechanical (familiar, simple)
    //   Levels 5-8:   Fire or Gravity (introduce danger)
    //   Levels 9-12:  Ice or Electrical (exotic)
    //   Levels 13-16: Mixed (all elements)
    //   Levels 17-20: Weighted toward fire and mechanical (the classics, escalated)
    TArray<float> Weights;
    Weights.SetNumZeroed(ElementPool.Num());
    // Indices: 0=Fire, 1=Mech, 2=Grav, 3=Ice, 4=Elec

    if (LevelNumber <= 4)
    {
        Weights[0] = 0.2f; Weights[1] = 0.5f; Weights[2] = 0.2f; Weights[3] = 0.05f; Weights[4] = 0.05f;
    }
    else if (LevelNumber <= 8)
    {
        Weights[0] = 0.3f; Weights[1] = 0.2f; Weights[2] = 0.3f; Weights[3] = 0.1f; Weights[4] = 0.1f;
    }
    else if (LevelNumber <= 12)
    {
        Weights[0] = 0.15f; Weights[1] = 0.15f; Weights[2] = 0.15f; Weights[3] = 0.3f; Weights[4] = 0.25f;
    }
    else if (LevelNumber <= 16)
    {
        Weights[0] = 0.2f; Weights[1] = 0.2f; Weights[2] = 0.2f; Weights[3] = 0.2f; Weights[4] = 0.2f;
    }
    else
    {
        Weights[0] = 0.35f; Weights[1] = 0.3f; Weights[2] = 0.15f; Weights[3] = 0.1f; Weights[4] = 0.1f;
    }

    // Weighted selection for primary.
    float Roll = Stream.FRandRange(0.0f, 1.0f);
    int32 PrimaryIdx = 0;
    for (int32 i = 0; i < Weights.Num(); ++i)
    {
        Roll -= Weights[i];
        if (Roll <= 0.0f) { PrimaryIdx = i; break; }
    }
    Cond.PrimaryElement = ElementPool[PrimaryIdx];

    // Secondary: pick from remaining elements.
    int32 SecondaryIdx = (PrimaryIdx + 1 + Stream.RandRange(0, ElementPool.Num() - 2)) % ElementPool.Num();
    Cond.SecondaryElement = ElementPool[SecondaryIdx];

    // --- Hazard density ---
    // Curve: 0.5 at level 1, peaks at 1.5 around level 15, sustains through 20.
    const float LevelFraction = static_cast<float>(LevelNumber - 1) / static_cast<float>(MXConstants::MAX_LEVELS - 1);
    Cond.HazardDensity = 0.5f + LevelFraction * 1.0f;

    // Tier bonus to density.
    Cond.HazardDensity += static_cast<float>(Tier) * 0.08f;
    Cond.HazardDensity = FMath::Clamp(Cond.HazardDensity, 0.3f, 2.0f);

    // --- Ambient threat level ---
    // Follows the documentary arc: gentle start, rising tension, climax.
    if (LevelNumber <= 3)
    {
        Cond.AmbientThreatLevel = 0.1f + LevelFraction * 0.3f;
    }
    else if (LevelNumber <= 10)
    {
        Cond.AmbientThreatLevel = 0.3f + (static_cast<float>(LevelNumber - 3) / 7.0f) * 0.4f;
    }
    else if (LevelNumber <= 17)
    {
        Cond.AmbientThreatLevel = 0.7f + (static_cast<float>(LevelNumber - 10) / 7.0f) * 0.2f;
    }
    else
    {
        Cond.AmbientThreatLevel = 0.9f + (static_cast<float>(LevelNumber - 17) / 3.0f) * 0.1f;
    }
    Cond.AmbientThreatLevel = FMath::Clamp(Cond.AmbientThreatLevel, 0.0f, 1.0f);

    // --- Target duration ---
    // Later levels are slightly shorter (more intense).
    Cond.TargetDurationSeconds = 300.0f - LevelFraction * 60.0f; // 300s → 240s

    // --- Reduced visibility ---
    Cond.bReducedVisibility = false; // Set by modifier in GenerateRunLayout.

    return Cond;
}

// ---------------------------------------------------------------------------
// Private: ComputeRescueCount
// ---------------------------------------------------------------------------

int32 UMXProceduralGen::ComputeRescueCount(int32 LevelNumber, ETier Tier, const TArray<FString>& Modifiers) const
{
    if (HasModifier(Modifiers, TEXT("Iron Swarm"))) return 0;

    // Mirror MXSpawnManager's logic.
    if (Tier == ETier::Legendary)
    {
        return (LevelNumber % 3 == 0) ? 1 : 0;
    }

    int32 Min = 2, Max = 3;
    if (LevelNumber <= 5)       { Min = 2; Max = 3; }
    else if (LevelNumber <= 10) { Min = 3; Max = 4; }
    else if (LevelNumber <= 15) { Min = 2; Max = 3; }
    else                        { Min = 1; Max = 2; }

    const int32 TierIndex = static_cast<int32>(Tier);
    int32 Count = Min + ((LevelNumber + TierIndex) % (Max - Min + 1));

    // Tier reductions.
    if (Tier == ETier::Nightmare)  Count = FMath::Max(1, Count / 2);
    if (Tier == ETier::Extinction) Count = FMath::Max(0, Count / 4);

    return Count;
}

// ---------------------------------------------------------------------------
// Private: HasModifier
// ---------------------------------------------------------------------------

bool UMXProceduralGen::HasModifier(const TArray<FString>& Modifiers, const FString& Name)
{
    for (const FString& M : Modifiers)
    {
        if (M.Equals(Name, ESearchCase::IgnoreCase)) return true;
    }
    return false;
}
