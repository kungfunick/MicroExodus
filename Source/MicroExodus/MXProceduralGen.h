// MXProceduralGen.h — Procedural Generation Module v1.0
// Created: 2026-03-01
// Agent 12: Procedural Generation
// Master orchestrator for procedural level content generation.
// Takes a seed + tier + modifiers, produces a complete FMXRunLayout
// describing all 20 levels. Deterministic: same inputs = same output.
//
// Integration point: MXRunManager calls GenerateRunLayout at StartRun
// and queries GetLevelLayout during AdvanceLevel.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXProceduralData.h"
#include "MXRoomGenerator.h"
#include "MXHazardPlacer.h"
#include "MXConstants.h"
#include "MXProceduralGen.generated.h"

// Forward declaration
class UDataTable;

// ---------------------------------------------------------------------------
// UMXProceduralGen
// ---------------------------------------------------------------------------

/**
 * UMXProceduralGen
 * Top-level procedural generation coordinator.
 *
 * Owns the room generator, hazard placer, and pacing curve.
 * Produces a complete FMXRunLayout in one deterministic pass.
 *
 * Pacing philosophy (the "documentary arc"):
 *   Levels  1–3  : Introduction. Low density, common elements, sanctuaries.
 *   Levels  4–7  : Rising tension. Mixed elements, first bottlenecks.
 *   Levels  8–12 : Mid-run crisis. High density, first gauntlets, gate at 10.
 *   Levels 13–16 : Escalation. Dense arenas, rare elements appear, gate at 15.
 *   Levels 17–19 : Climax build. Maximum density, complex layouts.
 *   Level  20    : Finale. Gauntlet-heavy, final gate, peak threat.
 *
 * Usage:
 *   GameInstance::Init → CreateDefaultSubobject<UMXProceduralGen>
 *   RunManager::StartRun → ProceduralGen->GenerateRunLayout(seed, tier, mods)
 *   RunManager::AdvanceLevel → ProceduralGen->GetLevelLayout(levelNumber)
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXProceduralGen : public UObject
{
    GENERATED_BODY()

public:

    UMXProceduralGen();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Initialise subsystems with DataTable references.
     * Must be called once before GenerateRunLayout.
     * @param HazardDataTable  DT_HazardFields DataTable for hazard pool.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Procedural")
    void Initialize(UDataTable* HazardDataTable);

    // -------------------------------------------------------------------------
    // Generation
    // -------------------------------------------------------------------------

    /**
     * Generate a complete 20-level run layout from a master seed.
     * Deterministic: identical inputs always produce identical output.
     * Logs a comprehensive summary to the output log.
     *
     * @param MasterSeed     The seed value. Can be derived from system time,
     *                       run number, or player input for shareable runs.
     * @param Tier           Difficulty tier — affects room count, hazard density,
     *                       hazard speed, and pacing curve.
     * @param ActiveModifiers  Modifier names active for this run. Checked for
     *                        "Random Hazards", "Reduced Visibility", etc.
     * @return               The complete FMXRunLayout.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Procedural")
    FMXRunLayout GenerateRunLayout(int64 MasterSeed, ETier Tier, const TArray<FString>& ActiveModifiers);

    /**
     * Retrieve the layout for a specific level from the most recently generated run.
     * @param LevelNumber  1-indexed level (1–20).
     * @return             The level layout, or a default-constructed layout if out of range.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Procedural")
    FMXLevelLayout GetLevelLayout(int32 LevelNumber) const;

    /**
     * Return the full run layout from the most recent GenerateRunLayout call.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Procedural")
    const FMXRunLayout& GetRunLayout() const { return CurrentRunLayout; }

    /**
     * Return the master seed of the current run.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Procedural")
    int64 GetMasterSeed() const { return CurrentRunLayout.MasterSeed; }

    /**
     * Generate a master seed from the current system time.
     * Convenience method for non-shareable runs.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Procedural")
    static int64 GenerateSeedFromTime();

    /**
     * Log a human-readable summary of the entire run layout to the output log.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Procedural|Debug")
    void LogRunSummary() const;

    /**
     * Log detailed info for a single level.
     * @param LevelNumber  1-indexed level.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Procedural|Debug")
    void LogLevelDetail(int32 LevelNumber) const;

private:

    // -------------------------------------------------------------------------
    // Subsystems
    // -------------------------------------------------------------------------

    /** Room layout generator. */
    UPROPERTY()
    TObjectPtr<UMXRoomGenerator> RoomGenerator;

    /** Hazard selection and placement. */
    UPROPERTY()
    TObjectPtr<UMXHazardPlacer> HazardPlacer;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /** The most recently generated run layout. */
    FMXRunLayout CurrentRunLayout;

    /** Whether Initialize has been called. */
    bool bInitialized = false;

    // -------------------------------------------------------------------------
    // Pacing Curve
    // -------------------------------------------------------------------------

    /**
     * Derive a per-level seed from the master seed and level number.
     * Each level gets a unique, deterministic seed.
     */
    static int64 DeriveLevelSeed(int64 MasterSeed, int32 LevelNumber);

    /**
     * Generate level conditions (element theme, density, pacing) for a given level.
     * This is the core pacing curve — it controls the emotional arc of the run.
     * @param LevelNumber   1-indexed level.
     * @param Tier          Difficulty tier.
     * @param Modifiers     Active modifier names.
     * @param Stream        Seeded RNG for element selection.
     * @return              Level conditions.
     */
    FMXLevelConditions GenerateConditions(
        int32                   LevelNumber,
        ETier                   Tier,
        const TArray<FString>&  Modifiers,
        FRandomStream&          Stream) const;

    /**
     * Compute the rescue spawn count for a level.
     * Mirrors MXSpawnManager's logic but integrated with the procedural layout.
     */
    int32 ComputeRescueCount(int32 LevelNumber, ETier Tier, const TArray<FString>& Modifiers) const;

    /**
     * Check if a modifier is active by name.
     */
    static bool HasModifier(const TArray<FString>& Modifiers, const FString& Name);
};
