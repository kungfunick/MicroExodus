// MXRobotActor.h — Phase 2A: Robot Actor for Spawn Test
// Created: 2026-03-06
// Purpose: ACharacter subclass representing a miniaturised mannequin robot.
//          Binds to the Identity module via RobotId/RobotName.
//          Displays the robot's name via a UTextRenderComponent above its head.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MXRobotActor.generated.h"

class UTextRenderComponent;

/**
 * AMXRobotActor
 * A tiny mannequin robot character. Spawned by the GameMode (or later,
 * the Swarm module) and bound to an FMXRobotProfile via its RobotId.
 *
 * The actor is uniformly scaled to RobotScale (default 0.20) in BeginPlay,
 * producing a ~36cm robot from the standard ~180cm mannequin.
 *
 * The skeletal mesh is NOT hardcoded — set it in a Blueprint child class
 * (BP_MXRobot) or assign SkeletalMeshAsset in the editor. This avoids
 * coupling to a specific content path that may differ between setups.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXRobotActor : public ACharacter
{
    GENERATED_BODY()

public:

    AMXRobotActor();

    // =========================================================================
    // Identity Binding
    // =========================================================================

    /** The robot's unique GUID from the Identity module. Set after spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot")
    FGuid RobotId;

    /** The robot's display name from UMXNameGenerator. Set after spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot")
    FString RobotName;

    /**
     * Bind this actor to a robot profile. Sets the GUID, name, and
     * updates the floating name text.
     * @param InRobotId    GUID from MXRobotManager::CreateRobot().
     * @param InRobotName  Name from FMXRobotProfile::name.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Robot")
    void BindToProfile(const FGuid& InRobotId, const FString& InRobotName);

    // =========================================================================
    // Scale
    // =========================================================================

    /**
     * Uniform scale applied to the entire actor in BeginPlay.
     * Default 0.20 — produces a ~36cm robot from a ~180cm mannequin.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    float RobotScale = 0.20f;

    // =========================================================================
    // Skeletal Mesh (optional C++ assignment)
    // =========================================================================

    /**
     * Optional soft reference to the skeletal mesh asset.
     * If set, loaded and applied in BeginPlay. If not set, relies on
     * whatever mesh is assigned in a Blueprint child or the editor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    TSoftObjectPtr<USkeletalMesh> SkeletalMeshAsset;

    // =========================================================================
    // Components
    // =========================================================================

    /** Floating name text above the robot's head. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot")
    TObjectPtr<UTextRenderComponent> NameTextComponent;

protected:

    virtual void BeginPlay() override;

public:

    virtual void Tick(float DeltaTime) override;

private:

    /** Update the NameTextComponent to face the camera. */
    void UpdateNameBillboard();
};
