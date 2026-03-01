// MXLifeLog.h â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity
// Consumers: Identity (internal), DEMS, Reports, UI (robot inspection screen)
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXEventData.h"
#include "MXInterfaces.h"
#include "MXLifeLog.generated.h"

/**
 * UMXLifeLog
 * Stores the chronological event history for every robot on the roster.
 * Acts as the robot's autobiography â€” consumed by DEMS for personalized flavor,
 * by Reports for eulogies and highlights, and by the UI's robot inspection screen.
 *
 * Implements IMXEventListener so it can receive DEMS events directly from the message dispatcher.
 * Each robot's log is capped at 500 entries; overflow pruning preserves deaths, sacrifices, and level-ups.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXLifeLog : public UObject, public IMXEventListener
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** Maximum number of log entries stored per robot before pruning. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|LifeLog")
    int32 MaxEntriesPerRobot = 500;

    // -------------------------------------------------------------------------
    // Log Management
    // -------------------------------------------------------------------------

    /**
     * Appends a DEMS event to the specified robot's life log.
     * If the robot's log exceeds MaxEntriesPerRobot, pruning is triggered.
     * @param RobotId   The primary robot this event belongs to.
     * @param Event     The fully resolved DEMS event to append.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    void AppendEvent(const FGuid& RobotId, const FMXEventData& Event);

    /**
     * Returns the full chronological event history for a robot.
     * Returns an empty array if the robot has no log.
     * @param RobotId   The robot to query.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    TArray<FMXEventData> GetLifeLog(const FGuid& RobotId) const;

    /**
     * Returns the N most recent events from a robot's life log.
     * Useful for DEMS context injection ("recently, this robot...").
     * @param RobotId   The robot to query.
     * @param Count     Maximum number of recent events to return.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    TArray<FMXEventData> GetRecentEvents(const FGuid& RobotId, int32 Count) const;

    /**
     * Returns all events of a specific type from a robot's life log.
     * Useful for Reports querying all deaths witnessed, or all near-misses.
     * @param RobotId       The robot to query.
     * @param EventType     The event type to filter by.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    TArray<FMXEventData> GetEventsByType(const FGuid& RobotId, EEventType EventType) const;

    /**
     * Returns the number of log entries for a robot.
     * @param RobotId   The robot to query.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    int32 GetLogLength(const FGuid& RobotId) const;

    /**
     * Returns the robot's first recorded event (effectively their birth/first encounter).
     * @param RobotId   The robot to query.
     * @return          The oldest event, or a default struct if no log exists.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    FMXEventData GetFirstEvent(const FGuid& RobotId) const;

    /**
     * Clears the life log for a robot (called when a dead robot's data is fully archived).
     * Prevents unbounded memory growth for the dead robot archive.
     * @param RobotId   The robot whose log should be cleared.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    void ClearLog(const FGuid& RobotId);

    /**
     * Transfers a dead robot's log to the archive map (keyed by GUID) for Memorial Wall access.
     * Called by RobotManager when archiving a dead robot.
     * @param RobotId   The dead robot's GUID.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    void ArchiveLog(const FGuid& RobotId);

    /**
     * Returns the archived life log for a dead robot.
     * Used by the Memorial Wall UI.
     * @param RobotId   The dead robot's GUID.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|LifeLog")
    TArray<FMXEventData> GetArchivedLog(const FGuid& RobotId) const;

    // -------------------------------------------------------------------------
    // IMXEventListener Implementation
    // -------------------------------------------------------------------------

    /**
     * Receives DEMS events and routes them to the appropriate robot's log.
     * Also appends to the related_robot_id's log for witness events.
     * @param Event     The fully resolved DEMS event.
     */
    virtual void OnDEMSEvent_Implementation(const FMXEventData& Event) override;

private:

    /**
     * Live robot logs â€” keyed by robot GUID.
     * Cleared when a robot is archived or removed.
     */
    TMap<FGuid, TArray<FMXEventData>> LiveLogs;

    /**
     * Archived logs for dead robots â€” keyed by robot GUID.
     * Persisted so the Memorial Wall can display their full history.
     */
    TMap<FGuid, TArray<FMXEventData>> ArchivedLogs;

    /**
     * Prunes a robot's log when it exceeds MaxEntriesPerRobot.
     * Preserves: the first 10 entries (birth context), all Death/Sacrifice/LevelUp events,
     * and the most recent entries up to the cap.
     * @param RobotId   The robot whose log should be pruned.
     */
    void PruneLog(const FGuid& RobotId);

    /**
     * Returns true if an event type should always be preserved during pruning.
     * @param Type  The event type to evaluate.
     */
    static bool IsPreservedEventType(EEventType Type);
};
