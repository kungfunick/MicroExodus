// MXSpawnTestGameMode.h — Phase 2A: Spawn Test GameMode
// Created: 2026-03-06
// Purpose: Minimal GameMode for the L_SpawnTest level.
//          Spawns N miniature robots, assigns them names from the
//          Identity module, spawns the camera rig, and wires everything.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MXSpawnTestGameMode.generated.h"

class AMXRobotActor;
class AMXCameraRig;
class UMXGameInstance;
class UMXRobotManager;

/**
 * AMXSpawnTestGameMode
 * Drop-in GameMode for L_SpawnTest. Set as the level's GameMode Override
 * in World Settings, or set as the project default for testing.
 *
 * In BeginPlay:
 *   1. Gets UMXGameInstance → UMXRobotManager
 *   2. Creates N robot profiles via CreateRobot()
 *   3. Spawns AMXRobotActor at staggered positions on the test floor
 *   4. Binds each actor to its profile (GUID + name)
 *   5. Spawns AMXCameraRig and tells UMXSwarmCamera to track all robots
 *   6. Feeds initial robot positions to the camera
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXSpawnTestGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    AMXSpawnTestGameMode();

    // =========================================================================
    // Configuration
    // =========================================================================

    /** Number of robots to spawn. Default 8. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    int32 NumRobots = 8;

    /** Class to spawn for each robot. Default: AMXRobotActor. Override for BP child. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    TSubclassOf<AMXRobotActor> RobotActorClass;

    /** Class to spawn for the camera rig. Default: AMXCameraRig. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    TSubclassOf<AMXCameraRig> CameraRigClass;

    /**
     * Spawn area radius (cm). Robots are placed in a circle on the XY plane.
     * Default 200cm — at 0.20 robot scale, this gives comfortable spacing.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    float SpawnRadius = 200.0f;

    /**
     * Height at which the camera rig is placed (Z offset from the floor).
     * The SpringArm provides additional height via arm length + pitch.
     * Default 0 — rig sits at floor level, spring arm does the elevation.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    float CameraRigZ = 0.0f;

    /** Z coordinate of the spawn floor plane. Default 0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|SpawnTest")
    float FloorZ = 0.0f;

protected:

    virtual void BeginPlay() override;

private:

    /** Spawned robot actors — kept for reference / cleanup. */
    UPROPERTY()
    TArray<TObjectPtr<AMXRobotActor>> SpawnedRobots;

    /** The spawned camera rig. */
    UPROPERTY()
    TObjectPtr<AMXCameraRig> CameraRig;

    /** Calculate spawn positions in a circle on the XY plane. */
    TArray<FVector> CalculateSpawnPositions(int32 Count, float Radius, float Z) const;
};
