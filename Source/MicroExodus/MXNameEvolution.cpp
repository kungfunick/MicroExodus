// MXNameEvolution.cpp — Identity Module: Theme-Aware Dynamic Name Evolution
// Created: 2026-03-09

#include "MXNameEvolution.h"
#include "MXNameTheme.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXNameEvolution::UMXNameEvolution()
{
}

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void UMXNameEvolution::Initialize(UMXNameThemeManager* InThemeManager)
{
    ThemeManager = InThemeManager;

    if (!ThemeManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXNameEvolution: No ThemeManager provided. Themed names will fall back."));
    }
}

// ---------------------------------------------------------------------------
// EvaluateNameEvolution
// ---------------------------------------------------------------------------

bool UMXNameEvolution::EvaluateNameEvolution(FMXRobotProfile& Profile)
{
    // Already has a surname — no evolution needed.
    if (!Profile.surname.IsEmpty()) return false;

    // Check the milestone.
    if (Profile.runs_survived >= Config.SurnameThreshold)
    {
        Profile.surname = RollSurname(Profile.robot_id, Profile.role, Profile.name_theme);

        UE_LOG(LogTemp, Log,
            TEXT("MXNameEvolution: '%s' earned surname '%s' (theme=%d) after %d runs. Display: %s"),
            *Profile.name, *Profile.surname, (int32)Profile.name_theme,
            Profile.runs_survived, *GetDisplayName(Profile));

        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// GetDisplayName
// ---------------------------------------------------------------------------

FString UMXNameEvolution::GetDisplayName(const FMXRobotProfile& Profile) const
{
    FString Result = Profile.name;

    if (!Profile.surname.IsEmpty())
    {
        Result += TEXT(" ") + Profile.surname;
    }

    FString Title = GetFormattedTitle(Profile);
    if (!Title.IsEmpty())
    {
        Result += TEXT(" ") + Title;
    }

    return Result;
}

// ---------------------------------------------------------------------------
// GetShortName
// ---------------------------------------------------------------------------

FString UMXNameEvolution::GetShortName(const FMXRobotProfile& Profile) const
{
    if (Profile.surname.IsEmpty())
    {
        return Profile.name;
    }
    return Profile.name + TEXT(" ") + Profile.surname;
}

// ---------------------------------------------------------------------------
// GetFormattedTitle
// ---------------------------------------------------------------------------

FString UMXNameEvolution::GetFormattedTitle(const FMXRobotProfile& Profile) const
{
    if (Profile.displayed_title.IsEmpty()) return FString();

    // Resolve the themed alias for this robot's theme.
    FString ThemedTitle = Profile.displayed_title;
    FString Prefix = TEXT("The");

    if (ThemeManager)
    {
        ThemedTitle = ThemeManager->GetThemedTitle(Profile.name_theme, Profile.displayed_title);
        Prefix = ThemeManager->GetTitlePrefix(Profile.name_theme);
    }

    // Avoid double-prefix.
    if (ThemedTitle.StartsWith(Prefix, ESearchCase::IgnoreCase))
    {
        return ThemedTitle;
    }

    return Prefix + TEXT(" ") + ThemedTitle;
}

// ---------------------------------------------------------------------------
// RollSurname
// ---------------------------------------------------------------------------

FString UMXNameEvolution::RollSurname(const FGuid& RobotId, ERobotRole Role, ENameTheme Theme) const
{
    TArray<FString> Pool;

    if (ThemeManager)
    {
        Pool = ThemeManager->GetSurnames(Theme, Role);
    }

    if (Pool.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXNameEvolution: No surnames for theme=%d role=%d. Using fallback."),
            (int32)Theme, (int32)Role);
        return TEXT("II");
    }

    // Deterministic index from GUID.
    uint32 Seed = GetTypeHash(RobotId);
    FRandomStream Stream(Seed);
    int32 Index = Stream.RandRange(0, Pool.Num() - 1);

    return Pool[Index];
}
