// MXTiltShiftEffect.cpp — Camera Module v1.1
// Created: 2026-02-22
// Agent 7: Camera

#include "MXTiltShiftEffect.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// FMXTiltShiftParams::Lerp
// ---------------------------------------------------------------------------

FMXTiltShiftParams FMXTiltShiftParams::Lerp(const FMXTiltShiftParams& A,
                                              const FMXTiltShiftParams& B,
                                              float Alpha)
{
    FMXTiltShiftParams Out;
    Out.FocusBandCenter      = FMath::Lerp(A.FocusBandCenter,      B.FocusBandCenter,      Alpha);
    Out.FocusBandWidth       = FMath::Lerp(A.FocusBandWidth,       B.FocusBandWidth,       Alpha);
    Out.BlurIntensity        = FMath::Lerp(A.BlurIntensity,        B.BlurIntensity,        Alpha);
    Out.BlurRadius           = FMath::Lerp(A.BlurRadius,           B.BlurRadius,           Alpha);
    Out.ChromaticAberration  = FMath::Lerp(A.ChromaticAberration,  B.ChromaticAberration,  Alpha);
    Out.Vignette             = FMath::Lerp(A.Vignette,             B.Vignette,             Alpha);
    Out.SaturationBoost      = FMath::Lerp(A.SaturationBoost,      B.SaturationBoost,      Alpha);
    Out.bHexagonalBokeh      = A.bHexagonalBokeh; // Boolean — use source state; transitions don't blend booleans.
    return Out;
}

// ---------------------------------------------------------------------------
// Constructor — build GDD-specified preset defaults
// ---------------------------------------------------------------------------

UMXTiltShiftEffect::UMXTiltShiftEffect()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // ---- Diorama: Default gameplay. Maximum miniature illusion. ----
    DioramaParams.FocusBandCenter     = 0.5f;
    DioramaParams.FocusBandWidth      = 0.3f;
    DioramaParams.BlurIntensity       = 0.7f;
    DioramaParams.BlurRadius          = 8.0f;
    DioramaParams.ChromaticAberration = 0.1f;
    DioramaParams.Vignette            = 0.15f;
    DioramaParams.SaturationBoost     = 0.15f;
    DioramaParams.bHexagonalBokeh     = true;

    // ---- Cinematic: Epic/Cinematic events. Tight focus on subject. ----
    CinematicParams.FocusBandCenter     = 0.5f;
    CinematicParams.FocusBandWidth      = 0.15f;
    CinematicParams.BlurIntensity       = 0.85f;
    CinematicParams.BlurRadius          = 10.0f;
    CinematicParams.ChromaticAberration = 0.12f;
    CinematicParams.Vignette            = 0.2f;
    CinematicParams.SaturationBoost     = 0.10f;
    CinematicParams.bHexagonalBokeh     = true;

    // ---- Wide: Close zoom, low robot count. Gentle hint. ----
    WideParams.FocusBandCenter     = 0.5f;
    WideParams.FocusBandWidth      = 0.6f;
    WideParams.BlurIntensity       = 0.2f;
    WideParams.BlurRadius          = 4.0f;
    WideParams.ChromaticAberration = 0.05f;
    WideParams.Vignette            = 0.1f;
    WideParams.SaturationBoost     = 0.05f;
    WideParams.bHexagonalBokeh     = true;

    // ---- Report: Run Report snapshot. ----
    ReportParams.FocusBandCenter     = 0.5f;
    ReportParams.FocusBandWidth      = 0.25f;
    ReportParams.BlurIntensity       = 0.8f;
    ReportParams.BlurRadius          = 9.0f;
    ReportParams.ChromaticAberration = 0.08f;
    ReportParams.Vignette            = 0.18f;
    ReportParams.SaturationBoost     = 0.15f;
    ReportParams.bHexagonalBokeh     = true;

    // ---- Off: All zeros. Debug / accessibility. ----
    OffParams.FocusBandCenter     = 0.5f;
    OffParams.FocusBandWidth      = 1.0f;
    OffParams.BlurIntensity       = 0.0f;
    OffParams.BlurRadius          = 0.0f;
    OffParams.ChromaticAberration = 0.0f;
    OffParams.Vignette            = 0.0f;
    OffParams.SaturationBoost     = 0.0f;
    OffParams.bHexagonalBokeh     = false;

    // Start in Diorama state.
    CurrentParams = DioramaParams;
    TargetParams  = DioramaParams;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::BeginPlay()
{
    Super::BeginPlay();

    // If no PostProcessComponent is set explicitly, try to find one on the owner.
    if (!PostProcessComponent)
    {
        if (AActor* Owner = GetOwner())
        {
            PostProcessComponent = Owner->FindComponentByClass<UPostProcessComponent>();
        }
    }

    if (PostProcessComponent)
    {
        // Initial push.
        ApplyToPostProcess(PostProcessComponent);
    }
}

// ---------------------------------------------------------------------------
// TickComponent
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Tick sub-state machines.
    TickEventTighten(DeltaTime);
    TickRackFocus(DeltaTime);

    // Build effective target (preset + modifiers).
    FMXTiltShiftParams Effective = ApplyModifiers(TargetParams);

    // Smooth interpolation toward effective target.
    float Alpha = FMath::Clamp(DeltaTime * InterpSpeed, 0.0f, 1.0f);
    CurrentParams = FMXTiltShiftParams::Lerp(CurrentParams, Effective, Alpha);

    // Push to post-process.
    if (PostProcessComponent)
    {
        ApplyToPostProcess(PostProcessComponent);
    }
}

// ---------------------------------------------------------------------------
// SetPreset
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::SetPreset(ETiltShiftPreset Preset, float TransitionTime)
{
    if (CurrentPreset == Preset && !bTighteningForEvent) return;

    CurrentPreset = Preset;
    TargetParams  = GetPresetParams(Preset);

    // Override the interp speed temporarily if a specific transition time was given.
    // We express this as a one-frame alpha if TransitionTime ≈ 0, or use InterpSpeed
    // as the natural rate otherwise. (Full timeline control is in TickComponent.)
    // Designers can set InterpSpeed directly for finer control.
    // For instant transitions (TransitionTime == 0): snap CurrentParams directly.
    if (TransitionTime <= KINDA_SMALL_NUMBER)
    {
        CurrentParams = TargetParams;
    }

    if (bLogFocusUpdates)
    {
        UE_LOG(LogTemp, Log, TEXT("[TiltShift] Preset → %d (transition %.2fs)"),
               (int32)Preset, TransitionTime);
    }
}

// ---------------------------------------------------------------------------
// UpdateFocusTarget
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::UpdateFocusTarget(FVector WorldPosition)
{
    // Skip during a rack-focus — RackFocus owns the band center until it completes.
    if (bRackingFocus) return;

    float VertFraction = ProjectWorldToVerticalFraction(WorldPosition);

    // Gently nudge the target band center without overriding the current preset entirely.
    // This allows the band to follow the swarm without snapping abruptly.
    TargetParams.FocusBandCenter = FMath::Clamp(VertFraction, 0.1f, 0.9f);

    if (bLogFocusUpdates)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[TiltShift] FocusTarget vertical fraction: %.3f"), VertFraction);
    }
}

// ---------------------------------------------------------------------------
// ScaleToZoomLevel
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::ScaleToZoomLevel(float CameraHeight)
{
    // Don't override parameters during an active tighten-for-event.
    if (bTighteningForEvent) return;

    FMXTiltShiftParams ZoomParams = ComputeZoomDrivenParams(CameraHeight);

    // Only override blur-related fields — FocusBandCenter still tracks the swarm.
    TargetParams.FocusBandWidth  = ZoomParams.FocusBandWidth;
    TargetParams.BlurIntensity   = ZoomParams.BlurIntensity;
    TargetParams.BlurRadius      = ZoomParams.BlurRadius;
    TargetParams.SaturationBoost = ZoomParams.SaturationBoost;

    if (bLogFocusUpdates)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[TiltShift] ScaleToZoom height=%.0fcm → BandWidth=%.2f BlurIntensity=%.2f"),
               CameraHeight, ZoomParams.FocusBandWidth, ZoomParams.BlurIntensity);
    }
}

// ---------------------------------------------------------------------------
// TightenForEvent
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::TightenForEvent(FVector EventLocation, float Duration)
{
    if (!bTighteningForEvent)
    {
        PreEventPreset = CurrentPreset;
    }

    bTighteningForEvent       = true;
    EventTightenTimeRemaining = Duration;

    // Set Cinematic preset, but update band center to the event location.
    TargetParams                  = GetPresetParams(ETiltShiftPreset::Cinematic);
    TargetParams.FocusBandCenter  = FMath::Clamp(
        ProjectWorldToVerticalFraction(EventLocation), 0.1f, 0.9f);

    if (bLogFocusUpdates)
    {
        UE_LOG(LogTemp, Log, TEXT("[TiltShift] TightenForEvent duration=%.2fs"), Duration);
    }
}

// ---------------------------------------------------------------------------
// RackFocus
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::RackFocus(FVector FromWorld, FVector ToWorld, float Duration)
{
    bRackingFocus  = true;
    RackFromWorld  = FromWorld;
    RackToWorld    = ToWorld;
    RackDuration   = FMath::Max(Duration, KINDA_SMALL_NUMBER);
    RackElapsed    = 0.0f;

    if (bLogFocusUpdates)
    {
        UE_LOG(LogTemp, Log, TEXT("[TiltShift] RackFocus started duration=%.2fs"), Duration);
    }
}

// ---------------------------------------------------------------------------
// NotifySlowMotion
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::NotifySlowMotion(bool bSlowMotionActive, float DilationScale)
{
    if (bSlowMotionActive)
    {
        // Scale the bonus by how slow we are — peak at DilationScale ≈ 0.
        float SlowFactor = FMath::Clamp(1.0f - DilationScale, 0.0f, 1.0f);
        ActiveSlowMotionBonus = SlowMotionBlurBonus * SlowFactor;
    }
    else
    {
        ActiveSlowMotionBonus = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// NotifyFastForward
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::NotifyFastForward(bool bFastForwardActive)
{
    ActiveFastForwardBonus = bFastForwardActive ? FastForwardSatBonus : 0.0f;
}

// ---------------------------------------------------------------------------
// ApplyToPostProcess
// ---------------------------------------------------------------------------

void UMXTiltShiftEffect::ApplyToPostProcess(UPostProcessComponent* PPComp)
{
    if (!PPComp) return;

    // UE5 post-process settings are configured via FPostProcessSettings.
    // We map our logical parameters to the relevant UE5 fields:
    //
    //   BlurIntensity / FocusBandWidth → DepthOfField focus region + near/far blur amounts
    //   BlurRadius                    → DepthOfFieldFstop or GaussianDOFFarBlurSize
    //   ChromaticAberration           → SceneFringeIntensity
    //   Vignette                      → VignetteIntensity
    //   SaturationBoost               → ColorSaturation (via color grading)
    //
    // Tilt-shift in UE5 is best achieved through a custom post-process material
    // that reads FocusBandCenter and FocusBandWidth as scalar parameters.
    // We drive those material parameters here via dynamic material instances.
    //
    // The approach:
    //   - PPComp holds a UMaterialInstanceDynamic* for the tilt-shift material.
    //   - We call SetScalarParameterValue on it for band center/width/blur.
    //   - Standard UE5 post-process settings handle chromatic aberration and vignette.

    FPostProcessSettings& Settings = PPComp->Settings;

    // Chromatic aberration.
    Settings.bOverride_SceneFringeIntensity = true;
    Settings.SceneFringeIntensity = CurrentParams.ChromaticAberration * 5.0f; // UE scale: 0-5.

    // Vignette.
    Settings.bOverride_VignetteIntensity = true;
    Settings.VignetteIntensity = CurrentParams.Vignette;

    // Saturation boost via color grading (additive on top of 1.0 base).
    Settings.bOverride_ColorSaturation = true;
    // ColorSaturation is a FVector4 (RGBA). We boost all channels equally.
    float Sat = 1.0f + CurrentParams.SaturationBoost;
    Settings.ColorSaturation = FVector4(Sat, Sat, Sat, 1.0f);

    // Depth of field — use Gaussian DOF for the tilt-shift band.
    // Near/far blur amounts map to our BlurIntensity.
    Settings.bOverride_DepthOfFieldFocalDistance = true;
    Settings.DepthOfFieldFocalDistance = 0.0f; // Overridden by material; placeholder.

    Settings.bOverride_DepthOfFieldFarBlurSize = true;
    Settings.DepthOfFieldFarBlurSize = CurrentParams.BlurRadius * CurrentParams.BlurIntensity;

    Settings.bOverride_DepthOfFieldNearBlurSize = true;
    Settings.DepthOfFieldNearBlurSize = CurrentParams.BlurRadius * CurrentParams.BlurIntensity;

    // Custom tilt-shift material parameters (if a material is bound).
    // The material is responsible for the actual horizontal band mask.
    // Parameter names are conventional — the material artist defines these.
    for (FWeightedBlendable& Blendable : Settings.WeightedBlendables.Array)
    {
        if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Blendable.Object))
        {
            MID->SetScalarParameterValue(TEXT("TiltShift_BandCenter"),    CurrentParams.FocusBandCenter);
            MID->SetScalarParameterValue(TEXT("TiltShift_BandWidth"),     CurrentParams.FocusBandWidth);
            MID->SetScalarParameterValue(TEXT("TiltShift_BlurIntensity"), CurrentParams.BlurIntensity);
            MID->SetScalarParameterValue(TEXT("TiltShift_BlurRadius"),    CurrentParams.BlurRadius);
            MID->SetScalarParameterValue(TEXT("TiltShift_UseHexBokeh"),   CurrentParams.bHexagonalBokeh ? 1.0f : 0.0f);
        }
    }
}

// ===========================================================================
// Private Helpers
// ===========================================================================

FMXTiltShiftParams UMXTiltShiftEffect::GetPresetParams(ETiltShiftPreset Preset) const
{
    switch (Preset)
    {
        case ETiltShiftPreset::Diorama:   return DioramaParams;
        case ETiltShiftPreset::Cinematic: return CinematicParams;
        case ETiltShiftPreset::Wide:      return WideParams;
        case ETiltShiftPreset::Report:    return ReportParams;
        case ETiltShiftPreset::Off:       return OffParams;
        default:                          return DioramaParams;
    }
}

float UMXTiltShiftEffect::ProjectWorldToVerticalFraction(FVector WorldPosition) const
{
    UWorld* World = GetWorld();
    if (!World) return 0.5f;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC) return 0.5f;

    FVector2D ScreenPos;
    if (!PC->ProjectWorldLocationToScreen(WorldPosition, ScreenPos, true)) return 0.5f;

    int32 ViewportX, ViewportY;
    PC->GetViewportSize(ViewportX, ViewportY);

    if (ViewportY <= 0) return 0.5f;

    // Flip Y: UE screen space has 0 at top, 1 at bottom.
    return FMath::Clamp(ScreenPos.Y / static_cast<float>(ViewportY), 0.0f, 1.0f);
}

FMXTiltShiftParams UMXTiltShiftEffect::ComputeZoomDrivenParams(float CameraHeight) const
{
    // Normalize height within [MinCameraHeight, MaxCameraHeight].
    float Range = MaxCameraHeight - MinCameraHeight;
    float Alpha = (Range > KINDA_SMALL_NUMBER)
        ? FMath::Clamp((CameraHeight - MinCameraHeight) / Range, 0.0f, 1.0f)
        : 0.0f;

    // Alpha 0 = close (Wide preset feel), Alpha 1 = far (Diorama preset feel).
    return FMXTiltShiftParams::Lerp(WideParams, DioramaParams, Alpha);
}

FMXTiltShiftParams UMXTiltShiftEffect::ApplyModifiers(FMXTiltShiftParams Base) const
{
    Base.BlurIntensity  = FMath::Clamp(Base.BlurIntensity  + ActiveSlowMotionBonus,  0.0f, 1.0f);
    Base.SaturationBoost = FMath::Clamp(Base.SaturationBoost + ActiveFastForwardBonus, 0.0f, 0.25f);
    return Base;
}

void UMXTiltShiftEffect::TickRackFocus(float DeltaTime)
{
    if (!bRackingFocus) return;

    RackElapsed += DeltaTime;
    float Alpha = FMath::Clamp(RackElapsed / RackDuration, 0.0f, 1.0f);

    // Smooth step for a cinematic feel.
    float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

    float FromV = ProjectWorldToVerticalFraction(RackFromWorld);
    float ToV   = ProjectWorldToVerticalFraction(RackToWorld);

    TargetParams.FocusBandCenter = FMath::Lerp(FromV, ToV, SmoothedAlpha);

    if (Alpha >= 1.0f)
    {
        bRackingFocus = false;

        if (bLogFocusUpdates)
        {
            UE_LOG(LogTemp, Log, TEXT("[TiltShift] RackFocus complete."));
        }
    }
}

void UMXTiltShiftEffect::TickEventTighten(float DeltaTime)
{
    if (!bTighteningForEvent) return;

    EventTightenTimeRemaining -= DeltaTime;

    if (EventTightenTimeRemaining <= 0.0f)
    {
        bTighteningForEvent = false;
        // Revert to the preset we were in before the event.
        SetPreset(PreEventPreset);

        if (bLogFocusUpdates)
        {
            UE_LOG(LogTemp, Log, TEXT("[TiltShift] TightenForEvent complete. Reverting to preset %d."),
                   (int32)PreEventPreset);
        }
    }
}
