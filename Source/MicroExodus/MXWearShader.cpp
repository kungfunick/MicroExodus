// MXWearShader.cpp — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Change Log:
//   v1.0 — Initial implementation.

#include "MXWearShader.h"
#include "MXEvolutionCalculator.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =========================================================================
// Material Instance Lifecycle
// =========================================================================

UMaterialInstanceDynamic* UMXWearShader::CreateMaterialInstance(
    AActor*             RobotActor,
    UMaterialInterface* BaseMaterial,
    EPaintJob           PaintJob)
{
    if (!IsValid(RobotActor) || !IsValid(BaseMaterial))
    {
        UE_LOG(LogTemp, Warning, TEXT("MXWearShader::CreateMaterialInstance — invalid RobotActor or BaseMaterial."));
        return nullptr;
    }

    UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, RobotActor);
    if (!IsValid(MID))
    {
        UE_LOG(LogTemp, Warning, TEXT("MXWearShader::CreateMaterialInstance — MID creation failed."));
        return nullptr;
    }

    // Apply the paint job base immediately so the robot never shows with wrong base color.
    SetPaintJobBase(MID, PaintJob);

    // Zero out flash parameters so there is no stray glow on a fresh instance.
    MID->SetScalarParameterValue(MXMaterialParams::FlashIntensity, 0.0f);
    MID->SetVectorParameterValue(MXMaterialParams::FlashColor, FLinearColor::White);

    UE_LOG(LogTemp, Log, TEXT("MXWearShader: Created MID for actor '%s' with paint job index %.0f."),
           *RobotActor->GetName(), GetPaintJobIndex(PaintJob));

    return MID;
}

// =========================================================================
// Parameter Application
// =========================================================================

void UMXWearShader::SetWearParameters(UMaterialInstanceDynamic* Mat, const FMXVisualEvolutionState& State)
{
    if (!IsValid(Mat))
    {
        return;
    }

    // --- Wear layer ---
    Mat->SetScalarParameterValue(MXMaterialParams::WearLevel,
                                 UMXEvolutionCalculator::GetWearLevelAsFloat(State.wear_level));

    // --- Encounter-driven intensities ---
    Mat->SetScalarParameterValue(MXMaterialParams::BurnIntensity,     State.burn_intensity);
    Mat->SetScalarParameterValue(MXMaterialParams::CrackIntensity,    State.crack_intensity);
    Mat->SetScalarParameterValue(MXMaterialParams::WeldCoverage,      State.weld_coverage);
    Mat->SetScalarParameterValue(MXMaterialParams::PatinaIntensity,   State.patina_intensity);

    // --- Eye parameters ---
    Mat->SetScalarParameterValue(MXMaterialParams::EyeBrightness,     State.eye_brightness);
    Mat->SetScalarParameterValue(MXMaterialParams::EyeSomber,         State.eye_somber);

    // --- Element-specific scars ---
    Mat->SetScalarParameterValue(MXMaterialParams::FallScarIntensity, State.fall_scar_intensity);
    Mat->SetScalarParameterValue(MXMaterialParams::IceResidue,        State.ice_residue);
    Mat->SetScalarParameterValue(MXMaterialParams::ElectricalBurn,    State.electrical_burn);

    // --- Staged cosmetics (passed as float indices) ---
    Mat->SetScalarParameterValue(MXMaterialParams::AntennaStage,      static_cast<float>(State.antenna_stage));
    Mat->SetScalarParameterValue(MXMaterialParams::ArmorStage,        static_cast<float>(State.armor_stage));
    Mat->SetScalarParameterValue(MXMaterialParams::AuraStage,         static_cast<float>(State.aura_stage));
}

void UMXWearShader::SetPaintJobBase(UMaterialInstanceDynamic* Mat, EPaintJob PaintJob)
{
    if (!IsValid(Mat))
    {
        return;
    }

    Mat->SetScalarParameterValue(MXMaterialParams::PaintJobIndex, GetPaintJobIndex(PaintJob));
}

// =========================================================================
// Flash Effect
// =========================================================================

void UMXWearShader::TriggerVisualMarkFlash(
    UMaterialInstanceDynamic* Mat,
    UObject*                  WorldCtx,
    const FString&            MarkType,
    float                     Duration)
{
    if (!IsValid(Mat) || !IsValid(WorldCtx))
    {
        return;
    }

    UWorld* World = WorldCtx->GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogTemp, Warning, TEXT("MXWearShader::TriggerVisualMarkFlash — no valid world from context."));
        return;
    }

    // Set flash parameters on the material immediately.
    const FLinearColor FlashColor = GetFlashColorForMark(MarkType);
    Mat->SetScalarParameterValue(MXMaterialParams::FlashIntensity, 1.0f);
    Mat->SetVectorParameterValue(MXMaterialParams::FlashColor, FlashColor);

    // Schedule clear after Duration seconds. Uses a weak pointer to avoid keeping MID alive.
    TWeakObjectPtr<UMaterialInstanceDynamic> WeakMat(Mat);
    FTimerHandle TimerHandle; // discarded — we don't need to cancel individual flashes
    World->GetTimerManager().SetTimer(
        TimerHandle,
        FTimerDelegate::CreateLambda([WeakMat]()
        {
            ClearFlash(WeakMat);
        }),
        FMath::Max(Duration, 0.05f),
        /*bLoop=*/false
    );
}

// =========================================================================
// Helpers
// =========================================================================

float UMXWearShader::GetPaintJobIndex(EPaintJob PaintJob)
{
    // EPaintJob enum values map 1:1 to float index (None=0, MatteBlack=1, … PureGold=10).
    return static_cast<float>(static_cast<uint8>(PaintJob));
}

FLinearColor UMXWearShader::GetFlashColorForMark(const FString& MarkType)
{
    if (MarkType == TEXT("burn_mark"))      return MXFlashColors::BurnMark;
    if (MarkType == TEXT("impact_crack"))   return MXFlashColors::ImpactCrack;
    if (MarkType == TEXT("weld_repair"))    return MXFlashColors::WeldRepair;
    if (MarkType == TEXT("frost_residue"))  return MXFlashColors::FrostResidue;
    if (MarkType == TEXT("elec_burn"))      return MXFlashColors::ElecBurn;
    if (MarkType == TEXT("fall_scar"))      return MXFlashColors::FallScar;
    return MXFlashColors::Default;
}

// =========================================================================
// Private
// =========================================================================

/*static*/ void UMXWearShader::ClearFlash(TWeakObjectPtr<UMaterialInstanceDynamic> WeakMat)
{
    if (UMaterialInstanceDynamic* Mat = WeakMat.Get())
    {
        Mat->SetScalarParameterValue(MXMaterialParams::FlashIntensity, 0.0f);
    }
}
