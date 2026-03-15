// MXAnimInstance.h — MicroExodus Animation Instance
// Created: 2026-03-10
//
// Minimal C++ AnimInstance base class that auto-discovers UMXAnimBridge on the
// owning character and copies locomotion values each tick. BlueprintReadOnly
// properties are directly usable as BlendSpace axes in any child AnimBP.
//
// Usage:
//   1. Create an AnimBP parented to this class (Reparent in AnimBP editor)
//   2. In AnimGraph, add BS_Idle_Walk_Run driven by the Speed property
//   3. Set the AnimBP as AnimBlueprintClass on BP_MXRobot
//   4. Done — Speed/Direction/bIsMoving update automatically each tick

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MXAnimInstance.generated.h"

class UMXAnimBridge;

UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:

    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    // =========================================================================
    // Locomotion — fed from UMXAnimBridge, readable by BlendSpaces
    // =========================================================================

    /** Ground speed in cm/s. Primary axis for idle→walk→run BlendSpace. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Anim|Locomotion")
    float Speed = 0.0f;

    /** Movement direction relative to actor forward [-180, 180]. Secondary BlendSpace axis. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Anim|Locomotion")
    float Direction = 0.0f;

    /** True when robot has meaningful ground velocity. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Anim|Locomotion")
    bool bIsMoving = false;

    /** True when falling (not grounded). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Anim|Locomotion")
    bool bIsFalling = false;

    /** Lean angle for procedural lean blending. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Anim|Locomotion")
    float LeanAngle = 0.0f;

private:

    /** Cached reference to the owning actor's AnimBridge component. */
    UPROPERTY()
    TObjectPtr<UMXAnimBridge> CachedBridge;
};
