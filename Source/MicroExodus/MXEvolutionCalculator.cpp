// MXEvolutionCalculator.cpp — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Change Log:
//   v1.0 — Initial implementation. All functions are pure, side-effect-free static methods.

#include "MXEvolutionCalculator.h"
#include "Math/UnrealMathUtility.h"

// =========================================================================
// One-Shot Full State
// =========================================================================

FMXVisualEvolutionState UMXEvolutionCalculator::ComputeFullState(const FMXRobotProfile& Robot)
{
    FMXVisualEvolutionState State;

    // Preserve the decal_seed — it is set once at birth and never recomputed.
    State.decal_seed = Robot.visual_evolution_state.decal_seed;

    // Layer 1: Base Wear
    State.wear_level = ComputeWearLevel(Robot);

    // Layer 2: Burn Marks
    State.burn_intensity = ComputeBurnIntensity(Robot.fire_encounters);

    // Layer 3: Impact Cracks
    State.crack_intensity = ComputeCrackIntensity(Robot.crush_encounters, Robot.fall_encounters);

    // Layer 4: Weld Repairs
    State.weld_coverage = ComputeWeldCoverage(Robot.near_misses);

    // Layer 5: Rust / Patina
    const float CalendarAgeDays = GetCalendarAgeDays(Robot.date_of_birth);
    State.patina_intensity = ComputePatinaIntensity(CalendarAgeDays, Robot.active_age_seconds);

    // Layer 6: Eye Evolution
    State.eye_brightness = ComputeEyeBrightness(Robot.level);
    State.eye_somber     = ComputeEyeSomber(Robot.deaths_witnessed);

    // Layer 7: Antenna Growth
    State.antenna_stage = ComputeAntennaStage(Robot.level);

    // Layer 8: Armor Plating
    State.armor_stage = ComputeArmorStage(Robot.runs_survived);

    // Layer 9: Aura / Glow
    State.aura_stage = ComputeAuraStage(Robot.role, Robot.tier2_spec, Robot.tier3_spec, Robot.mastery_title);

    // Layer 10: Fall Scars
    State.fall_scar_intensity = ComputeFallScarIntensity(Robot.fall_encounters);

    // Layer 11: Ice Residue
    State.ice_residue = ComputeIceResidue(Robot.ice_encounters);

    // Layer 12: Electrical Burns
    State.electrical_burn = ComputeElectricalBurn(Robot.emp_encounters);

    return State;
}

// =========================================================================
// Layer 1: Base Wear
// =========================================================================

EWearLevel UMXEvolutionCalculator::ComputeWearLevel(const FMXRobotProfile& Robot)
{
    // "Ancient" via active age is checked first — it's the unconditional age gate.
    if (Robot.active_age_seconds >= ANCIENT_AGE_SECONDS)
    {
        return EWearLevel::Ancient;
    }

    const int32 Total = GetTotalEncounters(Robot);

    if (Total >= 100) return EWearLevel::Ancient;
    if (Total >= 60)  return EWearLevel::BattleScarred;
    if (Total >= 30)  return EWearLevel::Worn;
    if (Total >= 10)  return EWearLevel::Used;
    return EWearLevel::Fresh;
}

int32 UMXEvolutionCalculator::GetTotalEncounters(const FMXRobotProfile& Robot)
{
    return Robot.near_misses
         + Robot.fire_encounters
         + Robot.crush_encounters
         + Robot.fall_encounters
         + Robot.emp_encounters
         + Robot.ice_encounters;
}

// =========================================================================
// Layer 2: Burn Marks
// =========================================================================

float UMXEvolutionCalculator::ComputeBurnIntensity(int32 FireEncounters)
{
    return FMath::Clamp(static_cast<float>(FireEncounters) / BURN_SATURATION, 0.0f, 1.0f);
}

// =========================================================================
// Layer 3: Impact Cracks
// =========================================================================

float UMXEvolutionCalculator::ComputeCrackIntensity(int32 CrushEncounters, int32 FallEncounters)
{
    const float Combined = static_cast<float>(CrushEncounters + FallEncounters);
    return FMath::Clamp(Combined / CRACK_SATURATION, 0.0f, 1.0f);
}

// =========================================================================
// Layer 4: Weld Repairs
// =========================================================================

float UMXEvolutionCalculator::ComputeWeldCoverage(int32 NearMisses)
{
    return FMath::Clamp(static_cast<float>(NearMisses) / WELD_SATURATION, 0.0f, 1.0f);
}

// =========================================================================
// Layer 5: Rust / Patina
// =========================================================================

float UMXEvolutionCalculator::ComputePatinaIntensity(float CalendarAgeDays, float ActiveAgeSeconds)
{
    // Primary driver: real-world calendar age.
    const float CalendarContrib = FMath::Clamp(CalendarAgeDays / PATINA_SATURATION, 0.0f, 1.0f);

    // Secondary driver: active game time. Contributes up to PATINA_AGE_CONTRIB additional intensity
    // so a robot that has been played a lot ages faster even if calendar days are few.
    const float ActiveContrib   = FMath::Clamp(ActiveAgeSeconds / ACTIVE_AGE_PATINA, 0.0f, 1.0f)
                                  * PATINA_AGE_CONTRIB;

    return FMath::Clamp(CalendarContrib + ActiveContrib, 0.0f, 1.0f);
}

// =========================================================================
// Layer 6: Eye Evolution
// =========================================================================

float UMXEvolutionCalculator::ComputeEyeBrightness(int32 Level)
{
    // Ranges [0.5, 1.0]. Never below 0.5 — even a fresh robot has glowing eyes.
    return FMath::Clamp(0.5f + (static_cast<float>(Level) / EYE_LEVEL_DIVISOR), 0.5f, 1.0f);
}

float UMXEvolutionCalculator::ComputeEyeSomber(int32 DeathsWitnessed)
{
    return FMath::Clamp(static_cast<float>(DeathsWitnessed) / SOMBER_SATURATION, 0.0f, 1.0f);
}

// =========================================================================
// Layer 7: Antenna Growth
// =========================================================================

int32 UMXEvolutionCalculator::ComputeAntennaStage(int32 Level)
{
    // Breakpoints: 5, 10, 20, 35
    if (Level >= 35) return 4;
    if (Level >= 20) return 3;
    if (Level >= 10) return 2;
    if (Level >= 5)  return 1;
    return 0;
}

// =========================================================================
// Layer 8: Armor Plating
// =========================================================================

int32 UMXEvolutionCalculator::ComputeArmorStage(int32 RunsSurvived)
{
    if (RunsSurvived >= 25) return 2;
    if (RunsSurvived >= 10) return 1;
    return 0;
}

// =========================================================================
// Layer 9: Aura / Glow
// =========================================================================

int32 UMXEvolutionCalculator::ComputeAuraStage(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3, EMasteryTitle Mastery)
{
    if (Mastery != EMasteryTitle::None) return 3;
    if (Tier3   != ETier3Spec::None)   return 2;
    if (Tier2   != ETier2Spec::None)   return 1;
    return 0;
    // Role alone (without Tier2) yields stage 0 — role commit is just the start of the path.
    (void)Role; // intentionally unused; kept in signature for Blueprint clarity
}

// =========================================================================
// Layers 10–12: Element-Specific Scars
// =========================================================================

float UMXEvolutionCalculator::ComputeFallScarIntensity(int32 FallEncounters)
{
    return FMath::Clamp(static_cast<float>(FallEncounters) / FALL_SCAR_SAT, 0.0f, 1.0f);
}

float UMXEvolutionCalculator::ComputeIceResidue(int32 IceEncounters)
{
    return FMath::Clamp(static_cast<float>(IceEncounters) / ICE_SATURATION, 0.0f, 1.0f);
}

float UMXEvolutionCalculator::ComputeElectricalBurn(int32 EMPEncounters)
{
    return FMath::Clamp(static_cast<float>(EMPEncounters) / EMP_SATURATION, 0.0f, 1.0f);
}

// =========================================================================
// Utility / Summary
// =========================================================================

float UMXEvolutionCalculator::GetWearLevelAsFloat(EWearLevel WearLevel)
{
    return static_cast<float>(static_cast<uint8>(WearLevel));
}

float UMXEvolutionCalculator::GetCalendarAgeDays(const FDateTime& DateOfBirth)
{
    const FTimespan Elapsed = FDateTime::Now() - DateOfBirth;
    return static_cast<float>(Elapsed.GetTotalSeconds()) / 86400.0f;
}

FString UMXEvolutionCalculator::GetEvolutionSummary(const FMXVisualEvolutionState& State)
{
    // Build a descriptive sentence from the most prominent visual traits.
    TArray<FString> Traits;

    // --- Wear tier prefix ---
    FString WearPrefix;
    switch (State.wear_level)
    {
        case EWearLevel::Fresh:        WearPrefix = TEXT("Factory-fresh chassis");       break;
        case EWearLevel::Used:         WearPrefix = TEXT("Lightly worn chassis");        break;
        case EWearLevel::Worn:         WearPrefix = TEXT("Weathered chassis");           break;
        case EWearLevel::BattleScarred:WearPrefix = TEXT("Battle-scarred veteran");      break;
        case EWearLevel::Ancient:      WearPrefix = TEXT("Ancient, battle-hardened hull");break;
        default:                       WearPrefix = TEXT("Unknown chassis condition");   break;
    }

    // --- Collect prominent traits (threshold: > 0.3 intensity) ---
    if (State.burn_intensity      > 0.3f) Traits.Add(TEXT("burn marks"));
    if (State.crack_intensity     > 0.3f) Traits.Add(TEXT("impact cracks"));
    if (State.weld_coverage       > 0.3f) Traits.Add(TEXT("weld repairs"));
    if (State.patina_intensity    > 0.3f) Traits.Add(TEXT("rust patina"));
    if (State.fall_scar_intensity > 0.3f) Traits.Add(TEXT("fall gouges"));
    if (State.ice_residue         > 0.3f) Traits.Add(TEXT("frost residue"));
    if (State.electrical_burn     > 0.3f) Traits.Add(TEXT("electrical burn traces"));
    if (State.eye_somber          > 0.3f) Traits.Add(TEXT("haunted eyes"));

    // --- Staged unlocks ---
    if (State.antenna_stage >= 4) Traits.Add(TEXT("legendary antenna array"));
    else if (State.antenna_stage >= 3) Traits.Add(TEXT("elaborate antenna"));
    if (State.armor_stage >= 2)  Traits.Add(TEXT("full armor plating"));
    else if (State.armor_stage == 1) Traits.Add(TEXT("partial armor plating"));
    if (State.aura_stage >= 3)   Traits.Add(TEXT("mastery aura"));
    else if (State.aura_stage >= 1) Traits.Add(TEXT("specialist glow"));

    if (Traits.Num() == 0)
    {
        return WearPrefix + TEXT(". No visible damage yet.");
    }

    // Build "x, y, and z" natural-language list.
    FString TraitList;
    for (int32 i = 0; i < Traits.Num(); ++i)
    {
        if (i == 0)
        {
            TraitList = Traits[i];
        }
        else if (i == Traits.Num() - 1)
        {
            TraitList += (Traits.Num() > 2) ? TEXT(", and ") : TEXT(" and ");
            TraitList += Traits[i];
        }
        else
        {
            TraitList += TEXT(", ") + Traits[i];
        }
    }

    return FString::Printf(TEXT("%s with %s."), *WearPrefix, *TraitList);
}
