// MXCameraPersonality.cpp — Camera Module v1.0
// Created: 2026-02-22
// Agent 7: Camera

#include "MXCameraPersonality.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXCameraPersonality::UMXCameraPersonality()
{
    // Defaults set in header initializers.
}

// ---------------------------------------------------------------------------
// FilterEvent
// ---------------------------------------------------------------------------

bool UMXCameraPersonality::FilterEvent(ECameraPersonalityMode Mode, const FMXCameraEvent& Event) const
{
    switch (Mode)
    {
        // ----------------------------------------------------------------
        // Director: all events play. This is the designed experience.
        // ----------------------------------------------------------------
        case ECameraPersonalityMode::Director:
            return Event.Behavior != ECameraBehavior::Suppress;

        // ----------------------------------------------------------------
        // Operator: minimal intrusion. Only the most significant moments.
        // Per GDD: "Only Epic + Cinematic deaths/sacrifices play."
        // Dramatic deaths are included if they have a hat drop — the hat
        // falling off is always worth showing even in Operator.
        // ----------------------------------------------------------------
        case ECameraPersonalityMode::Operator:
        {
            if (Event.Behavior == ECameraBehavior::Epic)      return true;
            if (Event.Behavior == ECameraBehavior::Cinematic) return true;
            if (Event.Behavior == ECameraBehavior::Dramatic && Event.bHasHatDrop) return true;
            return false;
        }

        // ----------------------------------------------------------------
        // Locked: absolutely nothing — player has full manual control.
        // ----------------------------------------------------------------
        case ECameraPersonalityMode::Locked:
            return false;

        // ----------------------------------------------------------------
        // Replay: all events play (at extended duration set by GetModeMultiplier).
        // ----------------------------------------------------------------
        case ECameraPersonalityMode::Replay:
            return Event.Behavior != ECameraBehavior::Suppress;

        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// GetModeMultiplier
// ---------------------------------------------------------------------------

float UMXCameraPersonality::GetModeMultiplier(ECameraPersonalityMode Mode) const
{
    switch (Mode)
    {
        case ECameraPersonalityMode::Director:  return 1.0f;
        case ECameraPersonalityMode::Operator:  return 0.5f;
        case ECameraPersonalityMode::Locked:    return 0.0f;
        case ECameraPersonalityMode::Replay:    return 1.5f;
        default:                                return 1.0f;
    }
}

// ---------------------------------------------------------------------------
// HandleMultiEventQueue
// ---------------------------------------------------------------------------

void UMXCameraPersonality::HandleMultiEventQueue(TArray<FMXCameraEvent>& Queue,
                                                  ECameraPersonalityMode Mode) const
{
    const int32 MaxSize = GetMaxQueueSize(Mode);

    // If below prune threshold, nothing to do.
    if (Queue.Num() < PruneThreshold) return;

    // ----------------------------------------------------------------
    // Step 1: Remove all Subtle events when queue is congested.
    //         Epic events are ALWAYS immune.
    // ----------------------------------------------------------------
    if (Queue.Num() >= PruneThreshold)
    {
        Queue.RemoveAll([](const FMXCameraEvent& E)
        {
            // Preserve Epic events unconditionally.
            if (E.Behavior == ECameraBehavior::Epic) return false;
            return E.Behavior == ECameraBehavior::Subtle;
        });
    }

    // ----------------------------------------------------------------
    // Step 2: If still congested, compress Dramatic durations.
    //         Per GDD: "play Dramatic at 0.5× duration".
    // ----------------------------------------------------------------
    if (Queue.Num() >= PruneThreshold)
    {
        for (FMXCameraEvent& E : Queue)
        {
            if (E.Behavior == ECameraBehavior::Dramatic)
            {
                E.Duration      *= DramaticCompressionFactor;
                E.TimeRemaining  = E.Duration;
            }
        }
    }

    // ----------------------------------------------------------------
    // Step 3: Re-sort by priority so the best events still rise to the top.
    // ----------------------------------------------------------------
    SortQueueByPriority(Queue);

    // ----------------------------------------------------------------
    // Step 4: Cap queue size, never removing Epic events.
    // ----------------------------------------------------------------
    CapQueueSize(Queue, MaxSize);
}

// ---------------------------------------------------------------------------
// GetMaxQueueSize
// ---------------------------------------------------------------------------

int32 UMXCameraPersonality::GetMaxQueueSize(ECameraPersonalityMode Mode) const
{
    switch (Mode)
    {
        case ECameraPersonalityMode::Director:  return 5;
        case ECameraPersonalityMode::Operator:  return 2;
        case ECameraPersonalityMode::Locked:    return 0;
        case ECameraPersonalityMode::Replay:    return 10;
        default:                                return 5;
    }
}

// ---------------------------------------------------------------------------
// GetModeDisplayName
// ---------------------------------------------------------------------------

FString UMXCameraPersonality::GetModeDisplayName(ECameraPersonalityMode Mode)
{
    switch (Mode)
    {
        case ECameraPersonalityMode::Director:  return TEXT("Director");
        case ECameraPersonalityMode::Operator:  return TEXT("Operator");
        case ECameraPersonalityMode::Locked:    return TEXT("Locked");
        case ECameraPersonalityMode::Replay:    return TEXT("Replay");
        default:                                return TEXT("Unknown");
    }
}

// ---------------------------------------------------------------------------
// GetModeDescription
// ---------------------------------------------------------------------------

FString UMXCameraPersonality::GetModeDescription(ECameraPersonalityMode Mode)
{
    switch (Mode)
    {
        case ECameraPersonalityMode::Director:
            return TEXT("Full cinematic reactions to all events — "
                         "the camera is a character, drawn to every moment of drama.");

        case ECameraPersonalityMode::Operator:
            return TEXT("Minimal camera movement — only deaths and sacrifices trigger reactions, "
                         "so you stay in control during intense sequences.");

        case ECameraPersonalityMode::Locked:
            return TEXT("No automatic camera movement. You have complete manual control at all times.");

        case ECameraPersonalityMode::Replay:
            return TEXT("Post-level cinematic playback with extended framing and enhanced reactions "
                         "to help you relive the best (and worst) moments of your run.");

        default:
            return TEXT("Unknown camera mode.");
    }
}

// ===========================================================================
// Private Helpers
// ===========================================================================

void UMXCameraPersonality::SortQueueByPriority(TArray<FMXCameraEvent>& Queue) const
{
    Queue.Sort([](const FMXCameraEvent& A, const FMXCameraEvent& B)
    {
        return A.Priority > B.Priority;  // Highest priority first.
    });
}

void UMXCameraPersonality::CapQueueSize(TArray<FMXCameraEvent>& Queue, int32 MaxSize) const
{
    if (MaxSize <= 0)
    {
        // Preserve Epic events even in Locked mode edge-case (shouldn't reach here,
        // but guard anyway — sacrifices should never silently disappear).
        Queue.RemoveAll([](const FMXCameraEvent& E)
        {
            return E.Behavior != ECameraBehavior::Epic;
        });
        return;
    }

    while (Queue.Num() > MaxSize)
    {
        // Find the lowest-priority non-Epic event to remove.
        int32 DropIndex = -1;
        float LowestPriority = TNumericLimits<float>::Max();

        for (int32 i = 0; i < Queue.Num(); ++i)
        {
            // Epic events are immune from removal.
            if (Queue[i].Behavior == ECameraBehavior::Epic) continue;

            if (Queue[i].Priority < LowestPriority)
            {
                LowestPriority = Queue[i].Priority;
                DropIndex = i;
            }
        }

        if (DropIndex == -1) break;  // Only Epic events remain — stop trimming.
        Queue.RemoveAt(DropIndex);
    }
}
