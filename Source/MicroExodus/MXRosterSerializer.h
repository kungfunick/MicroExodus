// MXRosterSerializer.h — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Consumers: MXSaveManager (internal to Persistence module)
// Change Log:
//   v1.0 — Initial implementation: roster serialization, life log pruning, validation

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXRobotProfile.h"
#include "MXEventData.h"
#include "MXTypes.h"
#include "MXRosterSerializer.generated.h"

/**
 * UMXRosterSerializer
 * Handles binary serialization of the full robot roster and per-robot life logs.
 *
 * Design rules:
 *   - Every field in FMXRobotProfile is written in a stable, versioned order.
 *   - Life logs use block-size-prefixed entries so future fields can be skipped.
 *   - Pruning policy: keep the most recent N entries per robot, plus all
 *     Death / Sacrifice / LevelUp events regardless of recency.
 *   - Validation catches corrupt GUIDs, out-of-range levels, and negative counts
 *     before handing data to the Identity module.
 */
UCLASS()
class MICROEXODUS_API UMXRosterSerializer : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Roster Serialization
    // -------------------------------------------------------------------------

    /**
     * Serialize the full active roster into the archive.
     * Writes count first, then each FMXRobotProfile in sequence.
     *
     * @param Ar      FArchive opened for writing.
     * @param Roster  Map from RobotId to profile.
     */
    void SerializeRoster(FArchive& Ar, const TMap<FGuid, FMXRobotProfile>& Roster);

    /**
     * Deserialize the roster from the archive.
     *
     * @param Ar  FArchive opened for reading.
     * @return    Reconstructed map of profiles; empty on failure.
     */
    TMap<FGuid, FMXRobotProfile> DeserializeRoster(FArchive& Ar);

    // -------------------------------------------------------------------------
    // Life Log Serialization
    // -------------------------------------------------------------------------

    /**
     * Serialize per-robot life log arrays.
     * Writes outer map count, then for each robot: its GUID + inner event array.
     *
     * @param Ar    FArchive opened for writing.
     * @param Logs  Map from RobotId to ordered event history.
     */
    void SerializeLifeLogs(FArchive& Ar, const TMap<FGuid, TArray<FMXEventData>>& Logs);

    /**
     * Deserialize per-robot life logs.
     *
     * @param Ar  FArchive opened for reading.
     * @return    Reconstructed life log map; empty on failure.
     */
    TMap<FGuid, TArray<FMXEventData>> DeserializeLifeLogs(FArchive& Ar);

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /**
     * Validate roster integrity after deserialization.
     * Checks: valid GUIDs, level in [1, 100], xp >= 0, no duplicate IDs.
     *
     * @param Roster  The deserialized roster to inspect.
     * @return        True if all profiles pass validation.
     */
    bool ValidateRoster(const TMap<FGuid, FMXRobotProfile>& Roster) const;

    // -------------------------------------------------------------------------
    // Life Log Pruning
    // -------------------------------------------------------------------------

    /**
     * Trim life logs to avoid unbounded memory growth.
     * Always keeps the most recent MaxEntriesPerRobot events.
     * Additionally keeps ALL Death, Sacrifice, and LevelUp events regardless of age.
     *
     * @param Logs                Life log map to prune in-place.
     * @param MaxEntriesPerRobot  Maximum events retained per robot (default: 500).
     */
    void PruneLifeLogs(TMap<FGuid, TArray<FMXEventData>>& Logs, int32 MaxEntriesPerRobot = 500);

private:

    // -------------------------------------------------------------------------
    // Per-Profile Serialization Helpers
    // -------------------------------------------------------------------------

    /** Write a single FMXRobotProfile to the archive. */
    void SerializeProfile(FArchive& Ar, const FMXRobotProfile& Profile);

    /** Read a single FMXRobotProfile from the archive. */
    FMXRobotProfile DeserializeProfile(FArchive& Ar);

    /** Write a single FMXEventData to the archive. */
    void SerializeEvent(FArchive& Ar, const FMXEventData& Event);

    /** Read a single FMXEventData from the archive. */
    FMXEventData DeserializeEvent(FArchive& Ar);

    /** True if an event type should always be preserved during pruning. */
    bool IsPinnedEventType(EEventType Type) const;
};
