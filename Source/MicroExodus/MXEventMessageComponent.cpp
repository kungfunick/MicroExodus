// MXEventMessageComponent.cpp — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS

#include "MXEventMessageComponent.h"
#include "MXMessageBuilder.h"
#include "MXMessageDispatcher.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// UMXEventMessageComponent
// ---------------------------------------------------------------------------

UMXEventMessageComponent::UMXEventMessageComponent()
    : CachedRunNumber(1)
    , CachedLevelNumber(1)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMXEventMessageComponent::BeginPlay()
{
    Super::BeginPlay();

    // Attempt to cache run/level numbers from the RunProvider if it's available.
    // Full wiring happens when the RunManager registers itself.
    // For now, defaults of 1/1 are acceptable fallbacks.
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (GI)
    {
        TScriptInterface<IMXRunProvider> RunProvider(GI);
        if (RunProvider.GetInterface())
        {
            CachedRunNumber  = IMXRunProvider::Execute_GetRunNumber(GI);
        }
    }
}

// ---------------------------------------------------------------------------
// TriggerDeathEvent
// ---------------------------------------------------------------------------

void UMXEventMessageComponent::TriggerDeathEvent(const FGuid& RobotId)
{
    BuildAndDispatch(EEventType::Death, RobotId, 0, 0);
}

// ---------------------------------------------------------------------------
// TriggerNearMissEvent
// ---------------------------------------------------------------------------

void UMXEventMessageComponent::TriggerNearMissEvent(const FGuid& RobotId)
{
    BuildAndDispatch(EEventType::NearMiss, RobotId, 0, 0);
}

// ---------------------------------------------------------------------------
// TriggerRescueEvent
// ---------------------------------------------------------------------------

void UMXEventMessageComponent::TriggerRescueEvent(const FGuid& RobotId, int32 RobotCount)
{
    BuildAndDispatch(EEventType::Rescue, RobotId, 0, RobotCount);
}

// ---------------------------------------------------------------------------
// TriggerSacrificeEvent
// ---------------------------------------------------------------------------

void UMXEventMessageComponent::TriggerSacrificeEvent(const FGuid& RobotId, int32 SavedCount)
{
    BuildAndDispatch(EEventType::Sacrifice, RobotId, SavedCount, 0);
}

// ---------------------------------------------------------------------------
// BuildAndDispatch (private)
// ---------------------------------------------------------------------------

void UMXEventMessageComponent::BuildAndDispatch(EEventType EventType, const FGuid& RobotId, int32 SavedCount, int32 RobotCount)
{
    UMXMessageBuilder*    Builder    = GetMessageBuilder();
    UMXMessageDispatcher* Dispatcher = GetMessageDispatcher();

    if (!Builder || !Dispatcher)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEMS] MXEventMessageComponent: Builder or Dispatcher unavailable for actor %s — event dropped."),
               *GetOwner()->GetName());
        return;
    }

    // Query the robot profile.
    TScriptInterface<IMXRobotProvider> RobotProvider = GetRobotProvider();
    if (!RobotProvider.GetInterface())
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEMS] MXEventMessageComponent: RobotProvider unavailable — event dropped."));
        return;
    }

    const FMXRobotProfile Robot = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);

    // For rescue events, use SavedCount slot to pass RobotCount.
    const int32 ContextCount = (EventType == EEventType::Rescue) ? RobotCount : SavedCount;

    // Build the message.
    FMXEventData EventData = Builder->BuildEventMessage(
        EventType, EventFields, Robot,
        CachedRunNumber, CachedLevelNumber, ContextCount);

    // Populate hat name if the robot is wearing one.
    if (Robot.current_hat_id >= 0)
    {
        TScriptInterface<IMXHatProvider> HatProvider = GetHatProvider();
        if (HatProvider.GetInterface())
        {
            const FMXHatDefinition HatDef = IMXHatProvider::Execute_GetHatData(HatProvider.GetObject(), Robot.current_hat_id);
            EventData.hat_worn_name = HatDef.display_name;

            // Re-fill the hat token in message_text now that we have the name.
            EventData.message_text = EventData.message_text.Replace(
                TEXT("some hat"), *HatDef.display_name, ESearchCase::CaseSensitive);
        }
    }

    // Dispatch to all listeners.
    Dispatcher->DispatchEvent(EventData);
}

// ---------------------------------------------------------------------------
// Subsystem lookups (private)
// ---------------------------------------------------------------------------

UMXMessageBuilder* UMXEventMessageComponent::GetMessageBuilder() const
{
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (!GI) return nullptr;
    // The DEMS subsystem is expected to be a GameInstanceSubsystem that exposes
    // GetMessageBuilder(). When the DEMS subsystem is implemented (future agent),
    // replace this with: GI->GetSubsystem<UMXDEMSSubsystem>()->GetMessageBuilder();
    // For now, store the builder on the GameInstance via a known interface.
    return Cast<UMXMessageBuilder>(GI->GetSubsystemBase(UMXMessageBuilder::StaticClass()));
}

UMXMessageDispatcher* UMXEventMessageComponent::GetMessageDispatcher() const
{
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (!GI) return nullptr;
    return Cast<UMXMessageDispatcher>(GI->GetSubsystemBase(UMXMessageDispatcher::StaticClass()));
}

TScriptInterface<IMXRobotProvider> UMXEventMessageComponent::GetRobotProvider() const
{
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (!GI) return TScriptInterface<IMXRobotProvider>();
    // The RobotManager is expected to register itself on the GameInstance.
    if (GI->Implements<UMXRobotProvider>())
    {
        return TScriptInterface<IMXRobotProvider>(GI);
    }
    return TScriptInterface<IMXRobotProvider>();
}

TScriptInterface<IMXHatProvider> UMXEventMessageComponent::GetHatProvider() const
{
    UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    if (!GI) return TScriptInterface<IMXHatProvider>();
    if (GI->Implements<UMXHatProvider>())
    {
        return TScriptInterface<IMXHatProvider>(GI);
    }
    return TScriptInterface<IMXHatProvider>();
}
