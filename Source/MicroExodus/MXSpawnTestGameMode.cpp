// MXSpawnTestGameMode.cpp — Phase 2A: Spawn Test GameMode
// Created: 2026-03-06

#include "MXSpawnTestGameMode.h"
#include "MXRobotActor.h"
#include "MXCameraRig.h"
#include "MXRTSPlayerController.h"
#include "MXGameInstance.h"
#include "MXRobotManager.h"
#include "MXRobotProfile.h"
#include "MXSwarmCamera.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXSpawnTestGameMode::AMXSpawnTestGameMode()
{
    // Default classes — can be overridden in Blueprint or World Settings.
    RobotActorClass = AMXRobotActor::StaticClass();
    CameraRigClass  = AMXCameraRig::StaticClass();
    PlayerControllerClass = AMXRTSPlayerController::StaticClass();

    // No default pawn — the RTS controller drives the CameraRig directly.
    DefaultPawnClass = nullptr;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXSpawnTestGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("========================================"));
    UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] Phase 2A Spawn Test BEGIN"));
    UE_LOG(LogTemp, Log, TEXT("========================================"));

    // ---- Step 1: Get the GameInstance and RobotManager ----

    UMXGameInstance* MXGameInstance = Cast<UMXGameInstance>(GetGameInstance());
    if (!MXGameInstance)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[MXSpawnTest] GameInstance is not UMXGameInstance! "
                 "Check DefaultEngine.ini GameInstanceClass setting."));
        return;
    }

    UMXRobotManager* RobotManager = MXGameInstance->GetRobotManager();
    if (!RobotManager)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[MXSpawnTest] RobotManager is null! UMXGameInstance::Init() may have failed."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] GameInstance and RobotManager OK."));

    // ---- Step 2: Create robot profiles via the Identity module ----

    TArray<FGuid> RobotIds;
    const int32 RunNumber = 1;
    const int32 LevelNumber = 1;

    for (int32 i = 0; i < NumRobots; ++i)
    {
        FGuid NewId = RobotManager->CreateRobot(LevelNumber, RunNumber);
        if (NewId.IsValid())
        {
            RobotIds.Add(NewId);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[MXSpawnTest] CreateRobot failed at index %d (roster full?)."), i);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] Created %d robot profiles."), RobotIds.Num());

    // ---- Step 3: Spawn robot actors at staggered positions ----

    if (!RobotActorClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MXSpawnTest] RobotActorClass is null!"));
        return;
    }

    TArray<FVector> SpawnPositions = CalculateSpawnPositions(RobotIds.Num(), SpawnRadius, FloorZ);

    for (int32 i = 0; i < RobotIds.Num(); ++i)
    {
        FVector SpawnLocation = SpawnPositions.IsValidIndex(i)
            ? SpawnPositions[i]
            : FVector(i * 80.0f, 0.0f, FloorZ); // Fallback: line formation

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AMXRobotActor* RobotActor = GetWorld()->SpawnActor<AMXRobotActor>(
            RobotActorClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

        if (!RobotActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MXSpawnTest] Failed to spawn robot actor at index %d."), i);
            continue;
        }

        // Bind identity data to the actor.
        FMXRobotProfile Profile = RobotManager->GetRobotProfile_Implementation(RobotIds[i]);
        RobotActor->BindToProfile(Profile.robot_id, Profile.name);

        SpawnedRobots.Add(RobotActor);

        UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] Spawned robot '%s' at (%.0f, %.0f, %.0f)"),
               *Profile.name, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }

    UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] Spawned %d robot actors."), SpawnedRobots.Num());

    // ---- Step 4: Spawn the Camera Rig ----

    if (!CameraRigClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MXSpawnTest] CameraRigClass is null!"));
        return;
    }

    FVector CameraSpawnPos = FVector(0.0f, 0.0f, CameraRigZ);
    CameraRig = GetWorld()->SpawnActor<AMXCameraRig>(
        CameraRigClass, CameraSpawnPos, FRotator::ZeroRotator);

    if (!CameraRig)
    {
        UE_LOG(LogTemp, Error, TEXT("[MXSpawnTest] Failed to spawn CameraRig!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] CameraRig spawned at (%.0f, %.0f, %.0f)."),
           CameraSpawnPos.X, CameraSpawnPos.Y, CameraSpawnPos.Z);

    // ---- Step 5: Wire camera — initial position only, then hand off to RTS controller ----

    UMXSwarmCamera* SwarmCamera = CameraRig->GetSwarmCamera();
    if (SwarmCamera)
    {
        // Feed robot positions for future use (inspect mode, events, etc.).
        SwarmCamera->SetSwarmTarget(RobotIds);
        for (const AMXRobotActor* Robot : SpawnedRobots)
        {
            if (Robot)
            {
                SwarmCamera->UpdateRobotPosition(Robot->RobotId, Robot->GetActorLocation());
            }
        }

        // Position the rig at the swarm centroid so the camera starts centered.
        FVector Centroid = FVector::ZeroVector;
        for (const AMXRobotActor* Robot : SpawnedRobots)
        {
            if (Robot) Centroid += Robot->GetActorLocation();
        }
        if (SpawnedRobots.Num() > 0)
        {
            Centroid /= static_cast<float>(SpawnedRobots.Num());
        }
        CameraRig->SetActorLocation(FVector(Centroid.X, Centroid.Y, CameraRigZ));

        // Disable SwarmCamera tick — RTS controller now owns camera movement.
        // SwarmCamera's TickPositionTracking would fight the RTS pan/zoom.
        // Re-enable later when swarm tracking is needed during gameplay.
        SwarmCamera->SetComponentTickEnabled(false);

        UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] SwarmCamera positioned at centroid (%.0f, %.0f), tick DISABLED — RTS controller active."),
               Centroid.X, Centroid.Y);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MXSpawnTest] CameraRig has no SwarmCamera component!"));
    }

    // ---- Done ----

    UE_LOG(LogTemp, Log, TEXT("========================================"));
    UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] Phase 2A Spawn Test COMPLETE"));
    UE_LOG(LogTemp, Log, TEXT("  Robots: %d"), SpawnedRobots.Num());
    UE_LOG(LogTemp, Log, TEXT("  Camera: %s"), CameraRig ? TEXT("Active") : TEXT("FAILED"));
    UE_LOG(LogTemp, Log, TEXT("========================================"));
}

// ---------------------------------------------------------------------------
// CalculateSpawnPositions
// ---------------------------------------------------------------------------

TArray<FVector> AMXSpawnTestGameMode::CalculateSpawnPositions(
    int32 Count, float Radius, float Z) const
{
    TArray<FVector> Positions;
    Positions.Reserve(Count);

    if (Count <= 0) return Positions;

    if (Count == 1)
    {
        Positions.Add(FVector(0.0f, 0.0f, Z));
        return Positions;
    }

    // Arrange in a circle.
    const float AngleStep = 360.0f / static_cast<float>(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        float AngleRad = FMath::DegreesToRadians(AngleStep * i);
        float X = Radius * FMath::Cos(AngleRad);
        float Y = Radius * FMath::Sin(AngleRad);
        Positions.Add(FVector(X, Y, Z));
    }

    return Positions;
}
