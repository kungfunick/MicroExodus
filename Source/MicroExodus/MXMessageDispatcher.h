// MXMessageDispatcher.h — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS
// Routes constructed FMXEventData to all registered IMXEventListener implementations.
// Also exposes a Blueprint-assignable delegate for Blueprint-side consumers.
// Every significant moment in the game passes through here exactly once.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXEventData.h"
#include "MXInterfaces.h"
#include "MXMessageDispatcher.generated.h"

// ---------------------------------------------------------------------------
// Delegate declaration
// Blueprint-accessible broadcast fired alongside interface dispatch.
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMXEventDispatched, const FMXEventData&, Event);

// ---------------------------------------------------------------------------
// UMXMessageDispatcher
// ---------------------------------------------------------------------------

UCLASS(BlueprintType)
class MICROEXODUS_API UMXMessageDispatcher : public UObject
{
    GENERATED_BODY()

public:

    UMXMessageDispatcher();

    /**
     * Broadcast a fully-resolved FMXEventData to all registered listeners.
     * Calls OnDEMSEvent on every IMXEventListener in the registry,
     * then fires the OnEventDispatched Blueprint delegate.
     * Stale (destroyed) listeners are silently pruned during dispatch.
     * @param Event  The constructed event to broadcast.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dispatch")
    void DispatchEvent(const FMXEventData& Event);

    /**
     * Register an object as an event listener. The object must implement IMXEventListener.
     * Duplicate registrations are ignored (identity check by pointer).
     * @param Listener  The listener to add.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dispatch")
    void RegisterListener(TScriptInterface<IMXEventListener> Listener);

    /**
     * Unregister a previously registered listener.
     * Safe to call even if the listener was never registered.
     * @param Listener  The listener to remove.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dispatch")
    void UnregisterListener(TScriptInterface<IMXEventListener> Listener);

    /**
     * Remove all registered listeners.
     * Call between runs or on level teardown to avoid dispatching to stale actors.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dispatch")
    void ClearAllListeners();

    /**
     * Returns the number of currently registered listeners. Useful for debugging.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dispatch")
    int32 GetListenerCount() const;

    /**
     * Blueprint-assignable multicast delegate.
     * Fired after all interface listeners have been called.
     * Bind from Blueprint to receive DEMS events without C++ coupling.
     */
    UPROPERTY(BlueprintAssignable, Category = "MX|DEMS|Dispatch")
    FOnMXEventDispatched OnEventDispatched;

private:

    /** Registered listener objects. TScriptInterface keeps a strong UObject ref + interface pointer. */
    UPROPERTY()
    TArray<TScriptInterface<IMXEventListener>> Listeners;
};
