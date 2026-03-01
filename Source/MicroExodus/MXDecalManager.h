// MXDecalManager.h — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Consumers: MXEvolutionLayerSystem (applies decals after recalculation)
// Change Log:
//   v1.0 — Initial implementation. Deterministic seeded decal placement from visual evolution state.

#pragma once

#include "CoreMinimal.h"
#include "MXEvolutionData.h"
#include "MXDecalManager.generated.h"

// ---------------------------------------------------------------------------
// FMXDecalPlacement — a single procedurally placed surface mark.
// ---------------------------------------------------------------------------

/**
 * FMXDecalPlacement
 * Describes one decal spawn on the robot's mesh surface.
 * Generated deterministically from decal_seed + evolution intensities.
 * Not persisted — regenerated from stats on each recalculation.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXDecalPlacement
{
    GENERATED_BODY()

    /**
     * The visual type of this decal.
     * Valid values: "burn", "crack", "weld", "scratch", "frost", "elec_burn", "fall_gouge", "patina"
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DecalType;

    /**
     * Position on the robot mesh in local (object) space.
     * Derived from seeded scatter within hull bounds.
     * Shaders interpret this as a UV-space or bone-relative offset.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector LocalPosition = FVector::ZeroVector;

    /** Rotation of the decal in local space. Controls crack direction, burn smear angle, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator LocalRotation = FRotator::ZeroRotator;

    /** Uniform scale. Larger decals are placed at higher intensities. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 1.0f;

    /** Opacity [0, 1]. Driven by the corresponding intensity value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Opacity = 1.0f;
};

// ---------------------------------------------------------------------------
// Per-layer decal budgets — tunable constants.
// ---------------------------------------------------------------------------
namespace MXDecalBudgets
{
    static constexpr int32 MAX_BURN_DECALS   = 8;   // Max soot/char patches per robot
    static constexpr int32 MAX_CRACK_DECALS  = 6;   // Max crack lines
    static constexpr int32 MAX_WELD_DECALS   = 6;   // Max weld seam segments
    static constexpr int32 MAX_SCRATCH_DECALS= 10;  // Max general scratch marks
    static constexpr int32 MAX_FROST_DECALS  = 5;   // Max frost patches
    static constexpr int32 MAX_ELEC_DECALS   = 5;   // Max electrical scorch traces
    static constexpr int32 MAX_FALL_DECALS   = 5;   // Max fall gouge marks
    static constexpr int32 MAX_PATINA_DECALS = 4;   // Max rust spots
}

/**
 * UMXDecalManager
 *
 * Produces and applies procedural surface decals to robot actors.
 * All placement is deterministic: given the same decal_seed and FMXVisualEvolutionState,
 * GenerateDecalPlacements always returns the same TArray. Different robots with identical
 * stats look different because their seeds differ.
 *
 * The manager does NOT hold references to spawned decal components — it queries the actor
 * for existing tagged components in ApplyDecals and ClearDecals. Robot Blueprint actors
 * should tag their DecalComponent instances with "MXEvolutionDecal" for this to function.
 *
 * Thread safety: GenerateDecalPlacements is pure and can be called off the game thread.
 *               ApplyDecals / ClearDecals must be called on the game thread.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXDecalManager : public UObject
{
    GENERATED_BODY()

public:

    // =========================================================================
    // Decal Generation (pure / deterministic)
    // =========================================================================

    /**
     * GenerateDecalPlacements
     * Produces the full decal placement array for a robot based on its current
     * visual evolution state and birth seed. Every call with the same inputs returns
     * identical results — no random state used beyond the seed.
     *
     * Placement count per type scales linearly with intensity via GetDecalCountForIntensity.
     *
     * @param DecalSeed  The robot's decal_seed (name hash XOR birth timestamp hash).
     * @param State      The robot's current FMXVisualEvolutionState (all intensities).
     * @return           Ordered TArray of FMXDecalPlacement ready for ApplyDecals.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution|Decals")
    static TArray<FMXDecalPlacement> GenerateDecalPlacements(
        int32                          DecalSeed,
        const FMXVisualEvolutionState& State
    );

    /**
     * GetDecalCountForIntensity
     * Maps a normalized intensity [0, 1] and a per-type maximum to a decal count.
     * Uses a sqrt curve so even low-intensity robots show at least 1 decal.
     *
     * count = FMath::RoundToInt(sqrt(Intensity) * MaxCount)
     *
     * @param Intensity  Normalized intensity value [0, 1].
     * @param MaxCount   The per-type maximum decal budget.
     * @return           Integer decal count [0, MaxCount].
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Evolution|Decals")
    static int32 GetDecalCountForIntensity(float Intensity, int32 MaxCount);

    // =========================================================================
    // Decal Application / Cleanup (game thread only)
    // =========================================================================

    /**
     * ApplyDecals
     * Spawns or updates UDecalComponent instances on the robot actor to match the
     * provided placement array. Existing tagged components are recycled before new
     * ones are spawned. Component tag used: "MXEvolutionDecal".
     *
     * In a full implementation, decal material assets would be looked up from a
     * DataAsset by DecalType string. For now, component placement and scaling are
     * authoritative — material assignment is a Blueprint post-process step.
     *
     * @param RobotActor   The robot actor to decorate.
     * @param Placements   The placement array from GenerateDecalPlacements.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution|Decals")
    static void ApplyDecals(AActor* RobotActor, const TArray<FMXDecalPlacement>& Placements);

    /**
     * ClearDecals
     * Removes all UDecalComponents tagged "MXEvolutionDecal" from the robot actor.
     * Used for reset (new run preview, level editor mode, clean UI preview).
     *
     * @param RobotActor  The actor to strip of evolution decals.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution|Decals")
    static void ClearDecals(AActor* RobotActor);

private:

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    /**
     * GenerateLayerPlacements
     * Generates placements for a single decal type layer.
     *
     * @param InOutSeed   Seed state — modified in-place to advance the sequence.
     * @param DecalType   Type string ("burn", "crack", etc.).
     * @param Count       Number of decals to place.
     * @param BaseOpacity The intensity value to use as the base opacity.
     * @param OutArray    Destination array — placements are appended.
     */
    static void GenerateLayerPlacements(
        int32&                    InOutSeed,
        const FString&            DecalType,
        int32                     Count,
        float                     BaseOpacity,
        TArray<FMXDecalPlacement>& OutArray
    );

    /**
     * SeededRandFloat
     * Generates a float in [0, 1] from a seed integer using a fast LCG step.
     * Advances Seed in-place. Used instead of FRandomStream to keep it branchless.
     */
    FORCEINLINE static float SeededRandFloat(int32& Seed)
    {
        // Knuth LCG: multiplier 1664525, increment 1013904223
        Seed = static_cast<int32>(static_cast<uint32>(Seed) * 1664525u + 1013904223u);
        // Map signed int to [0, 1]
        return static_cast<float>(static_cast<uint32>(Seed)) / static_cast<float>(0xFFFFFFFF);
    }

    /**
     * SeededRandInRange
     * Returns a float in [Min, Max] using SeededRandFloat.
     */
    FORCEINLINE static float SeededRandInRange(int32& Seed, float Min, float Max)
    {
        return Min + SeededRandFloat(Seed) * (Max - Min);
    }

    // Approximate local space bounds for a generic robot chassis (unit-scale, centered at origin).
    // Actual bounds come from the mesh, but these drive decal scatter without needing mesh access.
    static constexpr float CHASSIS_HALF_EXTENT_X = 30.0f;  // cm
    static constexpr float CHASSIS_HALF_EXTENT_Y = 20.0f;  // cm
    static constexpr float CHASSIS_HALF_EXTENT_Z = 40.0f;  // cm (tall-ish bipedal form)
};
