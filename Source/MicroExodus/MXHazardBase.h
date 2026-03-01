// MXHazardBase.h — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm
// Abstract base class for all hazard Blueprint actors.
// Provides kill-volume + warning-volume collision, spec protection checks,
// and the DEMS event trigger pipeline.
//
// Level designers extend the subclass stubs below in Blueprint.
// Each hazard Blueprint must have an MXEventMessageComponent with EventFields filled in.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "MXEventMessageComponent.h"
#include "MXEventFields.h"
#include "MXSpecData.h"
#include "MXInterfaces.h"
#include "MXEvents.h"
#include "MXHazardBase.generated.h"

// Forward declarations
class UMXEventBus;

// ---------------------------------------------------------------------------
// AMXHazardBase
// ---------------------------------------------------------------------------

/**
 * AMXHazardBase
 * Abstract base class for all in-level hazards.
 *
 * Subclass in C++ (or Blueprint) to implement hazard-specific activation logic.
 * The base class handles:
 *   - Two collision spheres (KillVolume + WarningVolume)
 *   - Spec protection checks (Guardian shields, Ghost phase, etc.)
 *   - DEMS event triggering via the attached MXEventMessageComponent
 *   - EventBus broadcasting of OnRobotDied / OnNearMiss
 *   - Near-miss tracking (robot entered warning zone, escaped without dying)
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazardBase : public AActor
{
    GENERATED_BODY()

public:

    AMXHazardBase();

protected:

    virtual void BeginPlay() override;

public:

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    /**
     * The DEMS component that drives narrative message generation.
     * Configure EventFields (element, verbs, nouns, flavor) per-instance in Blueprint editor.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Hazard")
    UMXEventMessageComponent* EventMessageComp;

    /**
     * Spherical collision zone that triggers death when a robot overlaps.
     * Size should match the visual hazard extent (e.g., crusher platen footprint).
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Hazard")
    USphereComponent* KillVolume;

    /**
     * Slightly larger sphere around the KillVolume for near-miss detection.
     * Default: KillVolume radius + NEAR_MISS_RADIUS (200 cm).
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Hazard")
    USphereComponent* WarningVolume;

    // -------------------------------------------------------------------------
    // Configuration — editable per-instance in Blueprint editor
    // -------------------------------------------------------------------------

    /**
     * Copied from DT_HazardFields at startup and also editable per-instance.
     * The EventMessageComp reads this at trigger time — kept in sync automatically.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard")
    FMXObjectEventFields HazardFields;

    /**
     * Whether this hazard is currently in an active/lethal state.
     * Inactive hazards still track near-misses but do NOT kill.
     * Overridden in subclasses for periodic hazards (e.g., crusher mid-rise).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard")
    bool bHazardActive = true;

    /**
     * Radius of the KillVolume sphere (cm). Set this in the editor; the component
     * is resized in BeginPlay. Default 60 cm is appropriate for most floor hazards.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard")
    float KillRadius = 60.0f;

    /**
     * Additional buffer added to KillRadius to form the WarningVolume radius.
     * Default 200 cm (= NEAR_MISS_RADIUS * 100 for unit conversion).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard")
    float WarningBuffer = 200.0f;

    // -------------------------------------------------------------------------
    // Core Interface
    // -------------------------------------------------------------------------

    /**
     * Is the hazard currently in a lethal state?
     * Override in subclasses to implement periodic / trigger-based activation.
     * @return True if robots overlapping KillVolume should die.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MX|Hazard")
    bool IsHazardActive() const;
    virtual bool IsHazardActive_Implementation() const;

    /**
     * Activate the hazard (make it lethal). Called by the hazard's internal timer
     * or by a Blueprint event from an external trigger.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MX|Hazard")
    void Activate();
    virtual void Activate_Implementation();

    /**
     * Deactivate the hazard (safe to pass through). Called at the end of the hazard cycle.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MX|Hazard")
    void Deactivate();
    virtual void Deactivate_Implementation();

    /**
     * Return the full cycle time of this hazard (active + inactive period combined, seconds).
     * Override in subclasses. Default returns 0 (non-cycling hazard).
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MX|Hazard")
    float GetCycleTime() const;
    virtual float GetCycleTime_Implementation() const;

    // -------------------------------------------------------------------------
    // Spec Protection Check
    // -------------------------------------------------------------------------

    /**
     * Check whether a robot's specialization absorbs or blocks this hazard hit.
     * If this returns true, the hit is absorbed (decrement counters, fire near-miss) —
     * do NOT kill the robot.
     *
     * Checks:
     *   - Guardian/Sentinel: lethal_hits_per_run shield
     *   - Trailblazer/Ghost: phase-through charges
     *   - Bulwark: physics push immunity (only for Slide/Conveyor damage types)
     *
     * @param RobotId  The robot receiving the hit.
     * @return         True if the hit is absorbed (robot survives).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hazard")
    bool CheckSpecProtection(const FGuid& RobotId);

    // -------------------------------------------------------------------------
    // Robot Tracking
    // -------------------------------------------------------------------------

    /**
     * Return all robot GUIDs currently inside the KillVolume this frame.
     * Populated by OnKillVolumeOverlap overlap events.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Hazard")
    const TSet<FGuid>& GetRobotsInKillZone() const { return RobotsInKillZone; }

    /**
     * Return all robot GUIDs currently inside the WarningVolume but outside the KillVolume.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Hazard")
    const TSet<FGuid>& GetRobotsInWarningZone() const { return RobotsInWarningZone; }

protected:

    // -------------------------------------------------------------------------
    // Collision Handlers — registered in BeginPlay
    // -------------------------------------------------------------------------

    /**
     * Called when a robot enters the KillVolume.
     * Checks hazard active state and spec protection, then triggers death or absorption.
     */
    UFUNCTION()
    void OnKillVolumeOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                             bool bFromSweep, const FHitResult& SweepResult);

    /**
     * Called when a robot exits the KillVolume without dying.
     * Removes from kill-zone tracking set.
     */
    UFUNCTION()
    void OnKillVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    /**
     * Called when a robot enters the WarningVolume (near-miss candidate begins).
     * Adds to warning-zone tracking set and records entry.
     */
    UFUNCTION()
    void OnWarningVolumeOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                bool bFromSweep, const FHitResult& SweepResult);

    /**
     * Called when a robot exits the WarningVolume without entering the KillVolume.
     * This constitutes a confirmed near-miss — trigger DEMS and EventBus.
     */
    UFUNCTION()
    void OnWarningVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // -------------------------------------------------------------------------
    // Internal Helpers
    // -------------------------------------------------------------------------

    /**
     * Extract the robot GUID from an overlapping actor.
     * Robot Blueprint actors expose their RobotId via a well-known tag or component.
     * Returns an invalid GUID if the actor is not a robot.
     */
    FGuid ExtractRobotId(AActor* Actor) const;

    /**
     * Retrieve the EventBus from the GameInstance subsystem.
     * Returns nullptr if the bus is not available.
     */
    UMXEventBus* GetEventBus() const;

    /**
     * Retrieve the RobotProvider from the GameInstance.
     * Used for spec protection lookups.
     */
    TScriptInterface<IMXRobotProvider> GetRobotProvider() const;

    /** Robots currently overlapping the KillVolume. */
    TSet<FGuid> RobotsInKillZone;

    /**
     * Robots currently overlapping the WarningVolume (potential near-miss candidates).
     * When a robot leaves this set without ever entering RobotsInKillZone, it's a near-miss.
     */
    TSet<FGuid> RobotsInWarningZone;

    /**
     * Per-robot remaining Guardian/Sentinel shield hits for this level.
     * Loaded from FMXSpecBonus at BeginPlay and decremented on absorbed hits.
     * Key = RobotId, Value = remaining shield charges.
     */
    TMap<FGuid, int32> ShieldCharges;

    /**
     * Per-robot remaining Ghost/Trailblazer phase-through charges for this level.
     */
    TMap<FGuid, int32> PhaseCharges;

    /** Cached RobotProvider — resolved once at BeginPlay. */
    TScriptInterface<IMXRobotProvider> CachedRobotProvider;
};

// ===========================================================================
// Hazard Subclass Stubs
// These C++ stubs are the base for Blueprint extension.
// Level designers implement the visual/timing behavior in Blueprint.
// ===========================================================================

// ---------------------------------------------------------------------------
// Fire Vent — periodic activation, area burn damage
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_FireVent : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_FireVent();
    /** Duration the vent is active (spewing fire), seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|FireVent")
    float ActiveDuration = 2.0f;
    /** Duration the vent is inactive (safe to pass), seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|FireVent")
    float InactiveDuration = 3.0f;
    virtual float GetCycleTime_Implementation() const override { return ActiveDuration + InactiveDuration; }
protected:
    virtual void BeginPlay() override;
    FTimerHandle CycleTimer;
    void StartActiveCycle();
    void StartInactiveCycle();
};

// ---------------------------------------------------------------------------
// Crusher — rhythmic vertical press
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_Crusher : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_Crusher();
    /** Time the crusher plate takes to descend (seconds). Robots caught at bottom die. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Crusher")
    float DescendTime = 0.4f;
    /** Time the plate holds at the bottom before rising. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Crusher")
    float HoldTime = 0.3f;
    /** Time the plate takes to fully retract (safe). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Crusher")
    float AscendTime = 0.8f;
    /** Pause at top before next descent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Crusher")
    float TopPauseTime = 1.5f;
    virtual float GetCycleTime_Implementation() const override
    { return DescendTime + HoldTime + AscendTime + TopPauseTime; }
protected:
    virtual void BeginPlay() override;
    FTimerHandle CycleTimer;
    void StartDescend();
    void StartHold();
    void StartAscend();
    void StartTopPause();
};

// ---------------------------------------------------------------------------
// Spike Trap — pressure-triggered, instant impale
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_SpikeTrap : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_SpikeTrap();
    /** How many robots must stand on the pressure plate before spikes fire. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|SpikeTrap")
    int32 TriggerRobotCount = 1;
    /** Time spikes remain extended before retracting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|SpikeTrap")
    float ExtendedDuration = 1.0f;
    virtual float GetCycleTime_Implementation() const override { return ExtendedDuration + 1.5f; }
};

// ---------------------------------------------------------------------------
// Pit / Ledge — fatal drop, edge detection
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_Pit : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_Pit();
    /** Depth (cm) before a fall is considered fatal. Shallow pits may be crossable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Pit")
    float FatalDepth = 500.0f;
    /** If true, robots can slowly cross narrow pits (width < 80 cm) with careful mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Pit")
    bool bNarrowPit = false;
};

// ---------------------------------------------------------------------------
// Lava Pool — rising/receding area, instant melt kill
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_LavaPool : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_LavaPool();
    /** Whether this lava pool rises and recedes on a cycle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|LavaPool")
    bool bCycling = false;
    /** Time for lava to rise from safe floor to lethal level (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|LavaPool")
    float RiseTime = 4.0f;
    /** Time lava holds at peak. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|LavaPool")
    float PeakHoldTime = 2.0f;
    /** Time lava recedes back to safe level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|LavaPool")
    float ReceedTime = 3.0f;
    virtual float GetCycleTime_Implementation() const override { return RiseTime + PeakHoldTime + ReceedTime; }
};

// ---------------------------------------------------------------------------
// Conveyor Belt — pushes robots into hazards
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_Conveyor : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_Conveyor();
    /** World-space direction the conveyor pushes robots. Normalized. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Conveyor")
    FVector PushDirection = FVector(1.0f, 0.0f, 0.0f);
    /** Speed at which the conveyor pushes robots (cm/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|Conveyor")
    float PushSpeed = 200.0f;
    /**
     * Bulwark/Anchor robots are immune to conveyor push.
     * This is checked in CheckSpecProtection() — no override needed here.
     */
};

// ---------------------------------------------------------------------------
// Buzz Saw — moving along a track, instant cut kill on contact
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_BuzzSaw : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_BuzzSaw();
    /** Patrol path points the saw moves between. Set in editor via spline or array. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|BuzzSaw")
    TArray<FVector> PatrolPoints;
    /** Speed the saw travels along its path (cm/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|BuzzSaw")
    float TravelSpeed = 350.0f;
    /** If true, the saw reverses direction at each endpoint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|BuzzSaw")
    bool bPingPong = true;
};

// ---------------------------------------------------------------------------
// Ice Floor — reduces traction, robots slide toward hazards
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_IceFloor : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_IceFloor();
    /**
     * Traction multiplier when on ice (0.0 = frictionless, 1.0 = normal).
     * Applied as a modifier to robot velocity change rate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|IceFloor",
              meta=(ClampMin="0.0", ClampMax="1.0"))
    float TractionMultiplier = 0.15f;
    /**
     * Ice itself isn't lethal — robots slide INTO other hazards.
     * The KillVolume on this class is disabled by default.
     * Set bHazardActive to false at construction.
     */
};

// ---------------------------------------------------------------------------
// EMP Field — periodic stun pulse, disables robots briefly
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_EMPField : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_EMPField();
    /** Seconds between EMP pulses. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|EMP")
    float PulseInterval = 5.0f;
    /** Duration robots are stunned/disabled after being hit (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|EMP")
    float StunDuration = 2.0f;
    /** Radius of the EMP pulse (cm). May differ from KillVolume for visual flair. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|EMP")
    float PulseRadius = 400.0f;
    virtual float GetCycleTime_Implementation() const override { return PulseInterval; }
protected:
    virtual void BeginPlay() override;
    FTimerHandle PulseTimer;
    void FirePulse();
};

// ---------------------------------------------------------------------------
// Falling Debris — random timed drops, buries on impact
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXHazard_FallingDebris : public AMXHazardBase
{
    GENERATED_BODY()
public:
    AMXHazard_FallingDebris();
    /** Minimum seconds between random drop events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|FallingDebris")
    float MinDropInterval = 2.0f;
    /** Maximum seconds between random drop events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|FallingDebris")
    float MaxDropInterval = 6.0f;
    /** Height above the floor from which debris falls (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|FallingDebris")
    float DropHeight = 800.0f;
    /** Brief warning shadow shown on the floor before impact (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Hazard|FallingDebris")
    float WarnTime = 0.8f;
protected:
    virtual void BeginPlay() override;
    FTimerHandle DropTimer;
    void ScheduleNextDrop();
    void SpawnDebrisImpact();
};
