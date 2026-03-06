// MXCameraRig.h — Phase 2A: Camera Rig Actor
// Created: 2026-03-06
// Purpose: Host actor for the MicroExodus camera system.
//          Owns a SpringArm + Camera + PostProcess and attaches
//          UMXSwarmCamera, UMXTiltShiftEffect, UMXTimeDilation as components.
//          All cross-references are wired in the constructor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MXCameraRig.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPostProcessComponent;
class UMXSwarmCamera;
class UMXTiltShiftEffect;
class UMXTimeDilation;

/**
 * AMXCameraRig
 * The camera rig for the MicroExodus diorama view.
 *
 * Component hierarchy:
 *   RootComponent (USceneComponent)
 *     └── SpringArm (USpringArmComponent)  — pitch -45°, arm length 500cm
 *           └── Camera (UCameraComponent)
 *   PostProcess (UPostProcessComponent)     — infinite extent, for tilt-shift
 *
 * Actor components (no transform):
 *   - UMXSwarmCamera      → drives SpringArm position/zoom
 *   - UMXTiltShiftEffect  → drives PostProcess settings
 *   - UMXTimeDilation     → drives global time dilation + notifies TiltShift
 *
 * Placement: Spawn in the level or from the GameMode. The SwarmCamera will
 * move this actor's world position to track the swarm centroid each tick.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXCameraRig : public AActor
{
    GENERATED_BODY()

public:

    AMXCameraRig();

    // =========================================================================
    // Scene Components
    // =========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Camera")
    TObjectPtr<USceneComponent> RigRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Camera")
    TObjectPtr<UPostProcessComponent> PostProcess;

    // =========================================================================
    // MX Camera Components
    // =========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Camera")
    TObjectPtr<UMXSwarmCamera> SwarmCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Camera")
    TObjectPtr<UMXTiltShiftEffect> TiltShiftEffect;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Camera")
    TObjectPtr<UMXTimeDilation> TimeDilation;

    // =========================================================================
    // Convenience Accessors
    // =========================================================================

    UFUNCTION(BlueprintCallable, Category = "MX|Camera")
    UMXSwarmCamera* GetSwarmCamera() const { return SwarmCamera; }

    UFUNCTION(BlueprintCallable, Category = "MX|Camera")
    UMXTiltShiftEffect* GetTiltShiftEffect() const { return TiltShiftEffect; }

protected:

    virtual void BeginPlay() override;
};
