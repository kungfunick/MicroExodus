// MXMessageBuilder.h — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS
// The core sentence construction engine. Receives a raw event trigger and outputs
// a fully-resolved FMXEventData ready for dispatch. Performs variety rolls,
// template selection, token substitution, and deduplication checking.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXEventData.h"
#include "MXEventFields.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXTemplateSelector.h"
#include "MXDeduplicationBuffer.h"
#include "MXMessageBuilder.generated.h"

// ---------------------------------------------------------------------------
// FMXVarietyRollResult
// Intermediate result of a variety roll — three strings selected from
// the hazard's verb/source/flavor pools.
// ---------------------------------------------------------------------------

USTRUCT()
struct FMXVarietyRollResult
{
    GENERATED_BODY()

    /** Selected past-tense verb (from verb_past or verb_past_alt). */
    UPROPERTY()
    FString Verb;

    /** Selected source noun (from source_noun or source_noun_alt). */
    UPROPERTY()
    FString Source;

    /** Selected flavor suffix (from flavor_suffix, or empty string if none rolled). */
    UPROPERTY()
    FString Flavor;
};

// ---------------------------------------------------------------------------
// UMXMessageBuilder
// ---------------------------------------------------------------------------

UCLASS(BlueprintType)
class MICROEXODUS_API UMXMessageBuilder : public UObject
{
    GENERATED_BODY()

public:

    UMXMessageBuilder();

    /**
     * Initialize the builder. Must be called before any Build calls.
     * Triggers template loading and sets up internal references.
     * @param InTemplateSelector  Shared template selector instance (owned by the DEMS subsystem).
     * @param InDedupBuffer       Shared deduplication buffer instance.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Builder")
    void Initialize(UMXTemplateSelector* InTemplateSelector, UMXDeduplicationBuffer* InDedupBuffer);

    /**
     * Main entry point. Given a raw event trigger, construct a fully-resolved FMXEventData.
     * Performs variety rolling, deduplication, template selection, and token substitution.
     *
     * @param EventType     The class of this event (Death, Rescue, etc.).
     * @param HazardFields  The hazard object's event fields (verbs, nouns, flavor pools).
     * @param Robot         The robot's full persistent profile at the time of the event.
     * @param RunNumber     1-indexed run number for contextual tokens.
     * @param LevelNumber   1-indexed level number within the run.
     * @param SavedCount    For Sacrifice events: number of robots saved by the sacrifice. 0 otherwise.
     * @return              A fully constructed FMXEventData ready for dispatch.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Builder")
    FMXEventData BuildEventMessage(
        EEventType EventType,
        const FMXObjectEventFields& HazardFields,
        const FMXRobotProfile& Robot,
        int32 RunNumber,
        int32 LevelNumber,
        int32 SavedCount = 0);

    /**
     * Reset the deduplication buffer. Call at the start of each new run.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Builder")
    void ResetDedupBuffer();

private:

    /**
     * Select random verb, source, and flavor from the hazard's pools.
     * 70% chance to use the primary field, 30% chance to pick an alt if alts exist.
     * Flavor is selected from flavor_suffix pool with 80% probability; empty string otherwise.
     */
    FMXVarietyRollResult PerformVarietyRoll(const FMXObjectEventFields& Fields) const;

    /**
     * Check dedup safety and re-roll up to MAX_DEDUP_RETRIES times if needed.
     * Returns the accepted roll (or the last roll if all retries were exhausted).
     */
    FMXVarietyRollResult PerformVarietyRollWithDedup(const FMXObjectEventFields& Fields);

    /**
     * Substitute all {token} placeholders in Template with resolved values.
     * Unknown tokens are left in-place rather than silently dropped.
     */
    FString FillTokens(
        const FString& Template,
        const FMXRobotProfile& Robot,
        const FMXVarietyRollResult& Roll,
        const FString& Preposition,
        int32 RunNumber,
        int32 LevelNumber,
        int32 SavedCount,
        const FString& HatName) const;

    /**
     * Retrieve the best display string for the robot's specialization.
     * Prefers Mastery > Tier3 > Tier2 > Role > "None".
     */
    FString ResolveSpecLabel(const FMXRobotProfile& Robot) const;

    /**
     * Retrieve the mastery title string, or empty string if none.
     */
    FString ResolveMasteryLabel(const FMXRobotProfile& Robot) const;

    /**
     * Pick a random element from the robot's likes array, or a placeholder if empty.
     */
    FString ResolveRandomLike(const FMXRobotProfile& Robot) const;

    /**
     * Pick a random element from the robot's dislikes array, or a placeholder if empty.
     */
    FString ResolveRandomDislike(const FMXRobotProfile& Robot) const;

    /**
     * Format active_age_seconds into a human-readable string (e.g., "14h 22m active").
     */
    FString FormatActiveAge(float ActiveAgeSeconds) const;

    // Maximum number of variety re-rolls before forcing dispatch regardless of dedup state.
    static constexpr int32 MAX_DEDUP_RETRIES = 3;

    // Probability (0–100) of picking from the alt pool when alts exist.
    static constexpr int32 ALT_PICK_CHANCE = 30;

    // Probability (0–100) of appending a flavor suffix when the pool is non-empty.
    static constexpr int32 FLAVOR_PICK_CHANCE = 80;

    /** Shared template selector — not owned here. */
    UPROPERTY()
    TObjectPtr<UMXTemplateSelector> TemplateSelector;

    /** Shared deduplication buffer — not owned here. */
    UPROPERTY()
    TObjectPtr<UMXDeduplicationBuffer> DedupBuffer;
};
