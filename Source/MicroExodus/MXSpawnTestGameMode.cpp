// MXSpawnTestGameMode.cpp — Phase 2C-Move Update
// Originally created: Phase 2A
// Updated: 2026-03-09

#include "MXSpawnTestGameMode.h"
#include "MXRobotActor.h"
#include "MXCameraRig.h"
#include "MXTestFloorGenerator.h"
#include "MXRTSPlayerController.h"
#include "MXRobotManager.h"
#include "MXInterfaces.h"
#include "MXSwarmCamera.h"
#include "MXGameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXSpawnTestGameMode::AMXSpawnTestGameMode()
{
    // Use the RTS controller (handles camera + selection + movement).
    PlayerControllerClass = AMXRTSPlayerController::StaticClass();

    // No pawn — the controller drives the camera rig directly.
    DefaultPawnClass = nullptr;

    // Default classes (can be overridden in Blueprint).
    RobotActorClass = AMXRobotActor::StaticClass();
    CameraRigClass = AMXCameraRig::StaticClass();
    FloorGeneratorClass = AMXTestFloorGenerator::StaticClass();
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXSpawnTestGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Order matters: floor first (so robots have collision surface),
    // then robots, then camera (needs robot positions for centroid).
    SpawnFloor();
    SpawnRobots();
    SpawnCamera();

    UE_LOG(LogTemp, Log,
        TEXT("MXSpawnTestGameMode: Setup complete — %d robots on %dx%d floor."),
        SpawnedRobots.Num(), FloorGridX, FloorGridY);
}

// ---------------------------------------------------------------------------
// SpawnFloor
// ---------------------------------------------------------------------------

void AMXSpawnTestGameMode::SpawnFloor()
{
    UWorld* World = GetWorld();
    if (!World || !FloorGeneratorClass) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    SpawnedFloor = World->SpawnActor<AMXTestFloorGenerator>(
        FloorGeneratorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

    if (SpawnedFloor)
    {
        // Apply config.
        SpawnedFloor->GridSizeX = FloorGridX;
        SpawnedFloor->GridSizeY = FloorGridY;
        SpawnedFloor->TileSize = FloorTileSize;
        SpawnedFloor->FloorZ = 0.0f;

        // Generate (bAutoGenerate is true by default, but we set config after spawn
        // so we need to regenerate with our values).
        SpawnedFloor->GenerateFloor();

        UE_LOG(LogTemp, Log, TEXT("MXSpawnTestGameMode: Floor generated (%dx%d)."),
               FloorGridX, FloorGridY);
    }
}

// ---------------------------------------------------------------------------
// SpawnRobots
// ---------------------------------------------------------------------------

void AMXSpawnTestGameMode::SpawnRobots()
{
    UWorld* World = GetWorld();
    if (!World || !RobotActorClass) return;

    // Get the RobotManager from GameInstance.
    UMXGameInstance* GI = Cast<UMXGameInstance>(UGameplayStatics::GetGameInstance(this));
    UMXRobotManager* RobotManager = GI ? GI->GetRobotManager() : nullptr;

    if (!RobotManager)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXSpawnTestGameMode: No RobotManager — spawning robots without Identity module."));
    }

    for (int32 i = 0; i < NumRobots; ++i)
    {
        // Get a position on the floor.
        FVector SpawnPos = FVector::ZeroVector;
        if (SpawnedFloor)
        {
            SpawnPos = SpawnedFloor->GetRandomFloorPosition(100.0f);
            SpawnPos.Z += 20.0f; // Slightly above floor for spawn safety.
        }
        else
        {
            // Fallback: circle layout.
            float Angle = (2.0f * PI * i) / NumRobots;
            SpawnPos = FVector(FMath::Cos(Angle) * 200.0f, FMath::Sin(Angle) * 200.0f, 20.0f);
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AMXRobotActor* Robot = World->SpawnActor<AMXRobotActor>(
            RobotActorClass, SpawnPos, FRotator::ZeroRotator, SpawnParams);

        if (!Robot) continue;

        // Bind to Identity module profile (if available).
        FGuid RobotId;
        FString RobotNameStr;

        if (RobotManager)
        {
            RobotId = RobotManager->CreateRobot(1, 1); // LevelNumber=1, RunNumber=1
            if (RobotId.IsValid())
            {
                FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotManager, RobotId);
                RobotNameStr = Profile.name;
            }
        }

        // Fallback name if Identity module isn't available.
        if (!RobotId.IsValid())
        {
            RobotId = FGuid::NewGuid();
            RobotNameStr = FString::Printf(TEXT("Robot-%02d"), i + 1);
        }

        Robot->BindToProfile(RobotId, RobotNameStr);
        SpawnedRobots.Add(Robot);
    }

    UE_LOG(LogTemp, Log, TEXT("MXSpawnTestGameMode: Spawned %d robots."), SpawnedRobots.Num());
}

// ---------------------------------------------------------------------------
// SpawnCamera
// ---------------------------------------------------------------------------

void AMXSpawnTestGameMode::SpawnCamera()
{
    UWorld* World = GetWorld();
    if (!World || !CameraRigClass) return;

    // Position camera at floor center, elevated.
    FVector CameraPos = FVector::ZeroVector;
    if (SpawnedFloor)
    {
        CameraPos = SpawnedFloor->GetFloorCenter();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    SpawnedCameraRig = World->SpawnActor<AMXCameraRig>(
        CameraRigClass, CameraPos, FRotator::ZeroRotator, SpawnParams);

    if (!SpawnedCameraRig) return;

    // Feed robot positions to the SwarmCamera for initial centroid calculation.
    UMXSwarmCamera* SwarmCam = SpawnedCameraRig->FindComponentByClass<UMXSwarmCamera>();
    if (SwarmCam)
    {
        TArray<FGuid> RobotIds;
        for (AMXRobotActor* Robot : SpawnedRobots)
        {
            if (Robot)
            {
                RobotIds.Add(Robot->RobotId);
                SwarmCam->UpdateRobotPosition(Robot->RobotId, Robot->GetActorLocation());
            }
        }
        SwarmCam->SetSwarmTarget(RobotIds);

        // Disable SwarmCamera tick — RTS controller drives camera position.
        SwarmCam->SetComponentTickEnabled(false);
    }

    UE_LOG(LogTemp, Log, TEXT("MXSpawnTestGameMode: Camera rig spawned at floor center."));
}
