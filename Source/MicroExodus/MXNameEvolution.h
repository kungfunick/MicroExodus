// MXNameEvolution.h — Identity Module: Theme-Aware Dynamic Name Evolution
// Created: 2026-03-09
//
// Builds compound robot names from lifecycle milestones, themed per-robot.
//
// Name structure:  [FirstName] [Surname] [Prefix Title]
//   Robot theme:   "Bolt Sprocket The Fireproof"
//   Pirate theme:  "Barnacle Planksworth Captain Scourge"
//   Wizard theme:  "Ember Hexfire The Great Pyromancer"
//   Samurai theme: "Kumo Hayabusa Honourable Flame Dancer"
//
// Each robot's name_theme (ENameTheme) is stamped on its profile at birth,
// based on the run's selected theme. This class reads that field to pick
// the correct surname pool and title aliases from UMXNameThemeManager.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXTypes.h"
#include "MXRobotProfile.h"
#include "MXNameTheme.h"
#include "MXNameEvolution.generated.h"

// Forward declarations
class UMXNameThemeManager;

// ---------------------------------------------------------------------------
// FMXNameEvolutionConfig
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXNameEvolutionConfig
{
    GENERATED_BODY()

    /** Runs survived before earning a surname. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SurnameThreshold = 5;
};

// ---------------------------------------------------------------------------
// UMXNameEvolution
// ---------------------------------------------------------------------------

/**
 * UMXNameEvolution
 * Theme-aware name composer. Reads each robot's name_theme to select
 * the correct surname pool and title aliases from UMXNameThemeManager.
 *
 * Workflow:
 *   1. Robot born in a Pirate-themed run → profile.name_theme = Pirate
 *   2. NameGenerator uses Pirate first name pool → "Barnacle"
 *   3. After 5 runs survived, EvaluateNameEvolution rolls a Pirate surname
 *      → "Barnacle Planksworth"
 *   4. Robot earns "Firewalker" title → themed to "Cannoneer"
 *      → "Barnacle Planksworth Captain Cannoneer"
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXNameEvolution : public UObject
{
    GENERATED_BODY()

public:

    UMXNameEvolution();

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Supply the theme manager for pool lookups.
     * @param InThemeManager  The theme registry (owned by GameInstance).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names|Evolution")
    void Initialize(UMXNameThemeManager* InThemeManager);

    // -------------------------------------------------------------------------
    // Name Evolution
    // -------------------------------------------------------------------------

    /**
     * Evaluate whether a robot has hit a name evolution milestone.
     * If the robot qualifies for a surname and doesn't have one, rolls one
     * from the robot's own theme pool.
     *
     * @param Profile  The robot's profile (modified in-place if surname earned).
     * @return         True if the name evolved this call.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names|Evolution")
    bool EvaluateNameEvolution(UPARAM(ref) FMXRobotProfile& Profile);

    /**
     * Build the full compound display name for a robot.
     * Uses the robot's name_theme for surname pool and title aliases.
     *
     * @param Profile  The robot's profile.
     * @return         The composed display name string.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Evolution")
    FString GetDisplayName(const FMXRobotProfile& Profile) const;

    /** Name + Surname only, no title. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Evolution")
    FString GetShortName(const FMXRobotProfile& Profile) const;

    /** Title portion with themed prefix and alias. Empty if no title. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Evolution")
    FString GetFormattedTitle(const FMXRobotProfile& Profile) const;

    // -------------------------------------------------------------------------
    // Surname Generation
    // -------------------------------------------------------------------------

    /**
     * Roll a surname from the robot's theme pool for its role.
     * Deterministic from GUID.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Names|Evolution")
    FString RollSurname(const FGuid& RobotId, ERobotRole Role, ENameTheme Theme) const;

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Names|Evolution")
    FMXNameEvolutionConfig Config;

private:

    UPROPERTY()
    TObjectPtr<UMXNameThemeManager> ThemeManager;
};
