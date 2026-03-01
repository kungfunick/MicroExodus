// MXMessageDispatcher.cpp — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS

#include "MXMessageDispatcher.h"

UMXMessageDispatcher::UMXMessageDispatcher()
{
}

// ---------------------------------------------------------------------------
// DispatchEvent
// ---------------------------------------------------------------------------

void UMXMessageDispatcher::DispatchEvent(const FMXEventData& Event)
{
    UE_LOG(LogTemp, Verbose, TEXT("[DEMS] Dispatching event: type=%d robot=%s msg=\"%s\""),
           (int32)Event.event_type, *Event.robot_name, *Event.message_text);

    // Iterate with index so we can remove stale entries mid-loop.
    for (int32 i = Listeners.Num() - 1; i >= 0; --i)
    {
        TScriptInterface<IMXEventListener>& ListenerRef = Listeners[i];

        // Prune destroyed objects.
        if (!ListenerRef.GetObject() || !IsValid(ListenerRef.GetObject()))
        {
            UE_LOG(LogTemp, Warning, TEXT("[DEMS] Pruning stale listener at index %d."), i);
            Listeners.RemoveAt(i);
            continue;
        }

        // Dispatch via interface.
        IMXEventListener::Execute_OnDEMSEvent(ListenerRef.GetObject(), Event);
    }

    // Blueprint delegate broadcast.
    OnEventDispatched.Broadcast(Event);
}

// ---------------------------------------------------------------------------
// RegisterListener
// ---------------------------------------------------------------------------

void UMXMessageDispatcher::RegisterListener(TScriptInterface<IMXEventListener> Listener)
{
    if (!Listener.GetObject())
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEMS] RegisterListener called with null object — ignored."));
        return;
    }

    // Check for duplicate.
    for (const TScriptInterface<IMXEventListener>& Existing : Listeners)
    {
        if (Existing.GetObject() == Listener.GetObject())
        {
            return; // Already registered.
        }
    }

    Listeners.Add(Listener);
    UE_LOG(LogTemp, Log, TEXT("[DEMS] Registered listener: %s (total: %d)"),
           *Listener.GetObject()->GetName(), Listeners.Num());
}

// ---------------------------------------------------------------------------
// UnregisterListener
// ---------------------------------------------------------------------------

void UMXMessageDispatcher::UnregisterListener(TScriptInterface<IMXEventListener> Listener)
{
    if (!Listener.GetObject()) return;

    const int32 Removed = Listeners.RemoveAll([&Listener](const TScriptInterface<IMXEventListener>& Entry)
    {
        return Entry.GetObject() == Listener.GetObject();
    });

    if (Removed > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[DEMS] Unregistered listener: %s (remaining: %d)"),
               *Listener.GetObject()->GetName(), Listeners.Num());
    }
}

// ---------------------------------------------------------------------------
// ClearAllListeners
// ---------------------------------------------------------------------------

void UMXMessageDispatcher::ClearAllListeners()
{
    Listeners.Empty();
    UE_LOG(LogTemp, Log, TEXT("[DEMS] All listeners cleared."));
}

// ---------------------------------------------------------------------------
// GetListenerCount
// ---------------------------------------------------------------------------

int32 UMXMessageDispatcher::GetListenerCount() const
{
    return Listeners.Num();
}
