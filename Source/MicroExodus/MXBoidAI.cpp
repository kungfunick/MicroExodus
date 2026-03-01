// MXBoidAI.cpp — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm

#include "MXBoidAI.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXBoidAI::UMXBoidAI()
{
    PrimaryComponentTick.bCanEverTick = false; // driven by SwarmController tick

    // Default "Normal" config
    NormalConfig.SeparationWeight   = 1.5f;
    NormalConfig.AlignmentWeight    = 1.0f;
    NormalConfig.CohesionWeight     = 1.2f;
    NormalConfig.PlayerInputWeight  = 3.0f;
    NormalConfig.SeparationRadius   = 80.0f;
    NormalConfig.NeighborRadius     = 300.0f;
    NormalConfig.MaxSpeed           = 400.0f;
    NormalConfig.MaxTurnRateDegrees = 180.0f;

    // "Careful" — tight cohesion, heavy separation (don't crowd), slow input response
    CarefulConfig.SeparationWeight   = 2.5f;
    CarefulConfig.AlignmentWeight    = 1.5f;
    CarefulConfig.CohesionWeight     = 2.0f;
    CarefulConfig.PlayerInputWeight  = 2.0f;
    CarefulConfig.SeparationRadius   = 60.0f;
    CarefulConfig.NeighborRadius     = 200.0f;
    CarefulConfig.MaxSpeed           = 200.0f;
    CarefulConfig.MaxTurnRateDegrees = 90.0f;

    // "Sprint" — loose cohesion, fast input response, wide spread
    SprintConfig.SeparationWeight   = 1.0f;
    SprintConfig.AlignmentWeight    = 0.8f;
    SprintConfig.CohesionWeight     = 0.8f;
    SprintConfig.PlayerInputWeight  = 4.5f;
    SprintConfig.SeparationRadius   = 100.0f;
    SprintConfig.NeighborRadius     = 400.0f;
    SprintConfig.MaxSpeed           = 700.0f;
    SprintConfig.MaxTurnRateDegrees = 270.0f;

    Config = NormalConfig;
}

// ---------------------------------------------------------------------------
// Core Boid Calculation
// ---------------------------------------------------------------------------

FVector UMXBoidAI::CalculateBoidForces(const FGuid& RobotId,
                                        const TArray<FVector>& AllGroupPositions,
                                        FVector2D PlayerInput) const
{
    if (AllGroupPositions.Num() == 0) return FVector::ZeroVector;

    // We don't have MyPosition passed directly — need to find it.
    // For this calculation, the SwarmController always includes the robot's own position
    // in AllGroupPositions. We use the personality offset map to infer which one is "me."
    // In practice, SwarmController passes position index separately — but the API here
    // takes the full array. We assume index 0 is always the focal robot (caller's responsibility).
    // (SwarmController builds the array with the focal robot first.)
    const FVector& MyPosition = AllGroupPositions[0];

    // Compute group center (for cohesion)
    FVector GroupCenter = FVector::ZeroVector;
    for (const FVector& Pos : AllGroupPositions) GroupCenter += Pos;
    GroupCenter /= static_cast<float>(AllGroupPositions.Num());

    // Boid component forces
    FVector Sep = CalculateSeparation(MyPosition, AllGroupPositions);
    FVector Ali = CalculateAlignment(MyPosition, AllGroupPositions);
    FVector Coh = CalculateCohesion(MyPosition, GroupCenter);

    // Player input — convert 2D screen-space input to world-space XY force
    FVector InputForce(
        PlayerInput.Y * Config.MaxSpeed,  // Y input = forward (world X)
        PlayerInput.X * Config.MaxSpeed,  // X input = right   (world Y)
        0.0f
    );

    // Personality offset
    FVector PersonalityForce = FVector::ZeroVector;
    if (const FVector* Offset = PersonalityOffsets.Find(RobotId))
    {
        PersonalityForce = *Offset;
    }

    // Combine
    FVector TotalForce =
        Sep       * Config.SeparationWeight  +
        Ali       * Config.AlignmentWeight   +
        Coh       * Config.CohesionWeight    +
        InputForce * Config.PlayerInputWeight +
        PersonalityForce;

    // Clamp magnitude to max speed
    if (TotalForce.Size() > Config.MaxSpeed)
    {
        TotalForce = TotalForce.GetSafeNormal() * Config.MaxSpeed;
    }

    return TotalForce;
}

// ---------------------------------------------------------------------------
// Personality Offsets
// ---------------------------------------------------------------------------

void UMXBoidAI::SetPersonalityOffset(const FGuid& RobotId, const FString& Quirk)
{
    PersonalityOffsets.Add(RobotId, QuirkToOffset(Quirk));
}

FVector UMXBoidAI::GetPersonalityOffset(const FGuid& RobotId) const
{
    if (const FVector* Offset = PersonalityOffsets.Find(RobotId))
    {
        return *Offset;
    }
    return FVector::ZeroVector;
}

void UMXBoidAI::ClearAllOffsets()
{
    PersonalityOffsets.Empty();
}

void UMXBoidAI::ApplyConfig(const FMXBoidConfig& NewConfig)
{
    Config = NewConfig;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

FVector UMXBoidAI::CalculateSeparation(const FVector& MyPosition,
                                        const TArray<FVector>& Neighbors) const
{
    FVector Steer = FVector::ZeroVector;
    int32 Count = 0;

    for (const FVector& Neighbor : Neighbors)
    {
        if (Neighbor.Equals(MyPosition, 1.0f)) continue; // skip self

        float Dist = FVector::Dist(MyPosition, Neighbor);
        if (Dist < Config.SeparationRadius && Dist > 0.0f)
        {
            // Repulsion inversely proportional to distance
            FVector Away = (MyPosition - Neighbor).GetSafeNormal();
            Steer += Away / Dist;
            ++Count;
        }
    }

    if (Count > 0)
    {
        Steer /= static_cast<float>(Count);
        if (!Steer.IsNearlyZero())
        {
            Steer = Steer.GetSafeNormal() * Config.MaxSpeed;
        }
    }

    return Steer;
}

FVector UMXBoidAI::CalculateAlignment(const FVector& MyPosition,
                                       const TArray<FVector>& Neighbors) const
{
    // Simplified alignment: average of normalized neighbor-to-center direction vectors
    // (Classic alignment requires velocity data we don't pass here — we approximate
    // by aligning toward the average neighbor heading relative to group centroid.)
    FVector AvgDir = FVector::ZeroVector;
    int32 Count = 0;

    FVector GroupCenter = FVector::ZeroVector;
    for (const FVector& N : Neighbors) GroupCenter += N;
    if (Neighbors.Num() > 0) GroupCenter /= static_cast<float>(Neighbors.Num());

    for (const FVector& Neighbor : Neighbors)
    {
        if (Neighbor.Equals(MyPosition, 1.0f)) continue;

        float Dist = FVector::Dist(MyPosition, Neighbor);
        if (Dist < Config.NeighborRadius)
        {
            FVector ToCenter = (GroupCenter - Neighbor);
            if (!ToCenter.IsNearlyZero()) ToCenter.Normalize();
            AvgDir += ToCenter;
            ++Count;
        }
    }

    if (Count > 0)
    {
        AvgDir /= static_cast<float>(Count);
        if (!AvgDir.IsNearlyZero()) AvgDir.Normalize();
        return AvgDir * Config.MaxSpeed;
    }

    return FVector::ZeroVector;
}

FVector UMXBoidAI::CalculateCohesion(const FVector& MyPosition,
                                      const FVector& GroupCenter) const
{
    FVector ToCenter = GroupCenter - MyPosition;
    if (ToCenter.IsNearlyZero()) return FVector::ZeroVector;

    // Scale force by distance — further from center = stronger pull
    float Dist = ToCenter.Size();
    float Strength = FMath::Clamp(Dist / Config.NeighborRadius, 0.0f, 1.0f);
    return ToCenter.GetSafeNormal() * Config.MaxSpeed * Strength;
}

FVector UMXBoidAI::QuirkToOffset(const FString& Quirk) const
{
    const FString QuirkLower = Quirk.ToLower();

    // Reckless archetypes — forward bias
    if (QuirkLower.Contains(TEXT("reckless"))  ||
        QuirkLower.Contains(TEXT("bold"))       ||
        QuirkLower.Contains(TEXT("fearless"))   ||
        QuirkLower.Contains(TEXT("daring"))     ||
        QuirkLower.Contains(TEXT("impulsive"))  ||
        QuirkLower.Contains(TEXT("aggressive")))
    {
        return FVector(40.0f, 0.0f, 0.0f);
    }

    // Cautious archetypes — backward / lagging bias
    if (QuirkLower.Contains(TEXT("cautious"))  ||
        QuirkLower.Contains(TEXT("timid"))      ||
        QuirkLower.Contains(TEXT("nervous"))    ||
        QuirkLower.Contains(TEXT("careful"))    ||
        QuirkLower.Contains(TEXT("hesitant"))   ||
        QuirkLower.Contains(TEXT("paranoid")))
    {
        return FVector(-40.0f, 0.0f, 0.0f);
    }

    // Wandering types — slight lateral offset (random L/R)
    if (QuirkLower.Contains(TEXT("curious"))   ||
        QuirkLower.Contains(TEXT("distracted")) ||
        QuirkLower.Contains(TEXT("scatter")))
    {
        // Deterministic side-bias based on quirk hash
        int32 Hash = GetTypeHash(Quirk);
        float Side = (Hash % 2 == 0) ? 25.0f : -25.0f;
        return FVector(0.0f, Side, 0.0f);
    }

    return FVector::ZeroVector;
}
