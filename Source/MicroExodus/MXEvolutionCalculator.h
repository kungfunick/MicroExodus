// MXEvolutionCalculator.h — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Consumers: MXEvolutionLayerSystem, UI (preview), tests
// Change Log:
//   v1.0 — Initial implementation. Pure static calculation layer for all 12 evolution layers.
//           No side effects. No UObject state. Safe to call from any thread.

#pragma once

#include "CoreMinimal.h"
#include "MXRobotProfile.h"
#include "MXEvolutionData.h"
#include "MXTypes.h"
#include "MXEvolutionCalculator.generated.h"

/**
 * UMXEvolutionCalculator
 *
 * Pure static math layer for the Evolution system. Every function is a deterministic
 * transformation from robot stats to a float (or enum) result. No state is held.
 * All intensity values are clamped to [0.0, 1.0] unless documented otherwise.
 *
 * Dependency graph: Depends only on shared contract headers (MXRobotProfile, MXEvolutionData,
 * MXTypes). No other Evolution classes depend on this being a UObject — it is one purely for
 * Blueprint exposure.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXEvolutionCalculator : public UObject
{
    GENERATED_BODY()

public:

    // =========================================================================
    // One-Shot Full State Computation
    // =========================================================================

    /**
     * ComputeFullState
     * Derives a complete FMXVisualEvolutionState from a robot profile in a single call.
     * Preserves decal_seed (set at birth; never recomputed here).
     * Called by MXEvolutionLayerSystem::RecalculateEvolution.
     *
     * @param Robot   The source profile. Read-only.
     * @return        Fully populated FMXVisualEvolutionState ready for material application.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static FMXVisualEvolutionState ComputeFullState(const FMXRobotProfile& Robot);

    // =========================================================================
    // Layer 1: Base Wear
    // =========================================================================

    /**
     * ComputeWearLevel
     * Maps total encounters + active_age_seconds to an EWearLevel tier.
     *
     * Thresholds:
     *   Fresh        — < 10 total encounters
     *   Used         — 10–29 encounters
     *   Worn         — 30–59 encounters
     *   BattleScarred— 60–99 encounters
     *   Ancient      — 100+ encounters OR active_age_seconds > 36000 (10 hours)
     *
     * @param Robot   Full profile; reads all encounter counters and active_age_seconds.
     * @return        EWearLevel enum value.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static EWearLevel ComputeWearLevel(const FMXRobotProfile& Robot);

    /**
     * GetTotalEncounters
     * Sums all element encounter counters. Used as input to wear level and summaries.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static int32 GetTotalEncounters(const FMXRobotProfile& Robot);

    // =========================================================================
    // Layer 2: Burn Marks (Fire)
    // =========================================================================

    /**
     * ComputeBurnIntensity
     * burn_intensity = Clamp(fire_encounters / 50.0, 0, 1)
     * Saturates at 50 fire encounters.
     *
     * @param FireEncounters   Robot's accumulated fire_encounters count.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeBurnIntensity(int32 FireEncounters);

    // =========================================================================
    // Layer 3: Impact Cracks (Crush + Fall)
    // =========================================================================

    /**
     * ComputeCrackIntensity
     * crack_intensity = Clamp((crush_encounters + fall_encounters) / 40.0, 0, 1)
     * Saturates at 40 combined encounters.
     *
     * @param CrushEncounters  Robot's accumulated crush_encounters.
     * @param FallEncounters   Robot's accumulated fall_encounters.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeCrackIntensity(int32 CrushEncounters, int32 FallEncounters);

    // =========================================================================
    // Layer 4: Weld Repairs (Near-Misses)
    // =========================================================================

    /**
     * ComputeWeldCoverage
     * weld_coverage = Clamp(near_misses / 60.0, 0, 1)
     * Saturates at 60 near-misses. Weld lines appear where cracks would have formed.
     *
     * @param NearMisses  Robot's accumulated near_misses count.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeWeldCoverage(int32 NearMisses);

    // =========================================================================
    // Layer 5: Rust / Patina (Age)
    // =========================================================================

    /**
     * ComputePatinaIntensity
     * patina_intensity = Clamp(calendar_age_days / 30.0, 0, 1)
     * Saturates at 30 real-world calendar days.
     * active_age_seconds provides secondary contribution at a slower rate.
     *
     * @param CalendarAgeDays    Robot's age in real-world days since date_of_birth.
     * @param ActiveAgeSeconds   Accumulated in-game active time.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputePatinaIntensity(float CalendarAgeDays, float ActiveAgeSeconds);

    // =========================================================================
    // Layer 6: Eye Evolution (Level + Deaths Witnessed)
    // =========================================================================

    /**
     * ComputeEyeBrightness
     * eye_brightness = Clamp(0.5 + (Level / 200.0), 0.5, 1.0)
     * Ranges from 0.5 (Level 1) to 1.0 (Level 100+).
     *
     * @param Level  Robot's current level.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeEyeBrightness(int32 Level);

    /**
     * ComputeEyeSomber
     * eye_somber = Clamp(deaths_witnessed / 50.0, 0, 1)
     * Saturates at 50 witnessed deaths. Desaturates eye color toward somber blue-grey.
     *
     * @param DeathsWitnessed  Robot's accumulated deaths_witnessed count.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeEyeSomber(int32 DeathsWitnessed);

    // =========================================================================
    // Layer 7: Antenna Growth (Level Milestones)
    // =========================================================================

    /**
     * ComputeAntennaStage
     * Stage thresholds: 0 (default), 1 (Lvl 5), 2 (Lvl 10), 3 (Lvl 20), 4 (Lvl 35).
     *
     * @param Level  Robot's current level.
     * @return       Antenna stage index 0–4.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static int32 ComputeAntennaStage(int32 Level);

    // =========================================================================
    // Layer 8: Armor Plating (Runs Survived)
    // =========================================================================

    /**
     * ComputeArmorStage
     * Stage thresholds: 0 (bare), 1 (10+ runs survived), 2 (25+ runs survived).
     *
     * @param RunsSurvived  Robot's accumulated runs_survived count.
     * @return              Armor stage index 0–2.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static int32 ComputeArmorStage(int32 RunsSurvived);

    // =========================================================================
    // Layer 9: Aura / Glow (Mastery)
    // =========================================================================

    /**
     * ComputeAuraStage
     * Stage thresholds:
     *   0 — No spec (role == None)
     *   1 — Any Tier 2 spec chosen
     *   2 — Any Tier 3 spec chosen
     *   3 — Mastery title attained (mastery_title != None)
     *
     * @param Role    Robot's ERobotRole.
     * @param Tier2   Robot's ETier2Spec.
     * @param Tier3   Robot's ETier3Spec.
     * @param Mastery Robot's EMasteryTitle.
     * @return        Aura stage index 0–3.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static int32 ComputeAuraStage(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3, EMasteryTitle Mastery);

    // =========================================================================
    // Layers 10–12: Element-Specific Scars
    // =========================================================================

    /**
     * ComputeFallScarIntensity
     * fall_scar_intensity = Clamp(fall_encounters / 30.0, 0, 1)
     * Gouges and dents distinct from crush-based crack_intensity.
     *
     * @param FallEncounters  Robot's accumulated fall_encounters count.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeFallScarIntensity(int32 FallEncounters);

    /**
     * ComputeIceResidue
     * ice_residue = Clamp(ice_encounters / 25.0, 0, 1)
     * Frost crystals and frozen patches. Saturates at 25 encounters.
     *
     * @param IceEncounters  Robot's accumulated ice_encounters count.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeIceResidue(int32 IceEncounters);

    /**
     * ComputeElectricalBurn
     * electrical_burn = Clamp(emp_encounters / 25.0, 0, 1)
     * Scorch traces and conductive burn patterns. Saturates at 25 encounters.
     *
     * @param EMPEncounters  Robot's accumulated emp_encounters count.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float ComputeElectricalBurn(int32 EMPEncounters);

    // =========================================================================
    // Utility / Summary
    // =========================================================================

    /**
     * GetEvolutionSummary
     * Produces a human-readable description of a robot's visual state for UI display.
     * Example: "Battle-scarred veteran with burn marks and frost residue"
     *
     * @param State  The pre-computed visual evolution state.
     * @return       Descriptive FString, never empty (worst case: "Factory-fresh chassis").
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static FString GetEvolutionSummary(const FMXVisualEvolutionState& State);

    /**
     * GetWearLevelAsFloat
     * Converts EWearLevel enum to a float [0.0–4.0] for use as a material parameter.
     * Fresh=0, Used=1, Worn=2, BattleScarred=3, Ancient=4.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float GetWearLevelAsFloat(EWearLevel WearLevel);

    /**
     * GetCalendarAgeDays
     * Converts a robot's date_of_birth to elapsed real-world days from now.
     * Helper to avoid FTimespan arithmetic in calling code.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Calculator")
    static float GetCalendarAgeDays(const FDateTime& DateOfBirth);

private:

    // -------------------------------------------------------------------------
    // Saturation constants — centralised for easy tuning.
    // -------------------------------------------------------------------------

    static constexpr float BURN_SATURATION     = 50.0f;   // fire encounters to reach intensity 1.0
    static constexpr float CRACK_SATURATION    = 40.0f;   // crush+fall encounters for full cracks
    static constexpr float WELD_SATURATION     = 60.0f;   // near-misses for full weld coverage
    static constexpr float PATINA_SATURATION   = 30.0f;   // calendar days for full patina
    static constexpr float PATINA_AGE_CONTRIB  = 0.3f;    // max additional patina from active_age
    static constexpr float ACTIVE_AGE_PATINA   = 36000.0f;// active seconds for max age contribution
    static constexpr float EYE_LEVEL_DIVISOR   = 200.0f;  // levels for eye brightness to reach 1.0
    static constexpr float SOMBER_SATURATION   = 50.0f;   // deaths witnessed for full somber
    static constexpr float FALL_SCAR_SAT       = 30.0f;   // fall encounters for full fall scars
    static constexpr float ICE_SATURATION      = 25.0f;   // ice encounters for full frost
    static constexpr float EMP_SATURATION      = 25.0f;   // emp encounters for full elec burn
    static constexpr float ANCIENT_AGE_SECONDS = 36000.0f;// 10 hours marks "Ancient" age path
};
