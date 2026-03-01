// MXInterfaces.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: All agents implement or consume these interfaces
// Change Log:
//   v1.0 â€” Initial definition of all cross-module interfaces

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MXTypes.h"
#include "MXEventData.h"
#include "MXRobotProfile.h"
#include "MXHatData.h"
#include "MXRunData.h"
#include "MXInterfaces.generated.h"

// ---------------------------------------------------------------------------
// UMXEventListener
// ---------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Blueprintable)
class UMXEventListener : public UInterface
{
    GENERATED_BODY()
};

/**
 * IMXEventListener
 * Implemented by any system that needs to receive DEMS events as they are dispatched.
 * Consumers: Identity (life log), Camera, Reports (event accumulation), UI (live feed).
 */
class MICROEXODUS_API IMXEventListener
{
    GENERATED_BODY()

public:
    /**
     * Called by the DEMS MessageDispatcher whenever a new event is fully constructed.
     * @param Event  The complete, resolved event record including message_text and visual_mark.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Events")
    void OnDEMSEvent(const FMXEventData& Event);
};

// ---------------------------------------------------------------------------
// UMXRobotProvider
// ---------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Blueprintable)
class UMXRobotProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * IMXRobotProvider
 * Implemented by the Identity module's RobotManager.
 * Used by DEMS, Reports, Hats, Evolution, and UI to query robot data without direct coupling.
 */
class MICROEXODUS_API IMXRobotProvider
{
    GENERATED_BODY()

public:
    /**
     * Returns the full persistent profile for a specific robot.
     * @param RobotId  The GUID of the robot to query.
     * @return         A copy of the robot's profile, or a default-initialized struct if not found.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Robots")
    FMXRobotProfile GetRobotProfile(const FGuid& RobotId) const;

    /**
     * Returns all robot profiles currently on the roster.
     * @return  Array of all FMXRobotProfile records (up to MAX_ROBOTS entries).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Robots")
    TArray<FMXRobotProfile> GetAllRobotProfiles() const;

    /**
     * Returns the current number of robots on the roster.
     * @return  Integer count (0â€“MAX_ROBOTS).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Robots")
    int32 GetRobotCount() const;
};

// ---------------------------------------------------------------------------
// UMXHatProvider
// ---------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Blueprintable)
class UMXHatProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * IMXHatProvider
 * Implemented by the Hats module's HatManager.
 * Used by DEMS, Reports, and UI to query hat data and manage collection inventory.
 */
class MICROEXODUS_API IMXHatProvider
{
    GENERATED_BODY()

public:
    /**
     * Returns the static definition for a specific hat ID.
     * @param HatId  The hat_id to look up.
     * @return       The FMXHatDefinition row, or a default-initialized struct if not found.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    FMXHatDefinition GetHatData(int32 HatId) const;

    /**
     * Returns the player's full hat collection (stacks, unlocked IDs, discovered combos).
     * @return  A copy of the current FMXHatCollection state.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    FMXHatCollection GetHatCollection() const;

    /**
     * Attempts to consume one copy of the specified hat from the collection stack.
     * Called when a robot equips a hat at run start.
     * @param HatId  Hat to consume.
     * @return       True if a copy was available and consumed; false if stack is empty.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    bool ConsumeHat(int32 HatId);

    /**
     * Returns one copy of the specified hat to the collection stack.
     * Called when a hatted robot survives to the end of a run.
     * @param HatId  Hat to return.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    void ReturnHat(int32 HatId);
};

// ---------------------------------------------------------------------------
// UMXRunProvider
// ---------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Blueprintable)
class UMXRunProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * IMXRunProvider
 * Implemented by the Roguelike module's RunManager.
 * Used by Reports and UI to query the current run's state.
 */
class MICROEXODUS_API IMXRunProvider
{
    GENERATED_BODY()

public:
    /**
     * Returns a snapshot of the current run data (events accumulated so far, robots deployed, etc.).
     * @return  A copy of the in-progress FMXRunData.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Run")
    FMXRunData GetCurrentRunData() const;

    /**
     * Returns the sequential run number for the current run.
     * @return  1-indexed run counter.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Run")
    int32 GetRunNumber() const;

    /**
     * Returns the difficulty tier the current run is being played on.
     * @return  The active ETier value.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Run")
    ETier GetCurrentTier() const;
};

// ---------------------------------------------------------------------------
// UMXEvolutionTarget
// ---------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Blueprintable)
class UMXEvolutionTarget : public UInterface
{
    GENERATED_BODY()
};

/**
 * IMXEvolutionTarget
 * Implemented by the robot Blueprint actor (via Evolution module component).
 * Called by the DEMS dispatcher to apply visual marks to a robot after a significant event.
 */
class MICROEXODUS_API IMXEvolutionTarget
{
    GENERATED_BODY()

public:
    /**
     * Instructs the robot's Evolution component to apply a named visual mark at a given intensity.
     * @param MarkType   Identifies the type of mark (e.g., "burn_mark", "impact_crack", "frost_residue").
     * @param Intensity  The strength of the mark to blend in (0.0â€“1.0).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Evolution")
    void ApplyVisualMark(const FString& MarkType, float Intensity);
};

// ---------------------------------------------------------------------------
// UMXPersistable
// ---------------------------------------------------------------------------

UINTERFACE(MinimalAPI, Blueprintable)
class UMXPersistable : public UInterface
{
    GENERATED_BODY()
};

/**
 * IMXPersistable
 * Implemented by module managers that need their state saved and loaded by the Persistence module.
 * The Persistence module calls MXSerialize/MXDeserialize on all registered IMXPersistable implementors.
 */
class MICROEXODUS_API IMXPersistable
{
    GENERATED_BODY()

public:
    /**
     * Serialize this module's state into the archive for saving.
     * @param Ar  The FArchive being written to.
     */
    // Not Blueprint-exposed: FArchive is a C++ type not visible to UHT/BP.
    // Implemented as plain virtual C++ overrides in each module manager.
    virtual void MXSerialize(FArchive& Ar) = 0;

    /**
     * Deserialize this module's state from the archive on load.
     * @param Ar  The FArchive being read from.
     */
    virtual void MXDeserialize(FArchive& Ar) = 0;
};
