// MXAnimBridge.h — Animation Bridge v1.0
// Created: 2026-03-13
// Chat: GASP Animation
// Phase: GASP-A
//
// Pure C++ ActorComponent that reads CharacterMovementComponent state each tick
// and exposes animation-relevant UPROPERTYs for ANY Animation Blueprint to consume.
//
// Design principles:
//   - ZERO GASP headers. This is a decoupling layer — the C++ engine never
//     references GASP Blueprints, and the AnimBP never references game logic.
//   - All exposed values are BlueprintReadOnly so the AnimBP can bind to them
//     in its AnimGraph via property access or "Get Anim Instance" → cast.
//   - Trigger functions (traversal, montages) are BlueprintCallable so C++ game
//     logic can fire animation events without knowing how they're displayed.
//   - Follows Editor SDK philosophy: all config is EditAnywhere with MX|Animation categories.
//
// Attach to AMXRobotActor. In BeginPlay, auto-discovers the owning actor's CMC
// and SkeletalMeshComponent. Tick reads CMC velocity/state and updates exposed vars.
//
// Change Log:
//   v1.0 — Initial implementation (Phase GASP-A)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXTypes.h"
#include "MXAnimBridge.generated.h"

// Forward declarations — no concrete GASP or module includes
class UCharacterMovementComponent;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;

// ---------------------------------------------------------------------------
// Delegates
// ---------------------------------------------------------------------------

/** Broadcast when locomotion state changes (e.g., Idle → Walking). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FMXOnLocomotionStateChanged,
    EMXLocomotionState, OldState,
    EMXLocomotionState, NewState);

/** Broadcast when a traversal action begins. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FMXOnTraversalStarted,
    ETraversalType, TraversalType);

/** Broadcast when an action montage finishes or is interrupted. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FMXOnActionMontageEnded,
    EMXActionMontage, MontageType,
    bool, bInterrupted);

// ---------------------------------------------------------------------------
// UMXAnimBridge
// ---------------------------------------------------------------------------

/**
 * UMXAnimBridge
 *
 * The decoupling layer between C++ game logic and the Animation Blueprint.
 *
 * Data flow:
 *   CMC (velocity, movement mode, acceleration)
 *     → UMXAnimBridge (reads each tick, computes derived values)
 *       → AnimBP (reads UPROPERTYs via property access or GetAnimBridge())
 *
 * Action flow:
 *   C++ game logic calls PlayTraversalAction() or PlayActionMontage()
 *     → UMXAnimBridge sets trigger flags / plays montage
 *       → AnimBP reacts to flags in its state machine transitions
 *
 * The AnimBP can access this component via:
 *   - TryGetPawnOwner() → GetComponentByClass<UMXAnimBridge>()
 *   - Or cache a reference in the AnimBP's NativeInitializeAnimation()
 */
UCLASS(ClassGroup = (MicroExodus), meta = (BlueprintSpawnableComponent))
class MICROEXODUS_API UMXAnimBridge : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXAnimBridge();

protected:

    virtual void BeginPlay() override;

public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // =========================================================================
    // Locomotion State — READ BY AnimBP
    // =========================================================================

    /** Current ground speed in cm/s. Primary driver for locomotion blend. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Locomotion")
    float Speed = 0.0f;

    /** Normalized ground speed (0–1) relative to MaxWalkSpeed. Useful for blend weights. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Locomotion")
    float SpeedNormalized = 0.0f;

    /** Movement direction relative to actor forward, in degrees [-180, 180]. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Locomotion")
    float Direction = 0.0f;

    /** True when the robot has meaningful ground velocity (above MovingThreshold). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Locomotion")
    bool bIsMoving = false;

    /** True when the CMC reports the character is falling (not grounded). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Locomotion")
    bool bIsFalling = false;

    /** True when the robot has a pending movement target (MoveToLocation active). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Locomotion")
    bool bHasMoveTarget = false;

    /** Current high-level locomotion state. Drives AnimBP state machine transitions. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Locomotion")
    EMXLocomotionState LocomotionState = EMXLocomotionState::Idle;

    // =========================================================================
    // Turn-in-Place — READ BY AnimBP
    // =========================================================================

    /**
     * Angle between the actor's current facing and the desired movement direction.
     * Positive = right turn, negative = left turn. Used for turn-in-place triggers.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Turn")
    float TurnAngle = 0.0f;

    /** True when TurnAngle exceeds TurnInPlaceThreshold while Speed is near zero. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Turn")
    bool bShouldTurnInPlace = false;

    // =========================================================================
    // Acceleration / Lean — READ BY AnimBP
    // =========================================================================

    /** Current acceleration magnitude from CMC. Used for start/stop blending. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Acceleration")
    float Acceleration = 0.0f;

    /** Lateral lean angle computed from velocity change. Positive = lean right. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Acceleration")
    float LeanAngle = 0.0f;

    // =========================================================================
    // Traversal State — READ BY AnimBP
    // =========================================================================

    /** True while a traversal action (vault, climb, mantle) is active. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Traversal")
    bool bIsTraversing = false;

    /** The type of traversal currently active (only valid when bIsTraversing). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Traversal")
    ETraversalType ActiveTraversalType = ETraversalType::None;

    // =========================================================================
    // Idle — READ BY AnimBP
    // =========================================================================

    /** Index of the current idle variant (driven by robot personality/quirk). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Idle")
    int32 IdleVariant = 0;

    /** Seconds spent continuously in Idle state. Drives fidget/boredom transitions. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Animation|Idle")
    float IdleTime = 0.0f;

    // =========================================================================
    // Configuration — TUNABLE IN EDITOR
    // =========================================================================

    /** Speed below this threshold = considered stopped. Drives Idle vs Moving. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Animation|Config")
    float MovingThreshold = 5.0f;

    /** Turn angle (degrees) above which turn-in-place activates while idle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Animation|Config")
    float TurnInPlaceThreshold = 45.0f;

    /** Interpolation speed for the lean angle (higher = snappier lean response). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Animation|Config")
    float LeanInterpSpeed = 8.0f;

    /** Maximum lean angle in degrees. Prevents extreme leaning at high turn rates. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Animation|Config")
    float MaxLeanAngle = 15.0f;

    /** Seconds of idling before incrementing idle fidget counter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Animation|Config")
    float IdleFidgetDelay = 5.0f;

    /** Time (seconds) after stopping before transitioning from Stopping → Idle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Animation|Config")
    float StopToIdleDelay = 0.2f;

    // =========================================================================
    // Action Triggers — CALLED BY C++ GAME LOGIC
    // =========================================================================

    /**
     * Begin a traversal action. Sets bIsTraversing and ActiveTraversalType.
     * The AnimBP reacts to these flags in its traversal state machine layer.
     * Call EndTraversal() when the traversal completes (or montage notifies).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Animation|Actions")
    void PlayTraversalAction(ETraversalType Type);

    /** End the current traversal action and return to locomotion. */
    UFUNCTION(BlueprintCallable, Category = "MX|Animation|Actions")
    void EndTraversal();

    /**
     * Play an action montage (rescue, flinch, death, sacrifice, celebrate).
     * Looks up the montage from the MontageMap and plays it on the AnimInstance.
     * Returns true if the montage was found and play was initiated.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Animation|Actions")
    bool PlayActionMontage(EMXActionMontage MontageType, float PlayRate = 1.0f);

    /** Stop any currently playing action montage. */
    UFUNCTION(BlueprintCallable, Category = "MX|Animation|Actions")
    void StopActionMontage(float BlendOutTime = 0.25f);

    /**
     * Set the idle variant index. Intended to be called once at robot spawn,
     * driven by the robot's personality quirk.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Animation|Actions")
    void SetIdleVariant(int32 Variant);

    // =========================================================================
    // Montage Asset Map — CONFIGURABLE IN EDITOR
    // =========================================================================

    /**
     * Map from action montage type to the actual UAnimMontage asset.
     * Populated in the robot Blueprint's Details panel or via C++ at spawn.
     * If an entry is missing, PlayActionMontage() returns false.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Animation|Montages")
    TMap<EMXActionMontage, TSoftObjectPtr<UAnimMontage>> MontageMap;

    // =========================================================================
    // Events — BINDABLE BY AnimBP OR C++
    // =========================================================================

    /** Fires when locomotion state transitions. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Animation|Events")
    FMXOnLocomotionStateChanged OnLocomotionStateChanged;

    /** Fires when a traversal action begins. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Animation|Events")
    FMXOnTraversalStarted OnTraversalStarted;

    /** Fires when an action montage ends (completed or interrupted). */
    UPROPERTY(BlueprintAssignable, Category = "MX|Animation|Events")
    FMXOnActionMontageEnded OnActionMontageEnded;

    // =========================================================================
    // Utility — FOR AnimBP or C++ CALLERS
    // =========================================================================

    /** Returns the owning actor's CharacterMovementComponent (cached). */
    UFUNCTION(BlueprintCallable, Category = "MX|Animation|Utility")
    UCharacterMovementComponent* GetCMC() const { return CachedCMC; }

    /** Returns the owning actor's SkeletalMeshComponent (cached). */
    UFUNCTION(BlueprintCallable, Category = "MX|Animation|Utility")
    USkeletalMeshComponent* GetSkeletalMesh() const { return CachedMesh; }

private:

    // =========================================================================
    // Internal State
    // =========================================================================

    /** Cached reference to the owning ACharacter's CMC. Found in BeginPlay. */
    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> CachedCMC;

    /** Cached reference to the owning actor's skeletal mesh. Found in BeginPlay. */
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> CachedMesh;

    /** Previous frame's velocity — used for acceleration and lean calculations. */
    FVector PreviousVelocity = FVector::ZeroVector;

    /** Timer for stop-to-idle transition delay. */
    float StopTimer = 0.0f;

    /** The montage type currently playing (for EndMontage callback routing). */
    EMXActionMontage CurrentMontageType = EMXActionMontage::None;

    // =========================================================================
    // Internal Methods
    // =========================================================================

    /** Compute all locomotion variables from CMC state. Called each tick. */
    void UpdateLocomotionState(float DeltaTime);

    /** Compute lean angle from velocity delta. Called each tick. */
    void UpdateLean(float DeltaTime);

    /** Compute turn-in-place state from facing vs desired direction. */
    void UpdateTurnInPlace();

    /** Update idle timer and fidget triggers. */
    void UpdateIdleState(float DeltaTime);

    /** Set new locomotion state and broadcast if changed. */
    void SetLocomotionState(EMXLocomotionState NewState);

    /** Callback for montage blend-out (bound dynamically in PlayActionMontage). */
    UFUNCTION()
    void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
};
