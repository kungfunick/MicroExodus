// MXAnimBridge.cpp — Animation Bridge v1.0
// Created: 2026-03-13
// Chat: GASP Animation
// Phase: GASP-A
//
// Change Log:
//   v1.0 — Initial implementation

#include "MXAnimBridge.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetMathLibrary.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXAnimBridge::UMXAnimBridge()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Match the animation tick rate — every frame for smooth blending.
    PrimaryComponentTick.TickInterval = 0.0f;
}

// ---------------------------------------------------------------------------
// BeginPlay — discover CMC and mesh on owning actor
// ---------------------------------------------------------------------------

void UMXAnimBridge::BeginPlay()
{
    Super::BeginPlay();

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXAnimBridge: Owner '%s' is not an ACharacter — disabling tick."),
            *GetOwner()->GetName());
        SetComponentTickEnabled(false);
        return;
    }

    CachedCMC = OwnerChar->GetCharacterMovement();
    CachedMesh = OwnerChar->GetMesh();

    if (!CachedCMC)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXAnimBridge: No CharacterMovementComponent on '%s' — disabling tick."),
            *OwnerChar->GetName());
        SetComponentTickEnabled(false);
        return;
    }

    if (!CachedMesh)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXAnimBridge: No SkeletalMeshComponent on '%s' — montages unavailable."),
            *OwnerChar->GetName());
    }

    PreviousVelocity = FVector::ZeroVector;

    UE_LOG(LogTemp, Log,
        TEXT("MXAnimBridge: Initialized on '%s'. CMC MaxWalkSpeed=%.0f."),
        *OwnerChar->GetName(),
        CachedCMC->MaxWalkSpeed);
}

// ---------------------------------------------------------------------------
// TickComponent — main update loop
// ---------------------------------------------------------------------------

void UMXAnimBridge::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CachedCMC) return;

    // Don't update locomotion state during traversal — traversal owns the state.
    if (!bIsTraversing)
    {
        UpdateLocomotionState(DeltaTime);
        UpdateTurnInPlace();
    }

    UpdateLean(DeltaTime);
    UpdateIdleState(DeltaTime);

    // Store velocity for next frame's delta calculations.
    PreviousVelocity = CachedCMC->Velocity;
}

// ---------------------------------------------------------------------------
// UpdateLocomotionState
// ---------------------------------------------------------------------------

void UMXAnimBridge::UpdateLocomotionState(float DeltaTime)
{
    const FVector Velocity = CachedCMC->Velocity;
    const FVector GroundVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);

    // Core values
    Speed = GroundVelocity.Size();
    SpeedNormalized = (CachedCMC->MaxWalkSpeed > 0.0f)
        ? FMath::Clamp(Speed / CachedCMC->MaxWalkSpeed, 0.0f, 1.0f)
        : 0.0f;
    bIsFalling = CachedCMC->IsFalling();

    // Direction relative to actor forward [-180, 180]
    if (Speed > MovingThreshold)
    {
        const FRotator ActorRot = GetOwner()->GetActorRotation();
        const FRotator VelocityRot = GroundVelocity.Rotation();
        Direction = FRotator::NormalizeAxis(VelocityRot.Yaw - ActorRot.Yaw);
    }
    // else: keep last Direction — prevents jitter at low speed

    // Acceleration magnitude
    Acceleration = CachedCMC->GetCurrentAcceleration().Size();

    // Determine moving state
    const bool bWasMoving = bIsMoving;
    bIsMoving = Speed > MovingThreshold;

    // State machine transitions
    if (bIsFalling)
    {
        SetLocomotionState(EMXLocomotionState::Falling);
        StopTimer = 0.0f;
    }
    else if (bIsMoving)
    {
        // Distinguish starting from walking
        if (!bWasMoving && Acceleration > 0.0f)
        {
            SetLocomotionState(EMXLocomotionState::Starting);
        }
        else
        {
            SetLocomotionState(EMXLocomotionState::Walking);
        }
        StopTimer = 0.0f;
    }
    else
    {
        // Just stopped or continuing to be stopped
        if (bWasMoving)
        {
            SetLocomotionState(EMXLocomotionState::Stopping);
            StopTimer = 0.0f;
        }
        else if (LocomotionState == EMXLocomotionState::Stopping)
        {
            StopTimer += DeltaTime;
            if (StopTimer >= StopToIdleDelay)
            {
                SetLocomotionState(EMXLocomotionState::Idle);
            }
        }
        // If already Idle, do nothing — UpdateIdleState handles fidgets.
    }
}

// ---------------------------------------------------------------------------
// UpdateLean — compute lateral lean from velocity change
// ---------------------------------------------------------------------------

void UMXAnimBridge::UpdateLean(float DeltaTime)
{
    if (!CachedCMC || DeltaTime <= 0.0f) return;

    const FVector CurrentVelocity = CachedCMC->Velocity;
    const FVector VelocityDelta = CurrentVelocity - PreviousVelocity;

    // Project velocity delta onto the actor's right vector for lateral acceleration.
    const FVector RightVector = GetOwner()->GetActorRightVector();
    const float LateralAccel = FVector::DotProduct(VelocityDelta / DeltaTime, RightVector);

    // Convert lateral acceleration to a lean angle.
    // Normalize by MaxWalkSpeed for a consistent feel across different speeds.
    const float MaxSpeed = FMath::Max(CachedCMC->MaxWalkSpeed, 1.0f);
    const float TargetLean = FMath::Clamp(
        (LateralAccel / MaxSpeed) * MaxLeanAngle,
        -MaxLeanAngle,
        MaxLeanAngle);

    // Smooth interpolation to target lean.
    LeanAngle = FMath::FInterpTo(LeanAngle, TargetLean, DeltaTime, LeanInterpSpeed);
}

// ---------------------------------------------------------------------------
// UpdateTurnInPlace
// ---------------------------------------------------------------------------

void UMXAnimBridge::UpdateTurnInPlace()
{
    if (!CachedCMC) return;

    // Turn-in-place only applies when stationary.
    if (bIsMoving)
    {
        bShouldTurnInPlace = false;
        return;
    }

    // Compute angle between current facing and desired direction.
    // Desired direction comes from the CMC's current acceleration (player intent).
    const FVector DesiredDir = CachedCMC->GetCurrentAcceleration().GetSafeNormal2D();
    if (DesiredDir.IsNearlyZero())
    {
        // No input — no turn needed. Angle decays naturally.
        bShouldTurnInPlace = false;
        return;
    }

    const FRotator ActorRot = GetOwner()->GetActorRotation();
    const FRotator DesiredRot = DesiredDir.Rotation();
    TurnAngle = FRotator::NormalizeAxis(DesiredRot.Yaw - ActorRot.Yaw);

    bShouldTurnInPlace = FMath::Abs(TurnAngle) > TurnInPlaceThreshold;
}

// ---------------------------------------------------------------------------
// UpdateIdleState
// ---------------------------------------------------------------------------

void UMXAnimBridge::UpdateIdleState(float DeltaTime)
{
    if (LocomotionState == EMXLocomotionState::Idle)
    {
        IdleTime += DeltaTime;
    }
    else
    {
        IdleTime = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// SetLocomotionState — transitions with delegate broadcast
// ---------------------------------------------------------------------------

void UMXAnimBridge::SetLocomotionState(EMXLocomotionState NewState)
{
    if (LocomotionState == NewState) return;

    const EMXLocomotionState OldState = LocomotionState;
    LocomotionState = NewState;

    // Reset idle timer on any state change.
    if (NewState != EMXLocomotionState::Idle)
    {
        IdleTime = 0.0f;
    }

    OnLocomotionStateChanged.Broadcast(OldState, NewState);
}

// ---------------------------------------------------------------------------
// Traversal Actions
// ---------------------------------------------------------------------------

void UMXAnimBridge::PlayTraversalAction(ETraversalType Type)
{
    if (Type == ETraversalType::None) return;

    bIsTraversing = true;
    ActiveTraversalType = Type;

    // Override locomotion state for the duration of the traversal.
    SetLocomotionState(EMXLocomotionState::Traversing);

    OnTraversalStarted.Broadcast(Type);

    UE_LOG(LogTemp, Log,
        TEXT("MXAnimBridge: Traversal started — Type=%d on '%s'."),
        static_cast<int32>(Type),
        *GetOwner()->GetName());
}

void UMXAnimBridge::EndTraversal()
{
    if (!bIsTraversing) return;

    bIsTraversing = false;
    ActiveTraversalType = ETraversalType::None;

    // Return to Idle — the next tick will re-evaluate based on CMC state.
    SetLocomotionState(EMXLocomotionState::Idle);

    UE_LOG(LogTemp, Log,
        TEXT("MXAnimBridge: Traversal ended on '%s'."),
        *GetOwner()->GetName());
}

// ---------------------------------------------------------------------------
// Action Montages
// ---------------------------------------------------------------------------

bool UMXAnimBridge::PlayActionMontage(EMXActionMontage MontageType, float PlayRate)
{
    if (MontageType == EMXActionMontage::None) return false;
    if (!CachedMesh) return false;

    // Look up the montage asset from the configurable map.
    const TSoftObjectPtr<UAnimMontage>* Found = MontageMap.Find(MontageType);
    if (!Found || !Found->IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXAnimBridge: No montage mapped for type %d on '%s'."),
            static_cast<int32>(MontageType),
            *GetOwner()->GetName());
        return false;
    }

    UAnimMontage* Montage = Found->LoadSynchronous();
    if (!Montage) return false;

    UAnimInstance* AnimInst = CachedMesh->GetAnimInstance();
    if (!AnimInst) return false;

    // Play the montage.
    const float Duration = AnimInst->Montage_Play(Montage, PlayRate);
    if (Duration <= 0.0f) return false;

    CurrentMontageType = MontageType;

    // Bind blend-out delegate for completion notification.
    FOnMontageBlendingOutStarted BlendOutDelegate;
    BlendOutDelegate.BindUObject(this, &UMXAnimBridge::OnMontageBlendingOut);
    AnimInst->Montage_SetBlendingOutDelegate(BlendOutDelegate, Montage);

    UE_LOG(LogTemp, Log,
        TEXT("MXAnimBridge: Playing montage type %d (%.1fs) on '%s'."),
        static_cast<int32>(MontageType), Duration,
        *GetOwner()->GetName());

    return true;
}

void UMXAnimBridge::StopActionMontage(float BlendOutTime)
{
    if (!CachedMesh) return;

    UAnimInstance* AnimInst = CachedMesh->GetAnimInstance();
    if (!AnimInst) return;

    AnimInst->StopAllMontages(BlendOutTime);
    CurrentMontageType = EMXActionMontage::None;
}

void UMXAnimBridge::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    const EMXActionMontage FinishedType = CurrentMontageType;
    CurrentMontageType = EMXActionMontage::None;

    OnActionMontageEnded.Broadcast(FinishedType, bInterrupted);

    UE_LOG(LogTemp, Log,
        TEXT("MXAnimBridge: Montage type %d ended (interrupted=%d) on '%s'."),
        static_cast<int32>(FinishedType), bInterrupted,
        *GetOwner()->GetName());
}

// ---------------------------------------------------------------------------
// Idle Variant
// ---------------------------------------------------------------------------

void UMXAnimBridge::SetIdleVariant(int32 Variant)
{
    IdleVariant = FMath::Max(0, Variant);
}
