// MXSwarmCamera.h — Camera Module v1.0
// Created: 2026-02-22
// Agent 7: Camera
// Primary camera manager component for Micro Exodus.
// Owns swarm-scaled zoom, personality modes, event queuing, and
// all positional transitions. Designed to feel like a documentary
// filmmaker — drawn to drama, always pulling back to the bigger picture.
//
// Attach to the player pawn or a dedicated Camera Manager actor.
// Requires a USpringArmComponent and ACameraActor (or UCameraComponent) wired at BeginPlay.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraActor.h"
#include "GameFramework/SpringArmComponent.h"
#include "MXTypes.h"
#include "MXEventData.h"
#include "MXInterfaces.h"
#include "MXSwarmCamera.generated.h"

// ---------------------------------------------------------------------------
// ECameraPersonalityMode
// ---------------------------------------------------------------------------

/**
 * The four personality modes the player can cycle through.
 * Controls how aggressively the camera reacts to in-game events.
 */
UENUM(BlueprintType)
enum class ECameraPersonalityMode : uint8
{
    Director   UMETA(DisplayName = "Director"),   // Full cinematic reactions. Default.
    Operator   UMETA(DisplayName = "Operator"),   // Minimal reactions — deaths only.
    Locked     UMETA(DisplayName = "Locked"),     // No automatic movement. Full manual control.
    Replay     UMETA(DisplayName = "Replay"),     // Post-level playback with enhanced framing.
};

// ---------------------------------------------------------------------------
// FMXCameraZoomEntry
// ---------------------------------------------------------------------------

/**
 * FMXCameraZoomEntry
 * One row of the swarm-scaled zoom table.
 * Camera interpolates between entries as swarm count changes.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXCameraZoomEntry
{
    GENERATED_BODY()

    /** Minimum swarm count (inclusive) for this zoom band. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinRobots = 0;

    /** Maximum swarm count (inclusive) for this zoom band. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRobots = 5;

    /** Target camera arm length / height in Unreal units (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TargetHeight = 300.0f;
};

// ---------------------------------------------------------------------------
// FMXCameraEvent (forward-declared here — defined fully in MXCameraBehaviors.h)
#include "MXCameraBehaviors.h"

// ---------------------------------------------------------------------------
// UMXSwarmCamera
// ---------------------------------------------------------------------------

/**
 * UMXSwarmCamera
 * The central camera manager. Owns zoom interpolation, personality mode,
 * the event queue, and all positional transitions.
 *
 * Receives FMXCameraEvent packets from MXCameraBehaviors and executes them
 * in priority order each tick via TickCameraEvents().
 */
UCLASS(ClassGroup=(MicroExodus), BlueprintType, Blueprintable,
       meta=(BlueprintSpawnableComponent))
class MICROEXODUS_API UMXSwarmCamera : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXSwarmCamera();

protected:

    virtual void BeginPlay() override;

public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // =========================================================================
    // Swarm Targeting
    // =========================================================================

    /**
     * Update the camera's tracked robot set.
     * Each tick, the camera follows the centroid of all active robots.
     * Call whenever robots are added, removed, or the swarm composition changes.
     * @param ActiveRobots  GUIDs of all robots currently alive in the level.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Target")
    void SetSwarmTarget(const TArray<FGuid>& ActiveRobots);

    /**
     * Set camera framing for a split swarm.
     * The camera follows the largest group (GroupA is assumed dominant).
     * A secondary indicator is shown for GroupB.
     * @param GroupA  Dominant sub-group (camera follows their centroid).
     * @param GroupB  Secondary sub-group (shown with overlay indicator).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Target")
    void SetSplitTarget(const TArray<FGuid>& GroupA, const TArray<FGuid>& GroupB);

    /**
     * Focus the camera on a single robot for an inspect-mode close-up.
     * Camera stays locked until ExitInspectMode() is called.
     * @param RobotId  The robot to inspect.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Target")
    void EnterInspectMode(const FGuid& RobotId);

    /**
     * Exit inspect mode and return to following the swarm centroid.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Target")
    void ExitInspectMode();

    // =========================================================================
    // Zoom
    // =========================================================================

    /**
     * Drive the zoom system each tick. Interpolates the camera arm length
     * toward the ideal height for the given swarm count.
     * @param SwarmCount  Current number of active robots.
     * @param DeltaTime   Frame delta for smooth interpolation.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Zoom")
    void UpdateZoom(int32 SwarmCount, float DeltaTime);

    /**
     * Map a swarm count to the ideal camera height using the zoom table.
     * Linearly interpolates between the nearest two entries.
     * @param SwarmCount  Number of active robots.
     * @return            Target camera arm height in Unreal units.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Camera|Zoom")
    float GetIdealHeight(int32 SwarmCount) const;

    // =========================================================================
    // Robot Focus
    // =========================================================================

    /**
     * Temporarily focus the camera on a specific robot.
     * After Duration seconds the camera smoothly returns to the swarm.
     * @param RobotId  The robot to focus on.
     * @param Duration How long to hold the focus before pulling back.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Focus")
    void FocusOnRobot(const FGuid& RobotId, float Duration);

    /**
     * Initiate a smooth pullback from the current focus target to the swarm centroid.
     * @param Duration  Transition time in seconds.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Focus")
    void ReturnToSwarm(float Duration);

    // =========================================================================
    // Personality Mode
    // =========================================================================

    /**
     * Switch the camera personality mode at runtime.
     * Immediately affects which queued events play and how long they last.
     * @param Mode  The new personality mode to apply.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Mode")
    void SetCameraMode(ECameraPersonalityMode Mode);

    /**
     * Return the current personality mode.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Camera|Mode")
    ECameraPersonalityMode GetCurrentMode() const { return CurrentMode; }

    // =========================================================================
    // Event Queue
    // =========================================================================

    /**
     * Add a camera event to the priority queue.
     * Events are sorted by priority (Epic > Cinematic > Dramatic > Subtle).
     * Epic events are never skipped; lower-priority events may be pruned during
     * intense sequences.
     * @param Event  The event packet to enqueue.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Events")
    void QueueCameraEvent(const FMXCameraEvent& Event);

    /**
     * Process the camera event queue each tick.
     * Executes the active event, decrements its timer, and pops it when done.
     * @param DeltaTime  Frame delta.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Events")
    void TickCameraEvents(float DeltaTime);

    /**
     * Immediately flush the event queue. Called on level complete / reset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Events")
    void FlushEventQueue();

    // =========================================================================
    // Scene References
    // =========================================================================

    /**
     * The spring arm component attached to the camera rig.
     * Set this from Blueprint or the owning actor's constructor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Refs")
    TObjectPtr<USpringArmComponent> SpringArm;

    /**
     * The camera actor being driven by this manager.
     * Set this from Blueprint.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Refs")
    TObjectPtr<ACameraActor> CameraActor;

    // =========================================================================
    // Zoom Table Configuration
    // =========================================================================

    /**
     * The zoom band table. Populated in the constructor with GDD values.
     * Can be overridden in Blueprint if designers want to tweak.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    TArray<FMXCameraZoomEntry> ZoomTable;

    /**
     * Interpolation speed for zoom changes (higher = snappier).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float ZoomInterpSpeed = 2.0f;

    /**
     * Interpolation speed for position following (higher = tighter tracking).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float PositionInterpSpeed = 4.0f;

    /**
     * Close-up height when inspecting a single robot (cm).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Config")
    float InspectHeight = 150.0f;

    // =========================================================================
    // Debug
    // =========================================================================

    /** If true, log camera events to output log for debugging. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Debug")
    bool bLogEvents = false;

private:

    // -------------------------------------------------------------------------
    // Internal State
    // -------------------------------------------------------------------------

    /** Active personality mode. */
    ECameraPersonalityMode CurrentMode = ECameraPersonalityMode::Director;

    /** GUIDs of all robots currently being tracked. */
    TArray<FGuid> TrackedRobots;

    /** GUIDs of the secondary (split) group for indicator framing. */
    TArray<FGuid> SecondaryGroupRobots;

    /** Whether we are currently in split-swarm mode. */
    bool bIsSplit = false;

    /** Whether we are in robot inspect mode. */
    bool bInInspectMode = false;

    /** The robot being inspected, if bInInspectMode is true. */
    FGuid InspectTargetId;

    /** Whether a focus-on-robot transition is active. */
    bool bFocusingOnRobot = false;

    /** Robot being focused. */
    FGuid FocusTargetId;

    /** Remaining hold time on a robot focus. */
    float FocusTimeRemaining = 0.0f;

    /** Whether we are in a return-to-swarm transition. */
    bool bReturningToSwarm = false;

    /** Remaining time on the return-to-swarm transition. */
    float ReturnTimeRemaining = 0.0f;

    /** Total duration of the return-to-swarm transition (for alpha calculation). */
    float ReturnDuration = 0.5f;

    /** Current camera arm length (cm). Interpolates toward target. */
    float CurrentHeight = 600.0f;

    /** Target camera arm length driven by swarm count. */
    float TargetHeight = 600.0f;

    /** The event currently being executed (if any). */
    TOptional<FMXCameraEvent> ActiveEvent;

    /**
     * Pending event queue, sorted by priority (highest first).
     * FMXCameraEvent is defined in MXCameraBehaviors.h — included in the .cpp.
     */
    TArray<FMXCameraEvent> EventQueue;

    /** World position the camera was tracking when a focus event started (for blending). */
    FVector PreFocusWorldPosition = FVector::ZeroVector;

    // -------------------------------------------------------------------------
    // Private Helpers
    // -------------------------------------------------------------------------

    /** Compute the average world position of a given list of robot GUIDs. */
    FVector ComputeCentroid(const TArray<FGuid>& RobotIds) const;

    /** Move the spring arm target toward TargetPosition this tick. */
    void TickPositionTracking(float DeltaTime);

    /** Execute one frame of the active camera event. */
    void TickActiveEvent(float DeltaTime);

    /** Pop the next event from the queue into ActiveEvent. */
    void ActivateNextEvent();

    /** Apply a zoom override from an active event if set. */
    void ApplyZoomOverride(float ZoomOverride, float DeltaTime);

    /** The current world-space target position the spring arm tracks toward. */
    FVector CurrentTargetPosition = FVector::ZeroVector;

    /** Map of robot GUID → world position, updated by SetSwarmTarget. */
    TMap<FGuid, FVector> RobotPositions;

    /**
     * Companion reference — MXSwarmController is queried each tick if available
     * to get live robot positions without storing a stale copy.
     * Set via Blueprint or during BeginPlay via GameInstance lookup.
     */
    UPROPERTY()
    TObjectPtr<UActorComponent> SwarmControllerRef;
};
