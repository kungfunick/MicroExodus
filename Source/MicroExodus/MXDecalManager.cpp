// MXDecalManager.cpp — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Change Log:
//   v1.0 — Initial implementation. Deterministic LCG-seeded decal placement.
//           ApplyDecals uses a recycle-first strategy to minimise component churn.

#include "MXDecalManager.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

// Tag applied to all evolution-owned DecalComponents so we can find and recycle them.
static const FName EVOLUTION_DECAL_TAG(TEXT("MXEvolutionDecal"));

// =========================================================================
// Decal Generation (pure / deterministic)
// =========================================================================

TArray<FMXDecalPlacement> UMXDecalManager::GenerateDecalPlacements(
    int32                          DecalSeed,
    const FMXVisualEvolutionState& State)
{
    TArray<FMXDecalPlacement> Placements;
    // Pre-reserve a reasonable upper bound to avoid re-allocations.
    Placements.Reserve(
        MXDecalBudgets::MAX_BURN_DECALS   +
        MXDecalBudgets::MAX_CRACK_DECALS  +
        MXDecalBudgets::MAX_WELD_DECALS   +
        MXDecalBudgets::MAX_SCRATCH_DECALS+
        MXDecalBudgets::MAX_FROST_DECALS  +
        MXDecalBudgets::MAX_ELEC_DECALS   +
        MXDecalBudgets::MAX_FALL_DECALS   +
        MXDecalBudgets::MAX_PATINA_DECALS
    );

    // Each layer gets a deterministically offset seed so placements don't cluster.
    // We XOR the base seed with a per-layer constant to get independent sequences.

    // Layer 2: Burn marks
    if (State.burn_intensity > 0.0f)
    {
        int32 LayerSeed = DecalSeed ^ 0xBEEF0001;
        const int32 Count = GetDecalCountForIntensity(State.burn_intensity, MXDecalBudgets::MAX_BURN_DECALS);
        GenerateLayerPlacements(LayerSeed, TEXT("burn"), Count, State.burn_intensity, Placements);
    }

    // Layer 3: Impact Cracks
    if (State.crack_intensity > 0.0f)
    {
        int32 LayerSeed = DecalSeed ^ 0xCAFE0002;
        const int32 Count = GetDecalCountForIntensity(State.crack_intensity, MXDecalBudgets::MAX_CRACK_DECALS);
        GenerateLayerPlacements(LayerSeed, TEXT("crack"), Count, State.crack_intensity, Placements);
    }

    // Layer 4: Weld Repairs
    if (State.weld_coverage > 0.0f)
    {
        int32 LayerSeed = DecalSeed ^ 0xDEAD0003;
        const int32 Count = GetDecalCountForIntensity(State.weld_coverage, MXDecalBudgets::MAX_WELD_DECALS);
        GenerateLayerPlacements(LayerSeed, TEXT("weld"), Count, State.weld_coverage, Placements);
    }

    // Layer 1: Scratches — scaled by wear level (0 scratches at Fresh)
    {
        const float WearFloat = static_cast<float>(static_cast<uint8>(State.wear_level)) / 4.0f;
        if (WearFloat > 0.0f)
        {
            int32 LayerSeed = DecalSeed ^ 0xFACE0004;
            const int32 Count = GetDecalCountForIntensity(WearFloat, MXDecalBudgets::MAX_SCRATCH_DECALS);
            GenerateLayerPlacements(LayerSeed, TEXT("scratch"), Count, WearFloat, Placements);
        }
    }

    // Layer 5: Patina rust spots
    if (State.patina_intensity > 0.0f)
    {
        int32 LayerSeed = DecalSeed ^ 0xACE00005;
        const int32 Count = GetDecalCountForIntensity(State.patina_intensity, MXDecalBudgets::MAX_PATINA_DECALS);
        GenerateLayerPlacements(LayerSeed, TEXT("patina"), Count, State.patina_intensity, Placements);
    }

    // Layer 10: Fall gouges
    if (State.fall_scar_intensity > 0.0f)
    {
        int32 LayerSeed = DecalSeed ^ 0xF00D0006;
        const int32 Count = GetDecalCountForIntensity(State.fall_scar_intensity, MXDecalBudgets::MAX_FALL_DECALS);
        GenerateLayerPlacements(LayerSeed, TEXT("fall_gouge"), Count, State.fall_scar_intensity, Placements);
    }

    // Layer 11: Frost
    if (State.ice_residue > 0.0f)
    {
        int32 LayerSeed = DecalSeed ^ 0xB0BA0007;
        const int32 Count = GetDecalCountForIntensity(State.ice_residue, MXDecalBudgets::MAX_FROST_DECALS);
        GenerateLayerPlacements(LayerSeed, TEXT("frost"), Count, State.ice_residue, Placements);
    }

    // Layer 12: Electrical burns
    if (State.electrical_burn > 0.0f)
    {
        int32 LayerSeed = DecalSeed ^ 0xC0DE0008;
        const int32 Count = GetDecalCountForIntensity(State.electrical_burn, MXDecalBudgets::MAX_ELEC_DECALS);
        GenerateLayerPlacements(LayerSeed, TEXT("elec_burn"), Count, State.electrical_burn, Placements);
    }

    return Placements;
}

int32 UMXDecalManager::GetDecalCountForIntensity(float Intensity, int32 MaxCount)
{
    if (Intensity <= 0.0f || MaxCount <= 0)
    {
        return 0;
    }
    // sqrt curve: low intensity still shows something, high intensity fills the budget.
    const float SqrtIntensity = FMath::Sqrt(FMath::Clamp(Intensity, 0.0f, 1.0f));
    return FMath::RoundToInt(SqrtIntensity * static_cast<float>(MaxCount));
}

// =========================================================================
// Decal Application / Cleanup (game thread only)
// =========================================================================

void UMXDecalManager::ApplyDecals(AActor* RobotActor, const TArray<FMXDecalPlacement>& Placements)
{
    if (!IsValid(RobotActor))
    {
        return;
    }

    // Collect existing evolution-owned decal components for recycling.
    TArray<UDecalComponent*> ExistingDecals;
    RobotActor->GetComponents<UDecalComponent>(ExistingDecals);
    // Filter to only Evolution-tagged ones.
    ExistingDecals = ExistingDecals.FilterByPredicate([](const UDecalComponent* DC)
    {
        return IsValid(DC) && DC->ComponentHasTag(EVOLUTION_DECAL_TAG);
    });

    int32 RecycleIdx = 0;

    for (const FMXDecalPlacement& P : Placements)
    {
        UDecalComponent* Decal = nullptr;

        if (ExistingDecals.IsValidIndex(RecycleIdx))
        {
            // Recycle an existing component — cheaper than spawn/destroy.
            Decal = ExistingDecals[RecycleIdx];
            ++RecycleIdx;
        }
        else
        {
            // Need a new component.
            Decal = NewObject<UDecalComponent>(RobotActor, UDecalComponent::StaticClass());
            Decal->ComponentTags.Add(EVOLUTION_DECAL_TAG);
            Decal->SetupAttachment(RobotActor->GetRootComponent());
            Decal->RegisterComponent();
        }

        if (!IsValid(Decal))
        {
            continue;
        }

        // Set transform in local space. DecalComponent uses world space — convert if needed.
        // For now we apply relative to the root component at the specified local position.
        Decal->SetRelativeLocation(P.LocalPosition);
        Decal->SetRelativeRotation(P.LocalRotation);
        Decal->SetRelativeScale3D(FVector(P.Scale));
        // Store the decal type as a component tag for the Blueprint to map to a material.
        // We replace any existing type tag by using a naming convention "MXDecalType:[type]".
        Decal->ComponentTags.RemoveAll([](const FName& Tag)
        {
            return Tag.ToString().StartsWith(TEXT("MXDecalType:"));
        });
        Decal->ComponentTags.Add(FName(*(FString(TEXT("MXDecalType:")) + P.DecalType)));
        // Opacity is stored in the decal's FadeScreenSize as a 0–1 proxy.
        // Real opacity would be driven by a material parameter on the decal material.
        Decal->FadeScreenSize = P.Opacity;
        Decal->SetVisibility(true);
    }

    // Hide any recycled components we didn't use (fewer placements than before).
    for (int32 i = RecycleIdx; i < ExistingDecals.Num(); ++i)
    {
        if (IsValid(ExistingDecals[i]))
        {
            ExistingDecals[i]->SetVisibility(false);
        }
    }
}

void UMXDecalManager::ClearDecals(AActor* RobotActor)
{
    if (!IsValid(RobotActor))
    {
        return;
    }

    TArray<UDecalComponent*> AllDecals;
    RobotActor->GetComponents<UDecalComponent>(AllDecals);

    for (UDecalComponent* DC : AllDecals)
    {
        if (IsValid(DC) && DC->ComponentHasTag(EVOLUTION_DECAL_TAG))
        {
            DC->DestroyComponent();
        }
    }
}

// =========================================================================
// Internal helpers
// =========================================================================

/*static*/ void UMXDecalManager::GenerateLayerPlacements(
    int32&                    InOutSeed,
    const FString&            DecalType,
    int32                     Count,
    float                     BaseOpacity,
    TArray<FMXDecalPlacement>& OutArray)
{
    for (int32 i = 0; i < Count; ++i)
    {
        FMXDecalPlacement P;
        P.DecalType = DecalType;

        // Scatter position within the chassis bounding box.
        P.LocalPosition = FVector(
            SeededRandInRange(InOutSeed, -CHASSIS_HALF_EXTENT_X, CHASSIS_HALF_EXTENT_X),
            SeededRandInRange(InOutSeed, -CHASSIS_HALF_EXTENT_Y, CHASSIS_HALF_EXTENT_Y),
            SeededRandInRange(InOutSeed, -CHASSIS_HALF_EXTENT_Z, CHASSIS_HALF_EXTENT_Z)
        );

        // Random rotation around all axes — cracks and burns have no preferred orientation.
        P.LocalRotation = FRotator(
            SeededRandInRange(InOutSeed, -180.0f, 180.0f),
            SeededRandInRange(InOutSeed, -180.0f, 180.0f),
            SeededRandInRange(InOutSeed,  -90.0f,  90.0f)
        );

        // Scale: base 1.0, randomised ±40%, then boosted by intensity.
        const float SizeJitter = SeededRandInRange(InOutSeed, 0.6f, 1.4f);
        P.Scale   = SizeJitter * (0.5f + BaseOpacity * 0.5f); // more intense = larger decals

        // Opacity: base intensity with per-decal jitter so they aren't all the same.
        const float OpacityJitter = SeededRandInRange(InOutSeed, 0.7f, 1.0f);
        P.Opacity = FMath::Clamp(BaseOpacity * OpacityJitter, 0.0f, 1.0f);

        OutArray.Add(MoveTemp(P));
    }
}
