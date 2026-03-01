// MXHatPhysics.h — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Consumers: Robot actor (called by swarm movement / death flow)
// Change Log:
//   v1.0 — Initial implementation: hat mesh attachment, wobble simulation, death-fall & dissolve

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "MXHatPhysics.generated.h"

/**
 * FMXHatPhysicsInstance
 * Per-robot runtime state for an attached hat.
 * Tracks wobble simulation and death-fall animation progress.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXHatPhysicsInstance
{
    GENERATED_BODY()

    /** ID of the robot this hat is attached to. */
    UPROPERTY()
    FGuid RobotId;

    /** Hat ID. */
    UPROPERTY()
    int32 HatId = -1;

    /** The mesh component created and attached to the robot's head socket. */
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> HatMeshComponent;

    /** Current wobble yaw offset (degrees) in the spring simulation. */
    float WobbleAngle = 0.0f;

    /** Current angular velocity of the wobble spring. */
    float WobbleVelocity = 0.0f;

    /** Whether the hat is currently in the death-fall animation. */
    bool bDying = false;

    /** Whether this is a sacrifice death (delays the fall slightly for drama). */
    bool bIsSacrifice = false;

    /** Time accumulated since the death-fall began (seconds). */
    float DyingElapsed = 0.0f;

    /** Total linger time before dissolve begins (seconds). */
    float LingerDuration = 3.0f;

    /** Dissolve progress (0=opaque, 1=invisible). Driven after DyingElapsed > LingerDuration. */
    float DissolveProgress = 0.0f;

    /** Whether this instance can be garbage-collected (dissolve complete). */
    bool bReadyForCleanup = false;
};

/**
 * UMXHatPhysics
 * Manages the runtime visual behavior of all hats currently attached to robots.
 *
 * Responsibilities:
 *   - AttachHat: Load the hat mesh (async via soft path), create a static mesh component,
 *     attach to the robot's "HatSocket" socket, enable wobble simulation.
 *   - DetachHat: On robot death, detach the mesh component, enable UE physics gravity,
 *     start the linger → dissolve timeline. Sacrifice variant inserts a pause before the fall.
 *   - UpdateHatPhysics (Tick): Advance wobble spring simulation for all live hats;
 *     advance death-fall timer and dissolve material scalar for dying hats; clean up finished.
 *   - SetWobbleIntensity: Scale the wobble spring stiffness with swarm movement speed.
 *
 * Created and owned by the robot actor (or a dedicated physics subsystem).
 * Should be ticked every frame from the owning actor's Tick.
 *
 * Hat mesh scale: designed for ~2cm on a 15cm robot — use RelativeScale3D (0.2, 0.2, 0.2).
 * Socket name convention: "HatSocket" (defined on the robot skeleton).
 * Dissolve material parameter: "DissolveAmount" (scalar, 0=opaque → 1=transparent).
 */
UCLASS(Blueprintable)
class MICROEXODUS_API UMXHatPhysics : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * Attach a hat mesh to a robot actor at runtime.
     * Loads the hat mesh from its soft path asynchronously.
     * Creates and registers a UStaticMeshComponent on the RobotActor.
     *
     * @param RobotActor  The actor to attach the hat to (must have a "HatSocket" socket).
     * @param RobotId     The robot's GUID (for tracking).
     * @param HatId       The hat to attach.
     * @param MeshPath    The soft path to the hat's StaticMesh asset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Physics")
    void AttachHat(AActor* RobotActor, const FGuid& RobotId, int32 HatId, const FSoftObjectPath& MeshPath);

    /**
     * Begin the death-fall sequence for a robot's hat.
     * Detaches from the socket, enables physics, starts the linger-then-dissolve timer.
     *
     * @param RobotId       The robot whose hat should fall.
     * @param bIsSacrifice  If true, adds an emotional pause (0.8s) before the hat detaches.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Physics")
    void DetachHat(const FGuid& RobotId, bool bIsSacrifice = false);

    /**
     * Advance all hat physics simulations by DeltaTime.
     * Call from the owning actor's Tick function.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Physics")
    void UpdateHatPhysics(float DeltaTime);

    /**
     * Set wobble intensity for all active hats.
     * Scales the spring stiffness so hats wobble more at higher swarm speeds.
     * @param Intensity  Normalized swarm speed [0.0, 1.0].
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Physics")
    void SetWobbleIntensity(float Intensity);

    /**
     * Immediately remove all hat instances (e.g., on level unload).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Physics")
    void ClearAllHats();

    /**
     * Returns the active hat physics instance for a robot, or nullptr if none.
     */
    FMXHatPhysicsInstance* FindInstance(const FGuid& RobotId);

private:

    // -------------------------------------------------------------------------
    // Wobble Spring Constants
    // -------------------------------------------------------------------------

    /** Spring stiffness at WobbleIntensity=0. */
    static constexpr float WOBBLE_STIFFNESS_MIN  = 8.0f;

    /** Spring stiffness at WobbleIntensity=1. */
    static constexpr float WOBBLE_STIFFNESS_MAX  = 28.0f;

    /** Spring damping coefficient (constant). */
    static constexpr float WOBBLE_DAMPING        = 5.0f;

    /** Maximum wobble displacement angle (degrees). */
    static constexpr float WOBBLE_MAX_ANGLE      = 12.0f;

    /** Sacrifice pause before hat detaches (seconds). */
    static constexpr float SACRIFICE_PAUSE       = 0.8f;

    /** Rate of dissolve once linger time has elapsed (opacity units per second). */
    static constexpr float DISSOLVE_RATE         = 0.6f;

    // -------------------------------------------------------------------------
    // Internal Helpers
    // -------------------------------------------------------------------------

    /** Advance wobble spring simulation for one instance. */
    void TickWobble(FMXHatPhysicsInstance& Instance, float DeltaTime);

    /** Advance death-fall for one instance. Returns true when cleanup is ready. */
    bool TickDeath(FMXHatPhysicsInstance& Instance, float DeltaTime);

    /** Apply DissolveAmount scalar to the hat mesh's material instance. */
    void ApplyDissolveMaterial(FMXHatPhysicsInstance& Instance, float NormalizedDissolve);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /** All currently tracked hat instances (live and dying). */
    TArray<FMXHatPhysicsInstance> ActiveInstances;

    /** Current wobble intensity [0, 1] — scaled by swarm speed. */
    float WobbleIntensity = 0.0f;

    /** Cached hat socket name. */
    static const FName HAT_SOCKET_NAME;

    /** Cached dissolve material parameter name. */
    static const FName DISSOLVE_PARAM_NAME;
};
