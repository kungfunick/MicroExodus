// MXEventBusSubsystem.cpp — Phase 1 Skeleton
#include "MXEventBusSubsystem.h"

void UMXEventBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    EventBus = NewObject<UMXEventBus>(this);
    UE_LOG(LogTemp, Log, TEXT("[MX] EventBusSubsystem initialized."));
}

void UMXEventBusSubsystem::Deinitialize()
{
    EventBus = nullptr;
    Super::Deinitialize();
}
