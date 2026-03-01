// MXHazardPlacer.h — Procedural Generation Module v1.0
// Created: 2026-03-01
// Agent 12: Procedural Generation
// Selects and places hazards within generated rooms based on element themes,
// difficulty curves, and tier modifiers. Also places rescue spawn points.
// Reads DT_HazardFields for the available hazard type pool.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MXTypes.h"
#include "MXEventFields.h"
#include "MXProceduralData.h"
#include "MXHazardPlacer.generated.h"

// ---------------------------------------------------------------------------
// UMXHazardPlacer
// ---------------------------------------------------------------------------

/**
 * UMXHazardPlacer
 * Populates rooms with hazard placements and rescue spawn points.
 *
 * Hazard selection respects the level's element theme: 60–80% of hazards
 * match the primary element, 20–40% match the secondary. The exact mix
 * is influenced by the pacing curve and tier effects.
 *
 * Hazard density scales with level number and tier:
 *   - Each room is filled to a fraction of its MaxHazardSlots.
 *   - Early levels use ~50% of slots; late levels use 80–100%.
 *   - Higher tiers add a flat +1 bonus per room.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXHazardPlacer : public UObject
{
    GENERATED_BODY()

public:

    UMXHazardPlacer();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Load the available hazard type pool from DT_HazardFields.
     * Must be called once before any placement calls.
     * @param HazardDataTable  The DT_HazardFields DataTable asset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Procedural|Hazards")
    void InitialiseFromDataTable(UDataTable* HazardDataTable);

    // -------------------------------------------------------------------------
    // Placement
    // -------------------------------------------------------------------------

    /**
     * Place hazards in all rooms for a single level.
     * @param Rooms             The room array from MXRoomGenerator.
     * @param Conditions        The level's element theme and density settings.
     * @param LevelSeed         For deterministic placement.
     * @param LevelNumber       1-indexed level number.
     * @param Tier              Difficulty tier.
     * @param HazardSpeedMult   Global hazard speed multiplier from tier effects.
     * @param bRandomHazards    If true, ignore element theme and randomise all hazard types.
     * @return                  Array of all hazard placements for the level.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Procedural|Hazards")
    TArray<FMXHazardPlacement> PlaceHazards(
        const TArray<FMXRoomDef>&     Rooms,
        const FMXLevelConditions&     Conditions,
        int64                         LevelSeed,
        int32                         LevelNumber,
        ETier                         Tier,
        float                         HazardSpeedMult,
        bool                          bRandomHazards);

    /**
     * Generate rescue spawn points for a level's rooms.
     * @param Rooms         The room array.
     * @param Hazards       The placed hazards (used to flag guarded spawns).
     * @param RescueCount   Total rescue robots to place (from SpawnManager budget).
     * @param LevelSeed     For deterministic placement.
     * @return              Array of rescue spawn points.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Procedural|Hazards")
    TArray<FMXRescueSpawn> PlaceRescueSpawns(
        const TArray<FMXRoomDef>&          Rooms,
        const TArray<FMXHazardPlacement>&  Hazards,
        int32                              RescueCount,
        int64                              LevelSeed);

private:

    // -------------------------------------------------------------------------
    // Internal: Hazard Type Pool
    // -------------------------------------------------------------------------

    /** Map of HazardType name → element, loaded from DT_HazardFields. */
    TMap<FString, EHazardElement> HazardElementMap;

    /** All available hazard type names. */
    TArray<FString> AllHazardTypes;

    /** Hazard types grouped by element. */
    TMap<EHazardElement, TArray<FString>> HazardsByElement;

    // -------------------------------------------------------------------------
    // Internal: Helpers
    // -------------------------------------------------------------------------

    /**
     * Compute how many hazards to actually place in a given room.
     * @param Room          The room definition.
     * @param Conditions    Level conditions (density multiplier).
     * @param LevelNumber   For difficulty curve.
     * @param Tier          For tier bonus.
     * @param Stream        Seeded RNG.
     * @return              Number of hazards to place (0 to MaxHazardSlots).
     */
    int32 ComputeHazardCount(
        const FMXRoomDef&         Room,
        const FMXLevelConditions& Conditions,
        int32                     LevelNumber,
        ETier                     Tier,
        FRandomStream&            Stream) const;

    /**
     * Pick a hazard type for a single slot.
     * @param PrimaryElement     Level's primary element.
     * @param SecondaryElement   Level's secondary element.
     * @param bRandomHazards     If true, ignore element weighting.
     * @param Stream             Seeded RNG.
     * @return                   Hazard type key string.
     */
    FString PickHazardType(
        EHazardElement PrimaryElement,
        EHazardElement SecondaryElement,
        bool           bRandomHazards,
        FRandomStream& Stream) const;

    /**
     * Compute a hazard's local position within a room.
     * Distributes hazards evenly across the room's footprint.
     * @param Room          The room definition.
     * @param SlotIndex     Which slot (0-based) this hazard occupies in the room.
     * @param TotalSlots    Total hazards being placed in this room.
     * @param Stream        Seeded RNG for jitter.
     * @return              Local-space position.
     */
    FVector ComputeHazardPosition(
        const FMXRoomDef& Room,
        int32             SlotIndex,
        int32             TotalSlots,
        FRandomStream&    Stream) const;
};
