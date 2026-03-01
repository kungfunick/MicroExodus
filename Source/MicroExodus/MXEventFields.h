// MXEventFields.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: DEMS, Swarm (hazards), Camera, Evolution
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXEventFields.generated.h"

/**
 * FMXObjectEventFields
 * Data component attached to any hazard or interactive object in the level.
 * The DEMS reads this struct when constructing event messages â€” no hard-coded strings in hazard actors.
 * Defined per hazard type in DT_HazardFields DataTable, then placed on actors via Blueprint.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXObjectEventFields
{
    GENERATED_BODY()

    // -------------------------------------------------------------------------
    // Classification
    // -------------------------------------------------------------------------

    /** Elemental category of this hazard. Determines which evolution mark to apply. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHazardElement element = EHazardElement::Mechanical;

    /** Specific damage type inflicted. Used for encounter stat tracking and DEMS template selection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDamageType damage_type = EDamageType::Crush;

    // -------------------------------------------------------------------------
    // DEMS Verb Pool
    // -------------------------------------------------------------------------

    /** Primary past-tense verb for this hazard's action (e.g., "crushed", "incinerated"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString verb_past;

    /**
     * Alternate past-tense verb variants for variety rolling.
     * DEMS selects from verb_past + verb_past_alt to avoid repetition.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> verb_past_alt;

    // -------------------------------------------------------------------------
    // DEMS Source Noun Pool
    // -------------------------------------------------------------------------

    /** Primary noun describing this hazard's source (e.g., "the conveyor belt", "a fireball"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString source_noun;

    /** Alternate source noun variants for variety rolling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> source_noun_alt;

    // -------------------------------------------------------------------------
    // DEMS Grammar / Flavor
    // -------------------------------------------------------------------------

    /** Preposition connecting the source to context (e.g., "by", "from", "beneath"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString preposition;

    /**
     * Optional flavor suffix phrases appended to the end of the DEMS sentence for color.
     * One is selected randomly per event (e.g., "without warning", "in spectacular fashion").
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> flavor_suffix;

    // -------------------------------------------------------------------------
    // Behavior
    // -------------------------------------------------------------------------

    /** Narrative weight for this hazard. Controls DEMS template tier and camera response. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESeverity severity = ESeverity::Minor;

    /** Instructs the camera system how to respond when this hazard triggers an event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECameraBehavior camera_behavior = ECameraBehavior::Subtle;

    // -------------------------------------------------------------------------
    // Visual Evolution
    // -------------------------------------------------------------------------

    /**
     * The visual mark identifier applied to the robot on survival or death.
     * Passed to UMXEvolutionTarget::ApplyVisualMark() on the robot.
     * Examples: "burn_mark", "impact_crack", "frost_residue", "emp_scorch".
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString visual_mark;
};
