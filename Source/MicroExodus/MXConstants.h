// MXConstants.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: All agents
// Change Log:
//   v1.0 â€” Initial definition of all shared constants

#pragma once

#include "CoreMinimal.h"

/**
 * MXConstants
 * Compile-time constants shared across all Micro Exodus modules.
 * Use these values instead of magic numbers. If a value needs to change, change it here.
 */
namespace MXConstants
{
    // -------------------------------------------------------------------------
    // Roster / Hat Caps
    // -------------------------------------------------------------------------

    /** Maximum number of robots allowed on the player's roster at any time. */
    constexpr int32 MAX_ROBOTS = 100;

    /** Total number of unique hat types in the game (IDs 0â€“99). */
    constexpr int32 MAX_HATS = 100;

    /** Maximum number of copies of a single hat type that can be held in the collection. */
    constexpr int32 MAX_HAT_STACK = 100;

    // -------------------------------------------------------------------------
    // Level / Tier Structure
    // -------------------------------------------------------------------------

    /** Total number of levels per run sequence. */
    constexpr int32 MAX_LEVELS = 20;

    /** Number of difficulty tiers available. Matches the ETier enum count. */
    constexpr int32 MAX_TIERS = 6;

    // -------------------------------------------------------------------------
    // Gameplay Timings
    // -------------------------------------------------------------------------

    /** Seconds a player must hold the rescue input to complete a rescue. */
    constexpr float RESCUE_HOLD_TIME = 1.0f;

    /**
     * Radius in meters within which a hazard triggers a near-miss event for a robot.
     * Robots entering this radius and escaping alive are logged as near-misses.
     */
    constexpr float NEAR_MISS_RADIUS = 2.0f;

    /**
     * Radius in meters within which a bystander robot witnesses a rescue, death, or sacrifice.
     * Robots inside this radius when an event fires have their social stats incremented.
     */
    constexpr float RESCUE_WITNESS_RADIUS = 5.0f;

    /**
     * Seconds a hat physics mesh lingers in the world after a robot dies.
     * After this time the hat is removed from the level geometry.
     */
    constexpr float HAT_DEATH_LINGER_TIME = 3.0f;

    // -------------------------------------------------------------------------
    // XP Progression Thresholds
    // -------------------------------------------------------------------------

    /**
     * Cumulative total_xp required to reach each level (index = level - 1).
     * Level 1 starts at 0 XP. Level 12 is the max tracked here (Legendary mastery).
     * The Roguelike module interpolates or extends for levels beyond this array.
     *
     * Index:  0       1      2      3       4       5       6        7        8        9        10       11
     * Level:  1â†’2    2â†’3   3â†’4    4â†’5     5â†’6     6â†’7     7â†’8      8â†’9      9â†’10    10â†’11   11â†’12   12â†’13+
     */
    constexpr int32 XP_THRESHOLDS[] = {
        0,          // Level 1 baseline
        1500,       // â†’ Level 2
        5000,       // â†’ Level 3
        12000,      // â†’ Level 4
        25000,      // â†’ Level 5
        45000,      // â†’ Level 6
        75000,      // â†’ Level 7
        120000,     // â†’ Level 8
        150000,     // â†’ Level 9
        300000,     // â†’ Level 10
        750000,     // â†’ Level 11
        2000000     // â†’ Level 12+
    };

    // -------------------------------------------------------------------------
    // Tier Score Multipliers
    // -------------------------------------------------------------------------

    /**
     * Score and XP multipliers applied per difficulty tier.
     * Index corresponds to ETier cast to int (Standard=0, Hardened=1, ... Legendary=5).
     */
    constexpr float TIER_MULTIPLIERS[] = {
        1.0f,   // Standard
        1.5f,   // Hardened
        2.0f,   // Brutal
        3.0f,   // Nightmare
        5.0f,   // Extinction
        10.0f   // Legendary
    };

    // -------------------------------------------------------------------------
    // Run Report Limits
    // -------------------------------------------------------------------------

    /** Maximum number of highlights shown in the run report Highlight Reel section. */
    constexpr int32 REPORT_MAX_HIGHLIGHTS = 10;

    /** Maximum number of awards distributed in the run report Awards Ceremony section. */
    constexpr int32 REPORT_MAX_AWARDS = 16;

    // -------------------------------------------------------------------------
    // DEMS
    // -------------------------------------------------------------------------

    /**
     * Size of the DEMS deduplication ring buffer.
     * Recent message templates are tracked to avoid repeating the same sentence structure.
     */
    constexpr int32 DEDUP_BUFFER_SIZE = 20;
}
