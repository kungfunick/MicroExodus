// MXCameraBehaviors.h — Camera Module v1.0
// Created: 2026-02-22
// Agent 7: Camera
// Translates incoming FMXEventData records from the DEMS MessageDispatcher
// into FMXCameraEvent packets and queues them on UMXSwarmCamera.
//
// Also implements IMXEventListener so it can be registered directly with
// the DEMS dispatcher — no polling required.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXTypes.h"
#include "MXEventData.h"
#include "MXInterfaces.h"
// Forward declaration — full type in MXSwarmCamera.h (included in .cpp to avoid circular dependency)
class UMXSwarmCamera;
#include "MXCameraBehaviors.generated.h"

// ---------------------------------------------------------------------------
// FMXCameraEvent
// ---------------------------------------------------------------------------

/**
 * FMXCameraEvent
 * A fully-resolved instruction packet that tells UMXSwarmCamera exactly
 * how to react to one DEMS event. Created by UMXCameraBehaviors and
 * submitted to UMXSwarmCamera::QueueCameraEvent().
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXCameraEvent
{
    GENERATED_BODY()

    // -------------------------------------------------------------------------
    // Core Behavior
    // -------------------------------------------------------------------------

    /** The camera behavior type. Determines the execution path in UMXSwarmCamera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECameraBehavior Behavior = ECameraBehavior::Subtle;

    // -------------------------------------------------------------------------
    // Targeting
    // -------------------------------------------------------------------------

    /** World-space position of the event. Used to aim the camera for Dramatic/Cinematic/Epic. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    /** Robot to track for focus-on-robot behaviors. Invalid GUID = no robot tracking. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid TargetRobotId;

    // -------------------------------------------------------------------------
    // Timing
    // -------------------------------------------------------------------------

    /** Total planned duration of this event in seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Duration = 0.3f;

    /** Remaining time on this event. Decremented each tick by UMXSwarmCamera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TimeRemaining = 0.3f;

    // -------------------------------------------------------------------------
    // Zoom Override
    // -------------------------------------------------------------------------

    /**
     * Target camera arm height while this event is active.
     * -1.0 = use normal swarm-scaled zoom (no override).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ZoomOverride = -1.0f;

    // -------------------------------------------------------------------------
    // Hat Drop
    // -------------------------------------------------------------------------

    /** Whether the camera should linger to show the hat falling off. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasHatDrop = false;

    /**
     * Extra hold time (seconds) beyond base Duration to show the hat falling.
     * Added to Duration when bHasHatDrop is true.
     * Corresponds to HAT_DEATH_LINGER_TIME (3.0s per GDD).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HatLingerTime = 3.0f;

    // -------------------------------------------------------------------------
    // Queue Priority
    // -------------------------------------------------------------------------

    /**
     * Priority value for queue ordering. Higher = processed first.
     *   Epic       = 4.0
     *   Cinematic  = 3.0
     *   Dramatic   = 2.0
     *   Subtle     = 1.0
     *   Suppress   = 0.0
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Priority = 1.0f;

    // -------------------------------------------------------------------------
    // Shake Parameters (for Subtle / Dramatic flinch)
    // -------------------------------------------------------------------------

    /** Shake amplitude (relative, 0.0 = none). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ShakeAmplitude = 0.0f;

    /** Shake frequency (Hz). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ShakeFrequency = 10.0f;
};

// ---------------------------------------------------------------------------
// UMXCameraBehaviors
// ---------------------------------------------------------------------------

/**
 * UMXCameraBehaviors
 * Implements IMXEventListener and translates every incoming DEMS event into
 * an FMXCameraEvent, potentially upgrading the base camera_behavior based on
 * robot level / hat rarity.
 *
 * Holds a reference to UMXSwarmCamera and calls QueueCameraEvent() directly.
 * Also holds a reference to IMXRobotProvider for behavior-upgrade lookups.
 */
UCLASS(BlueprintType)
class MICROEXODUS_API UMXCameraBehaviors : public UObject, public IMXEventListener
{
    GENERATED_BODY()

public:

    UMXCameraBehaviors();

    // =========================================================================
    // IMXEventListener
    // =========================================================================

    /**
     * Entry point for all DEMS events.
     * Reads camera_behavior, checks for upgrade conditions, creates the
     * appropriate FMXCameraEvent, and queues it on the SwarmCamera.
     * @param Event  The fully-resolved DEMS event record.
     */
    virtual void OnDEMSEvent_Implementation(const FMXEventData& Event) override;

    // =========================================================================
    // Behavior Factories
    // =========================================================================

    /**
     * Create a Subtle camera event — slight screen shake, no zoom change.
     * Duration: 0.3s per GDD.
     * @param Event  Source DEMS event (for location / robot info).
     * @return       Ready-to-queue FMXCameraEvent.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    FMXCameraEvent CreateSubtleBehavior(const FMXEventData& Event) const;

    /**
     * Create a Dramatic camera event — quick zoom toward event, slight hold.
     * Duration: 0.8s per GDD.
     * @param Event  Source DEMS event.
     * @return       Ready-to-queue FMXCameraEvent.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    FMXCameraEvent CreateDramaticBehavior(const FMXEventData& Event) const;

    /**
     * Create a Cinematic camera event — smooth dolly in, brief hold, slow pullback.
     * Duration: 1.5s per GDD.
     * @param Event  Source DEMS event.
     * @return       Ready-to-queue FMXCameraEvent.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    FMXCameraEvent CreateCinematicBehavior(const FMXEventData& Event) const;

    /**
     * Create an Epic camera event — full takeover, slow zoom, dramatic angle, holds on aftermath.
     * Duration: 3.0s per GDD.
     * @param Event  Source DEMS event.
     * @return       Ready-to-queue FMXCameraEvent.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    FMXCameraEvent CreateEpicBehavior(const FMXEventData& Event) const;

    /**
     * Create the special death camera sequence.
     * Adds hat-drop linger time if the robot was wearing a hat.
     * Upgrades from Cinematic to Epic if the robot is Lvl 20+ or wearing a Legendary hat.
     * @param Event      Source DEMS event.
     * @param bHasHat    Whether the robot was wearing a hat at time of death.
     * @param RobotLevel The robot's level at time of death.
     * @return           Ready-to-queue FMXCameraEvent.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    FMXCameraEvent CreateDeathSequence(const FMXEventData& Event, bool bHasHat, int32 RobotLevel) const;

    /**
     * Create the extended sacrifice camera sequence.
     * Always Epic. Camera dollies to gate, holds on robot as eyes dim, lingers on hat falling.
     * Total duration ~4-5 seconds per GDD.
     * @param Event  Source DEMS event.
     * @return       Ready-to-queue FMXCameraEvent.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    FMXCameraEvent CreateSacrificeSequence(const FMXEventData& Event) const;

    /**
     * Create a near-miss flinch — snap-zoom toward the robot, spring immediately back.
     * Only created for Dramatic+ severity near-misses.
     * @param Event  Source DEMS event.
     * @return       Ready-to-queue FMXCameraEvent.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    FMXCameraEvent CreateNearMissFlinch(const FMXEventData& Event) const;

    // =========================================================================
    // Behavior Upgrade Logic
    // =========================================================================

    /**
     * Determine whether the base camera_behavior on the event should be
     * upgraded based on the involved robot's level and hat.
     *
     * Upgrade rules:
     *   • Dramatic death → Cinematic if robot is Lvl 10+
     *   • Cinematic death → Epic if robot is Lvl 20+ OR wearing Legendary hat
     *   • Any death with hat → at minimum Cinematic
     *   • Sacrifice is always Epic (no downgrade possible)
     *
     * @param Event  The DEMS event, used to read robot_level and hat_worn_id.
     * @return       The (possibly upgraded) ECameraBehavior value.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Behaviors")
    ECameraBehavior ShouldUpgradeBehavior(const FMXEventData& Event) const;

    // =========================================================================
    // References
    // =========================================================================

    /**
     * The camera manager that receives queued events.
     * Set during initialization by the owning camera manager actor or subsystem.
     */
    UPROPERTY()
    TObjectPtr<UMXSwarmCamera> SwarmCamera;

    /**
     * Robot data provider for hat rarity lookups during behavior upgrade checks.
     * Wired at runtime by the owning actor.
     */
    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * Robot level threshold above which a death upgrades to Cinematic.
     * Default: 10 (per GDD).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    int32 CinematicDeathLevelThreshold = 10;

    /**
     * Robot level threshold above which a death upgrades to Epic.
     * Default: 20 (per GDD).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    int32 EpicDeathLevelThreshold = 20;

    /**
     * Hat linger time in seconds — how long after the death the camera waits
     * for the hat to fall before pulling back. Default: 3.0s (per GDD).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float HatDeathLingerTime = 3.0f;

    /**
     * Total sacrifice sequence duration target (seconds).
     * Broken into dolly-in (1s), hold (1.5-2s), linger on hat (1s), pullback (0.5s).
     * Default: 4.5s per GDD.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float SacrificeTotalDuration = 4.5f;

    /**
     * Zoom-in height override for cinematic robot focus (cm).
     * The camera drives its arm length to this value during Cinematic/Epic events.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float CinematicZoomHeight = 250.0f;

    /**
     * Zoom-in height override for Epic robot focus (cm). More extreme than Cinematic.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float EpicZoomHeight = 180.0f;

    /**
     * Zoom-in height for the near-miss flinch snap (cm).
     * Camera snaps to this, then springs back to swarm zoom.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float FlinchZoomHeight = 200.0f;

private:

    /**
     * Retrieve the hat rarity for a given hat_id.
     * Returns EHatRarity::Common if hat_id is -1 or RobotProvider is unavailable.
     */
    EHatRarity GetHatRarity(const FMXEventData& Event) const;

    /**
     * Build the priority float from an ECameraBehavior value.
     * Epic=4, Cinematic=3, Dramatic=2, Subtle=1, Suppress=0.
     */
    static float BehaviorToPriority(ECameraBehavior Behavior);

    /**
     * Return the base duration (seconds) for a given ECameraBehavior, per GDD.
     */
    static float BehaviorToBaseDuration(ECameraBehavior Behavior);
};
