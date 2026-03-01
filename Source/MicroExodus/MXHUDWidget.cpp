// MXHUDWidget.cpp — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI

#include "MXHUDWidget.h"
#include "MXCameraPersonality.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"

// ---------------------------------------------------------------------------
// NativeConstruct / NativeDestruct
// ---------------------------------------------------------------------------

void UMXHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Hide rescue progress on startup
    if (RescueProgressBar)
    {
        RescueProgressBar->SetVisibility(ESlateVisibility::Collapsed);
    }

    ActiveToasts.Empty();
}

void UMXHUDWidget::NativeDestruct()
{
    // Cancel all pending dismiss timers
    for (FMXToastEntry& Entry : ActiveToasts)
    {
        if (GetWorld() && Entry.DismissTimer.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(Entry.DismissTimer);
        }
    }
    ActiveToasts.Empty();

    Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// IMXEventListener
// ---------------------------------------------------------------------------

void UMXHUDWidget::OnDEMSEvent_Implementation(const FMXEventData& Event)
{
    ShowDEMSToast(Event);
}

// ---------------------------------------------------------------------------
// Swarm / Level / Timer / Score
// ---------------------------------------------------------------------------

void UMXHUDWidget::UpdateSwarmCounter(int32 Surviving, int32 Deployed)
{
    if (!SwarmCounterText) return;

    const FString CountStr = FString::Printf(TEXT("%d / %d"), Surviving, Deployed);
    SwarmCounterText->SetText(FText::FromString(CountStr));

    // Trigger flash animation if count dropped
    if (LastSurvivingCount > 0 && Surviving < LastSurvivingCount)
    {
        OnSwarmCounterFlash(true);
    }
    LastSurvivingCount = Surviving;
}

void UMXHUDWidget::UpdateLevelIndicator(int32 LevelNumber, const FString& LevelName)
{
    if (!LevelIndicatorText) return;

    const FString Indicator = FString::Printf(
        TEXT("Level %d / %d — %s"), LevelNumber, 20, *LevelName);
    LevelIndicatorText->SetText(FText::FromString(Indicator));
}

void UMXHUDWidget::UpdateTimer(float ElapsedSeconds)
{
    if (!TimerText) return;

    const int32 TotalSecs = FMath::FloorToInt(ElapsedSeconds);
    const int32 Minutes   = TotalSecs / 60;
    const int32 Seconds   = TotalSecs % 60;

    const FString TimeStr = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
    TimerText->SetText(FText::FromString(TimeStr));
}

void UMXHUDWidget::UpdateScore(int32 Score)
{
    if (!ScoreText) return;

    // Format with comma separators
    FString ScoreStr = FString::FromInt(Score);

    // Insert commas every 3 digits from the right
    int32 InsertPos = ScoreStr.Len() - 3;
    while (InsertPos > 0)
    {
        ScoreStr.InsertAt(InsertPos, TEXT(","));
        InsertPos -= 3;
    }

    ScoreText->SetText(FText::FromString(ScoreStr));
}

// ---------------------------------------------------------------------------
// DEMS Toast System
// ---------------------------------------------------------------------------

void UMXHUDWidget::ShowDEMSToast(const FMXEventData& Event)
{
    // Evict oldest if at cap
    if (ActiveToasts.Num() >= MaxVisibleToasts)
    {
        EvictOldestToast();
    }

    CreateAndEnqueueToast(Event);
}

void UMXHUDWidget::DismissToast(int32 ToastIndex)
{
    if (!ActiveToasts.IsValidIndex(ToastIndex)) return;

    FMXToastEntry& Entry = ActiveToasts[ToastIndex];

    // Cancel auto-dismiss timer
    if (GetWorld() && Entry.DismissTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(Entry.DismissTimer);
    }

    // Animate out and remove
    if (Entry.Widget)
    {
        PlayToastExitAnimation(Entry.Widget);
        if (ToastContainer)
        {
            ToastContainer->RemoveChild(Entry.Widget);
        }
    }

    ActiveToasts.RemoveAt(ToastIndex);
}

void UMXHUDWidget::DismissAllToasts()
{
    // Dismiss in reverse order to preserve valid indices
    for (int32 i = ActiveToasts.Num() - 1; i >= 0; --i)
    {
        DismissToast(i);
    }
}

// ---------------------------------------------------------------------------
// Rescue Progress
// ---------------------------------------------------------------------------

void UMXHUDWidget::ShowRescueProgress(float Progress)
{
    if (!RescueProgressBar) return;

    RescueProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
    RescueProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
}

void UMXHUDWidget::HideRescueProgress()
{
    if (!RescueProgressBar) return;
    RescueProgressBar->SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------
// Minimap
// ---------------------------------------------------------------------------

void UMXHUDWidget::UpdateMinimap(const TArray<FVector>& RobotPositions,
                                  const TArray<FVector>& HazardZones,
                                  const TArray<FVector>& RescuePoints)
{
    if (!MinimapOverlay) return;

    // Clear old dynamic dots — widgets named "MinimapDot_*" are transient overlays
    // Actual minimap dot placement is deferred to the Widget Blueprint implementation;
    // this method exposes the data and triggers a BP event for visual update.
    // (Full procedural canvas drawing requires a BP-side Custom Paint node.)
    OnMinimapDataUpdated(RobotPositions, HazardZones, RescuePoints);
}

// ---------------------------------------------------------------------------
// Camera Mode Icon
// ---------------------------------------------------------------------------

void UMXHUDWidget::SetCameraModeIcon(ECameraPersonalityMode Mode)
{
    if (!CameraModeIcon) return;

    UTexture2D* IconTex = nullptr;

    switch (Mode)
    {
    case ECameraPersonalityMode::Director:  IconTex = IconDirector;  break;
    case ECameraPersonalityMode::Operator:  IconTex = IconOperator;  break;
    case ECameraPersonalityMode::Locked:    IconTex = IconLocked;    break;
    case ECameraPersonalityMode::Replay:    IconTex = IconReplay;    break;
    default:                                IconTex = IconDirector;  break;
    }

    if (IconTex)
    {
        CameraModeIcon->SetBrushFromTexture(IconTex);
    }
}

// ---------------------------------------------------------------------------
// Modifier Badges
// ---------------------------------------------------------------------------

void UMXHUDWidget::SetModifierIcons(const TArray<FString>& ActiveModifiers)
{
    if (!ModifierBadgeBox) return;

    ModifierBadgeBox->ClearChildren();

    for (const FString& ModName : ActiveModifiers)
    {
        if (!WidgetTree) continue;

        UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        if (!Badge) continue;

        Badge->SetBrushColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.8f));

        UTextBlock* BadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (BadgeText)
        {
            BadgeText->SetText(FText::FromString(ModName));
            // Font size set in BP defaults
            Badge->SetContent(BadgeText);
        }

        ModifierBadgeBox->AddChild(Badge);
    }
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

FLinearColor UMXHUDWidget::GetToastColor(EEventType EventType) const
{
    switch (EventType)
    {
    case EEventType::Death:          return MXToastColors::Death;
    case EEventType::Rescue:         return MXToastColors::Rescue;
    case EEventType::NearMiss:       return MXToastColors::NearMiss;
    case EEventType::Sacrifice:      return MXToastColors::Sacrifice;
    case EEventType::LevelUp:        return MXToastColors::LevelUp;
    case EEventType::SpecChosen:     return MXToastColors::SpecChosen;
    default:                         return MXToastColors::Default;
    }
}

float UMXHUDWidget::GetToastDuration(const FMXEventData& Event) const
{
    if (Event.severity == ESeverity::Minor)
    {
        return ToastDurationMinor;
    }
    return ToastDurationMajor;
}

void UMXHUDWidget::CreateAndEnqueueToast(const FMXEventData& Event)
{
    if (!ToastContainer || !WidgetTree) return;

    // Build a Border widget as the card root
    UBorder* ToastCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (!ToastCard) return;

    ToastCard->SetBrushColor(GetToastColor(Event.event_type));

    // Inner vertical box
    UVerticalBox* InnerBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (InnerBox)
    {
        // Robot name (bold for deaths)
        UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (NameText)
        {
            NameText->SetText(FText::FromString(Event.robot_name));
            InnerBox->AddChild(NameText);
        }

        // Message text
        UTextBlock* MsgText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (MsgText)
        {
            MsgText->SetText(FText::FromString(Event.message_text));
            InnerBox->AddChild(MsgText);
        }

        // Hat name if present
        if (Event.hat_worn_id >= 0 && !Event.hat_worn_name.IsEmpty())
        {
            UTextBlock* HatText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            if (HatText)
            {
                HatText->SetText(FText::FromString(
                    FString::Printf(TEXT("[%s]"), *Event.hat_worn_name)));
                InnerBox->AddChild(HatText);
            }
        }

        ToastCard->SetContent(InnerBox);
    }

    // Add to container (newest at bottom)
    ToastContainer->AddChild(ToastCard);

    // Register entry
    FMXToastEntry Entry;
    Entry.Widget = ToastCard;
    Entry.Event  = Event;

    const int32 Index = ActiveToasts.Add(Entry);

    // Animate in
    PlayToastEnterAnimation(ToastCard);

    // Schedule auto-dismiss
    const float Duration = GetToastDuration(Event);
    if (GetWorld())
    {
        FTimerDelegate Delegate;
        Delegate.BindUObject(this, &UMXHUDWidget::AutoDismissToast, Index);
        GetWorld()->GetTimerManager().SetTimer(
            ActiveToasts[Index].DismissTimer, Delegate, Duration, false);
    }
}

void UMXHUDWidget::EvictOldestToast()
{
    if (ActiveToasts.IsEmpty()) return;
    DismissToast(0);
}

void UMXHUDWidget::AutoDismissToast(int32 ToastIndex)
{
    // The index may have shifted if earlier toasts were evicted; search by timer validity
    // Since timers are per-entry, find the entry whose timer just fired
    for (int32 i = 0; i < ActiveToasts.Num(); ++i)
    {
        if (i == ToastIndex)
        {
            DismissToast(i);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Stub for BP event (minimap data pass-through)
// ---------------------------------------------------------------------------

void UMXHUDWidget::OnMinimapDataUpdated_Implementation(
    const TArray<FVector>& RobotPositions,
    const TArray<FVector>& HazardZones,
    const TArray<FVector>& RescuePoints)
{
    // Default: no-op. Widget Blueprint overrides this to draw minimap dots.
}
