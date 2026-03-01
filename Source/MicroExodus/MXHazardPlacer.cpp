// MXHazardPlacer.cpp — Procedural Generation Module v1.0
// Agent 12: Procedural Generation

#include "MXHazardPlacer.h"
#include "MXConstants.h"

// ---------------------------------------------------------------------------
// Element mapping from DT_HazardFields.Element string to enum
// ---------------------------------------------------------------------------

static EHazardElement ParseElement(const FString& ElementStr)
{
    if (ElementStr.Equals(TEXT("Fire"), ESearchCase::IgnoreCase))        return EHazardElement::Fire;
    if (ElementStr.Equals(TEXT("Mechanical"), ESearchCase::IgnoreCase))  return EHazardElement::Mechanical;
    if (ElementStr.Equals(TEXT("Gravity"), ESearchCase::IgnoreCase))     return EHazardElement::Gravity;
    if (ElementStr.Equals(TEXT("Ice"), ESearchCase::IgnoreCase))         return EHazardElement::Ice;
    if (ElementStr.Equals(TEXT("Electrical"), ESearchCase::IgnoreCase))  return EHazardElement::Electrical;
    if (ElementStr.Equals(TEXT("Comedy"), ESearchCase::IgnoreCase))      return EHazardElement::Comedy;
    if (ElementStr.Equals(TEXT("Sacrifice"), ESearchCase::IgnoreCase))   return EHazardElement::Sacrifice;
    return EHazardElement::Mechanical; // Fallback.
}

UMXHazardPlacer::UMXHazardPlacer()
{
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXHazardPlacer::InitialiseFromDataTable(UDataTable* HazardDataTable)
{
    HazardElementMap.Empty();
    AllHazardTypes.Empty();
    HazardsByElement.Empty();

    if (!HazardDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXHazardPlacer: No DataTable provided. Using hardcoded hazard pool."));

        // Hardcoded fallback matching DT_HazardFields.
        struct FHardcodedEntry { FString Type; EHazardElement Element; };
        const TArray<FHardcodedEntry> Fallback = {
            { TEXT("FireVent"),       EHazardElement::Fire },
            { TEXT("Crusher"),        EHazardElement::Mechanical },
            { TEXT("SpikeTrap"),      EHazardElement::Mechanical },
            { TEXT("Pit"),            EHazardElement::Gravity },
            { TEXT("LavaPool"),       EHazardElement::Fire },
            { TEXT("Conveyor"),       EHazardElement::Mechanical },
            { TEXT("BuzzSaw"),        EHazardElement::Mechanical },
            { TEXT("IceFloor"),       EHazardElement::Ice },
            { TEXT("EMPField"),       EHazardElement::Electrical },
            { TEXT("FallingDebris"),  EHazardElement::Mechanical },
        };

        for (const FHardcodedEntry& E : Fallback)
        {
            AllHazardTypes.Add(E.Type);
            HazardElementMap.Add(E.Type, E.Element);
            HazardsByElement.FindOrAdd(E.Element).Add(E.Type);
        }
    }
    else
    {
        // Parse from DataTable.
        const TArray<FName> RowNames = HazardDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            const FString RowStr = RowName.ToString();

            // Skip the SacrificeGate — it's placed by RoomGenerator, not the hazard placer.
            if (RowStr.Equals(TEXT("SacrificeGate"), ESearchCase::IgnoreCase)) continue;

            // Read the Element column via the DataTable's generic row access.
            // Since FMXObjectEventFields is the row struct, we access it directly.
            const FMXObjectEventFields* Row = HazardDataTable->FindRow<FMXObjectEventFields>(RowName, TEXT("MXHazardPlacer"));
            if (Row)
            {
                AllHazardTypes.Add(RowStr);
                HazardElementMap.Add(RowStr, Row->element);
                HazardsByElement.FindOrAdd(Row->element).Add(RowStr);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXHazardPlacer: Initialised with %d hazard types across %d elements."),
        AllHazardTypes.Num(), HazardsByElement.Num());
}

// ---------------------------------------------------------------------------
// Public: PlaceHazards
// ---------------------------------------------------------------------------

TArray<FMXHazardPlacement> UMXHazardPlacer::PlaceHazards(
    const TArray<FMXRoomDef>&     Rooms,
    const FMXLevelConditions&     Conditions,
    int64                         LevelSeed,
    int32                         LevelNumber,
    ETier                         Tier,
    float                         HazardSpeedMult,
    bool                          bRandomHazards)
{
    FRandomStream Stream(static_cast<int32>((LevelSeed ^ 0x48415A44) & 0x7FFFFFFF)); // "HAZD"

    TArray<FMXHazardPlacement> AllPlacements;
    float RunningCycleOffset = 0.0f;

    for (const FMXRoomDef& Room : Rooms)
    {
        const int32 Count = ComputeHazardCount(Room, Conditions, LevelNumber, Tier, Stream);
        if (Count <= 0) continue;

        for (int32 i = 0; i < Count; ++i)
        {
            FMXHazardPlacement Placement;
            Placement.HazardType      = PickHazardType(Conditions.PrimaryElement, Conditions.SecondaryElement, bRandomHazards, Stream);
            Placement.RoomIndex       = Room.RoomIndex;
            Placement.LocalPosition   = ComputeHazardPosition(Room, i, Count, Stream);
            Placement.Rotation        = FRotator(0.0f, Stream.FRandRange(0.0f, 360.0f), 0.0f);
            Placement.SpeedMultiplier = HazardSpeedMult;
            Placement.bCycling        = true;

            // Stagger cycle offsets so hazards don't all activate simultaneously.
            Placement.CycleOffset = RunningCycleOffset;
            RunningCycleOffset += Stream.FRandRange(0.3f, 1.5f);

            AllPlacements.Add(Placement);
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXHazardPlacer: Level %d — placed %d hazards across %d rooms (primary=%d, secondary=%d, density=%.2f)."),
        LevelNumber, AllPlacements.Num(), Rooms.Num(),
        static_cast<int32>(Conditions.PrimaryElement),
        static_cast<int32>(Conditions.SecondaryElement),
        Conditions.HazardDensity);

    for (int32 i = 0; i < AllPlacements.Num(); ++i)
    {
        const FMXHazardPlacement& H = AllPlacements[i];
        UE_LOG(LogTemp, Verbose,
            TEXT("  Hazard[%d]: %s in Room[%d] at (%.0f, %.0f, %.0f) speed=%.2f offset=%.2f"),
            i, *H.HazardType, H.RoomIndex,
            H.LocalPosition.X, H.LocalPosition.Y, H.LocalPosition.Z,
            H.SpeedMultiplier, H.CycleOffset);
    }

    return AllPlacements;
}

// ---------------------------------------------------------------------------
// Public: PlaceRescueSpawns
// ---------------------------------------------------------------------------

TArray<FMXRescueSpawn> UMXHazardPlacer::PlaceRescueSpawns(
    const TArray<FMXRoomDef>&          Rooms,
    const TArray<FMXHazardPlacement>&  Hazards,
    int32                              RescueCount,
    int64                              LevelSeed)
{
    FRandomStream Stream(static_cast<int32>((LevelSeed ^ 0x52455343) & 0x7FFFFFFF)); // "RESC"

    TArray<FMXRescueSpawn> Spawns;
    if (RescueCount <= 0 || Rooms.Num() < 2) return Spawns;

    // Prefer placing rescues in sanctuary rooms, then corridors, then others.
    // Never place in entrance or exit.
    TArray<int32> PreferredRooms;
    TArray<int32> FallbackRooms;

    for (const FMXRoomDef& R : Rooms)
    {
        if (R.RoomType == ERoomType::Entrance || R.RoomType == ERoomType::Exit) continue;

        if (R.RoomType == ERoomType::Sanctuary)
        {
            PreferredRooms.Add(R.RoomIndex);
        }
        else
        {
            FallbackRooms.Add(R.RoomIndex);
        }
    }

    // Merge: preferred first, then fallback.
    TArray<int32> CandidateRooms;
    CandidateRooms.Append(PreferredRooms);
    CandidateRooms.Append(FallbackRooms);

    if (CandidateRooms.Num() == 0) return Spawns;

    // Build a set of hazard positions per room for guarded detection.
    TMap<int32, TArray<FVector>> HazardPositionsByRoom;
    for (const FMXHazardPlacement& H : Hazards)
    {
        HazardPositionsByRoom.FindOrAdd(H.RoomIndex).Add(H.LocalPosition);
    }

    for (int32 i = 0; i < RescueCount; ++i)
    {
        FMXRescueSpawn Spawn;

        // Round-robin through candidate rooms.
        const int32 RoomIdx = CandidateRooms[i % CandidateRooms.Num()];
        Spawn.RoomIndex = RoomIdx;

        // Place within the room bounds with some margin.
        const FMXRoomDef& Room = Rooms[RoomIdx];
        const float MarginX = Room.Size.X * 0.15f;
        const float MarginY = Room.Size.Y * 0.15f;
        Spawn.LocalPosition = FVector(
            Stream.FRandRange(MarginX, Room.Size.X - MarginX),
            Stream.FRandRange(MarginY, Room.Size.Y - MarginY),
            0.0f
        );

        // Check if any hazard in this room is within warning distance.
        Spawn.bGuarded = false;
        if (const TArray<FVector>* RoomHazards = HazardPositionsByRoom.Find(RoomIdx))
        {
            for (const FVector& HP : *RoomHazards)
            {
                if (FVector::Dist(Spawn.LocalPosition, HP) < MXConstants::NEAR_MISS_RADIUS * 100.0f) // cm
                {
                    Spawn.bGuarded = true;
                    break;
                }
            }
        }

        Spawns.Add(Spawn);
    }

    UE_LOG(LogTemp, Log, TEXT("MXHazardPlacer: Level — placed %d rescue spawns (%d guarded)."),
        Spawns.Num(),
        Spawns.FilterByPredicate([](const FMXRescueSpawn& S) { return S.bGuarded; }).Num());

    return Spawns;
}

// ---------------------------------------------------------------------------
// Private: ComputeHazardCount
// ---------------------------------------------------------------------------

int32 UMXHazardPlacer::ComputeHazardCount(
    const FMXRoomDef&         Room,
    const FMXLevelConditions& Conditions,
    int32                     LevelNumber,
    ETier                     Tier,
    FRandomStream&            Stream) const
{
    if (Room.MaxHazardSlots <= 0) return 0;

    // Base fill fraction: 40% at level 1, ramping to 90% at level 20.
    const float LevelFraction = static_cast<float>(LevelNumber - 1) / static_cast<float>(MXConstants::MAX_LEVELS - 1);
    const float BaseFill = 0.4f + LevelFraction * 0.5f;

    // Apply density multiplier from level conditions.
    float AdjustedFill = BaseFill * Conditions.HazardDensity;

    // Tier bonus: +10% per tier above Standard.
    AdjustedFill += static_cast<float>(Tier) * 0.1f;

    // Clamp to [0, 1].
    AdjustedFill = FMath::Clamp(AdjustedFill, 0.0f, 1.0f);

    // Compute count with some random variance.
    const float RawCount = static_cast<float>(Room.MaxHazardSlots) * AdjustedFill;
    const int32 MinCount = FMath::FloorToInt32(RawCount);
    const int32 MaxCount = FMath::CeilToInt32(RawCount);

    int32 Count = (MinCount == MaxCount) ? MinCount : Stream.RandRange(MinCount, MaxCount);
    return FMath::Clamp(Count, 0, Room.MaxHazardSlots);
}

// ---------------------------------------------------------------------------
// Private: PickHazardType
// ---------------------------------------------------------------------------

FString UMXHazardPlacer::PickHazardType(
    EHazardElement PrimaryElement,
    EHazardElement SecondaryElement,
    bool           bRandomHazards,
    FRandomStream& Stream) const
{
    if (AllHazardTypes.Num() == 0) return TEXT("Crusher"); // Ultimate fallback.

    if (bRandomHazards)
    {
        return AllHazardTypes[Stream.RandRange(0, AllHazardTypes.Num() - 1)];
    }

    // 70% chance primary element, 30% chance secondary.
    const EHazardElement ChosenElement = (Stream.FRandRange(0.0f, 1.0f) < 0.7f)
        ? PrimaryElement
        : SecondaryElement;

    const TArray<FString>* Pool = HazardsByElement.Find(ChosenElement);
    if (Pool && Pool->Num() > 0)
    {
        return (*Pool)[Stream.RandRange(0, Pool->Num() - 1)];
    }

    // If no hazards of the chosen element exist, fall back to any.
    return AllHazardTypes[Stream.RandRange(0, AllHazardTypes.Num() - 1)];
}

// ---------------------------------------------------------------------------
// Private: ComputeHazardPosition
// ---------------------------------------------------------------------------

FVector UMXHazardPlacer::ComputeHazardPosition(
    const FMXRoomDef& Room,
    int32             SlotIndex,
    int32             TotalSlots,
    FRandomStream&    Stream) const
{
    // Distribute hazards across the room using a grid with jitter.
    // For a corridor (narrow, long), space them along Y.
    // For an arena (square-ish), use a 2D grid.

    const float Margin = 100.0f; // cm margin from room edges.
    const float UsableW = FMath::Max(0.0f, Room.Size.X - 2.0f * Margin);
    const float UsableL = FMath::Max(0.0f, Room.Size.Y - 2.0f * Margin);

    if (TotalSlots <= 1)
    {
        // Single hazard: center with jitter.
        return FVector(
            Margin + UsableW * 0.5f + Stream.FRandRange(-UsableW * 0.2f, UsableW * 0.2f),
            Margin + UsableL * 0.5f + Stream.FRandRange(-UsableL * 0.2f, UsableL * 0.2f),
            0.0f
        );
    }

    // Multi-hazard: distribute along the room's length with lateral jitter.
    const float Step = UsableL / static_cast<float>(TotalSlots);
    const float BaseY = Margin + Step * 0.5f + Step * static_cast<float>(SlotIndex);
    const float JitterX = Stream.FRandRange(-UsableW * 0.3f, UsableW * 0.3f);
    const float JitterY = Stream.FRandRange(-Step * 0.3f, Step * 0.3f);

    return FVector(
        Margin + UsableW * 0.5f + JitterX,
        FMath::Clamp(BaseY + JitterY, Margin, Room.Size.Y - Margin),
        0.0f
    );
}
