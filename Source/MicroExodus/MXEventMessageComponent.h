// MXEventMessageComponent.h — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS
// UActorComponent that lives on every hazard Blueprint in the world.
// Holds that hazard's FMXObjectEventFields and provides the trigger entry points
// the Swarm module calls when a robot interacts with this hazard.
//
// Usage: Add this component to a hazard Blueprint, fill in EventFields in the editor,
// then call TriggerDeathEvent / TriggerNearMissEvent / TriggerRescueEvent / TriggerSacrificeEvent
// from the Blueprint's event graph or from C++ Swarm code.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXEventFields.h"
#include "MXEventData.h"
#include "MXTypes.h"
#include "MXInterfaces.h"
#include "MXEventMessageComponent.generated.h"

class UMXMessageBuilder;
class UMXMessageDispatcher;

// ---------------------------------------------------------------------------
// UMXEventMessageComponent
// ---------------------------------------------------------------------------

UCLASS(ClassGroup=(MicroExodus), meta=(BlueprintSpawnableComponent), BlueprintType)
class MICROEXODUS_API UMXEventMessageComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXEventMessageComponent();

protected:

    virtual void BeginPlay() override;

public:

    // -------------------------------------------------------------------------
    // Configuration — editable per-instance in the Blueprint editor
    // -------------------------------------------------------------------------

    /**
     * The complete event descriptor for this hazard.
     * Fill this in the Blueprint editor for every hazard actor type.
     * Determines what DEMS sentences this hazard can generate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DEMS")
    FMXObjectEventFields EventFields;

    // -------------------------------------------------------------------------
    // Trigger methods — called by Swarm collision code or Blueprint
    // -------------------------------------------------------------------------

    /**
     * Trigger a Death event for the specified robot.
     * Queries the robot's profile, constructs the DEMS message, dispatches it.
     * Call this from the Swarm module immediately before marking the robot as dead.
     * @param RobotId  The GUID of the robot that was killed by this hazard.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS")
    void TriggerDeathEvent(const FGuid& RobotId);

    /**
     * Trigger a NearMiss event for the specified robot.
     * Call this when a robot escapes a danger zone without dying.
     * @param RobotId  The GUID of the robot that survived the close call.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS")
    void TriggerNearMissEvent(const FGuid& RobotId);

    /**
     * Trigger a Rescue event. HazardFields are used only for severity/camera — verbs/nouns
     * are ignored for Rescue (no hazard involved). Override EventFields.source_noun
     * on rescue point actors to describe the rescue scenario if desired.
     * @param RobotId     The GUID of the robot being rescued.
     * @param RobotCount  Optional total roster size at time of rescue (used in {robot_count} token).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS")
    void TriggerRescueEvent(const FGuid& RobotId, int32 RobotCount = 0);

    /**
     * Trigger a Sacrifice event for the specified robot.
     * Sacrifice events use sincerity-mode templates. Camera will go Epic.
     * @param RobotId     The GUID of the robot performing the sacrifice.
     * @param SavedCount  Number of other robots saved by this sacrifice.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS")
    void TriggerSacrificeEvent(const FGuid& RobotId, int32 SavedCount);

private:

    /**
     * Internal helper used by all trigger methods.
     * Looks up the robot provider and hat provider from the game instance,
     * builds the message, applies the hat name, and dispatches.
     * @param EventType   The event class to generate.
     * @param RobotId     The robot this event is about.
     * @param SavedCount  Populated only for Sacrifice events.
     * @param RobotCount  Populated only for Rescue events.
     */
    void BuildAndDispatch(EEventType EventType, const FGuid& RobotId, int32 SavedCount, int32 RobotCount);

    /**
     * Locate the MessageBuilder from the game instance subsystem.
     * Returns null if DEMS subsystem is not initialized.
     */
    UMXMessageBuilder* GetMessageBuilder() const;

    /**
     * Locate the MessageDispatcher from the game instance subsystem.
     */
    UMXMessageDispatcher* GetMessageDispatcher() const;

    /**
     * Locate the RobotProvider interface from the game instance.
     * Returns null if Identity module is not registered.
     */
    TScriptInterface<IMXRobotProvider> GetRobotProvider() const;

    /**
     * Locate the HatProvider interface from the game instance.
     * Returns null if Hats module is not registered.
     */
    TScriptInterface<IMXHatProvider> GetHatProvider() const;

    /**
     * Cached run number for the current run. Set from RunProvider at BeginPlay.
     * Updated by subscribing to run-start events when the RunManager is available.
     */
    int32 CachedRunNumber;

    /**
     * Cached level number. Updated by the level manager each time a new level loads.
     * Defaults to 1 if no level manager is registered.
     */
    int32 CachedLevelNumber;
};
