// MXLifeLog.cpp â€” Identity Module Version 1.0
// Created: 2026-02-18
// Agent 2: Identity

#include "MXLifeLog.h"

void UMXLifeLog::AppendEvent(const FGuid& RobotId, const FMXEventData& Event)
{
    if (!RobotId.IsValid()) return;

    TArray<FMXEventData>& Log = LiveLogs.FindOrAdd(RobotId);
    Log.Add(Event);

    // Prune if over cap
    if (Log.Num() > MaxEntriesPerRobot)
    {
        PruneLog(RobotId);
    }
}

TArray<FMXEventData> UMXLifeLog::GetLifeLog(const FGuid& RobotId) const
{
    const TArray<FMXEventData>* Log = LiveLogs.Find(RobotId);
    return Log ? *Log : TArray<FMXEventData>();
}

TArray<FMXEventData> UMXLifeLog::GetRecentEvents(const FGuid& RobotId, int32 Count) const
{
    const TArray<FMXEventData>* Log = LiveLogs.Find(RobotId);
    if (!Log || Log->IsEmpty()) return TArray<FMXEventData>();

    int32 StartIndex = FMath::Max(0, Log->Num() - Count);
    TArray<FMXEventData> Out;
    Out.Reserve(Count);

    for (int32 i = StartIndex; i < Log->Num(); i++)
    {
        Out.Add((*Log)[i]);
    }

    return Out;
}

TArray<FMXEventData> UMXLifeLog::GetEventsByType(const FGuid& RobotId, EEventType EventType) const
{
    const TArray<FMXEventData>* Log = LiveLogs.Find(RobotId);
    if (!Log) return TArray<FMXEventData>();

    TArray<FMXEventData> Out;
    for (const FMXEventData& Event : *Log)
    {
        if (Event.event_type == EventType)
        {
            Out.Add(Event);
        }
    }

    return Out;
}

int32 UMXLifeLog::GetLogLength(const FGuid& RobotId) const
{
    const TArray<FMXEventData>* Log = LiveLogs.Find(RobotId);
    return Log ? Log->Num() : 0;
}

FMXEventData UMXLifeLog::GetFirstEvent(const FGuid& RobotId) const
{
    const TArray<FMXEventData>* Log = LiveLogs.Find(RobotId);
    if (Log && !Log->IsEmpty())
    {
        return (*Log)[0];
    }
    return FMXEventData();
}

void UMXLifeLog::ClearLog(const FGuid& RobotId)
{
    LiveLogs.Remove(RobotId);
}

void UMXLifeLog::ArchiveLog(const FGuid& RobotId)
{
    TArray<FMXEventData>* Log = LiveLogs.Find(RobotId);
    if (Log)
    {
        ArchivedLogs.Add(RobotId, *Log);
        LiveLogs.Remove(RobotId);
    }
}

TArray<FMXEventData> UMXLifeLog::GetArchivedLog(const FGuid& RobotId) const
{
    const TArray<FMXEventData>* Log = ArchivedLogs.Find(RobotId);
    return Log ? *Log : TArray<FMXEventData>();
}

void UMXLifeLog::OnDEMSEvent_Implementation(const FMXEventData& Event)
{
    // Append to primary robot's log
    if (Event.robot_id.IsValid())
    {
        AppendEvent(Event.robot_id, Event);
    }

    // Append witness events to the related robot's log
    if (Event.related_robot_id.IsValid() &&
        (Event.event_type == EEventType::RescueWitnessed ||
         Event.event_type == EEventType::DeathWitnessed  ||
         Event.event_type == EEventType::SacrificeWitnessed))
    {
        AppendEvent(Event.related_robot_id, Event);
    }
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void UMXLifeLog::PruneLog(const FGuid& RobotId)
{
    TArray<FMXEventData>* Log = LiveLogs.Find(RobotId);
    if (!Log) return;

    // Separate entries into preserved and general
    TArray<FMXEventData> Preserved;     // Always kept (death, sacrifice, level-up, etc.)
    TArray<FMXEventData> General;       // Kept only if space allows

    // Always keep the first 10 entries (birth context)
    int32 BirthContextCount = FMath::Min(10, Log->Num());
    for (int32 i = 0; i < BirthContextCount; i++)
    {
        Preserved.Add((*Log)[i]);
    }

    // Classify remaining entries
    for (int32 i = BirthContextCount; i < Log->Num(); i++)
    {
        const FMXEventData& Event = (*Log)[i];
        if (IsPreservedEventType(Event.event_type))
        {
            Preserved.Add(Event);
        }
        else
        {
            General.Add(Event);
        }
    }

    // Budget remaining slots for general events (newest first)
    int32 RemainingSlots = FMath::Max(0, MaxEntriesPerRobot - Preserved.Num());
    int32 GeneralStartIndex = FMath::Max(0, General.Num() - RemainingSlots);

    // Rebuild the log: preserved events + recent general events
    TArray<FMXEventData> Pruned;
    Pruned.Reserve(MaxEntriesPerRobot);
    Pruned.Append(Preserved);

    for (int32 i = GeneralStartIndex; i < General.Num(); i++)
    {
        Pruned.Add(General[i]);
    }

    // Sort by timestamp to restore chronological order
    Pruned.Sort([](const FMXEventData& A, const FMXEventData& B)
    {
        return A.timestamp < B.timestamp;
    });

    *Log = Pruned;

    UE_LOG(LogTemp, Log, TEXT("MXLifeLog: Pruned log for robot to %d entries."), Log->Num());
}

bool UMXLifeLog::IsPreservedEventType(EEventType Type)
{
    // These event types are too significant to lose during pruning
    return Type == EEventType::Death          ||
           Type == EEventType::Sacrifice      ||
           Type == EEventType::LevelUp        ||
           Type == EEventType::SpecChosen     ||
           Type == EEventType::Rescue         ||
           Type == EEventType::RunComplete    ||
           Type == EEventType::ComboDiscovered;
}
