// MXAnimInstance.cpp — MicroExodus Animation Instance
// Created: 2026-03-10

#include "MXAnimInstance.h"
#include "MXAnimBridge.h"
#include "GameFramework/Character.h"

// ---------------------------------------------------------------------------
// NativeInitializeAnimation — find the AnimBridge on the owning character
// ---------------------------------------------------------------------------

void UMXAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    APawn* Pawn = TryGetPawnOwner();
    if (!Pawn) return;

    CachedBridge = Pawn->FindComponentByClass<UMXAnimBridge>();

    if (CachedBridge)
    {
        UE_LOG(LogTemp, Log, TEXT("MXAnimInstance: Found AnimBridge on '%s'."), *Pawn->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MXAnimInstance: No AnimBridge on '%s' — anim values won't update."),
               *Pawn->GetName());
    }
}

// ---------------------------------------------------------------------------
// NativeUpdateAnimation — copy bridge values into BlendSpace-readable properties
// ---------------------------------------------------------------------------

void UMXAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!CachedBridge) return;

    Speed     = CachedBridge->Speed;
    Direction = CachedBridge->Direction;
    bIsMoving = CachedBridge->bIsMoving;
    bIsFalling = CachedBridge->bIsFalling;
    LeanAngle = CachedBridge->LeanAngle;
}
