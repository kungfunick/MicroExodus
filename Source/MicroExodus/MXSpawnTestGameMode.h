// MXSpawnTestGameMode.h — Phase 2C-Move Update
// Originally created: Phase 2A
// Updated: 2026-03-09 — Integrated AMXTestFloorGenerator, removed GravityScale hack,
//   spawns robots on procedural floor surface.
//
// Test game mode that spawns robots via UMXRobotManager and places them on a
// procedurally generated floor. Uses AMXRTSPlayerController for camera + selection.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MXSpawnTestGameMode.generated.h"

// Forward declarations
class AMXRobotActor;
class AMXCameraRig;
class AMXTestFloorGenerator;
class UMXRobotManager;

// ---------------------------------------------------------------------------
// AMXSpawnTestGameMode
// ---------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXSpawnTestGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    AMXSpawnTestGameMode();

protected:

    virtual void BeginPlay() override;

public:

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    /** Number of robots to spawn for testing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    int32 NumRobots = 8;

    /** Class to use for robot actors. Override in Blueprint if needed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    TSubclassOf<AMXRobotActor> RobotActorClass;

    /** Class to use for camera rig. Override in Blueprint if needed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    TSubclassOf<AMXCameraRig> CameraRigClass;

    /** Class to use for floor generator. Override in Blueprint if needed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    TSubclassOf<AMXTestFloorGenerator> FloorGeneratorClass;

    // ---- Floor Config (passed to generator if using default class) ----

    /** Floor grid size X. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest|Floor")
    int32 FloorGridX = 10;

    /** Floor grid size Y. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest|Floor")
    int32 FloorGridY = 10;

    /** Floor tile size in cm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest|Floor")
    float FloorTileSize = 200.0f;

private:

    // -------------------------------------------------------------------------
    // Spawned References
    // -------------------------------------------------------------------------

    UPROPERTY()
    TArray<AMXRobotActor*> SpawnedRobots;

    UPROPERTY()
    TObjectPtr<AMXCameraRig> SpawnedCameraRig;

    UPROPERTY()
    TObjectPtr<AMXTestFloorGenerator> SpawnedFloor;

    // -------------------------------------------------------------------------
    // Internal
    // -------------------------------------------------------------------------

    /** Spawn the floor generator and build the floor. */
    void SpawnFloor();

    /** Spawn robot actors using the Identity module. */
    void SpawnRobots();

    /** Spawn camera rig and configure it. */
    void SpawnCamera();
};
