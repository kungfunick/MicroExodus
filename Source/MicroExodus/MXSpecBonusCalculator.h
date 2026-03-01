// MXSpecBonusCalculator.h — Specialization Module Version 1.0
// Created: 2026-02-22
// Agent 5: Specialization
// Consumers: MXRoleComponent (primary), DEMS, UI (bonus display)
// Change Log:
//   v1.0 — Initial definition

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXTypes.h"
#include "MXSpecData.h"
#include "MXSpecBonusCalculator.generated.h"

/**
 * UMXSpecBonusCalculator
 * Pure stateless utility class that resolves a full specialization path into an FMXSpecBonus.
 * All methods are static — instantiation is not required.
 *
 * Bonus stacking rules:
 *   - Speed bonuses are additive (Scout +0.15, Pathfinder adds to reach +0.20 total, NOT +0.35).
 *     The higher tier replaces/supersedes the lower in speed terms per GDD.
 *   - Weight multipliers are set to their highest tier value (Foundation = ×5.0 supersedes Architect ×3.0).
 *   - XP modifiers are additive on top of 1.0 baseline (Engineer +0.50, Mechanic +0.25 = +0.75 on puzzles).
 *   - Lethal hits, revives, radii, and dodge chance are set by highest applicable tier.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXSpecBonusCalculator : public UObject
{
    GENERATED_BODY()

public:

    /**
     * Computes the full FMXSpecBonus for the given spec path.
     * Bonuses from each tier stack appropriately per the rules above.
     * Passing None values at any tier is valid — bonuses from lower tiers still apply.
     *
     * @param Role      The robot's chosen Role (or ERobotRole::None if unspecialized).
     * @param Tier2     The robot's Tier 2 spec (or ETier2Spec::None).
     * @param Tier3     The robot's Tier 3 spec (or ETier3Spec::None).
     * @param Mastery   The robot's Mastery title (or EMasteryTitle::None).
     * @return          The fully computed FMXSpecBonus ready for gameplay consumption.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static FMXSpecBonus CalculateBonus(ERobotRole Role,
                                       ETier2Spec  Tier2,
                                       ETier3Spec  Tier3,
                                       EMasteryTitle Mastery);

    /**
     * Returns a human-readable summary of the visual changes applied by this spec path.
     * Intended for the Evolution module and the spec selection UI preview.
     *
     * @param Role      The robot's chosen Role.
     * @param Tier2     The robot's Tier 2 spec.
     * @param Tier3     The robot's Tier 3 spec.
     * @param Mastery   The robot's Mastery title.
     * @return          Comma-separated list of visual indicator tags.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static FString GetVisualIndicator(ERobotRole Role,
                                      ETier2Spec  Tier2,
                                      ETier3Spec  Tier3,
                                      EMasteryTitle Mastery);

    // -------------------------------------------------------------------------
    // Tier-isolated queries (convenience accessors for Swarm/Hazard modules)
    // -------------------------------------------------------------------------

    /** Returns the speed modifier contribution from a given Role. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static float GetRoleSpeedModifier(ERobotRole Role);

    /** Returns the speed modifier contribution from a given Tier 2 spec. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static float GetTier2SpeedModifier(ETier2Spec Tier2);

    /** Returns the weight multiplier for a given spec path (highest tier wins). */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static float GetWeightModifier(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3);

    /** Returns the XP modifier for a given spec path. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static float GetXPModifier(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3);

    /** Returns the number of lethal hit absorptions per run for this path. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static int32 GetLethalHitsPerRun(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3);

    /** Returns the number of robot revives per run for this path. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static int32 GetRevivesPerRun(ETier2Spec Tier2, ETier3Spec Tier3);

    /** Returns the hazard reveal radius (Lookout / Oracle effect). */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static float GetHazardRevealRadius(ETier2Spec Tier2);

    /** Returns the auto-dodge chance (Lookout effect). */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization", meta = (BlueprintPure))
    static float GetAutoDodgeChance(ETier2Spec Tier2);
};
