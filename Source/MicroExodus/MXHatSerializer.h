// MXHatSerializer.h — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Consumers: MXSaveManager (internal to Persistence module)
// Change Log:
//   v1.0 — Initial implementation: hat collection + paint job serialization and validation

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXHatData.h"
#include "MXTypes.h"
#include "MXConstants.h"
#include "MXHatSerializer.generated.h"

/**
 * UMXHatSerializer
 * Handles binary serialization of the player's hat collection and paint job unlocks.
 *
 * Design rules:
 *   - Collection is compact: only stacks with count > 0 are written to avoid sparse bloat.
 *   - Validation enforces hat_id range [0, MAX_HATS - 1] and stack count [0, MAX_HAT_STACK].
 *   - Paint jobs are stored as a compact array of uint8 enum values.
 */
UCLASS()
class MICROEXODUS_API UMXHatSerializer : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Hat Collection Serialization
    // -------------------------------------------------------------------------

    /**
     * Serialize the full hat collection to the archive.
     * Writes: unlocked_hat_ids array, non-zero stacks, discovered_combo_ids array.
     *
     * @param Ar          FArchive opened for writing.
     * @param Collection  The collection to serialize.
     */
    void SerializeHatCollection(FArchive& Ar, const FMXHatCollection& Collection);

    /**
     * Deserialize the hat collection from the archive.
     *
     * @param Ar  FArchive opened for reading.
     * @return    Reconstructed FMXHatCollection.
     */
    FMXHatCollection DeserializeHatCollection(FArchive& Ar);

    // -------------------------------------------------------------------------
    // Paint Job Serialization
    // -------------------------------------------------------------------------

    /**
     * Serialize the array of unlocked paint job enum values.
     *
     * @param Ar                 FArchive opened for writing.
     * @param UnlockedPaintJobs  Array of EPaintJob values earned so far.
     */
    void SerializePaintJobs(FArchive& Ar, const TArray<EPaintJob>& UnlockedPaintJobs);

    /**
     * Deserialize the paint job unlock array.
     *
     * @param Ar  FArchive opened for reading.
     * @return    Array of EPaintJob values.
     */
    TArray<EPaintJob> DeserializePaintJobs(FArchive& Ar);

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /**
     * Validate hat collection integrity after deserialization.
     * Checks: hat_ids in range [0, MAX_HATS), stack counts in [0, MAX_HAT_STACK].
     *
     * @param Collection  The deserialized collection to inspect.
     * @return            True if all entries pass validation.
     */
    bool ValidateHatCollection(const FMXHatCollection& Collection) const;
};
