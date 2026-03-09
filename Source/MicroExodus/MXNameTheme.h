// MXNameTheme.h — Identity Module: Per-Robot Themed Name Packs
// Created: 2026-03-09
//
// Each RUN has a theme selected at launch. Every robot born or rescued during
// that run inherits the theme. The theme_id is stored on FMXRobotProfile and
// persists for the robot's lifetime. Over time, the player's roster becomes
// a mix of themes from different runs.
//
// A theme provides:
//   - First name pool (birth names for that theme's run)
//   - Surname pool (milestone names, per-role, themed)
//   - Title aliases (canonical title → themed version)
//   - Title prefix ("The" for robots, "Captain" for pirates, etc.)
//
// Example roster after several themed runs:
//   "Bolt Sprocket The Fireproof"       (Robot theme, run 1)
//   "Barnacle Planksworth The Scourge"   (Pirate theme, run 3)
//   "Ember Hexfire The Arcane"           (Wizard theme, run 5)
//   "Kumo Hayabusa The Shadow"           (Samurai theme, run 7)
//
// Built-in themes: Robot, Wizard, Pirate, Samurai, SciFi, Mythic
// Custom themes loadable from DataTables.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXTypes.h"
#include "Engine/DataTable.h"
#include "MXNameTheme.generated.h"

// ---------------------------------------------------------------------------
// ENameTheme — Theme identifiers (stored on FMXRobotProfile)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// FMXNameThemeData — Complete theme definition
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXNameThemeData
{
    GENERATED_BODY()

    /** Theme display name shown in run setup UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ThemeName;

    /** Short flavour description for the theme selector. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    /** Unlock condition (empty = always available). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString UnlockCondition;

    /** Whether this theme is unlocked. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUnlocked = false;

    /** First name pool (replaces DT_Names for robots born in this theme's run). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> FirstNames;

    /** Universal surname pool (no role requirement). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> UniversalSurnames;

    /** Scout-themed surnames. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> ScoutSurnames;

    /** Guardian-themed surnames. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> GuardianSurnames;

    /** Engineer-themed surnames. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> EngineerSurnames;

    /**
     * Title aliases: canonical title → themed version.
     * e.g., "Firewalker" → "Pyromancer" (Wizard), "Firewalker" → "Powder Keg" (Pirate)
     * Titles not in the map keep their original canonical name.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> TitleAliases;

    /**
     * Title prefix. "The" for robots, "Captain" for pirates,
     * "The Great" for wizards, "Honourable" for samurai, etc.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString TitlePrefix = TEXT("The");
};

// ---------------------------------------------------------------------------
// DataTable row types for custom themes
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXThemeFirstNameRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;
};

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXThemeSurnameRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Surname;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERobotRole Role = ERobotRole::None;
};

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXThemeTitleAliasRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString OriginalTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ThemedTitle;
};

// ---------------------------------------------------------------------------
// UMXNameThemeManager — Theme registry (stateless lookup, no "active" theme)
// ---------------------------------------------------------------------------

/**
 * UMXNameThemeManager
 * Registry of all available naming themes. Provides pool lookups by theme ID.
 *
 * This class has NO "active theme" state. Instead:
 *   - The RUN selects a theme at launch (stored on FMXRunData)
 *   - Robots born/rescued during that run get the theme stamped on their profile
 *   - NameGenerator/NameEvolution query the robot's theme_id to get the right pools
 *
 * Usage:
 *   GameInstance::Init → ThemeManager->Initialize()
 *   RunManager::StartRun → theme chosen by player, passed to NameGenerator
 *   NameGenerator::GenerateThemedName(ENameTheme) → picks from theme's first name pool
 *   NameEvolution::GetDisplayName(Profile) → uses profile.name_theme for surname/title
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXNameThemeManager : public UObject
{
    GENERATED_BODY()

public:

    UMXNameThemeManager();

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /** Register all built-in themes. Call once after construction. */
    UFUNCTION(BlueprintCallable, Category = "MX|Names|Theme")
    void Initialize();

    /** Register a custom theme from DataTables. */
    UFUNCTION(BlueprintCallable, Category = "MX|Names|Theme")
    void RegisterCustomTheme(const FString& ThemeName,
                              UDataTable* FirstNameTable,
                              UDataTable* SurnameTable,
                              UDataTable* TitleAliasTable = nullptr,
                              const FString& InTitlePrefix = TEXT("The"),
                              const FString& InDescription = TEXT(""),
                              const FString& InUnlockCondition = TEXT(""));

    // -------------------------------------------------------------------------
    // Theme Lookup (stateless — pass the theme, get the data)
    // -------------------------------------------------------------------------

    /** Get full theme data by enum. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Theme")
    const FMXNameThemeData& GetThemeData(ENameTheme Theme) const;

    /** Get the first name pool for a theme. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Theme")
    const TArray<FString>& GetFirstNames(ENameTheme Theme) const;

    /** Get the surname pool for a theme + role. Returns role-specific + universal combined. */
    UFUNCTION(BlueprintCallable, Category = "MX|Names|Theme")
    TArray<FString> GetSurnames(ENameTheme Theme, ERobotRole Role) const;

    /** Get the themed version of a title for a specific theme. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Theme")
    FString GetThemedTitle(ENameTheme Theme, const FString& CanonicalTitle) const;

    /** Get the title prefix for a theme ("The", "Captain", etc.). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Theme")
    FString GetTitlePrefix(ENameTheme Theme) const;

    // -------------------------------------------------------------------------
    // Registry Queries
    // -------------------------------------------------------------------------

    /** Get all registered theme enums. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Theme")
    TArray<ENameTheme> GetAllThemes() const;

    /** Get all unlocked themes. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Theme")
    TArray<ENameTheme> GetUnlockedThemes() const;

    /** Check if a theme is unlocked. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Names|Theme")
    bool IsThemeUnlocked(ENameTheme Theme) const;

    /** Unlock a theme. */
    UFUNCTION(BlueprintCallable, Category = "MX|Names|Theme")
    void UnlockTheme(ENameTheme Theme);

private:

    /** All registered themes. */
    TMap<ENameTheme, FMXNameThemeData> ThemeRegistry;

    /** Fallback empty data. */
    FMXNameThemeData EmptyTheme;

    /** Empty array fallback. */
    TArray<FString> EmptyArray;

    // Built-in theme registration.
    void RegisterRobotTheme();
    void RegisterWizardTheme();
    void RegisterPirateTheme();
    void RegisterSamuraiTheme();
    void RegisterSciFiTheme();
    void RegisterMythicTheme();
};
