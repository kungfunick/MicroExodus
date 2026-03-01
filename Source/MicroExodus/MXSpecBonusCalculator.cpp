// MXSpecBonusCalculator.cpp — Specialization Module Version 1.0
// Created: 2026-02-22
// Agent 5: Specialization
// Change Log:
//   v1.0 — Initial implementation

#include "MXSpecBonusCalculator.h"

// ---------------------------------------------------------------------------
// CalculateBonus
//
// Stacking logic per GDD:
//   Speed:       Role baseline + Tier2 adjustment = total (not additive beyond that).
//                Scout = +0.15 total. Pathfinder supersedes Scout's raw value to reach +0.20.
//                Guardian base = -0.05. Tier2/3 do not further modify speed in Guardian branch.
//   Weight:      Highest-tier value wins (Foundation ×5 supersedes Architect ×3, etc.)
//   XP:          Additive. Engineer base +0.50, Mechanic nearby +0.25 additional, Catalyst ×2.0.
//   Lethal hits: Highest tier value (Guardian=1, Sentinel=2, ImmortalGuard=3).
//   Revives:     Highest tier value (Mechanic=1, Savant=3).
//   Radius/Dodge: Fixed per tier.
// ---------------------------------------------------------------------------

FMXSpecBonus UMXSpecBonusCalculator::CalculateBonus(ERobotRole    Role,
                                                     ETier2Spec    Tier2,
                                                     ETier3Spec    Tier3,
                                                     EMasteryTitle Mastery)
{
    FMXSpecBonus Bonus;
    // Defaults: speed=1.0, weight=1.0, xp=1.0, lethal=0, revives=0, radius=0, dodge=0

    // ---- Speed modifier ----
    Bonus.speed_modifier = 1.0f + GetRoleSpeedModifier(Role) + GetTier2SpeedModifier(Tier2);

    // ---- Weight modifier ----
    Bonus.weight_modifier = GetWeightModifier(Role, Tier2, Tier3);

    // ---- XP modifier ----
    Bonus.xp_modifier = GetXPModifier(Role, Tier2, Tier3);

    // ---- Lethal hits ----
    Bonus.lethal_hits_per_run = GetLethalHitsPerRun(Role, Tier2, Tier3);

    // ---- Revives ----
    Bonus.revives_per_run = GetRevivesPerRun(Tier2, Tier3);

    // ---- Hazard reveal radius ----
    Bonus.hazard_reveal_radius = GetHazardRevealRadius(Tier2);

    // ---- Auto-dodge chance ----
    Bonus.auto_dodge_chance = GetAutoDodgeChance(Tier2);

    return Bonus;
}

// ---------------------------------------------------------------------------
// GetVisualIndicator
// ---------------------------------------------------------------------------

FString UMXSpecBonusCalculator::GetVisualIndicator(ERobotRole    Role,
                                                    ETier2Spec    Tier2,
                                                    ETier3Spec    Tier3,
                                                    EMasteryTitle Mastery)
{
    TArray<FString> Tags;

    // Role-level visuals
    switch (Role)
    {
    case ERobotRole::Scout:
        Tags.Add(TEXT("spec_scout_antenna_green, spec_scout_lean_posture"));
        break;
    case ERobotRole::Guardian:
        Tags.Add(TEXT("spec_guardian_thick_plating, spec_guardian_wide_stance"));
        break;
    case ERobotRole::Engineer:
        Tags.Add(TEXT("spec_engineer_wrench_antenna, spec_engineer_tool_belt"));
        break;
    default:
        break;
    }

    // Tier 2 visuals
    switch (Tier2)
    {
    case ETier2Spec::Pathfinder:  Tags.Add(TEXT("spec_pathfinder_room_reveal_pulse"));  break;
    case ETier2Spec::Lookout:     Tags.Add(TEXT("spec_lookout_warning_ring_8m"));        break;
    case ETier2Spec::Sentinel:    Tags.Add(TEXT("spec_sentinel_aura_damage_reduction")); break;
    case ETier2Spec::Bulwark:     Tags.Add(TEXT("spec_bulwark_conveyor_immunity_glow")); break;
    case ETier2Spec::Mechanic:    Tags.Add(TEXT("spec_mechanic_xp_glow_nearby"));       break;
    case ETier2Spec::Architect:   Tags.Add(TEXT("spec_architect_weight_plate_glow"));   break;
    default: break;
    }

    // Tier 3 visuals
    switch (Tier3)
    {
    case ETier3Spec::Trailblazer:   Tags.Add(TEXT("spec_trailblazer_speed_trail"));         break;
    case ETier3Spec::Ghost:         Tags.Add(TEXT("spec_ghost_phase_shimmer"));              break;
    case ETier3Spec::Oracle:        Tags.Add(TEXT("spec_oracle_hazard_prediction_hud"));     break;
    case ETier3Spec::Beacon:        Tags.Add(TEXT("spec_beacon_xp_boost_ring"));             break;
    case ETier3Spec::Vanguard:      Tags.Add(TEXT("spec_vanguard_front_shield_projection")); break;
    case ETier3Spec::ImmortalGuard: Tags.Add(TEXT("spec_immortal_guard_triple_shield"));     break;
    case ETier3Spec::Anchor:        Tags.Add(TEXT("spec_anchor_no_push_aura"));              break;
    case ETier3Spec::Wall:          Tags.Add(TEXT("spec_wall_hazard_block_cone"));           break;
    case ETier3Spec::Savant:        Tags.Add(TEXT("spec_savant_revive_pulse"));              break;
    case ETier3Spec::Catalyst:      Tags.Add(TEXT("spec_catalyst_xp_double_aura"));         break;
    case ETier3Spec::Demolitionist: Tags.Add(TEXT("spec_demolitionist_crack_hands"));        break;
    case ETier3Spec::Foundation:    Tags.Add(TEXT("spec_foundation_weight_x5_glow"));       break;
    default: break;
    }

    // Mastery visuals
    switch (Mastery)
    {
    case EMasteryTitle::TheWayfinder:   Tags.Add(TEXT("mastery_wayfinder_footprint_trail"));    break;
    case EMasteryTitle::ThePhantom:     Tags.Add(TEXT("mastery_phantom_translucent_shimmer"));   break;
    case EMasteryTitle::TheSeer:        Tags.Add(TEXT("mastery_seer_third_eye_glow"));           break;
    case EMasteryTitle::TheSignal:      Tags.Add(TEXT("mastery_signal_pulse_ring"));             break;
    case EMasteryTitle::TheShield:      Tags.Add(TEXT("mastery_shield_forward_projection"));     break;
    case EMasteryTitle::TheUndying:     Tags.Add(TEXT("mastery_undying_flicker_aura"));          break;
    case EMasteryTitle::TheImmovable:   Tags.Add(TEXT("mastery_immovable_stone_texture"));       break;
    case EMasteryTitle::TheFortress:    Tags.Add(TEXT("mastery_fortress_orbiting_beams"));       break;
    case EMasteryTitle::TheLifegiver:   Tags.Add(TEXT("mastery_lifegiver_heal_particles"));      break;
    case EMasteryTitle::TheAmplifier:   Tags.Add(TEXT("mastery_amplifier_xp_sparkle_aura"));    break;
    case EMasteryTitle::TheDestroyer:   Tags.Add(TEXT("mastery_destroyer_crack_aura_hands"));    break;
    case EMasteryTitle::TheCornerstone: Tags.Add(TEXT("mastery_cornerstone_orbiting_beams"));    break;
    default: break;
    }

    return FString::Join(Tags, TEXT(", "));
}

// ---------------------------------------------------------------------------
// Speed Modifiers
// ---------------------------------------------------------------------------

float UMXSpecBonusCalculator::GetRoleSpeedModifier(ERobotRole Role)
{
    switch (Role)
    {
    case ERobotRole::Scout:    return  0.15f;   // +15%
    case ERobotRole::Guardian: return -0.05f;   // -5%
    case ERobotRole::Engineer: return  0.0f;    // No inherent speed change
    default:                   return  0.0f;
    }
}

float UMXSpecBonusCalculator::GetTier2SpeedModifier(ETier2Spec Tier2)
{
    // Pathfinder's total is +20%. Scout base is +15%, so Pathfinder adds +0.05 more.
    // All other Tier 2 specs do not modify speed.
    switch (Tier2)
    {
    case ETier2Spec::Pathfinder: return 0.05f;  // Brings Scout total to +20%
    default:                     return 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Weight Modifier (highest tier wins)
// ---------------------------------------------------------------------------

float UMXSpecBonusCalculator::GetWeightModifier(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3)
{
    // Tier 3 checks first (highest priority)
    if (Tier3 == ETier3Spec::Foundation)   return 5.0f;

    // Tier 2 checks
    if (Tier2 == ETier2Spec::Architect)    return 3.0f;
    if (Tier2 == ETier2Spec::Bulwark)      return 2.0f;

    // Role checks
    if (Role == ERobotRole::Engineer)      return 1.5f;

    return 1.0f;  // No weight bonus
}

// ---------------------------------------------------------------------------
// XP Modifier
// ---------------------------------------------------------------------------

float UMXSpecBonusCalculator::GetXPModifier(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3)
{
    float Modifier = 1.0f;

    // Engineer: +50% puzzle XP — applied as a general modifier for this calculator
    if (Role == ERobotRole::Engineer)
    {
        Modifier += 0.50f;
    }

    // Mechanic: +25% XP for nearby robots (this robot's personal XP is not doubled;
    // nearby robots benefit — but we store it here for the XP distributor to query)
    if (Tier2 == ETier2Spec::Mechanic)
    {
        Modifier += 0.25f;
    }

    // Catalyst: Doubles all nearby XP gains (×2.0 on top of Mechanic's bonus)
    if (Tier3 == ETier3Spec::Catalyst)
    {
        Modifier *= 2.0f;
    }

    return Modifier;
}

// ---------------------------------------------------------------------------
// Lethal Hits (highest tier wins)
// ---------------------------------------------------------------------------

int32 UMXSpecBonusCalculator::GetLethalHitsPerRun(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3)
{
    if (Tier3 == ETier3Spec::ImmortalGuard) return 3;
    if (Tier2 == ETier2Spec::Sentinel)      return 2;
    if (Role  == ERobotRole::Guardian)      return 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Revives (highest tier wins)
// ---------------------------------------------------------------------------

int32 UMXSpecBonusCalculator::GetRevivesPerRun(ETier2Spec Tier2, ETier3Spec Tier3)
{
    if (Tier3 == ETier3Spec::Savant)    return 3;
    if (Tier2 == ETier2Spec::Mechanic)  return 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Hazard Reveal Radius
// ---------------------------------------------------------------------------

float UMXSpecBonusCalculator::GetHazardRevealRadius(ETier2Spec Tier2)
{
    if (Tier2 == ETier2Spec::Lookout) return 8.0f;
    return 0.0f;
}

// ---------------------------------------------------------------------------
// Auto-Dodge Chance
// ---------------------------------------------------------------------------

float UMXSpecBonusCalculator::GetAutoDodgeChance(ETier2Spec Tier2)
{
    if (Tier2 == ETier2Spec::Lookout) return 0.10f;
    return 0.0f;
}
