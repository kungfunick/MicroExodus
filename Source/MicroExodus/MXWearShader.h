// MXWearShader.h — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Consumers: MXEvolutionLayerSystem (applies materials), MXEvolutionComponent (actor-side bridge)
// Change Log:
//   v1.0 — Initial implementation. Wraps UMaterialInstanceDynamic creation and parameter writes.
//           All material parameter name constants are defined here for consistency with M_RobotMaster.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MXEvolutionData.h"
#include "MXTypes.h"
#include "MXWearShader.generated.h"

// ---------------------------------------------------------------------------
// Material parameter name constants — must match M_RobotMaster parameter names exactly.
// ---------------------------------------------------------------------------
namespace MXMaterialParams
{
    // Wear / damage layers
    static const FName BurnIntensity       (TEXT("BurnIntensity"));
    static const FName CrackIntensity      (TEXT("CrackIntensity"));
    static const FName WeldCoverage        (TEXT("WeldCoverage"));
    static const FName PatinaIntensity     (TEXT("PatinaIntensity"));
    static const FName WearLevel           (TEXT("WearLevel"));         // float 0–4

    // Eye / expressive
    static const FName EyeBrightness       (TEXT("EyeBrightness"));
    static const FName EyeSomber           (TEXT("EyeSomber"));

    // Element-specific scars
    static const FName FallScarIntensity   (TEXT("FallScarIntensity"));
    static const FName IceResidue          (TEXT("IceResidue"));
    static const FName ElectricalBurn      (TEXT("ElectricalBurn"));

    // Staged cosmetics (passed as float indices)
    static const FName AntennaStage        (TEXT("AntennaStage"));      // float 0–4
    static const FName ArmorStage          (TEXT("ArmorStage"));        // float 0–2
    static const FName AuraStage           (TEXT("AuraStage"));         // float 0–3

    // Paint job
    static const FName PaintJobIndex       (TEXT("PaintJobIndex"));     // float 0–10

    // Flash effect (transient — cleared after flash duration)
    static const FName FlashIntensity      (TEXT("FlashIntensity"));
    static const FName FlashColor          (TEXT("FlashColor"));        // FLinearColor
}

// ---------------------------------------------------------------------------
// Flash color table per mark type — these are sensible defaults tunable by artists.
// ---------------------------------------------------------------------------
namespace MXFlashColors
{
    static const FLinearColor BurnMark      (1.0f, 0.35f, 0.0f, 1.0f);   // Orange-red
    static const FLinearColor ImpactCrack   (0.8f, 0.8f,  0.8f, 1.0f);   // White-grey
    static const FLinearColor WeldRepair    (1.0f, 0.9f,  0.2f, 1.0f);   // Bright yellow
    static const FLinearColor FrostResidue  (0.4f, 0.7f,  1.0f, 1.0f);   // Icy blue
    static const FLinearColor ElecBurn      (0.5f, 0.3f,  1.0f, 1.0f);   // Electric violet
    static const FLinearColor FallScar      (0.6f, 0.5f,  0.3f, 1.0f);   // Dusty brown
    static const FLinearColor Default       (1.0f, 1.0f,  1.0f, 1.0f);   // White fallback
}

/**
 * UMXWearShader
 *
 * Wraps all material instance creation and parameter writes for robot evolution visuals.
 * Does NOT own the material instance — that lifetime is managed by the robot actor.
 * All methods are stateless (they act on passed-in pointers) except for the active flash
 * timer table, which is kept here to avoid cross-frame parameter stomping.
 *
 * Thread safety: All calls must happen on the game thread (material API requirement).
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXWearShader : public UObject
{
    GENERATED_BODY()

public:

    // =========================================================================
    // Material Instance Lifecycle
    // =========================================================================

    /**
     * CreateMaterialInstance
     * Creates a per-robot UMaterialInstanceDynamic from the base M_RobotMaster material.
     * Sets the paint job base immediately. Evolution layers are applied separately via
     * SetWearParameters.
     *
     * The caller (robot actor or component) should store the returned MID and pass it back
     * in subsequent SetWearParameters calls. This class does not cache it.
     *
     * @param RobotActor   The actor that will own the MID (outer for GC purposes).
     * @param BaseMaterial The M_RobotMaster material asset to instantiate from.
     * @param PaintJob     The paint job to apply as the base chassis color.
     * @return             The newly created MID, or nullptr if BaseMaterial is invalid.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution|Shader")
    static UMaterialInstanceDynamic* CreateMaterialInstance(
        AActor* RobotActor,
        UMaterialInterface* BaseMaterial,
        EPaintJob PaintJob
    );

    // =========================================================================
    // Parameter Application
    // =========================================================================

    /**
     * SetWearParameters
     * Writes all evolution layer intensities and staged cosmetic indices to the MID.
     * Safe to call every frame (no allocation, just scalar writes).
     * Does not touch FlashIntensity/FlashColor — those are managed by TriggerVisualMarkFlash.
     *
     * @param Mat    The robot's active MID. No-op if nullptr.
     * @param State  The pre-computed FMXVisualEvolutionState.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution|Shader")
    static void SetWearParameters(UMaterialInstanceDynamic* Mat, const FMXVisualEvolutionState& State);

    /**
     * SetPaintJobBase
     * Writes the PaintJobIndex float parameter to the material.
     * Called once on material creation and again if paint job changes.
     *
     * @param Mat      The robot's MID.
     * @param PaintJob The new paint job enum to apply.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution|Shader")
    static void SetPaintJobBase(UMaterialInstanceDynamic* Mat, EPaintJob PaintJob);

    // =========================================================================
    // Flash Effect
    // =========================================================================

    /**
     * TriggerVisualMarkFlash
     * Sets FlashIntensity to 1.0 and FlashColor for the given mark type on the material,
     * then schedules a Fadeout timer to clear it after Duration seconds.
     * The fade is accomplished by queuing a TimerDelegate on the world timer manager.
     *
     * If another flash is in progress it is overwritten (most recent event wins).
     *
     * @param Mat        The robot's MID. No-op if nullptr.
     * @param WorldCtx   Any UObject* with a valid World (for the timer manager).
     * @param MarkType   One of: "burn_mark", "impact_crack", "weld_repair", "frost_residue",
     *                   "elec_burn", "fall_scar". Unknown marks get the white fallback.
     * @param Duration   How long the flash stays fully bright before fading. Default 0.5s.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution|Shader")
    static void TriggerVisualMarkFlash(
        UMaterialInstanceDynamic* Mat,
        UObject* WorldCtx,
        const FString& MarkType,
        float Duration = 0.5f
    );

    // =========================================================================
    // Helpers
    // =========================================================================

    /**
     * GetPaintJobIndex
     * Converts EPaintJob enum to a float suitable for the PaintJobIndex material parameter.
     * Maps: None→0, MatteBlack→1, Chrome→2, … PureGold→10.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Shader")
    static float GetPaintJobIndex(EPaintJob PaintJob);

    /**
     * GetFlashColorForMark
     * Returns the FLinearColor for a given mark type string.
     * Unknown strings return MXFlashColors::Default (white).
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Shader")
    static FLinearColor GetFlashColorForMark(const FString& MarkType);

private:

    /**
     * ClearFlash
     * Resets FlashIntensity to 0.0 on the given material.
     * Called by the timer delegate registered in TriggerVisualMarkFlash.
     * Passed as a bound lambda — does not need to be a UFUNCTION.
     */
    static void ClearFlash(TWeakObjectPtr<UMaterialInstanceDynamic> WeakMat);
};
