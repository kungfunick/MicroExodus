// MXTypes.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: ALL agents
// Change Log:
//   v1.0 â€” Initial definition of all core enums

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.generated.h"

// ---------------------------------------------------------------------------
// Hazard / Damage Classification
// ---------------------------------------------------------------------------

/** The elemental category of a hazard source. Used by DEMS for sentence construction and visual mark selection. */
UENUM(BlueprintType)
enum class EHazardElement : uint8
{
    Fire        UMETA(DisplayName = "Fire"),
    Mechanical  UMETA(DisplayName = "Mechanical"),
    Gravity     UMETA(DisplayName = "Gravity"),
    Ice         UMETA(DisplayName = "Ice"),
    Electrical  UMETA(DisplayName = "Electrical"),
    Comedy      UMETA(DisplayName = "Comedy"),
    Sacrifice   UMETA(DisplayName = "Sacrifice"),
};

/** The specific type of damage inflicted. Drives visual evolution marks and DEMS verb selection. */
UENUM(BlueprintType)
enum class EDamageType : uint8
{
    Burn        UMETA(DisplayName = "Burn"),
    Crush       UMETA(DisplayName = "Crush"),
    Impale      UMETA(DisplayName = "Impale"),
    Fall        UMETA(DisplayName = "Fall"),
    Melt        UMETA(DisplayName = "Melt"),
    Slide       UMETA(DisplayName = "Slide"),
    Disable     UMETA(DisplayName = "Disable"),
    Bury        UMETA(DisplayName = "Bury"),
    Cut         UMETA(DisplayName = "Cut"),
    Bonk        UMETA(DisplayName = "Bonk"),
    Sacrifice   UMETA(DisplayName = "Sacrifice"),
};

// ---------------------------------------------------------------------------
// DEMS / Camera / Narrative
// ---------------------------------------------------------------------------

/** Narrative weight of an event. Controls DEMS template selection and camera response intensity. */
UENUM(BlueprintType)
enum class ESeverity : uint8
{
    Minor       UMETA(DisplayName = "Minor"),
    Major       UMETA(DisplayName = "Major"),
    Dramatic    UMETA(DisplayName = "Dramatic"),
};

/** Instructs the camera system how to respond to a DEMS event. */
UENUM(BlueprintType)
enum class ECameraBehavior : uint8
{
    Subtle      UMETA(DisplayName = "Subtle"),      // Small nudge, no interruption
    Dramatic    UMETA(DisplayName = "Dramatic"),    // Zoom + hold briefly
    Cinematic   UMETA(DisplayName = "Cinematic"),   // Full cinematic cut / orbit
    Epic        UMETA(DisplayName = "Epic"),        // Reserved for sacrifices and legendary events
    Suppress    UMETA(DisplayName = "Suppress"),    // Do not react (comedy events, spam prevention)
};

// ---------------------------------------------------------------------------
// Visual Evolution
// ---------------------------------------------------------------------------

/** The overall wear tier of a robot's chassis. Aggregates accumulated damage history. */
UENUM(BlueprintType)
enum class EWearLevel : uint8
{
    Fresh           UMETA(DisplayName = "Fresh"),
    Used            UMETA(DisplayName = "Used"),
    Worn            UMETA(DisplayName = "Worn"),
    BattleScarred   UMETA(DisplayName = "Battle Scarred"),
    Ancient         UMETA(DisplayName = "Ancient"),
};

// ---------------------------------------------------------------------------
// Specialization Tree
// ---------------------------------------------------------------------------

/** The broad role tier of a robot's specialization path. */
UENUM(BlueprintType)
enum class ERobotRole : uint8
{
    None        UMETA(DisplayName = "None"),
    Scout       UMETA(DisplayName = "Scout"),
    Guardian    UMETA(DisplayName = "Guardian"),
    Engineer    UMETA(DisplayName = "Engineer"),
};

UENUM(BlueprintType)
enum class ENameTheme : uint8
{
    Robot       UMETA(DisplayName = "Robot"),
    Wizard      UMETA(DisplayName = "Wizard"),
    Pirate      UMETA(DisplayName = "Pirate"),
    Samurai     UMETA(DisplayName = "Samurai"),
    SciFi       UMETA(DisplayName = "Sci-Fi"),
    Mythic      UMETA(DisplayName = "Mythic"),
    Custom      UMETA(DisplayName = "Custom"),
};

/** High-level locomotion state exposed by UMXAnimBridge. Drives AnimBP state machine. */
UENUM(BlueprintType)
enum class EMXLocomotionState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Starting    UMETA(DisplayName = "Starting"),
    Walking     UMETA(DisplayName = "Walking"),
    Stopping    UMETA(DisplayName = "Stopping"),
    Falling     UMETA(DisplayName = "Falling"),
    Traversing  UMETA(DisplayName = "Traversing"),
};

/** Traversal action types triggered by level obstacles. */
UENUM(BlueprintType)
enum class ETraversalType : uint8
{
    None    UMETA(DisplayName = "None"),
    Vault   UMETA(DisplayName = "Vault"),
    Climb   UMETA(DisplayName = "Climb"),
    Mantle  UMETA(DisplayName = "Mantle"),
    Slide   UMETA(DisplayName = "Slide"),
    Jump    UMETA(DisplayName = "Jump"),
};

/** Action montage types that C++ game logic can trigger on the AnimBP. */
UENUM(BlueprintType)
enum class EMXActionMontage : uint8
{
    None        UMETA(DisplayName = "None"),
    Rescue      UMETA(DisplayName = "Rescue"),
    Flinch      UMETA(DisplayName = "Flinch"),
    Death       UMETA(DisplayName = "Death"),
    Sacrifice   UMETA(DisplayName = "Sacrifice"),
    Celebrate   UMETA(DisplayName = "Celebrate"),
    Interact    UMETA(DisplayName = "Interact"),
    HatEquip    UMETA(DisplayName = "Hat Equip"),
};

/** Tier 2 specialization â€” chosen when the robot meets the role unlock threshold. */
UENUM(BlueprintType)
enum class ETier2Spec : uint8
{
    None        UMETA(DisplayName = "None"),
    Pathfinder  UMETA(DisplayName = "Pathfinder"),   // Scout branch
    Lookout     UMETA(DisplayName = "Lookout"),      // Scout branch
    Sentinel    UMETA(DisplayName = "Sentinel"),     // Guardian branch
    Bulwark     UMETA(DisplayName = "Bulwark"),      // Guardian branch
    Mechanic    UMETA(DisplayName = "Mechanic"),     // Engineer branch
    Architect   UMETA(DisplayName = "Architect"),    // Engineer branch
};

/** Tier 3 specialization â€” the final mastery-adjacent spec before the Mastery Title. */
UENUM(BlueprintType)
enum class ETier3Spec : uint8
{
    None            UMETA(DisplayName = "None"),
    Trailblazer     UMETA(DisplayName = "Trailblazer"),      // Pathfinder branch
    Ghost           UMETA(DisplayName = "Ghost"),             // Pathfinder branch
    Oracle          UMETA(DisplayName = "Oracle"),            // Lookout branch
    Beacon          UMETA(DisplayName = "Beacon"),            // Lookout branch
    Vanguard        UMETA(DisplayName = "Vanguard"),          // Sentinel branch
    ImmortalGuard   UMETA(DisplayName = "Immortal Guard"),    // Sentinel branch
    Anchor          UMETA(DisplayName = "Anchor"),            // Bulwark branch
    Wall            UMETA(DisplayName = "Wall"),              // Bulwark branch
    Savant          UMETA(DisplayName = "Savant"),            // Mechanic branch
    Catalyst        UMETA(DisplayName = "Catalyst"),          // Mechanic branch
    Demolitionist   UMETA(DisplayName = "Demolitionist"),     // Architect branch
    Foundation      UMETA(DisplayName = "Foundation"),        // Architect branch
};

/** Mastery Title â€” the legendary designation earned after completing a full Tier 3 specialization arc. */
UENUM(BlueprintType)
enum class EMasteryTitle : uint8
{
    None            UMETA(DisplayName = "None"),
    TheWayfinder    UMETA(DisplayName = "The Wayfinder"),
    ThePhantom      UMETA(DisplayName = "The Phantom"),
    TheSeer         UMETA(DisplayName = "The Seer"),
    TheSignal       UMETA(DisplayName = "The Signal"),
    TheShield       UMETA(DisplayName = "The Shield"),
    TheUndying      UMETA(DisplayName = "The Undying"),
    TheImmovable    UMETA(DisplayName = "The Immovable"),
    TheFortress     UMETA(DisplayName = "The Fortress"),
    TheLifegiver    UMETA(DisplayName = "The Lifegiver"),
    TheAmplifier    UMETA(DisplayName = "The Amplifier"),
    TheDestroyer    UMETA(DisplayName = "The Destroyer"),
    TheCornerstone  UMETA(DisplayName = "The Cornerstone"),
};

// ---------------------------------------------------------------------------
// Hat System
// ---------------------------------------------------------------------------

/** Rarity tier of a hat. Controls drop weight, visual flair, and DEMS mention likelihood. */
UENUM(BlueprintType)
enum class EHatRarity : uint8
{
    Common      UMETA(DisplayName = "Common"),
    Uncommon    UMETA(DisplayName = "Uncommon"),
    Rare        UMETA(DisplayName = "Rare"),
    Epic        UMETA(DisplayName = "Epic"),
    Legendary   UMETA(DisplayName = "Legendary"),
    Mythic      UMETA(DisplayName = "Mythic"),
};

// ---------------------------------------------------------------------------
// Event System
// ---------------------------------------------------------------------------

/** The category of a DEMS or life-log event. Determines which template pool is used. */
UENUM(BlueprintType)
enum class EEventType : uint8
{
    Death               UMETA(DisplayName = "Death"),
    Rescue              UMETA(DisplayName = "Rescue"),
    Sacrifice           UMETA(DisplayName = "Sacrifice"),
    NearMiss            UMETA(DisplayName = "Near Miss"),
    LevelComplete       UMETA(DisplayName = "Level Complete"),
    RunComplete         UMETA(DisplayName = "Run Complete"),
    RunFailed           UMETA(DisplayName = "Run Failed"),
    LevelUp             UMETA(DisplayName = "Level Up"),
    SpecChosen          UMETA(DisplayName = "Spec Chosen"),
    HatEquipped         UMETA(DisplayName = "Hat Equipped"),
    HatLost             UMETA(DisplayName = "Hat Lost"),
    ComboDiscovered     UMETA(DisplayName = "Combo Discovered"),
    HatUnlocked         UMETA(DisplayName = "Hat Unlocked"),
    PuzzleParticipation UMETA(DisplayName = "Puzzle Participation"),
    RescueWitnessed     UMETA(DisplayName = "Rescue Witnessed"),
    DeathWitnessed      UMETA(DisplayName = "Death Witnessed"),
    SacrificeWitnessed  UMETA(DisplayName = "Sacrifice Witnessed"),
};

// ---------------------------------------------------------------------------
// Roguelike Structure
// ---------------------------------------------------------------------------

/** The difficulty tier of a run. Higher tiers multiply XP and score rewards but increase lethality. */
UENUM(BlueprintType)
enum class ETier : uint8
{
    Standard    UMETA(DisplayName = "Standard"),
    Hardened    UMETA(DisplayName = "Hardened"),
    Brutal      UMETA(DisplayName = "Brutal"),
    Nightmare   UMETA(DisplayName = "Nightmare"),
    Extinction  UMETA(DisplayName = "Extinction"),
    Legendary   UMETA(DisplayName = "Legendary"),
};

/** Whether a run ended in success (all required levels cleared) or failure (team wipe or abort). */
UENUM(BlueprintType)
enum class ERunOutcome : uint8
{
    Success UMETA(DisplayName = "Success"),
    Failure UMETA(DisplayName = "Failure"),
};

// ---------------------------------------------------------------------------
// Run Report
// ---------------------------------------------------------------------------

/** Award categories distributed at the end of a run report. One robot per category. */
UENUM(BlueprintType)
enum class EAwardCategory : uint8
{
    MVP                 UMETA(DisplayName = "MVP"),
    NearMissChampion    UMETA(DisplayName = "Near Miss Champion"),
    RescueHero          UMETA(DisplayName = "Rescue Hero"),
    IronHide            UMETA(DisplayName = "Iron Hide"),
    Tourist             UMETA(DisplayName = "Tourist"),
    WorstLuck           UMETA(DisplayName = "Worst Luck"),
    FashionForward      UMETA(DisplayName = "Fashion Forward"),
    TheSacrifice        UMETA(DisplayName = "The Sacrifice"),
    NewcomerOfTheRun    UMETA(DisplayName = "Newcomer of the Run"),
    VeteranSurvivor     UMETA(DisplayName = "Veteran Survivor"),
    BestDressed         UMETA(DisplayName = "Best Dressed"),
    WorstDressed        UMETA(DisplayName = "Worst Dressed"),
    GuardianAngel       UMETA(DisplayName = "Guardian Angel"),
    TheMechanic         UMETA(DisplayName = "The Mechanic"),
    HatCasualty         UMETA(DisplayName = "Hat Casualty"),
    CowardOfTheRun      UMETA(DisplayName = "Coward of the Run"),
};

// ---------------------------------------------------------------------------
// Cosmetics
// ---------------------------------------------------------------------------

/** Chassis paint scheme options. Applied to the robot material instance at runtime. */
UENUM(BlueprintType)
enum class EPaintJob : uint8
{
    None            UMETA(DisplayName = "None"),
    MatteBlack      UMETA(DisplayName = "Matte Black"),
    Chrome          UMETA(DisplayName = "Chrome"),
    Camo            UMETA(DisplayName = "Camo"),
    NeonGlow        UMETA(DisplayName = "Neon Glow"),
    Wooden          UMETA(DisplayName = "Wooden"),
    Marble          UMETA(DisplayName = "Marble"),
    PixelArt        UMETA(DisplayName = "Pixel Art"),
    Holographic     UMETA(DisplayName = "Holographic"),
    Invisible       UMETA(DisplayName = "Invisible"),
    PureGold        UMETA(DisplayName = "Pure Gold"),
};
