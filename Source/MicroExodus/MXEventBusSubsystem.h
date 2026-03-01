// MXEventBusSubsystem.h — Phase 1 Skeleton
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MXEvents.h"
#include "MXEventBusSubsystem.generated.h"

/**
 * UMXEventBusSubsystem
 * GameInstance subsystem that owns the central event bus.
 * Access: GetGameInstance()->GetSubsystem<UMXEventBusSubsystem>()->EventBus
 */
UCLASS()
class MICROEXODUS_API UMXEventBusSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY()
    TObjectPtr<UMXEventBus> EventBus;
};
