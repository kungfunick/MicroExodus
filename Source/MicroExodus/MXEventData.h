// MXEventData.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: DEMS, Reports, Identity (life log), Persistence, Camera, UI
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXEventData.generated.h"

/**
 * FMXEventData
 * One fully-resolved event record produced by the DEMS after processing a game event.
 * Every significant moment in the game generates exactly one FMXEventData.
 * These are accumulated by the Roguelike module into FMXRunData, and consumed by
 * Reports, Identity (life log), Camera, and UI.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXEventData
{
    GENERATED_BODY()

    // -------------------------------------------------------------------------
    // Context
    // -------------------------------------------------------------------------

    /** Wall-clock time when this event was generated. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime timestamp;

    /** Which run number this event occurred in (1-indexed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 run_number = 0;

    /** Which level within the run this event occurred on (1â€“20). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 level_number = 0;

    /** Classification of the event, used to select the DEMS template pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEventType event_type = EEventType::Death;

    // -------------------------------------------------------------------------
    // Primary Robot
    // -------------------------------------------------------------------------

    /** The robot this event is about. Primary key for lookup. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid robot_id;

    /** Cached display name at time of event. Stored so reports don't need a live lookup. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString robot_name;

    /** Robot's level at the time of the event. Used in DEMS sentence and report scoring. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 robot_level = 1;

    /** Human-readable specialization label at time of event (e.g., "Trailblazer", "None"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString robot_spec;

    // -------------------------------------------------------------------------
    // Hat State at Time of Event
    // -------------------------------------------------------------------------

    /** Hat being worn by the robot when this event occurred. -1 if no hat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 hat_worn_id = -1;

    /** Cached hat display name. Empty string if no hat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString hat_worn_name;

    // -------------------------------------------------------------------------
    // Hazard Classification
    // -------------------------------------------------------------------------

    /** Elemental category of the hazard involved. EHazardElement::Fire for non-hazard events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHazardElement hazard_element = EHazardElement::Mechanical;

    /** Specific damage type. Used for encounter stat increments and template filtering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDamageType damage_type = EDamageType::Crush;

    // -------------------------------------------------------------------------
    // DEMS Resolved Strings (populated by MessageBuilder)
    // -------------------------------------------------------------------------

    /** Past-tense action verb resolved from the hazard's verb_past + variety roll. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString verb;

    /** Hazard source noun resolved from the object's source_noun + variety roll. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString source;

    /** Optional flavor suffix selected from the hazard's flavor_suffix pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString flavor;

    /** Preposition from the hazard object (e.g., "by", "from"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString preposition;

    /** Narrative severity of this event. Affects camera response and report highlight scoring. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESeverity severity = ESeverity::Minor;

    /** Camera behavior instruction dispatched alongside this event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECameraBehavior camera_behavior = ECameraBehavior::Subtle;

    // -------------------------------------------------------------------------
    // Visual Evolution
    // -------------------------------------------------------------------------

    /**
     * The visual mark string to apply to the robot's evolution state.
     * Forwarded to UMXEvolutionTarget::ApplyVisualMark() if the robot survived.
     * Example values: "burn_mark", "impact_crack", "frost_residue".
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString visual_mark;

    // -------------------------------------------------------------------------
    // Final Constructed Message
    // -------------------------------------------------------------------------

    /** The complete DEMS sentence assembled by MessageBuilder. Stored for the run report and life log. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString message_text;

    // -------------------------------------------------------------------------
    // Witness / Social Events
    // -------------------------------------------------------------------------

    /**
     * For witness-type events (RescueWitnessed, DeathWitnessed, SacrificeWitnessed):
     * the ID of the robot being witnessed. Invalid GUID if not a witness event.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid related_robot_id;

    /** Cached display name of the witnessed robot. Empty if not a witness event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString related_robot_name;
};
