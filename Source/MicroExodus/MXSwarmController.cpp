// MXSwarmController.cpp — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm

#include "MXSwarmController.h"
#include "MXBoidAI.h"
#include "MXConstants.h"
#include "MXRobotProfile.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CameraComponent.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXSwarmController::UMXSwarmController()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void UMXSwarmController::BeginPlay()
{
    Super::BeginPlay();
    CacheRobotProvider();
    EffectiveSpeed = BaseMovementSpeed;
}

// ---------------------------------------------------------------------------
// TickComponent
// ---------------------------------------------------------------------------

void UMXSwarmController::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentMode == ESwarmMode::Halt)
    {
        // Zero all velocities; robots stay put
        for (FMXSwarmRobotState& State : RobotStates)
        {
            State.Velocity = FVector::ZeroVector;
        }
        return;
    }

    // Determine which groups exist
    TSet<int32> ActiveGroups;
    for (const FMXSwarmRobotState& State : RobotStates)
    {
        if (State.bActive)
        {
            ActiveGroups.Add(State.GroupIndex);
        }
    }

    // Tick each sub-group independently
    for (int32 GroupIdx : ActiveGroups)
    {
        TickGroup(GroupIdx, DeltaTime);
    }

    // Clear single-frame input (player resubmits each tick)
    CurrentInputDirection = FVector2D::ZeroVector;
}

// ---------------------------------------------------------------------------
// Player Input
// ---------------------------------------------------------------------------

void UMXSwarmController::MoveSwarm(FVector2D InputDirection)
{
    CurrentInputDirection = InputDirection;
}

void UMXSwarmController::SetSwarmMode(ESwarmMode Mode)
{
    CurrentMode = Mode;
    EffectiveSpeed = GetSpeedForMode(Mode);
}

void UMXSwarmController::RecallSwarm()
{
    bRecallActive = true;
}

void UMXSwarmController::SplitSwarm(const TArray<FGuid>& GroupA, const TArray<FGuid>& GroupB)
{
    // Reset all to group 0 first
    for (FMXSwarmRobotState& State : RobotStates)
    {
        State.GroupIndex = 0;
    }
    // Assign GroupA -> 1
    for (const FGuid& Id : GroupA)
    {
        if (int32* Idx = RobotIndexMap.Find(Id))
        {
            RobotStates[*Idx].GroupIndex = 1;
        }
    }
    // Assign GroupB -> 2
    for (const FGuid& Id : GroupB)
    {
        if (int32* Idx = RobotIndexMap.Find(Id))
        {
            RobotStates[*Idx].GroupIndex = 2;
        }
    }
}

void UMXSwarmController::MergeSwarm()
{
    for (FMXSwarmRobotState& State : RobotStates)
    {
        State.GroupIndex = 0;
    }
}

TArray<FGuid> UMXSwarmController::LassoSelect(FVector2D ScreenStart, FVector2D ScreenEnd)
{
    TArray<FGuid> Selected;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return Selected;

    // Normalize screen rect
    float MinX = FMath::Min(ScreenStart.X, ScreenEnd.X);
    float MaxX = FMath::Max(ScreenStart.X, ScreenEnd.X);
    float MinY = FMath::Min(ScreenStart.Y, ScreenEnd.Y);
    float MaxY = FMath::Max(ScreenStart.Y, ScreenEnd.Y);

    for (const FMXSwarmRobotState& State : RobotStates)
    {
        if (!State.bActive) continue;

        FVector2D ScreenPos;
        if (PC->ProjectWorldLocationToScreen(State.Position, ScreenPos, true))
        {
            if (ScreenPos.X >= MinX && ScreenPos.X <= MaxX &&
                ScreenPos.Y >= MinY && ScreenPos.Y <= MaxY)
            {
                Selected.Add(State.RobotId);
            }
        }
    }

    // Mark lasso selection flag
    for (FMXSwarmRobotState& State : RobotStates)
    {
        State.bLassoSelected = Selected.Contains(State.RobotId);
    }

    return Selected;
}

// ---------------------------------------------------------------------------
// Swarm State Queries
// ---------------------------------------------------------------------------

FVector UMXSwarmController::GetSwarmCenter() const
{
    FVector Sum = FVector::ZeroVector;
    int32 Count = 0;
    for (const FMXSwarmRobotState& State : RobotStates)
    {
        if (State.bActive)
        {
            Sum += State.Position;
            ++Count;
        }
    }
    return Count > 0 ? Sum / static_cast<float>(Count) : FVector::ZeroVector;
}

float UMXSwarmController::GetSwarmRadius() const
{
    if (RobotStates.Num() < 2) return 0.0f;
    FVector Center = GetSwarmCenter();
    float MaxDist = 0.0f;
    for (const FMXSwarmRobotState& State : RobotStates)
    {
        if (State.bActive)
        {
            float Dist = FVector::Dist(State.Position, Center);
            MaxDist = FMath::Max(MaxDist, Dist);
        }
    }
    return MaxDist;
}

int32 UMXSwarmController::GetSwarmCount() const
{
    int32 Count = 0;
    for (const FMXSwarmRobotState& State : RobotStates)
    {
        if (State.bActive) ++Count;
    }
    return Count;
}

TArray<FGuid> UMXSwarmController::GetActiveRobots() const
{
    TArray<FGuid> Result;
    for (const FMXSwarmRobotState& State : RobotStates)
    {
        if (State.bActive)
        {
            Result.Add(State.RobotId);
        }
    }
    return Result;
}

bool UMXSwarmController::GetRobotState(const FGuid& RobotId, FMXSwarmRobotState& OutState) const
{
    if (const int32* Idx = RobotIndexMap.Find(RobotId))
    {
        OutState = RobotStates[*Idx];
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMXSwarmController::InitializeSwarm(const TMap<FGuid, FVector>& InitialPositions)
{
    RobotStates.Empty();
    RobotIndexMap.Empty();

    for (const TPair<FGuid, FVector>& Pair : InitialPositions)
    {
        FMXSwarmRobotState State;
        State.RobotId = Pair.Key;
        State.Position = Pair.Value;
        State.bActive = true;

        int32 Idx = RobotStates.Add(State);
        RobotIndexMap.Add(Pair.Key, Idx);
    }

    RefreshSpecAndPersonalityOffsets();
}

void UMXSwarmController::RemoveRobotFromSwarm(const FGuid& RobotId)
{
    if (int32* Idx = RobotIndexMap.Find(RobotId))
    {
        RobotStates[*Idx].bActive = false;
        RobotStates[*Idx].Velocity = FVector::ZeroVector;
    }
}

void UMXSwarmController::AddRobotToSwarm(const FGuid& RobotId, const FVector& SpawnPosition)
{
    // If robot exists (inactive), re-activate
    if (int32* Idx = RobotIndexMap.Find(RobotId))
    {
        RobotStates[*Idx].bActive = true;
        RobotStates[*Idx].Position = SpawnPosition;
        RobotStates[*Idx].Velocity = FVector::ZeroVector;
        return;
    }

    // New robot — add fresh state
    FMXSwarmRobotState State;
    State.RobotId = RobotId;
    State.Position = SpawnPosition;
    State.bActive = true;

    // Pull spec data if available
    if (RobotProvider.GetObject())
    {
        FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);
        // Personality offset applied by RefreshSpecAndPersonalityOffsets — do a targeted refresh
        // For now initialize defaults; full refresh called separately
    }

    int32 Idx = RobotStates.Add(State);
    RobotIndexMap.Add(RobotId, Idx);
    RefreshSpecAndPersonalityOffsets();
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

float UMXSwarmController::GetSpeedForMode(ESwarmMode Mode) const
{
    switch (Mode)
    {
        case ESwarmMode::Careful: return BaseMovementSpeed * CarefulSpeedMultiplier;
        case ESwarmMode::Sprint:  return BaseMovementSpeed * SprintSpeedMultiplier;
        case ESwarmMode::Halt:    return 0.0f;
        default:                  return BaseMovementSpeed;
    }
}

void UMXSwarmController::CacheRobotProvider()
{
    // Locate the RobotManager from the GameInstance (it registers itself as IMXRobotProvider)
    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        // Iterate subsystems or check directly — depends on how Identity registers itself.
        // Pattern: GameInstance implements IMXRobotProvider directly, or has a subsystem.
        if (GI->GetClass()->ImplementsInterface(UMXRobotProvider::StaticClass()))
        {
            RobotProvider = TScriptInterface<IMXRobotProvider>(GI);
        }
    }
}

void UMXSwarmController::RefreshSpecAndPersonalityOffsets()
{
    for (FMXSwarmRobotState& State : RobotStates)
    {
        if (!RobotProvider.GetObject()) continue;

        FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), State.RobotId);

        // Apply Scout speed bonuses
        float SpeedMod = 1.0f;
        if (Profile.role == ERobotRole::Scout)
        {
            if (Profile.tier3_spec == ETier3Spec::Trailblazer || Profile.tier3_spec == ETier3Spec::Ghost)
            {
                SpeedMod = 1.20f; // +20% at Tier 3 Scout
            }
            else if (Profile.tier2_spec != ETier2Spec::None)
            {
                SpeedMod = 1.15f; // +15% at Tier 2 Scout
            }
        }
        // Guardian slight negative (tanky archetype)
        else if (Profile.role == ERobotRole::Guardian)
        {
            SpeedMod = 0.95f;
        }
        State.SpecSpeedModifier = SpeedMod;

        // Personality-based positional offset (applied by BoidAI, cached here)
        // Reckless quirks: slight forward bias
        // Cautious quirks: slight backward bias
        FVector Offset = FVector::ZeroVector;
        if (Profile.quirk.Contains(TEXT("reckless")) || Profile.quirk.Contains(TEXT("bold")) ||
            Profile.quirk.Contains(TEXT("fearless")) || Profile.quirk.Contains(TEXT("daring")))
        {
            Offset = FVector(30.0f, 0.0f, 0.0f); // slight forward
        }
        else if (Profile.quirk.Contains(TEXT("cautious")) || Profile.quirk.Contains(TEXT("timid")) ||
                 Profile.quirk.Contains(TEXT("nervous")) || Profile.quirk.Contains(TEXT("careful")))
        {
            Offset = FVector(-30.0f, 0.0f, 0.0f); // slight backward
        }
        State.PersonalityOffset = Offset;
    }
}

void UMXSwarmController::TickGroup(int32 GroupIndex, float DeltaTime)
{
    // Collect positions for this group
    TArray<FVector> GroupPositions;
    TArray<int32> GroupIndices; // indices into RobotStates
    for (int32 i = 0; i < RobotStates.Num(); ++i)
    {
        const FMXSwarmRobotState& S = RobotStates[i];
        if (S.bActive && S.GroupIndex == GroupIndex)
        {
            GroupPositions.Add(S.Position);
            GroupIndices.Add(i);
        }
    }

    if (GroupPositions.Num() == 0) return;

    // Compute group center
    FVector GroupCenter = FVector::ZeroVector;
    for (const FVector& Pos : GroupPositions) GroupCenter += Pos;
    GroupCenter /= static_cast<float>(GroupPositions.Num());

    // For each robot in this group, compute boid forces and integrate
    UMXBoidAI* BoidAI = GetOwner() ? GetOwner()->FindComponentByClass<UMXBoidAI>() : nullptr;

    for (int32 i = 0; i < GroupIndices.Num(); ++i)
    {
        FMXSwarmRobotState& State = RobotStates[GroupIndices[i]];

        FVector Force = FVector::ZeroVector;

        if (BoidAI)
        {
            Force = BoidAI->CalculateBoidForces(State.RobotId, GroupPositions, CurrentInputDirection);
        }
        else
        {
            // Fallback inline boid if BoidAI component not present
            // Cohesion: steer toward group center
            FVector ToCentre = GroupCenter - State.Position;
            if (!ToCentre.IsNearlyZero()) ToCentre.Normalize();
            Force += ToCentre * 200.0f;

            // Player input
            FVector InputWorld(CurrentInputDirection.Y * 200.0f, CurrentInputDirection.X * 200.0f, 0.0f);
            Force += InputWorld;
        }

        // Recall override
        if (bRecallActive)
        {
            FVector ToCenter = GroupCenter - State.Position;
            if (ToCenter.Size() > StraggleDistanceThreshold)
            {
                ToCenter.Normalize();
                Force += ToCenter * RecallForceStrength * 1000.0f;
            }
            else if (GroupIndex == 0) // recall done for this robot
            {
                // Check if all robots are close enough to turn off recall
            }
        }

        // Personality offset (additive)
        Force += State.PersonalityOffset;

        // Apply speed cap
        float MaxSpeed = EffectiveSpeed * State.SpecSpeedModifier;
        FVector NewVelocity = State.Velocity + Force * DeltaTime;
        if (NewVelocity.Size() > MaxSpeed)
        {
            NewVelocity = NewVelocity.GetSafeNormal() * MaxSpeed;
        }
        State.Velocity = NewVelocity;
        State.Position += State.Velocity * DeltaTime;
    }

    // Check if recall can be deactivated
    if (bRecallActive)
    {
        bool bAllClose = true;
        for (int32 Idx : GroupIndices)
        {
            if (FVector::Dist(RobotStates[Idx].Position, GroupCenter) > StraggleDistanceThreshold * 0.5f)
            {
                bAllClose = false;
                break;
            }
        }
        if (bAllClose) bRecallActive = false;
    }
}
