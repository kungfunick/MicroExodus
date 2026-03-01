// MXHubUI.h — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI
// The player's main home base — hub, navigation, quick stats, and save state.
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "MXInterfaces.h"
#include "MXTypes.h"
#include "MXRunData.h"
#include "MXHubUI.generated.h"

// ---------------------------------------------------------------------------
// UMXHubUI
// ---------------------------------------------------------------------------

/**
 * UMXHubUI
 * The main menu hub screen between runs. Shows roster preview, hat collection
 * progress, quick stats, and navigation to all other screens.
 *
 * Bound widgets:
 *   RosterCountText       — "Robots: {N}"
 *   LevelDistributionBox  — HorizontalBox of level-bucket bars
 *   HatProgressText       — "{N}/100 hats"
 *   HatProgressBar        — Progress bar 0–1
 *   TotalRunsText         — "Total Runs: {N}"
 *   BestSurvivalText      — "Best Survival: {N}%"
 *   TotalRescuedText      — "Rescued All-Time: {N}"
 *   TotalHatsLostText     — "Hats Lost All-Time: {N}"
 *   SaveIndicatorText     — "Last saved: {timestamp}"
 *   StartRunButton
 *   RosterButton
 *   HatCollectionButton
 *   MemorialWallButton
 *   RunHistoryButton
 *   SettingsButton
 *   ContinueRunButton     — Shown only when a run is in progress
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXHubUI : public UUserWidget
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Data Refresh
    // -------------------------------------------------------------------------

    /**
     * Pull the latest data from all module providers and refresh all Hub panels.
     * Call on screen open and after each run completes.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void RefreshHubData();

    // -------------------------------------------------------------------------
    // Individual panel refreshers
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void ShowRosterPreview(int32 RobotCount, const TArray<int32>& LevelDistribution);

    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void ShowHatProgress(int32 Unlocked, int32 Total);

    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void ShowQuickStats(int32 TotalRuns, float BestSurvivalRate,
                        int32 TotalRescued, int32 TotalHatsLost);

    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void ShowSaveIndicator(const FDateTime& LastSaveTime);

    /** Show or hide the "Continue Run" button based on whether a run is in progress. */
    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void SetContinueRunVisible(bool bVisible);

    // -------------------------------------------------------------------------
    // Provider Injection (called once after widget creation)
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void SetRobotProvider(TScriptInterface<IMXRobotProvider> InProvider);

    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void SetHatProvider(TScriptInterface<IMXHatProvider> InProvider);

    UFUNCTION(BlueprintCallable, Category = "MX|Hub")
    void SetRunProvider(TScriptInterface<IMXRunProvider> InProvider);

    // -------------------------------------------------------------------------
    // BP Events — Screen Transitions
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub|Nav")
    void OnNavigateToStartRun();

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub|Nav")
    void OnNavigateToRoster();

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub|Nav")
    void OnNavigateToHatCollection();

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub|Nav")
    void OnNavigateToMemorialWall();

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub|Nav")
    void OnNavigateToRunHistory();

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub|Nav")
    void OnNavigateToSettings();

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub|Nav")
    void OnNavigateToContinueRun();

    /** Called by RefreshHubData after all panels are updated. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Hub")
    void OnHubDataRefreshed();

protected:

    virtual void NativeConstruct() override;

    // -------------------------------------------------------------------------
    // Bound Widgets
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> RosterCountText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UHorizontalBox> LevelDistributionBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> HatProgressText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UProgressBar> HatProgressBar;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> TotalRunsText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> BestSurvivalText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> TotalRescuedText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> TotalHatsLostText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> SaveIndicatorText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> StartRunButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> RosterButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> HatCollectionButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> MemorialWallButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> RunHistoryButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> SettingsButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> ContinueRunButton;

    // -------------------------------------------------------------------------
    // Provider References
    // -------------------------------------------------------------------------

    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

    UPROPERTY()
    TScriptInterface<IMXHatProvider> HatProvider;

    UPROPERTY()
    TScriptInterface<IMXRunProvider> RunProvider;

private:

    void BindButtonDelegates();

    UFUNCTION() void OnStartRunClicked();
    UFUNCTION() void OnRosterClicked();
    UFUNCTION() void OnHatCollectionClicked();
    UFUNCTION() void OnMemorialWallClicked();
    UFUNCTION() void OnRunHistoryClicked();
    UFUNCTION() void OnSettingsClicked();
    UFUNCTION() void OnContinueRunClicked();
};
