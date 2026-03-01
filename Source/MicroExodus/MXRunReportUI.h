// MXRunReportUI.h — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI
// The full cinematic post-run report screen. The most emotionally significant
// UI in the game — typewriter narrative, animated stats, fanfare awards,
// mournful death roll, sacrifice honour roll.
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "MXRunData.h"
#include "MXEventData.h"
#include "MXTypes.h"
#include "TimerManager.h"
#include "MXRunReportUI.generated.h"

// ---------------------------------------------------------------------------
// Section indices (matches the tab bar)
// ---------------------------------------------------------------------------

namespace MXReportSection
{
    constexpr int32 Header          = 0;
    constexpr int32 Opening         = 1;
    constexpr int32 Numbers         = 2;
    constexpr int32 HatCensus       = 3;
    constexpr int32 LevelBreakdown  = 4;
    constexpr int32 Highlights      = 5;
    constexpr int32 Awards          = 6;
    constexpr int32 DeathRoll       = 7;
    constexpr int32 SacrificeRoll   = 8;
    constexpr int32 Closing         = 9;
    constexpr int32 Count           = 10;
}

// ---------------------------------------------------------------------------
// UMXRunReportUI
// ---------------------------------------------------------------------------

/**
 * UMXRunReportUI
 * The post-run summary screen. Sequentially reveals 10 report sections,
 * each with distinct visual and audio styling.
 *
 * Master entry point: DisplayReport(const FMXRunReport&)
 *
 * Bound widgets:
 *   ReportScrollBox        — Main vertical scroll container
 *   SectionTabBar          — HorizontalBox of 10 tab buttons
 *   SectionSwitcher        — WidgetSwitcher (10 panels, jumped via tabs)
 *   HeaderPanel            — VerticalBox: run#, tier, date, outcome
 *   OpeningNarrativeText   — TextBlock driven by typewriter timer
 *   NumbersPanel           — VerticalBox of animated stat counter rows
 *   HatCensusPanel         — VerticalBox for hat stats + commentary
 *   LevelBreakdownPanel    — VerticalBox of collapsible level cards
 *   HighlightsPanel        — VerticalBox of fade-in highlight cards
 *   AwardsPanel            — VerticalBox of award cards (revealed sequentially)
 *   DeathRollPanel         — VerticalBox of chronological death entries
 *   SacrificePanel         — VerticalBox of gold-border sacrifice cards
 *   ClosingNarrativeText   — Closing text with delayed "Continue" reveal
 *   ContinueButton         — Shown after closing narrative finishes
 *   ShareButton            — Capture composite screenshot
 *   FullLogButton          — Toggle raw event log
 *   RunNumberText          — "Run #{N}" in header
 *   OutcomeText            — "SURVIVED" / "FAILED" in header
 *   DurationText           — Elapsed time in header
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXRunReportUI : public UUserWidget
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Master Entry Point
    // -------------------------------------------------------------------------

    /**
     * Populate and begin revealing the full 10-section report.
     * Call once per run at the transition to the post-run screen.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void DisplayReport(const FMXRunReport& Report);

    // -------------------------------------------------------------------------
    // Individual Section Builders (also called by DisplayReport internally)
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowHeader(const FMXRunData& RunData);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void PlayOpeningNarrative(const FString& Text);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowNumbers(const FMXRunData& RunData);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowHatCensus(const FMXHatCensus& Census, const TArray<FString>& Commentary);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowLevelBreakdown(const FMXRunData& RunData);

    /** Expand or collapse a level card in the breakdown. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ToggleLevelExpanded(int32 LevelNumber);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowHighlights(const TArray<FMXHighlight>& Highlights);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowAwards(const TArray<FMXAward>& Awards);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowDeathRoll(const TArray<FMXDeathEntry>& Deaths);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowSacrificeHonorRoll(const TArray<FMXSacrificeEntry>& Sacrifices);

    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void PlayClosingNarrative(const FString& Text);

    // -------------------------------------------------------------------------
    // Navigation
    // -------------------------------------------------------------------------

    /** Jump to a report section by index (0–9). Updates tab highlight. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void NavigateToSection(int32 SectionIndex);

    /** Toggle the raw full event log panel. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShowFullEventLog(const TArray<FMXEventData>& Events);

    /** Capture and share composite PNG. Calls BP implementation. */
    UFUNCTION(BlueprintCallable, Category = "MX|RunReport")
    void ShareReport();

    // -------------------------------------------------------------------------
    // BP Events — visual polish / sequencing
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnAwardRevealed(const FMXAward& Award, int32 AwardIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnDeathEntryRevealed(const FMXDeathEntry& DeathEntry, int32 EntryIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnSacrificeEntryRevealed(const FMXSacrificeEntry& Entry, int32 EntryIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnHighlightRevealed(const FMXHighlight& Highlight, int32 HighlightIndex);

    /** Called when typewriter effect finishes for the opening narrative. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnOpeningNarrativeComplete();

    /** Called when all closing text is shown — BP reveals the Continue button. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnClosingNarrativeComplete();

    /** Called to capture the composite share PNG — implement in BP with HighResScreenshot. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnShareRequested();

    /** Stat counter tick — implement number-roll animation in BP if desired. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|RunReport")
    void OnStatCounterTick(const FString& StatLabel, int32 CurrentValue, int32 TargetValue);

protected:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // -------------------------------------------------------------------------
    // Bound Widgets
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UScrollBox> ReportScrollBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UHorizontalBox> SectionTabBar;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UWidgetSwitcher> SectionSwitcher;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> HeaderPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> OpeningNarrativeText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> NumbersPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> HatCensusPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> LevelBreakdownPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> HighlightsPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> AwardsPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> DeathRollPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> SacrificePanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> ClosingNarrativeText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> ContinueButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> ShareButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UButton> FullLogButton;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> RunNumberText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> OutcomeText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> DurationText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UScrollBox> FullEventLogScrollBox;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** Characters-per-second for typewriter effect. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RunReport|Config")
    float TypewriterCPS = 40.0f;

    /** Delay (seconds) between sequential award reveals. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RunReport|Config")
    float AwardRevealInterval = 1.2f;

    /** Delay (seconds) between sequential death entry fade-ins. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RunReport|Config")
    float DeathRevealInterval = 0.6f;

    /** Delay (seconds) before showing Continue button after closing narrative ends. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RunReport|Config")
    float ContinueDelay = 3.0f;

private:

    // Typewriter state
    FString TypewriterFullText;
    int32   TypewriterCharIndex = 0;
    bool    bTypewriterClosing  = false;
    FTimerHandle TypewriterTimer;

    // Stat counter state
    struct FStatCounter
    {
        FString Label;
        int32   Current = 0;
        int32   Target  = 0;
        TObjectPtr<UTextBlock> Widget;
    };
    TArray<FStatCounter> StatCounters;
    FTimerHandle StatCounterTimer;
    static constexpr float StatTickInterval = 0.05f;

    // Award / death reveal queues
    TArray<FMXAward>        PendingAwards;
    TArray<FMXDeathEntry>   PendingDeaths;
    TArray<FMXSacrificeEntry> PendingSacrifices;
    int32 AwardRevealIndex    = 0;
    int32 DeathRevealIndex    = 0;
    int32 SacrificeRevealIndex = 0;
    FTimerHandle AwardRevealTimer;
    FTimerHandle DeathRevealTimer;
    FTimerHandle SacrificeRevealTimer;

    // Expanded level card tracking
    TSet<int32> ExpandedLevels;

    // Current report cache
    FMXRunReport CachedReport;

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    void TypewriterTick();
    void StatCounterTick();
    void RevealNextAward();
    void RevealNextDeath();
    void RevealNextSacrifice();

    UWidget* BuildAwardCard(const FMXAward& Award);
    UWidget* BuildDeathCard(const FMXDeathEntry& Entry);
    UWidget* BuildSacrificeCard(const FMXSacrificeEntry& Entry);
    UWidget* BuildHighlightCard(const FMXHighlight& Highlight);
    UWidget* BuildLevelCard(int32 LevelNumber, const TArray<FMXEventData>& LevelEvents);
    UWidget* BuildStatRow(const FString& Label, const FString& Value);
    UWidget* BuildTabButton(int32 SectionIndex, const FString& Label);

    /** Collect events for a specific level from RunData. */
    TArray<FMXEventData> GetLevelEvents(const FMXRunData& RunData, int32 LevelNumber) const;

    /** Format a duration in seconds as "Xm Ys". */
    static FString FormatDuration(float Seconds);

    UFUNCTION()
    void OnContinueClicked();

    UFUNCTION()
    void OnShareClicked();

    UFUNCTION()
    void OnFullLogClicked();
};
