// MXEvolutionData.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: Identity, Evolution, Persistence, UI
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXEvolutionData.generated.h"

/**
 * FMXVisualEvolutionState
 * Encodes the full visual history of a robot's chassis.
 * Driven entirely by accumulated encounter stats â€” not player customization.
 * Passed to the Evolution module which applies parameters to the robot material instance.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXVisualEvolutionState
{
    GENERATED_BODY()

    // -------------------------------------------------------------------------
    // Overall Wear
    // -------------------------------------------------------------------------

    /** Aggregate wear tier computed from all encounter data. Controls base material grime/aging. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EWearLevel wear_level = EWearLevel::Fresh;

    // -------------------------------------------------------------------------
    // Encounter-Driven Intensity Parameters (0.0 â€“ 1.0 unless noted)
    // -------------------------------------------------------------------------

    /** Charring and soot coverage from fire_encounters. Drives burn overlay opacity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float burn_intensity = 0.0f;

    /** Surface fractures from crush/fall encounters. Drives crack decal density. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float crack_intensity = 0.0f;

    /** Repair weld seam visibility from survived near-misses and rescues. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float weld_coverage = 0.0f;

    /** Oxidation and age patina driven by calendar_age and active_age_seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float patina_intensity = 0.0f;

    // -------------------------------------------------------------------------
    // Eye / Expressive Parameters
    // -------------------------------------------------------------------------

    /** Eye emissive brightness. Increases with level and spec attainment. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float eye_brightness = 1.0f;

    /** Eye desaturation toward somber blue-grey. Increases with deaths_witnessed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float eye_somber = 0.0f;

    // -------------------------------------------------------------------------
    // Element-Specific Marks
    // -------------------------------------------------------------------------

    /** Gouges and dents from fall_encounters. Separate from crack_intensity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float fall_scar_intensity = 0.0f;

    /** Ice crystal residue and frost texture from ice_encounters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ice_residue = 0.0f;

    /** Scorch patterns and conductive burn traces from emp_encounters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float electrical_burn = 0.0f;

    // -------------------------------------------------------------------------
    // Staged Cosmetic Unlocks (incremented at milestone thresholds)
    // -------------------------------------------------------------------------

    /** Antenna visual stage. 0 = default, 1â€“4 = progressively elaborate. Driven by level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 antenna_stage = 0;

    /** Armor plating stage. 0 = bare, 1 = partial, 2 = full. Driven by runs_survived. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 armor_stage = 0;

    /** Aura/glow effect stage. 0 = none, 1â€“3 = increasing intensity. Driven by mastery_title. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 aura_stage = 0;

    // -------------------------------------------------------------------------
    // Procedural Seed
    // -------------------------------------------------------------------------

    /**
     * Deterministic seed for procedural decal placement (scratches, dents, paint chips).
     * Derived from: hash(robot_name) XOR hash(date_of_birth timestamp).
     * Set once at birth â€” never changes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 decal_seed = 0;
};
