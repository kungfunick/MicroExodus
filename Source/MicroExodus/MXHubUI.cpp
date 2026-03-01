// MXHubUI.cpp — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI

#include "MXHubUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"

// ---------------------------------------------------------------------------
// NativeConstruct
// ---------------------------------------------------------------------------

void UMXHubUI::NativeConstruct()
{
    Super::NativeConstruct();

    BindButtonDelegates();

    // Continue button hidden until we know a run is in progress
    if (ContinueRunButton)
    {
        ContinueRunButton->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// ---------------------------------------------------------------------------
// Provider Injection
// ---------------------------------------------------------------------------

void UMXHubUI::SetRobotProvider(TScriptInterface<IMXRobotProvider> InProvider)
{
    RobotProvider = InProvider;
}

void UMXHubUI::SetHatProvider(TScriptInterface<IMXHatProvider> InProvider)
{
    HatProvider = InProvider;
}

void UMXHubUI::SetRunProvider(TScriptInterface<IMXRunProvider> InProvider)
{
    RunProvider = InProvider;
}

// ---------------------------------------------------------------------------
// RefreshHubData
// ---------------------------------------------------------------------------

void UMXHubUI::RefreshHubData()
{
    // --- Roster ---
    if (RobotProvider.GetInterface())
    {
        const int32 Count = IMXRobotProvider::Execute_GetRobotCount(RobotProvider.GetObject());
        const TArray<FMXRobotProfile> AllProfiles =
            IMXRobotProvider::Execute_GetAllRobotProfiles(RobotProvider.GetObject());

        // Build level distribution: bucket into bands 1-10, 11-20, 21-30, etc.
        TArray<int32> Distribution;
        constexpr int32 BucketSize = 10;
        constexpr int32 NumBuckets = 10;
        Distribution.SetNumZeroed(NumBuckets);

        for (const FMXRobotProfile& P : AllProfiles)
        {
            const int32 Bucket = FMath::Clamp((P.level - 1) / BucketSize, 0, NumBuckets - 1);
            Distribution[Bucket]++;
        }

        ShowRosterPreview(Count, Distribution);
    }

    // --- Hat collection ---
    if (HatProvider.GetInterface())
    {
        const FMXHatCollection Collection =
            IMXHatProvider::Execute_GetHatCollection(HatProvider.GetObject());
        ShowHatProgress(Collection.unlocked_hat_ids.Num(), 100);
    }

    // Persistent stats (TotalRuns, BestSurvival, etc.) come from the Persistence module.
    // Persistence is not directly injected here — it passes data through SaveMetadata
    // which the owning game mode / BP sets via ShowQuickStats().
    // BP subclass calls ShowQuickStats() with values fetched from Persistence.

    OnHubDataRefreshed();
}

// ---------------------------------------------------------------------------
// Panel Refreshers
// ---------------------------------------------------------------------------

void UMXHubUI::ShowRosterPreview(int32 RobotCount, const TArray<int32>& LevelDistribution)
{
    if (RosterCountText)
    {
        RosterCountText->SetText(FText::FromString(
            FString::Printf(TEXT("Robots: %d"), RobotCount)));
    }

    if (LevelDistributionBox && WidgetTree)
    {
        LevelDistributionBox->ClearChildren();

        for (int32 i = 0; i < LevelDistribution.Num(); ++i)
        {
            const int32 Count = LevelDistribution[i];

            UBorder* Bucket = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
            if (!Bucket) continue;

            // Colour gradient from green (low levels) to gold (high levels)
            const float T = (float)i / FMath::Max(1, LevelDistribution.Num() - 1);
            Bucket->SetBrushColor(FLinearColor(T * 0.9f, 0.8f - T * 0.5f, 0.1f, 1.0f));

            UTextBlock* BucketText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            if (BucketText)
            {
                const int32 MinLvl = i * 10 + 1;
                const int32 MaxLvl = (i + 1) * 10;
                BucketText->SetText(FText::FromString(
                    FString::Printf(TEXT("Lv%d–%d\n%d"), MinLvl, MaxLvl, Count)));
                Bucket->SetContent(BucketText);
            }

            LevelDistributionBox->AddChild(Bucket);
        }
    }
}

void UMXHubUI::ShowHatProgress(int32 Unlocked, int32 Total)
{
    if (HatProgressText)
    {
        HatProgressText->SetText(FText::FromString(
            FString::Printf(TEXT("%d / %d hats"), Unlocked, Total)));
    }

    if (HatProgressBar)
    {
        HatProgressBar->SetPercent(Total > 0 ? (float)Unlocked / (float)Total : 0.0f);
    }
}

void UMXHubUI::ShowQuickStats(int32 TotalRuns, float BestSurvivalRate,
                               int32 TotalRescued, int32 TotalHatsLost)
{
    if (TotalRunsText)
    {
        TotalRunsText->SetText(FText::FromString(
            FString::Printf(TEXT("Total Runs: %d"), TotalRuns)));
    }

    if (BestSurvivalText)
    {
        BestSurvivalText->SetText(FText::FromString(
            FString::Printf(TEXT("Best Survival: %.0f%%"), BestSurvivalRate * 100.0f)));
    }

    if (TotalRescuedText)
    {
        TotalRescuedText->SetText(FText::FromString(
            FString::Printf(TEXT("Rescued All-Time: %d"), TotalRescued)));
    }

    if (TotalHatsLostText)
    {
        TotalHatsLostText->SetText(FText::FromString(
            FString::Printf(TEXT("Hats Lost All-Time: %d"), TotalHatsLost)));
    }
}

void UMXHubUI::ShowSaveIndicator(const FDateTime& LastSaveTime)
{
    if (!SaveIndicatorText) return;

    const FString TimeStr = LastSaveTime.ToString(TEXT("%Y-%m-%d %H:%M"));
    SaveIndicatorText->SetText(FText::FromString(
        FString::Printf(TEXT("Last saved: %s"), *TimeStr)));
}

void UMXHubUI::SetContinueRunVisible(bool bVisible)
{
    if (!ContinueRunButton) return;

    ContinueRunButton->SetVisibility(bVisible
        ? ESlateVisibility::Visible
        : ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------
// Button Binding
// ---------------------------------------------------------------------------

void UMXHubUI::BindButtonDelegates()
{
    if (StartRunButton)
        StartRunButton->OnClicked.AddDynamic(this, &UMXHubUI::OnStartRunClicked);

    if (RosterButton)
        RosterButton->OnClicked.AddDynamic(this, &UMXHubUI::OnRosterClicked);

    if (HatCollectionButton)
        HatCollectionButton->OnClicked.AddDynamic(this, &UMXHubUI::OnHatCollectionClicked);

    if (MemorialWallButton)
        MemorialWallButton->OnClicked.AddDynamic(this, &UMXHubUI::OnMemorialWallClicked);

    if (RunHistoryButton)
        RunHistoryButton->OnClicked.AddDynamic(this, &UMXHubUI::OnRunHistoryClicked);

    if (SettingsButton)
        SettingsButton->OnClicked.AddDynamic(this, &UMXHubUI::OnSettingsClicked);

    if (ContinueRunButton)
        ContinueRunButton->OnClicked.AddDynamic(this, &UMXHubUI::OnContinueRunClicked);
}

// ---------------------------------------------------------------------------
// Navigation Handlers
// ---------------------------------------------------------------------------

void UMXHubUI::OnStartRunClicked()
{
    OnNavigateToStartRun();
}

void UMXHubUI::OnRosterClicked()
{
    OnNavigateToRoster();
}

void UMXHubUI::OnHatCollectionClicked()
{
    OnNavigateToHatCollection();
}

void UMXHubUI::OnMemorialWallClicked()
{
    OnNavigateToMemorialWall();
}

void UMXHubUI::OnRunHistoryClicked()
{
    OnNavigateToRunHistory();
}

void UMXHubUI::OnSettingsClicked()
{
    OnNavigateToSettings();
}

void UMXHubUI::OnContinueRunClicked()
{
    OnNavigateToContinueRun();
}
