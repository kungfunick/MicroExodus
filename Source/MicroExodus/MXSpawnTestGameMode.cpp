// MXSpawnTestGameMode.cpp — Phase 2A: Spawn Test GameMode
// Created: 2026-03-06

#include "MXSpawnTestGameMode.h"
#include "MXRobotActor.h"
#include "MXCameraRig.h"
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

    // ---- Step 5: Wire camera tracking ----

    UMXSwarmCamera* SwarmCamera = CameraRig->GetSwarmCamera();
    if (SwarmCamera)
    {
        // Tell the camera which robots to track.
        SwarmCamera->SetSwarmTarget(RobotIds);

        // Feed initial positions so the camera knows where each robot is.
        // UMXSwarmCamera::RobotPositions is private — we use UpdateRobotPosition()
        // (added in Phase 2A patch to MXSwarmCamera).
        for (const AMXRobotActor* Robot : SpawnedRobots)
        {
            if (Robot)
            {
                SwarmCamera->UpdateRobotPosition(Robot->RobotId, Robot->GetActorLocation());
            }
        }

        // Trigger initial zoom for the swarm count.
        SwarmCamera->UpdateZoom(RobotIds.Num(), 0.01f);

        UE_LOG(LogTemp, Log, TEXT("[MXSpawnTest] SwarmCamera tracking %d robots."), RobotIds.Num());
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
