// MXSpecData.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: Specialization, Reports, DEMS, UI, Persistence
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXSpecData.generated.h"

/**
 * FMXSpecDefinition
 * DataTable row definition for a single node in the specialization tree.
 * Loaded from DT_SpecTree at startup. One row per unique spec combination.
 * Used by the Specialization module to populate the spec selection UI and validate progression.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXSpecDefinition : public FTableRowBase
{
    GENERATED_BODY()

    /** The broad role this spec node belongs to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERobotRole role = ERobotRole::None;

    /** Tier 2 specialization. ETier2Spec::None for role-level nodes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier2Spec tier2 = ETier2Spec::None;

    /** Tier 3 specialization. ETier3Spec::None for Tier 2 or role-level nodes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier3Spec tier3 = ETier3Spec::None;

    /** Mastery title unlocked upon completing this node's arc. EMasteryTitle::None for non-terminal nodes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMasteryTitle mastery = EMasteryTitle::None;

    /** Player-facing description of what this spec does. Displayed in the spec selection UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString bonus_description;

    /**
     * Name of the visual indicator asset or tag applied to the robot when this spec is active.
     * References a decal or material overlay defined in the Evolution module.
     * Example: "spec_scout_trailblazer_glow", "spec_guardian_sentinel_shield".
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString visual_indicator;
};

/**
 * FMXSpecBonus
 * The concrete numeric bonuses applied to a robot based on its current spec path.
 * Calculated at runtime by MXSpecBonusCalculator by combining bonuses from role + tier2 + tier3.
 * Consumed by the Swarm module for movement/behavior, and by the Roguelike module for XP distribution.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXSpecBonus
{
    GENERATED_BODY()

    /** Multiplier applied to this robot's movement speed within the swarm (1.0 = baseline). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float speed_modifier = 1.0f;

    /** Multiplier applied to physics weight/mass for this robot (1.0 = baseline). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float weight_modifier = 1.0f;

    /** Multiplier applied to XP earned per event for this robot (1.0 = baseline). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float xp_modifier = 1.0f;

    /** Number of times per run this robot can survive a hit that would otherwise be lethal. 0 = no immunity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 lethal_hits_per_run = 0;

    /** Number of times per run this robot can be self-revived after death. 0 = no revives. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 revives_per_run = 0;

    /** Radius in Unreal units within which this robot reveals hidden hazards. 0.0 = no reveal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float hazard_reveal_radius = 0.0f;

    /** 0.0â€“1.0 chance per frame this robot automatically dodges a hazard when in near-miss range. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float auto_dodge_chance = 0.0f;
};
