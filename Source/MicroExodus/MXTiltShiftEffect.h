// MXTiltShiftEffect.h — Camera Module v1.1
// Created: 2026-02-22
// Agent 7: Camera
// Drives the tilt-shift depth-of-field post-process effect that makes Micro Exodus
// look like a tabletop diorama of tiny painted miniatures. This is core visual
// identity — not an optional filter.
//
// The effect is a horizontal focus band: robots in the center of the vertical frame
// are sharp; everything above and below falls into hexagonal bokeh blur. The band
// tracks the swarm in screen space, tightens during cinematic events, and racks
// focus during the sacrifice hat-fall beat.
//
// Change Log:
//   v1.1 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PostProcessComponent.h"
#include "MXTiltShiftEffect.generated.h"

// ---------------------------------------------------------------------------
// ETiltShiftPreset
// ---------------------------------------------------------------------------

/**
 * Named presets for the tilt-shift system.
 * The camera transitions between these smoothly (default 0.5s).
 */
UENUM(BlueprintType)
enum class ETiltShiftPreset : uint8
{
    /**
     * Default gameplay look.
     * Wide-ish blur, strong miniature illusion.
     * Band 0.3, Blur 0.7, Sat +15%.
     */
    Diorama     UMETA(DisplayName = "Diorama"),

    /**
     * Used during Epic/Cinematic camera events.
     * Very tight focus band — draws all attention to the subject.
     * Band 0.15, Blur 0.85, Sat +10%.
     */
    Cinematic   UMETA(DisplayName = "Cinematic"),

    /**
     * Low swarm count / close zoom. Gentle miniature hint.
     * Band 0.6, Blur 0.2, Sat +5%.
     */
    Wide        UMETA(DisplayName = "Wide"),

    /**
     * Run Report snapshot look.
     * Band 0.25, Blur 0.8, Sat +15%.
     */
    Report      UMETA(DisplayName = "Report"),

    /**
     * Debug / accessibility mode. Effect fully disabled.
     * Band 1.0, Blur 0.0, Sat 0%.
     */
    Off         UMETA(DisplayName = "Off"),
};

// ---------------------------------------------------------------------------
// FMXTiltShiftParams
// ---------------------------------------------------------------------------

/**
 * FMXTiltShiftParams
 * All parameters that describe a tilt-shift state.
 * Interpolated each tick between CurrentParams and TargetParams.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXTiltShiftParams
{
    GENERATED_BODY()

    /**
     * Vertical screen-space center of the sharp focus band.
     * 0.0 = top of screen, 1.0 = bottom. Default 0.5 (center).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float FocusBandCenter = 0.5f;

    /**
     * Vertical extent of the sharp zone as a fraction of screen height.
     * 0.3 at max zoom (20m), 0.6 at close zoom (3m).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float FocusBandWidth = 0.3f;

    /**
     * Bokeh strength outside the focus band.
     * 0.7 at max zoom, 0.2 at close zoom.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float BlurIntensity = 0.7f;

    /**
     * Pixel radius of bokeh circles at 1080p. Scales with resolution automatically.
     * Default 8px.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="32.0"))
    float BlurRadius = 8.0f;

    /**
     * Subtle color fringing at blur edges — sells the real-lens look.
     * Default 0.1.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="0.3"))
    float ChromaticAberration = 0.1f;

    /**
     * Edge darkening vignette. Default 0.15.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="0.4"))
    float Vignette = 0.15f;

    /**
     * Additive saturation boost — makes colors pop like hand-painted miniatures.
     * Default 0.15 (+15%).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="0.25"))
    float SaturationBoost = 0.15f;

    /**
     * If true, bokeh circles are hexagonal (sells the real-lens look).
     * If false, circular bokeh (softer, more digital feel).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHexagonalBokeh = true;

    /** Linearly interpolate between two FMXTiltShiftParams. */
    static FMXTiltShiftParams Lerp(const FMXTiltShiftParams& A, const FMXTiltShiftParams& B, float Alpha);
};

// ---------------------------------------------------------------------------
// UMXTiltShiftEffect
// ---------------------------------------------------------------------------

/**
 * UMXTiltShiftEffect
 * Actor component that owns the tilt-shift miniature post-process effect.
 * Attach to the same actor as UMXSwarmCamera.
 *
 * Workflow:
 *   1. Wired at BeginPlay to a UPostProcessComponent on the camera rig.
 *   2. UMXSwarmCamera calls ScaleToZoomLevel() whenever arm length changes.
 *   3. UMXCameraBehaviors calls TightenForEvent() when Cinematic/Epic events fire.
 *   4. Each tick, CurrentParams interpolates toward TargetParams and is pushed
 *      to the post-process component via ApplyToPostProcess().
 */
UCLASS(ClassGroup=(MicroExodus), BlueprintType, Blueprintable,
       meta=(BlueprintSpawnableComponent))
class MICROEXODUS_API UMXTiltShiftEffect : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXTiltShiftEffect();

protected:

    virtual void BeginPlay() override;

public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // =========================================================================
    // Preset Control
    // =========================================================================

    /**
     * Transition to a named preset.
     * Parameters smoothly interpolate over TransitionTime seconds.
     * @param Preset         The preset to transition to.
     * @param TransitionTime Blend duration in seconds. Default 0.5s.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void SetPreset(ETiltShiftPreset Preset, float TransitionTime = 0.5f);

    /**
     * Return the currently active preset.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TiltShift")
    ETiltShiftPreset GetCurrentPreset() const { return CurrentPreset; }

    // =========================================================================
    // Focus Tracking
    // =========================================================================

    /**
     * Move the focus band to track a world-space position projected to screen space.
     * Called each tick by MXSwarmCamera with the swarm centroid.
     * @param WorldPosition  The world position to keep in the focus band.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void UpdateFocusTarget(FVector WorldPosition);

    /**
     * Auto-adjust blur intensity and band width based on the current camera arm height.
     * Called by MXSwarmCamera::UpdateZoom() whenever zoom changes.
     * @param CameraHeight  Current spring arm length in Unreal units (cm).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void ScaleToZoomLevel(float CameraHeight);

    // =========================================================================
    // Event-Driven Focus
    // =========================================================================

    /**
     * Narrow the focus band around an event location for the duration of a
     * cinematic camera event. Automatically reverts to previous state when done.
     * @param EventLocation  World position of the event subject.
     * @param Duration       How long to hold the tight focus.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void TightenForEvent(FVector EventLocation, float Duration);

    /**
     * Smooth focus rack from one world point to another.
     * Used during the sacrifice sequence to rack from the robot to the fallen hat.
     * @param FromWorld   Starting world focus position.
     * @param ToWorld     Ending world focus position.
     * @param Duration    Total rack duration in seconds.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void RackFocus(FVector FromWorld, FVector ToWorld, float Duration);

    // =========================================================================
    // Slow-Motion Adjustments
    // =========================================================================

    /**
     * Bump blur intensity up slightly during slow-motion playback.
     * Creates the "looking closely at a tiny miniature" feel.
     * @param bSlowMotionActive  True when time dilation is below 1.0×.
     * @param DilationScale      The current time dilation scalar (0.0–1.0).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void NotifySlowMotion(bool bSlowMotionActive, float DilationScale);

    /**
     * Adjust saturation boost during fast-forward (time-lapse diorama feel).
     * @param bFastForwardActive  True when time dilation is above 1.0×.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void NotifyFastForward(bool bFastForwardActive);

    // =========================================================================
    // Direct Parameter Access
    // =========================================================================

    /**
     * Return the current (live, interpolated) tilt-shift parameter set.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TiltShift")
    FMXTiltShiftParams GetCurrentParams() const { return CurrentParams; }

    /**
     * Push all current parameters to the given post-process component.
     * Called each tick internally — expose for Blueprint override scenarios.
     * @param PPComp  The post-process component to drive.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TiltShift")
    void ApplyToPostProcess(UPostProcessComponent* PPComp);

    // =========================================================================
    // Scene References
    // =========================================================================

    /**
     * The post-process component this effect drives.
     * Set from the owning camera rig actor at BeginPlay or via Blueprint.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Refs")
    TObjectPtr<UPostProcessComponent> PostProcessComponent;

    // =========================================================================
    // Preset Parameter Tables (editable by designers)
    // =========================================================================

    /** Parameters for the Diorama preset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Presets")
    FMXTiltShiftParams DioramaParams;

    /** Parameters for the Cinematic preset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Presets")
    FMXTiltShiftParams CinematicParams;

    /** Parameters for the Wide preset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Presets")
    FMXTiltShiftParams WideParams;

    /** Parameters for the Report preset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Presets")
    FMXTiltShiftParams ReportParams;

    /** Parameters for the Off preset (all zeros). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Presets")
    FMXTiltShiftParams OffParams;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * Minimum camera height (cm) — maps to Wide preset parameters.
     * Matches GDD zoom table lower bound (300cm = 3m).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Config")
    float MinCameraHeight = 300.0f;

    /**
     * Maximum camera height (cm) — maps to Diorama preset parameters.
     * Matches GDD zoom table upper bound (2000cm = 20m).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Config")
    float MaxCameraHeight = 2000.0f;

    /**
     * Blur intensity bonus applied during slow-motion. Default 0.1.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Config")
    float SlowMotionBlurBonus = 0.1f;

    /**
     * Saturation boost added during fast-forward. Default 0.05.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Config")
    float FastForwardSatBonus = 0.05f;

    /**
     * Interpolation speed for parameter blending. Higher = snappier transitions.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Config")
    float InterpSpeed = 6.0f;

    /** If true, log focus band updates to output log. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TiltShift|Debug")
    bool bLogFocusUpdates = false;

private:

    // -------------------------------------------------------------------------
    // Live Interpolation State
    // -------------------------------------------------------------------------

    /** Currently displayed parameters (interpolated each tick). */
    FMXTiltShiftParams CurrentParams;

    /** Parameters we are interpolating toward. */
    FMXTiltShiftParams TargetParams;

    /** The active preset (the named state we last transitioned to). */
    ETiltShiftPreset CurrentPreset = ETiltShiftPreset::Diorama;

    /** Remaining time on a tighten-for-event hold before reverting. */
    float EventTightenTimeRemaining = 0.0f;

    /** Whether we are currently holding a tightened focus for an event. */
    bool bTighteningForEvent = false;

    /** The preset we were in before TightenForEvent() was called. */
    ETiltShiftPreset PreEventPreset = ETiltShiftPreset::Diorama;

    // -------------------------------------------------------------------------
    // Rack Focus State
    // -------------------------------------------------------------------------

    /** Whether a rack-focus transition is currently active. */
    bool bRackingFocus = false;

    /** World-space source of the current rack. */
    FVector RackFromWorld = FVector::ZeroVector;

    /** World-space destination of the current rack. */
    FVector RackToWorld = FVector::ZeroVector;

    /** Total duration of the current rack-focus. */
    float RackDuration = 0.0f;

    /** Elapsed time on the current rack-focus. */
    float RackElapsed = 0.0f;

    // -------------------------------------------------------------------------
    // Slow-Motion / Fast-Forward Modifiers
    // -------------------------------------------------------------------------

    /** Active blur bonus from slow-motion (0 or SlowMotionBlurBonus). */
    float ActiveSlowMotionBonus = 0.0f;

    /** Active saturation bonus from fast-forward (0 or FastForwardSatBonus). */
    float ActiveFastForwardBonus = 0.0f;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /** Return the FMXTiltShiftParams struct for a named preset. */
    FMXTiltShiftParams GetPresetParams(ETiltShiftPreset Preset) const;

    /**
     * Project a world position to a screen-space vertical fraction (0=top, 1=bottom).
     * Returns 0.5 (center) if projection fails.
     */
    float ProjectWorldToVerticalFraction(FVector WorldPosition) const;

    /**
     * Compute zoom-driven parameters by linearly interpolating between Wide and Diorama
     * based on the camera height's position within [MinCameraHeight, MaxCameraHeight].
     */
    FMXTiltShiftParams ComputeZoomDrivenParams(float CameraHeight) const;

    /** Apply all active modifiers (slow-mo, fast-forward) on top of TargetParams. */
    FMXTiltShiftParams ApplyModifiers(FMXTiltShiftParams Base) const;

    /** Tick the rack-focus transition. */
    void TickRackFocus(float DeltaTime);

    /** Tick the tighten-for-event hold timer. */
    void TickEventTighten(float DeltaTime);
};
