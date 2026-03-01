// MXRoomGenerator.cpp — Procedural Generation Module v1.0
// Agent 12: Procedural Generation

#include "MXRoomGenerator.h"
#include "MXConstants.h"

UMXRoomGenerator::UMXRoomGenerator()
{
}

// ---------------------------------------------------------------------------
// Public: GenerateRooms
// ---------------------------------------------------------------------------

TArray<FMXRoomDef> UMXRoomGenerator::GenerateRooms(int64 LevelSeed, int32 LevelNumber, ETier Tier)
{
    FRandomStream Stream(static_cast<int32>(LevelSeed & 0x7FFFFFFF));

    // Determine room count for this level.
    int32 MinRooms = 4, MaxRooms = 6;
    GetRoomCountRange(LevelNumber, Tier, MinRooms, MaxRooms);
    const int32 RoomCount = MinRooms + Stream.RandRange(0, MaxRooms - MinRooms);

    TArray<FMXRoomDef> Rooms;
    Rooms.Reserve(RoomCount);

    for (int32 i = 0; i < RoomCount; ++i)
    {
        FMXRoomDef Room;
        Room.RoomIndex = i;
        Room.RoomType  = PickRoomType(i, RoomCount, LevelNumber, Stream);
        Room.Size      = ComputeRoomSize(Room.RoomType, Stream);
        Room.MaxHazardSlots = GetMaxHazardSlots(Room.RoomType);

        Rooms.Add(Room);
    }

    // Build connectivity graph.
    BuildConnectivity(Rooms, Stream);

    // Assign world-space offsets for spatial layout.
    AssignWorldOffsets(Rooms, Stream);

    UE_LOG(LogTemp, Log,
        TEXT("MXRoomGenerator: Level %d — generated %d rooms (seed=%lld, tier=%d)."),
        LevelNumber, RoomCount, LevelSeed, static_cast<int32>(Tier));

    for (int32 i = 0; i < Rooms.Num(); ++i)
    {
        const FMXRoomDef& R = Rooms[i];
        UE_LOG(LogTemp, Verbose,
            TEXT("  Room[%d]: Type=%d, Size=(%.0f, %.0f), HazardSlots=%d, Connections=%d"),
            R.RoomIndex, static_cast<int32>(R.RoomType),
            R.Size.X, R.Size.Y, R.MaxHazardSlots, R.ConnectedRoomIndices.Num());
    }

    return Rooms;
}

// ---------------------------------------------------------------------------
// Public: ComputeCriticalPath
// ---------------------------------------------------------------------------

TArray<int32> UMXRoomGenerator::ComputeCriticalPath(const TArray<FMXRoomDef>& Rooms)
{
    TArray<int32> Path;
    if (Rooms.Num() < 2) return Path;

    const int32 StartIndex = 0;
    const int32 EndIndex   = Rooms.Num() - 1;

    // BFS from entrance to exit.
    TArray<int32> Parent;
    Parent.SetNumZeroed(Rooms.Num());
    for (int32& P : Parent) P = -1;

    TArray<bool> Visited;
    Visited.SetNumZeroed(Rooms.Num());

    TArray<int32> Queue;
    Queue.Add(StartIndex);
    Visited[StartIndex] = true;

    while (Queue.Num() > 0)
    {
        const int32 Current = Queue[0];
        Queue.RemoveAt(0);

        if (Current == EndIndex) break;

        for (const int32 Neighbor : Rooms[Current].ConnectedRoomIndices)
        {
            if (Neighbor >= 0 && Neighbor < Rooms.Num() && !Visited[Neighbor])
            {
                Visited[Neighbor] = true;
                Parent[Neighbor]  = Current;
                Queue.Add(Neighbor);
            }
        }
    }

    // Reconstruct path from exit back to entrance.
    if (!Visited[EndIndex])
    {
        UE_LOG(LogTemp, Error,
            TEXT("MXRoomGenerator: No path from entrance to exit! Graph is disconnected."));
        // Fallback: linear sequence.
        for (int32 i = 0; i < Rooms.Num(); ++i) Path.Add(i);
        return Path;
    }

    int32 Current = EndIndex;
    while (Current != -1)
    {
        Path.Insert(Current, 0);
        Current = Parent[Current];
    }

    return Path;
}

// ---------------------------------------------------------------------------
// Public: PickGateRoom
// ---------------------------------------------------------------------------

int32 UMXRoomGenerator::PickGateRoom(const TArray<FMXRoomDef>& Rooms, int32 LevelNumber, int64 LevelSeed)
{
    // Gates appear on milestone levels: 5, 10, 15, 20.
    if (LevelNumber % 5 != 0) return -1;

    // Find rooms that are neither entrance nor exit.
    TArray<int32> Candidates;
    for (const FMXRoomDef& R : Rooms)
    {
        if (R.RoomType != ERoomType::Entrance && R.RoomType != ERoomType::Exit)
        {
            Candidates.Add(R.RoomIndex);
        }
    }

    if (Candidates.Num() == 0) return -1;

    // Deterministic pick using the level seed.
    FRandomStream Stream(static_cast<int32>(LevelSeed ^ 0x47415445)); // "GATE"
    const int32 Pick = Stream.RandRange(0, Candidates.Num() - 1);

    UE_LOG(LogTemp, Log,
        TEXT("MXRoomGenerator: Level %d — sacrifice gate placed in Room[%d]."),
        LevelNumber, Candidates[Pick]);

    return Candidates[Pick];
}

// ---------------------------------------------------------------------------
// Private: GetRoomCountRange
// ---------------------------------------------------------------------------

void UMXRoomGenerator::GetRoomCountRange(int32 LevelNumber, ETier Tier, int32& OutMin, int32& OutMax) const
{
    if (LevelNumber <= 5)
    {
        OutMin = 4; OutMax = 6;
    }
    else if (LevelNumber <= 10)
    {
        OutMin = 6; OutMax = 8;
    }
    else if (LevelNumber <= 15)
    {
        OutMin = 7; OutMax = 10;
    }
    else
    {
        OutMin = 8; OutMax = 12;
    }

    // Higher tiers increase both bounds by 1–2.
    const int32 TierBonus = FMath::Min(static_cast<int32>(Tier), 3);
    OutMin += TierBonus / 2;
    OutMax += TierBonus;
}

// ---------------------------------------------------------------------------
// Private: PickRoomType
// ---------------------------------------------------------------------------

ERoomType UMXRoomGenerator::PickRoomType(int32 RoomIndex, int32 RoomCount, int32 LevelNumber, FRandomStream& Stream) const
{
    // First room is always Entrance, last is always Exit.
    if (RoomIndex == 0)            return ERoomType::Entrance;
    if (RoomIndex == RoomCount - 1) return ERoomType::Exit;

    // Second room is always a Corridor (ease in).
    if (RoomIndex == 1) return ERoomType::Corridor;

    // Penultimate room leans toward Arena or Gauntlet for climax.
    if (RoomIndex == RoomCount - 2)
    {
        return (LevelNumber >= 10) ? ERoomType::Gauntlet : ERoomType::Arena;
    }

    // General pool with weighted selection based on level difficulty.
    // Early levels favor corridors and sanctuaries.
    // Late levels favor arenas and gauntlets.
    struct FWeightedType { ERoomType Type; float Weight; };

    const float LevelFactor = static_cast<float>(LevelNumber) / static_cast<float>(MXConstants::MAX_LEVELS);

    TArray<FWeightedType> Pool;
    Pool.Add({ ERoomType::Corridor,    3.0f - LevelFactor * 1.5f });
    Pool.Add({ ERoomType::Arena,       1.5f + LevelFactor * 2.0f });
    Pool.Add({ ERoomType::Bottleneck,  1.0f + LevelFactor * 0.5f });
    Pool.Add({ ERoomType::Sanctuary,   1.5f - LevelFactor * 1.0f });
    Pool.Add({ ERoomType::Gauntlet,    LevelFactor * 2.5f });
    Pool.Add({ ERoomType::Junction,    0.8f });

    // Clamp all weights to >= 0.
    float TotalWeight = 0.0f;
    for (FWeightedType& WT : Pool)
    {
        WT.Weight = FMath::Max(0.0f, WT.Weight);
        TotalWeight += WT.Weight;
    }

    if (TotalWeight <= 0.0f) return ERoomType::Corridor;

    float Roll = Stream.FRandRange(0.0f, TotalWeight);
    for (const FWeightedType& WT : Pool)
    {
        Roll -= WT.Weight;
        if (Roll <= 0.0f) return WT.Type;
    }

    return ERoomType::Corridor;
}

// ---------------------------------------------------------------------------
// Private: ComputeRoomSize
// ---------------------------------------------------------------------------

FVector2D UMXRoomGenerator::ComputeRoomSize(ERoomType RoomType, FRandomStream& Stream) const
{
    float MinW, MaxW, MinL, MaxL;

    switch (RoomType)
    {
    case ERoomType::Entrance:
        MinW = 600.0f;  MaxW = 900.0f;  MinL = 600.0f;  MaxL = 900.0f;
        break;
    case ERoomType::Exit:
        MinW = 500.0f;  MaxW = 800.0f;  MinL = 500.0f;  MaxL = 800.0f;
        break;
    case ERoomType::Corridor:
        MinW = 300.0f;  MaxW = 500.0f;  MinL = 1000.0f; MaxL = 2000.0f;
        break;
    case ERoomType::Arena:
        MinW = 1200.0f; MaxW = 2000.0f; MinL = 1200.0f; MaxL = 2000.0f;
        break;
    case ERoomType::Bottleneck:
        MinW = 200.0f;  MaxW = 350.0f;  MinL = 800.0f;  MaxL = 1500.0f;
        break;
    case ERoomType::Sanctuary:
        MinW = 600.0f;  MaxW = 1000.0f; MinL = 600.0f;  MaxL = 1000.0f;
        break;
    case ERoomType::Gauntlet:
        MinW = 400.0f;  MaxW = 700.0f;  MinL = 2000.0f; MaxL = 3500.0f;
        break;
    case ERoomType::Junction:
        MinW = 800.0f;  MaxW = 1200.0f; MinL = 800.0f;  MaxL = 1200.0f;
        break;
    case ERoomType::GateRoom:
        MinW = 800.0f;  MaxW = 1200.0f; MinL = 800.0f;  MaxL = 1200.0f;
        break;
    default:
        MinW = 500.0f;  MaxW = 1000.0f; MinL = 500.0f;  MaxL = 1000.0f;
        break;
    }

    return FVector2D(
        Stream.FRandRange(MinW, MaxW),
        Stream.FRandRange(MinL, MaxL)
    );
}

// ---------------------------------------------------------------------------
// Private: GetMaxHazardSlots
// ---------------------------------------------------------------------------

int32 UMXRoomGenerator::GetMaxHazardSlots(ERoomType RoomType) const
{
    switch (RoomType)
    {
    case ERoomType::Entrance:    return 0;
    case ERoomType::Exit:        return 1;
    case ERoomType::Sanctuary:   return 0;
    case ERoomType::Corridor:    return 2;
    case ERoomType::Bottleneck:  return 2;
    case ERoomType::Junction:    return 2;
    case ERoomType::Arena:       return 5;
    case ERoomType::Gauntlet:    return 6;
    case ERoomType::GateRoom:    return 1;
    default:                     return 2;
    }
}

// ---------------------------------------------------------------------------
// Private: BuildConnectivity
// ---------------------------------------------------------------------------

void UMXRoomGenerator::BuildConnectivity(TArray<FMXRoomDef>& Rooms, FRandomStream& Stream) const
{
    const int32 N = Rooms.Num();
    if (N < 2) return;

    // Step 1: Build a linear backbone (0 → 1 → 2 → ... → N-1).
    // This guarantees a path from entrance to exit.
    for (int32 i = 0; i < N - 1; ++i)
    {
        Rooms[i].ConnectedRoomIndices.AddUnique(i + 1);
        Rooms[i + 1].ConnectedRoomIndices.AddUnique(i);
    }

    // Step 2: Add some random shortcuts/branches for variety.
    // More connections at higher levels for complexity.
    const int32 ExtraEdges = FMath::Max(0, N / 3);
    for (int32 e = 0; e < ExtraEdges; ++e)
    {
        const int32 A = Stream.RandRange(0, N - 1);
        int32 B = Stream.RandRange(0, N - 1);

        // Ensure B != A and they're not already adjacent in the backbone.
        if (B == A) continue;
        if (FMath::Abs(A - B) <= 1) continue;

        // Don't create too many long shortcuts — limit distance.
        if (FMath::Abs(A - B) > N / 2) continue;

        Rooms[A].ConnectedRoomIndices.AddUnique(B);
        Rooms[B].ConnectedRoomIndices.AddUnique(A);
    }
}

// ---------------------------------------------------------------------------
// Private: AssignWorldOffsets
// ---------------------------------------------------------------------------

void UMXRoomGenerator::AssignWorldOffsets(TArray<FMXRoomDef>& Rooms, FRandomStream& Stream) const
{
    // Simple linear layout with some lateral jitter.
    // Rooms are spaced along the Y axis with random X offsets for branching feel.
    float CurrentY = 0.0f;

    for (int32 i = 0; i < Rooms.Num(); ++i)
    {
        FMXRoomDef& R = Rooms[i];

        const float LateralJitter = Stream.FRandRange(-300.0f, 300.0f);
        R.WorldOffset = FVector(LateralJitter, CurrentY, 0.0f);

        // Advance Y by room length + a gap.
        CurrentY += R.Size.Y + Stream.FRandRange(200.0f, 500.0f);
    }
}
