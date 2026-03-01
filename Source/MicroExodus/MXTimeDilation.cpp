// MXTimeDilation.cpp — Camera Module v1.1
// Created: 2026-02-22
// Agent 7: Camera

#include "MXTimeDilation.h"
#include "MXTiltShiftEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXTimeDilation::UMXTimeDilation()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    // Use real-time tick so we aren't phase-shifted by our own dilation.
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void UMXTimeDilation::BeginPlay()
{
    Super::BeginPlay();

    // Ensure the world starts at 1.0×.
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

// ---------------------------------------------------------------------------
// EndPlay
// ---------------------------------------------------------------------------

void UMXTimeDilation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Always restore time to 1.0× when this component is destroyed.
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// TickComponent
// ---------------------------------------------------------------------------

void UMXTimeDilation::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Use real (unscaled) delta time so our ramps aren't affected by our own dilation.
    float RealDT = GetRealDeltaTime(DeltaTime);

    TickAutoDilation(RealDT);
    TickPlayerDilation(RealDT);

    CompositeScale = ComputeComposite();
    ApplyDilation();
}

// ---------------------------------------------------------------------------
// SetDilation
// ---------------------------------------------------------------------------

void UMXTimeDilation::SetDilation(EMXDilationMode Mode)
{
    float TargetScale = ModeToScale(Mode);

    switch (Mode)
    {
        case EMXDilationMode::PlayerSlow:
        case EMXDilationMode::FastForward:
        case EMXDilationMode::HyperSpeed:
            BeginPlayerDilation(Mode);
            break;
        default:
            // Auto or replay modes.
            if (bAutoDilationEnabled)
            {
                // Treat as a persistent auto-dilation with no hold limit.
                // BeginAutoDilation with a very long duration.
                BeginAutoDilation(TargetScale, 99999.0f);
            }
            break;
    }
}

// ---------------------------------------------------------------------------
// GetCurrentTimeScale
// ---------------------------------------------------------------------------

float UMXTimeDilation::GetCurrentTimeScale() const
{
    return CompositeScale;
}

// ---------------------------------------------------------------------------
// BeginAutoDilation
// ---------------------------------------------------------------------------

void UMXTimeDilation::BeginAutoDilation(float TargetScale, float Duration)
{
    if (!bAutoDilationEnabled) return;

    // If already holding a more dramatic dilation, extend hold time but don't
    // raise the scale (more dramatic = lower number = don't override with a
    // higher value).
    if (AutoPhase == EAutoDilationPhase::Holding || AutoPhase == EAutoDilationPhase::RampingIn)
    {
        if (TargetScale <= AutoTargetScale)
        {
            // More dramatic — override and extend.
            AutoTargetScale         = TargetScale;
            AutoHoldTimeRemaining   = FMath::Max(AutoHoldTimeRemaining, Duration);
        }
        else
        {
            // Less dramatic — just extend the hold time if we're already committed.
            AutoHoldTimeRemaining = FMath::Max(AutoHoldTimeRemaining, Duration);
        }
        return;
    }

    AutoTargetScale       = TargetScale;
    AutoHoldTimeRemaining = Duration;
    AutoRampProgress      = 0.0f;
    AutoPhase             = EAutoDilationPhase::RampingIn;

    if (bLogDilation)
    {
        UE_LOG(LogTemp, Log, TEXT("[TimeDilation] BeginAutoDilation: scale=%.2f duration=%.2fs"),
               TargetScale, Duration);
    }
}

// ---------------------------------------------------------------------------
// CancelAutoDilation
// ---------------------------------------------------------------------------

void UMXTimeDilation::CancelAutoDilation()
{
    if (AutoPhase == EAutoDilationPhase::Inactive) return;

    AutoPhase        = EAutoDilationPhase::RampingOut;
    AutoRampProgress = 0.0f;

    if (bLogDilation)
    {
        UE_LOG(LogTemp, Log, TEXT("[TimeDilation] CancelAutoDilation: ramping out."));
    }
}

// ---------------------------------------------------------------------------
// SetAutoDilationEnabled
// ---------------------------------------------------------------------------

void UMXTimeDilation::SetAutoDilationEnabled(bool bEnabled)
{
    bAutoDilationEnabled = bEnabled;

    if (!bEnabled)
    {
        CancelAutoDilation();
    }

    if (bLogDilation)
    {
        UE_LOG(LogTemp, Log, TEXT("[TimeDilation] AutoDilation %s."),
               bEnabled ? TEXT("enabled") : TEXT("disabled"));
    }
}

// ---------------------------------------------------------------------------
// IsAutoDilationActive
// ---------------------------------------------------------------------------

bool UMXTimeDilation::IsAutoDilationActive() const
{
    return AutoPhase != EAutoDilationPhase::Inactive;
}

// ---------------------------------------------------------------------------
// BeginPlayerDilation
// ---------------------------------------------------------------------------

void UMXTimeDilation::BeginPlayerDilation(EMXDilationMode Mode)
{
    bPlayerDilationActive = true;
    PlayerTargetScale     = ModeToScale(Mode);

    if (bLogDilation)
    {
        UE_LOG(LogTemp, Log, TEXT("[TimeDilation] BeginPlayerDilation: mode=%d scale=%.2f"),
               (int32)Mode, PlayerTargetScale);
    }
}

// ---------------------------------------------------------------------------
// EndPlayerDilation
// ---------------------------------------------------------------------------

void UMXTimeDilation::EndPlayerDilation()
{
    bPlayerDilationActive = false;
    PlayerTargetScale     = 1.0f;

    if (bLogDilation)
    {
        UE_LOG(LogTemp, Log, TEXT("[TimeDilation] EndPlayerDilation."));
    }
}

// ---------------------------------------------------------------------------
// GetAudioPitchMultiplier
// ---------------------------------------------------------------------------

float UMXTimeDilation::GetAudioPitchMultiplier() const
{
    // SFX pitch follows time scale.
    // Music always stays at 1.0 (caller is responsible for not applying this to music).
    return FMath::Clamp(CompositeScale, 0.1f, 3.0f);
}

// ---------------------------------------------------------------------------
// ApplyDilation
// ---------------------------------------------------------------------------

void UMXTimeDilation::ApplyDilation()
{
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), CompositeScale);

    // Notify TiltShift of current dilation state.
    NotifyTiltShift(CompositeScale);
}

// ===========================================================================
// Private — State Machine Ticks
// ===========================================================================

void UMXTimeDilation::TickAutoDilation(float RealDeltaTime)
{
    switch (AutoPhase)
    {
        // ----------------------------------------------------------------
        case EAutoDilationPhase::Inactive:
            CurrentAutoScale = 1.0f;
            break;

        // ----------------------------------------------------------------
        case EAutoDilationPhase::RampingIn:
        {
            AutoRampProgress += RealDeltaTime / FMath::Max(RampInDuration, KINDA_SMALL_NUMBER);

            // Smooth step ease-in.
            float Alpha = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(AutoRampProgress, 0.0f, 1.0f));
            CurrentAutoScale = FMath::Lerp(1.0f, AutoTargetScale, Alpha);

            if (AutoRampProgress >= 1.0f)
            {
                AutoPhase    = EAutoDilationPhase::Holding;
                CurrentAutoScale = AutoTargetScale;

                if (bLogDilation)
                {
                    UE_LOG(LogTemp, Verbose, TEXT("[TimeDilation] AutoDilation: holding at %.2f for %.2fs"),
                           AutoTargetScale, AutoHoldTimeRemaining);
                }
            }
            break;
        }

        // ----------------------------------------------------------------
        case EAutoDilationPhase::Holding:
        {
            CurrentAutoScale       = AutoTargetScale;
            AutoHoldTimeRemaining -= RealDeltaTime;

            if (AutoHoldTimeRemaining <= 0.0f)
            {
                AutoPhase        = EAutoDilationPhase::RampingOut;
                AutoRampProgress = 0.0f;
            }
            break;
        }

        // ----------------------------------------------------------------
        case EAutoDilationPhase::RampingOut:
        {
            AutoRampProgress += RealDeltaTime / FMath::Max(RampOutDuration, KINDA_SMALL_NUMBER);

            float Alpha = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(AutoRampProgress, 0.0f, 1.0f));
            CurrentAutoScale = FMath::Lerp(AutoTargetScale, 1.0f, Alpha);

            if (AutoRampProgress >= 1.0f)
            {
                AutoPhase        = EAutoDilationPhase::Inactive;
                CurrentAutoScale = 1.0f;

                if (bLogDilation)
                {
                    UE_LOG(LogTemp, Verbose, TEXT("[TimeDilation] AutoDilation: complete, returned to 1.0×"));
                }
            }
            break;
        }
    }
}

void UMXTimeDilation::TickPlayerDilation(float RealDeltaTime)
{
    float TargetPlayer = bPlayerDilationActive ? PlayerTargetScale : 1.0f;

    // Smooth ramp for player dilation using RampInDuration / RampOutDuration.
    float RampSpeed = (TargetPlayer < CurrentPlayerScale)
        ? (1.0f / FMath::Max(RampInDuration, KINDA_SMALL_NUMBER))   // Slowing down = ramp in.
        : (1.0f / FMath::Max(RampOutDuration, KINDA_SMALL_NUMBER));  // Speeding back = ramp out.

    CurrentPlayerScale = FMath::FInterpTo(CurrentPlayerScale, TargetPlayer, RealDeltaTime, RampSpeed * 5.0f);

    // Snap when very close to avoid infinite approach.
    if (FMath::IsNearlyEqual(CurrentPlayerScale, TargetPlayer, 0.001f))
    {
        CurrentPlayerScale = TargetPlayer;
    }
}

// ===========================================================================
// Private — Helpers
// ===========================================================================

float UMXTimeDilation::ModeToScale(EMXDilationMode Mode) const
{
    switch (Mode)
    {
        case EMXDilationMode::Normal:       return 1.0f;
        case EMXDilationMode::DramaticSlow: return DramaticSlowScale;
        case EMXDilationMode::SubtleSlow:   return SubtleSlowScale;
        case EMXDilationMode::PlayerSlow:   return PlayerSlowScale;
        case EMXDilationMode::FastForward:  return FastForwardScale;
        case EMXDilationMode::HyperSpeed:   return HyperSpeedScale;
        case EMXDilationMode::ReplaySlow:   return ReplaySlowScale;
        default:                            return 1.0f;
    }
}

float UMXTimeDilation::ComputeComposite() const
{
    float Combined = CurrentAutoScale * CurrentPlayerScale;
    return FMath::Clamp(Combined, MinScale, MaxScale);
}

void UMXTimeDilation::NotifyTiltShift(float Scale)
{
    if (!TiltShiftEffect) return;

    bool bInSlowMo      = Scale < 0.99f;
    bool bInFastForward = Scale > 1.01f;

    TiltShiftEffect->NotifySlowMotion(bInSlowMo, Scale);
    TiltShiftEffect->NotifyFastForward(bInFastForward);
}

float UMXTimeDilation::GetRealDeltaTime(float ScaledDeltaTime) const
{
    // Undo the global time dilation that UE already applied to DeltaTime.
    float GlobalScale = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
    if (GlobalScale < KINDA_SMALL_NUMBER) return ScaledDeltaTime;
    return ScaledDeltaTime / GlobalScale;
}
