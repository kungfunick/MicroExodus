// MXCameraPersonality.h — Camera Module v1.0
// Created: 2026-02-22
// Agent 7: Camera
// Manages the four camera personality modes and their filtering / pruning behavior.
// Called by UMXSwarmCamera before committing any event to the active slot and
// during queue maintenance when events pile up during intense sequences.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXSwarmCamera.h"
#include "MXCameraBehaviors.h"
#include "MXCameraPersonality.generated.h"

// ---------------------------------------------------------------------------
// UMXCameraPersonality
// ---------------------------------------------------------------------------

/**
 * UMXCameraPersonality
 * Stateless helper that encodes the filtering and timing rules for each
 * ECameraPersonalityMode. UMXSwarmCamera delegates all mode-specific
 * decisions here so the queue logic stays in one place.
 *
 * Usage:
 *   1. Before queueing: call FilterEvent() — if false, discard the event.
 *   2. After building the event's Duration: multiply by GetModeMultiplier().
 *   3. During intense queue build-up: call HandleMultiEventQueue() to prune.
 */
UCLASS(BlueprintType)
class MICROEXODUS_API UMXCameraPersonality : public UObject
{
    GENERATED_BODY()

public:

    UMXCameraPersonality();

    // =========================================================================
    // Per-Event Filtering
    // =========================================================================

    /**
     * Determine whether a given FMXCameraEvent should play in the current mode.
     *
     * Director  → All events play.
     * Operator  → Only Cinematic deaths/sacrifices and Epic events play.
     * Locked    → Nothing plays (returns false for all events).
     * Replay    → All events play (at extended duration via GetModeMultiplier).
     *
     * @param Mode   The active personality mode.
     * @param Event  The event to evaluate.
     * @return       True if the event should be queued / executed.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Camera|Personality")
    bool FilterEvent(ECameraPersonalityMode Mode, const FMXCameraEvent& Event) const;

    // =========================================================================
    // Duration Multiplier
    // =========================================================================

    /**
     * Return the duration multiplier for the given mode.
     * Multiply the event's base Duration by this value before queuing.
     *
     * Director  → 1.0×  (normal)
     * Operator  → 0.5×  (abbreviated — less intrusive)
     * Locked    → 0.0×  (should never be reached; FilterEvent returns false)
     * Replay    → 1.5×  (extended for post-level review)
     *
     * @param Mode  The active personality mode.
     * @return      Multiplier float.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Camera|Personality")
    float GetModeMultiplier(ECameraPersonalityMode Mode) const;

    // =========================================================================
    // Queue Management
    // =========================================================================

    /**
     * Prune and compress the camera event queue during intense sequences.
     *
     * Rules (applied in order):
     *   1. If queue has 3+ pending events, remove all Subtle events.
     *   2. If queue still has 3+ events, halve the Duration of all Dramatic events.
     *   3. Epic events are NEVER removed or compressed — sacrifice is never skipped.
     *   4. At most GetMaxQueueSize(Mode) events remain after pruning.
     *
     * @param Queue  Reference to the live event queue. Modified in-place.
     * @param Mode   The active personality mode (affects max size).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Camera|Personality")
    void HandleMultiEventQueue(TArray<FMXCameraEvent>& Queue, ECameraPersonalityMode Mode) const;

    // =========================================================================
    // Queue Size Cap
    // =========================================================================

    /**
     * Maximum number of events allowed in the queue for a given mode.
     * Events beyond this cap are discarded (lowest priority first).
     *
     * Director  → 5
     * Operator  → 2
     * Locked    → 0
     * Replay    → 10
     *
     * @param Mode  The active personality mode.
     * @return      Integer max queue size.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Camera|Personality")
    int32 GetMaxQueueSize(ECameraPersonalityMode Mode) const;

    // =========================================================================
    // Intensity Thresholds
    // =========================================================================

    /**
     * Minimum queue depth that triggers pruning in HandleMultiEventQueue().
     * Default: 3 (per GDD — "if a queue builds up (3+ events)").
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Personality|Config")
    int32 PruneThreshold = 3;

    /**
     * Scale factor applied to Dramatic event Duration during queue compression.
     * Default: 0.5 (per GDD — "play Dramatic at 0.5× duration").
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Camera|Personality|Config")
    float DramaticCompressionFactor = 0.5f;

    // =========================================================================
    // Mode Description Helpers (for UI / debug)
    // =========================================================================

    /**
     * Return a human-readable label for a mode. Used by the UI overlay.
     * @param Mode  The mode to label.
     * @return      Short display string.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Camera|Personality")
    static FString GetModeDisplayName(ECameraPersonalityMode Mode);

    /**
     * Return a tooltip description for a mode. Used in the settings screen.
     * @param Mode  The mode to describe.
     * @return      One-sentence description string.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Camera|Personality")
    static FString GetModeDescription(ECameraPersonalityMode Mode);

private:

    /**
     * Sort the queue by descending priority so highest-priority events execute first.
     * Called internally by HandleMultiEventQueue after pruning.
     */
    void SortQueueByPriority(TArray<FMXCameraEvent>& Queue) const;

    /**
     * Remove the lowest-priority events until the queue is at or below MaxSize.
     * Epic events are immune from removal.
     */
    void CapQueueSize(TArray<FMXCameraEvent>& Queue, int32 MaxSize) const;
};
