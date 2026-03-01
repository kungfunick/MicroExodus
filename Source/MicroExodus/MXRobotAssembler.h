// MXRobotAssembler.h — Robot Assembly Module v1.0
// Created: 2026-03-01
// Agent 13: Robot Assembly
// Manages the part variant pool and deterministically generates assembly
// recipes from robot GUIDs. Stateless — recipes are pure functions of GUID
// and part pool, so they don't need to be saved.
//
// Part pool can be loaded from a DataTable (DT_RobotParts) or populated
// via Blueprint with AddPart calls for prototyping.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MXRobotAssemblyData.h"
#include "MXRobotAssembler.generated.h"

// Forward declarations
struct FMXRobotProfile;

// ---------------------------------------------------------------------------
// UMXRobotAssembler
// ---------------------------------------------------------------------------

/**
 * UMXRobotAssembler
 * Pure-data system: takes a GUID, produces a deterministic FMXAssemblyRecipe.
 *
 * The recipe describes WHICH parts go WHERE — it does not create any
 * USceneComponents or meshes. That's MXRobotMeshComponent's job.
 *
 * Determinism guarantee:
 *   Same GUID + same part pool = same recipe. Always.
 *   The GUID's four uint32 words are hashed into per-slot RNG seeds.
 *   Adding new parts to the pool does NOT change existing robots' recipes
 *   because selection uses modular arithmetic on stable part indices.
 *
 * Performance note for 100 robots:
 *   GenerateRecipe is pure math — ~0.01ms per robot. Safe to call 100x
 *   in a single frame during level setup.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXRobotAssembler : public UObject
{
    GENERATED_BODY()

public:

    UMXRobotAssembler();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Load the part variant pool from a DataTable.
     * The DataTable row struct must be FMXPartDefinition.
     * @param PartsDataTable  DT_RobotParts asset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly")
    void InitialiseFromDataTable(UDataTable* PartsDataTable);

    /**
     * Manually register a part variant (for prototyping or Blueprint-driven pools).
     * @param Part  The part definition to add.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly")
    void AddPart(const FMXPartDefinition& Part);

    /**
     * Register a batch of placeholder parts for testing when no DataTable exists.
     * Creates N synthetic variants per slot with ascending IDs.
     * @param VariantsPerSlot  Number of variants to generate per slot (default 6).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Debug")
    void GeneratePlaceholderParts(int32 VariantsPerSlot = 6);

    // -------------------------------------------------------------------------
    // Recipe Generation
    // -------------------------------------------------------------------------

    /**
     * Generate a deterministic assembly recipe for a robot.
     * @param RobotId       The robot's GUID (the seed for all randomisation).
     * @param ChassisColor  Color index from FMXRobotProfile (passed through).
     * @param EyeColor      Eye color index from FMXRobotProfile (passed through).
     * @return               Complete assembly recipe.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly")
    FMXAssemblyRecipe GenerateRecipe(const FGuid& RobotId, int32 ChassisColor = 0, int32 EyeColor = 0) const;

    /**
     * Generate a recipe directly from a robot profile.
     * Convenience wrapper that pulls GUID, colors from the profile.
     * @param Profile  The robot's full profile.
     * @return         Complete assembly recipe.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly")
    FMXAssemblyRecipe GenerateRecipeFromProfile(const FMXRobotProfile& Profile) const;

    /**
     * Batch-generate recipes for multiple robots.
     * @param RobotIds  Array of GUIDs to process.
     * @return          Array of recipes in the same order.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly")
    TArray<FMXAssemblyRecipe> BatchGenerateRecipes(const TArray<FGuid>& RobotIds) const;

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /**
     * Look up a part definition by ID.
     * @param PartId  The part's unique identifier.
     * @param OutPart  The definition if found.
     * @return         True if the part was found.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly")
    bool GetPartDefinition(const FString& PartId, FMXPartDefinition& OutPart) const;

    /**
     * Get all registered parts for a specific slot.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly")
    TArray<FMXPartDefinition> GetPartsForSlot(EPartSlot Slot) const;

    /**
     * Get the total number of registered part variants.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Assembly")
    int32 GetTotalPartCount() const;

    /**
     * Get the number of registered variants for a specific slot.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Assembly")
    int32 GetPartCountForSlot(EPartSlot Slot) const;

    /**
     * Compute the total number of unique visual combinations possible.
     * (Product of variant counts across all slots.)
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Assembly")
    int64 GetTotalCombinations() const;

    /**
     * Log a summary of the part pool to the output log.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Debug")
    void LogPartPoolSummary() const;

    /**
     * Log a recipe's details.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Debug")
    void LogRecipe(const FMXAssemblyRecipe& Recipe) const;

private:

    // -------------------------------------------------------------------------
    // Part Pool
    // -------------------------------------------------------------------------

    /** All registered part definitions, indexed by PartId. */
    TMap<FString, FMXPartDefinition> AllParts;

    /** Parts grouped by slot for fast per-slot lookups. */
    TMap<EPartSlot, TArray<FString>> PartIdsBySlot;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /**
     * Derive a deterministic seed for a specific slot from a GUID.
     * Each slot gets a different seed so parts are independently randomised.
     */
    static int32 DeriveSlotSeed(const FGuid& RobotId, EPartSlot Slot);

    /**
     * Select a part from a slot's pool using weighted random with a seeded stream.
     * @param Slot    The slot to pick for.
     * @param Stream  Seeded RNG.
     * @return        The chosen PartId, or empty string if no parts for this slot.
     */
    FString PickPartForSlot(EPartSlot Slot, FRandomStream& Stream) const;
};
