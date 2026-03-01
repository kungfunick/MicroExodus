// MXCameraBehaviors.cpp — Camera Module v1.0
// Created: 2026-02-22
// Agent 7: Camera

#include "MXCameraBehaviors.h"
#include "MXSwarmCamera.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXCameraBehaviors::UMXCameraBehaviors()
{
    // Defaults set in header initializers.
}

// ---------------------------------------------------------------------------
// IMXEventListener — OnDEMSEvent_Implementation
// ---------------------------------------------------------------------------

void UMXCameraBehaviors::OnDEMSEvent_Implementation(const FMXEventData& Event)
{
    if (!SwarmCamera) return;

    // Resolve the camera behavior (potentially upgraded).
    const ECameraBehavior EffectiveBehavior = ShouldUpgradeBehavior(Event);

    // Check personality filter.
    // Personality object is stateless; we instantiate it or call its logic inline.
    // For Suppress: do nothing.
    if (EffectiveBehavior == ECameraBehavior::Suppress) return;

    FMXCameraEvent CameraEvent;

    // Route to the appropriate factory based on event type and behavior.
    const bool bIsDeath      = (Event.event_type == EEventType::Death);
    const bool bIsSacrifice  = (Event.event_type == EEventType::Sacrifice);
    const bool bIsNearMiss   = (Event.event_type == EEventType::NearMiss);
    const bool bHasHat       = (Event.hat_worn_id != -1);

    if (bIsSacrifice)
    {
        CameraEvent = CreateSacrificeSequence(Event);
    }
    else if (bIsDeath)
    {
        CameraEvent = CreateDeathSequence(Event, bHasHat, Event.robot_level);
    }
    else if (bIsNearMiss && EffectiveBehavior >= ECameraBehavior::Dramatic)
    {
        CameraEvent = CreateNearMissFlinch(Event);
    }
    else
    {
        // Generic behavior routing.
        switch (EffectiveBehavior)
        {
            case ECameraBehavior::Subtle:
                CameraEvent = CreateSubtleBehavior(Event);
                break;
            case ECameraBehavior::Dramatic:
                CameraEvent = CreateDramaticBehavior(Event);
                break;
            case ECameraBehavior::Cinematic:
                CameraEvent = CreateCinematicBehavior(Event);
                break;
            case ECameraBehavior::Epic:
                CameraEvent = CreateEpicBehavior(Event);
                break;
            default:
                return;
        }
    }

    SwarmCamera->QueueCameraEvent(CameraEvent);
}

// ---------------------------------------------------------------------------
// CreateSubtleBehavior
// ---------------------------------------------------------------------------

FMXCameraEvent UMXCameraBehaviors::CreateSubtleBehavior(const FMXEventData& Event) const
{
    FMXCameraEvent Out;
    Out.Behavior        = ECameraBehavior::Subtle;
    Out.TargetLocation  = FVector::ZeroVector;   // No positional move — shake in place.
    Out.TargetRobotId   = FGuid();               // No robot focus.
    Out.Duration        = BehaviorToBaseDuration(ECameraBehavior::Subtle);
    Out.TimeRemaining   = Out.Duration;
    Out.ZoomOverride    = -1.0f;                 // No zoom change.
    Out.bHasHatDrop     = false;
    Out.Priority        = BehaviorToPriority(ECameraBehavior::Subtle);
    Out.ShakeAmplitude  = 0.3f;
    Out.ShakeFrequency  = 12.0f;
    return Out;
}

// ---------------------------------------------------------------------------
// CreateDramaticBehavior
// ---------------------------------------------------------------------------

FMXCameraEvent UMXCameraBehaviors::CreateDramaticBehavior(const FMXEventData& Event) const
{
    FMXCameraEvent Out;
    Out.Behavior        = ECameraBehavior::Dramatic;
    Out.TargetRobotId   = Event.robot_id;
    Out.Duration        = BehaviorToBaseDuration(ECameraBehavior::Dramatic);
    Out.TimeRemaining   = Out.Duration;
    Out.ZoomOverride    = CinematicZoomHeight * 1.3f;  // Partial zoom — not full cinematic.
    Out.bHasHatDrop     = false;
    Out.Priority        = BehaviorToPriority(ECameraBehavior::Dramatic);
    Out.ShakeAmplitude  = 0.6f;
    Out.ShakeFrequency  = 8.0f;
    return Out;
}

// ---------------------------------------------------------------------------
// CreateCinematicBehavior
// ---------------------------------------------------------------------------

FMXCameraEvent UMXCameraBehaviors::CreateCinematicBehavior(const FMXEventData& Event) const
{
    FMXCameraEvent Out;
    Out.Behavior        = ECameraBehavior::Cinematic;
    Out.TargetRobotId   = Event.robot_id;
    Out.Duration        = BehaviorToBaseDuration(ECameraBehavior::Cinematic);
    Out.TimeRemaining   = Out.Duration;
    Out.ZoomOverride    = CinematicZoomHeight;
    Out.bHasHatDrop     = false;
    Out.Priority        = BehaviorToPriority(ECameraBehavior::Cinematic);
    Out.ShakeAmplitude  = 0.0f;   // Cinematic events don't shake — smooth dolly only.
    return Out;
}

// ---------------------------------------------------------------------------
// CreateEpicBehavior
// ---------------------------------------------------------------------------

FMXCameraEvent UMXCameraBehaviors::CreateEpicBehavior(const FMXEventData& Event) const
{
    FMXCameraEvent Out;
    Out.Behavior        = ECameraBehavior::Epic;
    Out.TargetRobotId   = Event.robot_id;
    Out.Duration        = BehaviorToBaseDuration(ECameraBehavior::Epic);
    Out.TimeRemaining   = Out.Duration;
    Out.ZoomOverride    = EpicZoomHeight;
    Out.bHasHatDrop     = false;
    Out.Priority        = BehaviorToPriority(ECameraBehavior::Epic);
    Out.ShakeAmplitude  = 0.0f;   // Epic events rely on dolly movement, not shake.
    return Out;
}

// ---------------------------------------------------------------------------
// CreateDeathSequence
// ---------------------------------------------------------------------------

FMXCameraEvent UMXCameraBehaviors::CreateDeathSequence(const FMXEventData& Event,
                                                        bool bHasHat,
                                                        int32 RobotLevel) const
{
    // Per GDD death sequence:
    //   Zoom toward robot (0.5s)
    //   Brief hold on death moment (0.3s)
    //   [If hatted] linger as hat falls (3.0s)
    //   Slow pullback (0.5s)
    //
    // Base duration without hat: 0.5 + 0.3 + 0.5 = 1.3s
    // With hat: 1.3 + 3.0 = 4.3s

    float BaseDuration = 1.3f;
    float LingerTime   = 0.0f;

    if (bHasHat)
    {
        LingerTime    = HatDeathLingerTime;
        BaseDuration += LingerTime;
    }

    // Determine behavior tier: death events start at Cinematic, can upgrade to Epic.
    ECameraBehavior Behavior = ECameraBehavior::Cinematic;

    if (RobotLevel >= EpicDeathLevelThreshold)
    {
        Behavior = ECameraBehavior::Epic;
    }
    else if (GetHatRarity(Event) == EHatRarity::Legendary)
    {
        Behavior = ECameraBehavior::Epic;
    }

    // Build the event.
    FMXCameraEvent Out;
    Out.Behavior       = Behavior;
    Out.TargetRobotId  = Event.robot_id;
    Out.Duration       = BaseDuration;
    Out.TimeRemaining  = BaseDuration;
    Out.ZoomOverride   = (Behavior == ECameraBehavior::Epic) ? EpicZoomHeight : CinematicZoomHeight;
    Out.bHasHatDrop    = bHasHat;
    Out.HatLingerTime  = LingerTime;
    Out.Priority       = BehaviorToPriority(Behavior);
    Out.ShakeAmplitude = 0.0f;

    return Out;
}

// ---------------------------------------------------------------------------
// CreateSacrificeSequence
// ---------------------------------------------------------------------------

FMXCameraEvent UMXCameraBehaviors::CreateSacrificeSequence(const FMXEventData& Event) const
{
    // Per GDD sacrifice sequence (always Epic):
    //   Dolly to sacrifice gate
    //   Hold on robot as eyes dim
    //   Linger on hat falling last
    //   Slow mournful pullback
    //   Total ~4-5 seconds
    //
    // Hat always lingers at end of a sacrifice — the hat falling is the emotional beat.

    FMXCameraEvent Out;
    Out.Behavior       = ECameraBehavior::Epic;
    Out.TargetRobotId  = Event.robot_id;
    Out.Duration       = SacrificeTotalDuration;
    Out.TimeRemaining  = SacrificeTotalDuration;
    Out.ZoomOverride   = EpicZoomHeight;
    Out.bHasHatDrop    = true;    // Hat falling is always part of the sacrifice sequence.
    Out.HatLingerTime  = 1.5f;    // Hat gets its own moment within the total duration.
    Out.Priority       = BehaviorToPriority(ECameraBehavior::Epic);
    Out.ShakeAmplitude = 0.0f;

    return Out;
}

// ---------------------------------------------------------------------------
// CreateNearMissFlinch
// ---------------------------------------------------------------------------

FMXCameraEvent UMXCameraBehaviors::CreateNearMissFlinch(const FMXEventData& Event) const
{
    // Near-miss flinch: snap-zoom toward robot, immediately spring back.
    // Very short — 0.4s total. Creates a "recoil with the player" feel.

    FMXCameraEvent Out;
    Out.Behavior       = ECameraBehavior::Dramatic;    // Uses Dramatic slot in queue.
    Out.TargetRobotId  = Event.robot_id;
    Out.Duration       = 0.4f;
    Out.TimeRemaining  = 0.4f;
    Out.ZoomOverride   = FlinchZoomHeight;
    Out.bHasHatDrop    = false;
    Out.Priority       = BehaviorToPriority(ECameraBehavior::Dramatic);
    Out.ShakeAmplitude = 1.0f;    // Stronger shake for the flinch effect.
    Out.ShakeFrequency = 20.0f;   // Fast frequency for snappy feel.

    return Out;
}

// ---------------------------------------------------------------------------
// ShouldUpgradeBehavior
// ---------------------------------------------------------------------------

ECameraBehavior UMXCameraBehaviors::ShouldUpgradeBehavior(const FMXEventData& Event) const
{
    ECameraBehavior Base = Event.camera_behavior;

    // Sacrifices are always Epic — never downgrade.
    if (Event.event_type == EEventType::Sacrifice)
    {
        return ECameraBehavior::Epic;
    }

    // Death-specific upgrade logic.
    if (Event.event_type == EEventType::Death)
    {
        // Any death → minimum Cinematic.
        if (Base < ECameraBehavior::Cinematic)
        {
            Base = ECameraBehavior::Cinematic;
        }

        // Hat-wearing robot → at least Cinematic (already set above).
        // Legendary hat → Epic.
        if (Event.hat_worn_id != -1)
        {
            if (GetHatRarity(Event) == EHatRarity::Legendary)
            {
                Base = ECameraBehavior::Epic;
            }
        }

        // High-level robot → upgrade tiers.
        if (Event.robot_level >= EpicDeathLevelThreshold)
        {
            Base = ECameraBehavior::Epic;
        }
        else if (Event.robot_level >= CinematicDeathLevelThreshold)
        {
            if (Base < ECameraBehavior::Cinematic)
            {
                Base = ECameraBehavior::Cinematic;
            }
        }
    }

    return Base;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

EHatRarity UMXCameraBehaviors::GetHatRarity(const FMXEventData& Event) const
{
    if (Event.hat_worn_id == -1) return EHatRarity::Common;
    if (!RobotProvider.GetInterface()) return EHatRarity::Common;

    const FMXRobotProfile Profile =
        IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), Event.robot_id);

    // Hat rarity is not cached directly on FMXRobotProfile.
    // The hat_worn_id in the event matches FMXRobotProfile.current_hat_id.
    // To get rarity we need the Hat module's DataTable — but we have no direct dependency.
    // We approximate: any hat the robot has worn long enough to be "notable" gets Cinematic;
    // we'll only return Legendary if the hat_worn_id maps into the known Legendary band.
    // The Hat module agent (Agent 4) owns rarity; for now return Common as safe default.
    // When Agent 4 wires the HatManager subsystem, it can provide a UMXHatProvider interface
    // and this method can call Execute_GetHatDefinition to get real rarity.

    // Conservative default: treat any hat-death as at least notable, not auto-Legendary.
    return EHatRarity::Common;
}

float UMXCameraBehaviors::BehaviorToPriority(ECameraBehavior Behavior)
{
    switch (Behavior)
    {
        case ECameraBehavior::Epic:      return 4.0f;
        case ECameraBehavior::Cinematic: return 3.0f;
        case ECameraBehavior::Dramatic:  return 2.0f;
        case ECameraBehavior::Subtle:    return 1.0f;
        case ECameraBehavior::Suppress:  return 0.0f;
        default:                         return 1.0f;
    }
}

float UMXCameraBehaviors::BehaviorToBaseDuration(ECameraBehavior Behavior)
{
    // Durations per GDD camera behavior table.
    switch (Behavior)
    {
        case ECameraBehavior::Subtle:    return 0.3f;
        case ECameraBehavior::Dramatic:  return 0.8f;
        case ECameraBehavior::Cinematic: return 1.5f;
        case ECameraBehavior::Epic:      return 3.0f;
        case ECameraBehavior::Suppress:  return 0.0f;
        default:                         return 0.3f;
    }
}
