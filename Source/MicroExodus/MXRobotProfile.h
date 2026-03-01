// MXRobotProfile.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: Identity, DEMS, Hats, Evolution, Specialization, Reports, Persistence, UI
// Change Log:
//   v1.0 â€” Initial definition of all persistent robot fields

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXEvolutionData.h"
#include "MXRobotProfile.generated.h"

/**
 * FMXRobotProfile
 * The canonical persistent data record for a single robot.
 * This struct is serialized to disk via the Persistence module and read by nearly every other module.
 * All fields reflect accumulated lifetime history â€” nothing here resets between runs.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRobotProfile
{
    GENERATED_BODY()

    // -------------------------------------------------------------------------
    // Identity
    // -------------------------------------------------------------------------

    /** Display name generated at birth. Immutable for the robot's lifetime. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString name;

    /** Wall-clock timestamp of when this robot was created (first added to the roster). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime date_of_birth;

    /** Unique identifier for this robot. Used as the primary key across all systems. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid robot_id;

    // -------------------------------------------------------------------------
    // Age
    // -------------------------------------------------------------------------

    /**
     * Wall-clock age since date_of_birth. Computed at runtime â€” not stored directly.
     * Persisted indirectly through date_of_birth. Drives patina_intensity in evolution.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTimespan calendar_age;

    /** Total seconds this robot has been actively deployed in-mission. Separate from calendar time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float active_age_seconds = 0.0f;

    // -------------------------------------------------------------------------
    // Progression
    // -------------------------------------------------------------------------

    /** Current level (1â€“100). Determines which specialization tiers are available. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 level = 1;

    /** XP accumulated toward the next level threshold. Resets on level-up. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 xp = 0;

    /** Lifetime XP earned across all runs. Never resets. Used for memorial stats. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 total_xp = 0;

    // -------------------------------------------------------------------------
    // Run / Level History
    // -------------------------------------------------------------------------

    /** Number of complete runs this robot has survived from start to finish. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 runs_survived = 0;

    /** Total individual levels completed across all runs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 levels_completed = 0;

    /** Consecutive runs survived without dying. Resets on death, not on run failure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 current_run_streak = 0;

    // -------------------------------------------------------------------------
    // Encounter Statistics
    // -------------------------------------------------------------------------

    /** Number of times this robot has survived a near-miss event (entered danger zone, escaped). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 near_misses = 0;

    /** Number of fire hazard encounters survived. Drives burn_intensity in evolution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 fire_encounters = 0;

    /** Number of crush/mechanical hazard encounters survived. Drives crack_intensity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 crush_encounters = 0;

    /** Number of fall hazard encounters survived. Drives fall_scar_intensity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 fall_encounters = 0;

    /** Number of EMP/electrical hazard encounters survived. Drives electrical_burn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 emp_encounters = 0;

    /** Number of ice hazard encounters survived. Drives ice_residue in evolution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ice_encounters = 0;

    // -------------------------------------------------------------------------
    // Social / Witnessed Events
    // -------------------------------------------------------------------------

    /** Number of other robots this robot has witnessed being rescued (within RESCUE_WITNESS_RADIUS). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 rescues_witnessed = 0;

    /** Number of other robots this robot has witnessed dying. Drives eye_somber in evolution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 deaths_witnessed = 0;

    /** Number of other robots this robot has witnessed sacrificing themselves. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 sacrifices_witnessed = 0;

    /** Number of puzzle objectives this robot has participated in solving. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 puzzles_participated = 0;

    // -------------------------------------------------------------------------
    // Appearance (indices into predefined palettes/meshes)
    // -------------------------------------------------------------------------

    /** Chassis base color index (0â€“11). Maps to the color palette in the master material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 chassis_color = 0;

    /** Eye color index (0â€“7). Maps to the eye emissive color array in the master material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 eye_color = 0;

    /** Antenna mesh variant index (0â€“4). Overridden by antenna_stage milestones in evolution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 antenna_style = 0;

    /** Paint scheme applied over the chassis color. None = default metallic. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPaintJob paint_job = EPaintJob::None;

    // -------------------------------------------------------------------------
    // Specialization
    // -------------------------------------------------------------------------

    /** The broad role this robot has committed to. Unlocks at level 10. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERobotRole role = ERobotRole::None;

    /** Tier 2 specialization chosen within the role. Unlocks at level 25. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier2Spec tier2_spec = ETier2Spec::None;

    /** Tier 3 specialization chosen within Tier 2. Unlocks at level 50. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier3Spec tier3_spec = ETier3Spec::None;

    /** Mastery title earned after completing the full Tier 3 arc. Unlocks at level 100. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMasteryTitle mastery_title = EMasteryTitle::None;

    // -------------------------------------------------------------------------
    // Hat
    // -------------------------------------------------------------------------

    /** ID of the hat currently equipped on this robot. -1 if no hat is worn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 current_hat_id = -1;

    /** Complete history of all hat IDs this robot has ever worn. Used for personality flavor and awards. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> hat_history;

    // -------------------------------------------------------------------------
    // Hat Wear Tracking (for "FashionForward" unlock and combo detection)
    // -------------------------------------------------------------------------

    /** Number of consecutive runs this robot has worn the same hat. Resets if hat changes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 consecutive_hat_runs = 0;

    /** Hat ID that the consecutive_hat_runs counter is tracking. -1 if none. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 consecutive_hat_id = -1;

    // -------------------------------------------------------------------------
    // Personality
    // -------------------------------------------------------------------------

    /** Short personality archetype description generated at birth (e.g., "cautious optimist"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString description;

    /** A list of things this robot "likes" â€” used for flavor text in DEMS and the roster screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> likes;

    /** A list of things this robot "dislikes" â€” used for flavor text in DEMS and the roster screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> dislikes;

    /** A single defining quirk that colors this robot's narrative voice in DEMS messages. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString quirk;

    // -------------------------------------------------------------------------
    // Titles
    // -------------------------------------------------------------------------

    /** All narrative titles this robot has earned through milestones and events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> earned_titles;

    /** The title currently displayed beneath this robot's name in the UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString displayed_title;

    // -------------------------------------------------------------------------
    // Visual Evolution State
    // -------------------------------------------------------------------------

    /** Full visual evolution state for this robot. Drives the Evolution module's material parameters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXVisualEvolutionState visual_evolution_state;
};
