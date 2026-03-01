// MXTimeDilation.h — Camera Module v1.1
// Created: 2026-02-22
// Agent 7: Camera
// Manages time dilation (slow-motion and fast-forward) to pair with
// the tilt-shift effect and complete the "watching tiny miniatures" aesthetic.
//
// Auto-dilation is driven by camera events (Epic → 0.3×, Cinematic → 0.7×).
// Player-triggered dilation is always available regardless of camera mode.
// The two stack multiplicatively. All values are clamped to [0.1, 6.0].
//
// UI timers and score always use unscaled real time. Music pitch never changes.
// SFX pitch-shifts with dilation via GetAudioPitchMultiplier().
//
// Change Log:
//   v1.1 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXTimeDilation.generated.h"

// ---------------------------------------------------------------------------
// EMXDilationMode
// ---------------------------------------------------------------------------

/**
 * Named time dilation modes.
 * Auto modes are driven by camera events; Player modes are manually triggered.
 */
UENUM(BlueprintType)
enum class EMXDilationMode : uint8
{
    /** Default gameplay — no dilation. */
    Normal          UMETA(DisplayName = "Normal"),

    /** Triggered automatically on Epic camera events (deaths, sacrifices). 0.3×. */
    DramaticSlow    UMETA(DisplayName = "Dramatic Slow"),

    /** Triggered automatically on Cinematic camera events. 0.7×. */
    SubtleSlow      UMETA(DisplayName = "Subtle Slow"),

    /** Manual hold by player. 0.5×. */
    PlayerSlow      UMETA(DisplayName = "Player Slow"),

    /** Manual hold by player. 2.0×. */
    FastForward     UMETA(DisplayName = "Fast Forward"),

    /** Manual double-tap fast forward. 4.0×. */
    HyperSpeed      UMETA(DisplayName = "Hyper Speed"),

    /** Replay mode scrub speed. 0.15×. */
    ReplaySlow      UMETA(DisplayName = "Replay Slow"),
};

// ---------------------------------------------------------------------------
// UMXTimeDilation
// ---------------------------------------------------------------------------

/**
 * UMXTimeDilation
 * Actor component that owns all time dilation logic.
 * Attach to the same actor as UMXSwarmCamera.
 *
 * Each tick, ApplyDilation() calls UGameplayStatics::SetGlobalTimeDilation()
 * with the combined (auto × player) scale, smoothly ramped to avoid hard cuts.
 *
 * Also notifies UMXTiltShiftEffect of dilation state so blur and saturation
 * adjust accordingly.
 */
UCLASS(ClassGroup=(MicroExodus), BlueprintType, Blueprintable,
       meta=(BlueprintSpawnableComponent))
class MICROEXODUS_API UMXTimeDilation : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXTimeDilation();

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // =========================================================================
    // Mode Control
    // =========================================================================

    /**
     * Set a named dilation mode and smoothly ramp to its target scale.
     * For auto modes (DramaticSlow, SubtleSlow, ReplaySlow) prefer BeginAutoDilation()
     * which handles automatic reversion.
     * @param Mode  The dilation mode to apply.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TimeDilation")
    void SetDilation(EMXDilationMode Mode);

    /**
     * Return the current combined time scale (auto × player), post-clamp.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TimeDilation")
    float GetCurrentTimeScale() const;

    // =========================================================================
    // Auto Dilation (Camera-Driven)
    // =========================================================================

    /**
     * Begin an auto-dilation for the duration of a camera event.
     * The system ramps to TargetScale, holds for Duration seconds (real time),
     * then ramps back to 1.0×.
     *
     * Multiple calls while auto-dilation is active extend the hold time if the
     * new scale is lower (more dramatic) than the current auto scale.
     *
     * @param TargetScale  The dilation scale to ramp to (e.g., 0.3 for DramaticSlow).
     * @param Duration     How long to hold the dilation before easing back (real seconds).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TimeDilation")
    void BeginAutoDilation(float TargetScale, float Duration);

    /**
     * Immediately cancel the active auto-dilation and ramp back to 1.0×.
     * Called on level complete or camera mode change to Locked/Operator.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TimeDilation")
    void CancelAutoDilation();

    /**
     * Enable or disable auto-dilation. When disabled, camera events will not
     * trigger slow-motion. Per GDD: Operator and Locked modes disable auto-dilation.
     * @param bEnabled  True to allow auto-dilation; false to suppress it.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TimeDilation")
    void SetAutoDilationEnabled(bool bEnabled);

    /**
     * Return whether auto-dilation is currently suppressed.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TimeDilation")
    bool IsAutoDilationEnabled() const { return bAutoDilationEnabled; }

    /**
     * Return whether an auto-dilation is currently active (ramping or holding).
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TimeDilation")
    bool IsAutoDilationActive() const;

    // =========================================================================
    // Player Dilation
    // =========================================================================

    /**
     * Begin player-triggered dilation. Held until EndPlayerDilation() is called.
     * Always works regardless of camera mode or auto-dilation state.
     * @param Mode  One of: PlayerSlow, FastForward, HyperSpeed.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TimeDilation")
    void BeginPlayerDilation(EMXDilationMode Mode);

    /**
     * Release player dilation — ramp back to whatever the auto scale is (or 1.0×).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TimeDilation")
    void EndPlayerDilation();

    /**
     * Return whether the player is currently holding a dilation.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TimeDilation")
    bool IsPlayerDilationActive() const { return bPlayerDilationActive; }

    // =========================================================================
    // Audio
    // =========================================================================

    /**
     * Return the pitch multiplier SFX should use this frame.
     * Follows the combined time scale so effects feel appropriate at any speed.
     * Music always returns 1.0 — pitch-shift should never be applied to music.
     * @return  Pitch multiplier for SFX. Clamped to [0.1, 3.0].
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TimeDilation|Audio")
    float GetAudioPitchMultiplier() const;

    // =========================================================================
    // Application
    // =========================================================================

    /**
     * Apply the combined dilation scale to the world this tick.
     * Calls UGameplayStatics::SetGlobalTimeDilation() and notifies TiltShift.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TimeDilation")
    void ApplyDilation();

    // =========================================================================
    // References
    // =========================================================================

    /**
     * The tilt-shift effect component to notify of dilation state changes.
     * Set from the owning actor at BeginPlay or via Blueprint.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Refs")
    TObjectPtr<UMXTiltShiftEffect> TiltShiftEffect;

    // =========================================================================
    // Scale Table (designer-editable)
    // =========================================================================

    /** Scale for DramaticSlow (Epic events). Default 0.3×. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Scales")
    float DramaticSlowScale = 0.3f;

    /** Scale for SubtleSlow (Cinematic events). Default 0.7×. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Scales")
    float SubtleSlowScale = 0.7f;

    /** Scale for PlayerSlow. Default 0.5×. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Scales")
    float PlayerSlowScale = 0.5f;

    /** Scale for FastForward. Default 2.0×. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Scales")
    float FastForwardScale = 2.0f;

    /** Scale for HyperSpeed. Default 4.0×. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Scales")
    float HyperSpeedScale = 4.0f;

    /** Scale for ReplaySlow. Default 0.15×. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Scales")
    float ReplaySlowScale = 0.15f;

    /** Minimum allowed combined time scale. Per GDD: 0.1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Config")
    float MinScale = 0.1f;

    /** Maximum allowed combined time scale. Per GDD: 6.0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Config")
    float MaxScale = 6.0f;

    /** Ease-in ramp duration when entering a dilation (real seconds). Per GDD: 0.2s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Config")
    float RampInDuration = 0.2f;

    /** Ease-out ramp duration when leaving a dilation (real seconds). Per GDD: 0.3s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Config")
    float RampOutDuration = 0.3f;

    /** If true, log dilation changes to output. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TimeDilation|Debug")
    bool bLogDilation = false;

private:

    // -------------------------------------------------------------------------
    // Auto Dilation State Machine
    // -------------------------------------------------------------------------

    enum class EAutoDilationPhase : uint8
    {
        Inactive,
        RampingIn,
        Holding,
        RampingOut,
    };

    /** Current phase of the auto-dilation state machine. */
    EAutoDilationPhase AutoPhase = EAutoDilationPhase::Inactive;

    /** Target scale for the current auto-dilation. */
    float AutoTargetScale = 1.0f;

    /** Remaining hold time for the current auto-dilation (real seconds). */
    float AutoHoldTimeRemaining = 0.0f;

    /** Current ramp progress (0-1) for ease-in / ease-out. */
    float AutoRampProgress = 0.0f;

    /** Live auto scale value (interpolated, not yet combined with player). */
    float CurrentAutoScale = 1.0f;

    /** Whether auto-dilation is permitted (disabled in Operator/Locked modes). */
    bool bAutoDilationEnabled = true;

    // -------------------------------------------------------------------------
    // Player Dilation State
    // -------------------------------------------------------------------------

    /** Whether the player is holding a dilation. */
    bool bPlayerDilationActive = false;

    /** Target scale for player dilation. */
    float PlayerTargetScale = 1.0f;

    /** Live player scale value (interpolated). */
    float CurrentPlayerScale = 1.0f;

    // -------------------------------------------------------------------------
    // Composite Output
    // -------------------------------------------------------------------------

    /** The combined (auto × player) scale actually applied to the world. */
    float CompositeScale = 1.0f;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /** Convert an EMXDilationMode to its target float scale. */
    float ModeToScale(EMXDilationMode Mode) const;

    /** Tick the auto-dilation state machine using real (undilated) delta time. */
    void TickAutoDilation(float RealDeltaTime);

    /** Tick the player dilation ramp. */
    void TickPlayerDilation(float RealDeltaTime);

    /**
     * Compute the combined scale and clamp it.
     * Returns auto × player, clamped to [MinScale, MaxScale].
     */
    float ComputeComposite() const;

    /** Notify the TiltShift component of the current dilation state. */
    void NotifyTiltShift(float Scale);

    /**
     * Get real (unscaled) delta time from the World.
     * UE5: UWorld::GetDeltaSeconds() is already unscaled if you use
     * UGameplayStatics::GetRealTimeSeconds delta, but for tick-based logic
     * we use the component's own DeltaTime divided by the current global scale.
     */
    float GetRealDeltaTime(float ScaledDeltaTime) const;
};
