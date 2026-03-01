// MXHUDWidget.h — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI
// In-game heads-up display widget. Displayed during active runs.
// Implements IMXEventListener for live DEMS toast feed.
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "MXInterfaces.h"
#include "MXEventData.h"
#include "MXTypes.h"
#include "TimerManager.h"
#include "MXHUDWidget.generated.h"

// ---------------------------------------------------------------------------
// Toast colour constants (matched to UMG linear colours in BP)
// ---------------------------------------------------------------------------

namespace MXToastColors
{
    static const FLinearColor Death      = FLinearColor(0.8f, 0.05f, 0.05f, 1.0f);
    static const FLinearColor Rescue     = FLinearColor(0.1f, 0.75f, 0.2f,  1.0f);
    static const FLinearColor NearMiss   = FLinearColor(0.9f, 0.75f, 0.05f, 1.0f);
    static const FLinearColor Sacrifice  = FLinearColor(0.85f, 0.6f, 0.0f,  1.0f);
    static const FLinearColor LevelUp    = FLinearColor(0.1f, 0.55f, 0.9f,  1.0f);
    static const FLinearColor SpecChosen = FLinearColor(0.55f, 0.1f, 0.9f,  1.0f);
    static const FLinearColor Default    = FLinearColor(0.9f, 0.9f, 0.9f,   1.0f);
}

// ---------------------------------------------------------------------------
// FMXToastEntry — one live toast card
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMXToastEntry
{
    GENERATED_BODY()

    UPROPERTY() class UWidget* Widget = nullptr;
    UPROPERTY() FMXEventData   Event;
    FTimerHandle               DismissTimer;
    bool                       bExpanded = false;
};

// ---------------------------------------------------------------------------
// UMXHUDWidget
// ---------------------------------------------------------------------------

/**
 * UMXHUDWidget
 * The in-game heads-up display. Owns the DEMS toast stack, swarm counter,
 * level indicator, timer, score, rescue radial, minimap, and modifier badges.
 *
 * Bound widgets (wire in the Widget Blueprint):
 *   SwarmCounterText  — "12/30" top-left
 *   LevelIndicatorText — "Level 4/20 — The Grinder"
 *   TimerText          — "03:42"
 *   ScoreText          — "12,840"
 *   ToastContainer     — VerticalBox; toasts inserted here
 *   RescueProgressBar  — radial / standard ProgressBar, hidden by default
 *   MinimapOverlay     — Canvas for dot/zone overlays
 *   CameraModeIcon     — Image swapped per mode
 *   ModifierBadgeBox   — HorizontalBox for modifier badges
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXHUDWidget : public UUserWidget, public IMXEventListener
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // IMXEventListener
    // -------------------------------------------------------------------------

    /** Receive a live DEMS event and display it as a toast. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|HUD")
    void OnDEMSEvent(const FMXEventData& Event);
    virtual void OnDEMSEvent_Implementation(const FMXEventData& Event) override;

    // -------------------------------------------------------------------------
    // Swarm / Level / Timer / Score
    // -------------------------------------------------------------------------

    /** Update the top-left swarm counter: "{Surviving}/{Deployed}" */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void UpdateSwarmCounter(int32 Surviving, int32 Deployed);

    /** Update the top-center level indicator: "Level {N}/20 — {LevelName}" */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void UpdateLevelIndicator(int32 LevelNumber, const FString& LevelName);

    /** Update the elapsed-time display (seconds → "MM:SS"). */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void UpdateTimer(float ElapsedSeconds);

    /** Update the running score display. */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void UpdateScore(int32 Score);

    // -------------------------------------------------------------------------
    // DEMS Toast System
    // -------------------------------------------------------------------------

    /**
     * Create, colour-code, and animate in a DEMS toast card.
     * Stacks vertically (newest bottom), max 3 visible, auto-dismisses.
     * @param Event  The fully resolved DEMS event.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD|Toast")
    void ShowDEMSToast(const FMXEventData& Event);

    /** Manually dismiss a toast by its stack index (0 = oldest). */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD|Toast")
    void DismissToast(int32 ToastIndex);

    /** Dismiss all active toasts immediately (e.g., on level transition). */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD|Toast")
    void DismissAllToasts();

    // -------------------------------------------------------------------------
    // Rescue progress
    // -------------------------------------------------------------------------

    /** Show and update the radial rescue progress bar (0.0–1.0). */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void ShowRescueProgress(float Progress);

    /** Hide the rescue progress indicator. */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void HideRescueProgress();

    // -------------------------------------------------------------------------
    // Minimap
    // -------------------------------------------------------------------------

    /**
     * Redraw minimap dot overlay.
     * @param RobotPositions  World positions of all living robots.
     * @param HazardZones     World positions of active hazard centres.
     * @param RescuePoints    World positions of active rescue cages.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void UpdateMinimap(const TArray<FVector>& RobotPositions,
                       const TArray<FVector>& HazardZones,
                       const TArray<FVector>& RescuePoints);

    // -------------------------------------------------------------------------
    // Camera & modifiers
    // -------------------------------------------------------------------------

    /** Swap the camera-mode icon to reflect the current personality mode. */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void SetCameraModeIcon(ECameraPersonalityMode Mode);

    /** Rebuild the modifier badge row from active modifier name strings. */
    UFUNCTION(BlueprintCallable, Category = "MX|HUD")
    void SetModifierIcons(const TArray<FString>& ActiveModifiers);

    // -------------------------------------------------------------------------
    // Blueprint-overridable animations
    // -------------------------------------------------------------------------

    /** Called when a toast should animate in from the right edge. Implement in BP. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|HUD|Toast")
    void PlayToastEnterAnimation(UWidget* ToastWidget);

    /** Called when a toast should animate out. Implement in BP. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|HUD|Toast")
    void PlayToastExitAnimation(UWidget* ToastWidget);

    /** Called when the swarm counter changes drastically (e.g. mass death). */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|HUD")
    void OnSwarmCounterFlash(bool bDecreased);

    /**
     * Called after minimap data is updated. Widget Blueprint implements
     * the actual dot-drawing logic using UMG Canvas.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "MX|HUD")
    void OnMinimapDataUpdated(const TArray<FVector>& RobotPositions,
                              const TArray<FVector>& HazardZones,
                              const TArray<FVector>& RescuePoints);
    virtual void OnMinimapDataUpdated_Implementation(const TArray<FVector>& RobotPositions,
                                                     const TArray<FVector>& HazardZones,
                                                     const TArray<FVector>& RescuePoints);

protected:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // -------------------------------------------------------------------------
    // Bound Widgets (wire in Widget Blueprint)
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> SwarmCounterText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> LevelIndicatorText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> TimerText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> ScoreText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> ToastContainer;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UProgressBar> RescueProgressBar;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UCanvasPanel> MinimapOverlay;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UImage> CameraModeIcon;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UHorizontalBox> ModifierBadgeBox;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** Toast duration (seconds) for Minor/Major severity events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|Config")
    float ToastDurationMinor = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|Config")
    float ToastDurationMajor = 5.0f;

    /** Maximum toasts displayed simultaneously before oldest is evicted. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|Config")
    int32 MaxVisibleToasts = 3;

    // Icon textures for camera modes — assign in BP defaults
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|Icons")
    TObjectPtr<UTexture2D> IconDirector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|Icons")
    TObjectPtr<UTexture2D> IconOperator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|Icons")
    TObjectPtr<UTexture2D> IconLocked;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|Icons")
    TObjectPtr<UTexture2D> IconReplay;

private:

    // Active toasts (oldest first)
    TArray<FMXToastEntry> ActiveToasts;

    int32 LastSurvivingCount = -1;

    /** Resolve border colour from event type. */
    FLinearColor GetToastColor(EEventType EventType) const;

    /** Resolve toast display duration from event severity. */
    float GetToastDuration(const FMXEventData& Event) const;

    /** Internal: build toast widget, add to container, schedule dismiss. */
    void CreateAndEnqueueToast(const FMXEventData& Event);

    /** Evict the oldest toast to make room. */
    void EvictOldestToast();

    /** Called by timer to auto-dismiss a toast. */
    void AutoDismissToast(int32 ToastIndex);
};
