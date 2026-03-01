// MXEvents.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: All agents (event bus is the primary cross-module communication channel)
// Change Log:
//   v1.0 â€” Initial definition of all delegates and FMXEventBus

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXTypes.h"
#include "MXEventData.h"
#include "MXEventFields.h"
#include "MXRunData.h"
#include "MXEvents.generated.h"

// ---------------------------------------------------------------------------
// Delegate Declarations
// ---------------------------------------------------------------------------

/** Fired when a robot is rescued from captivity mid-level. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnRobotRescued,
    FGuid, RobotId,
    int32, LevelNumber
);

/** Fired when a robot dies. Includes the fully constructed DEMS death event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnRobotDied,
    FGuid, RobotId,
    FMXEventData, DeathEvent
);

/** Fired when a robot voluntarily sacrifices itself. Includes the sacrifice event data. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnRobotSacrificed,
    FGuid, RobotId,
    FMXEventData, SacrificeEvent
);

/** Fired when a robot survives a near-miss detection (entered hazard radius, escaped). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnNearMiss,
    FGuid, RobotId,
    FMXObjectEventFields, HazardFields
);

/** Fired when a level is fully cleared. Includes the list of robots that survived it. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnLevelComplete,
    int32, LevelNumber,
    const TArray<FGuid>&, SurvivingRobots
);

/** Fired when a run ends in success. Includes the finalized run data. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnRunComplete,
    FMXRunData, RunData
);

/** Fired when a run ends in failure (team wipe or abort). Includes the last level reached. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnRunFailed,
    FMXRunData, RunData,
    int32, FailureLevel
);

/** Fired when a hat is equipped on a robot at the start of a run. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnHatEquipped,
    FGuid, RobotId,
    int32, HatId
);

/** Fired when a hatted robot dies and the hat is permanently lost. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnHatLost,
    FGuid, RobotId,
    int32, HatId,
    FMXEventData, DeathEvent
);

/** Fired when a robot levels up. Reports the new level reached. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnLevelUp,
    FGuid, RobotId,
    int32, NewLevel
);

/** Fired when a robot commits to a spec choice. Reports the full chosen path. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FOnSpecChosen,
    FGuid, RobotId,
    ERobotRole, Role,
    ETier2Spec, Tier2,
    ETier3Spec, Tier3
);

/** Fired when the ComboDetector discovers a new hat combo from active hat assignments. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnComboDiscovered,
    const TArray<int32>&, HatIds,
    int32, UnlockedHatId
);

/** Fired when a hat's unlock condition is met and it is added to the collection for the first time. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnHatUnlocked,
    int32, HatId
);

// ---------------------------------------------------------------------------
// FMXEventBus
// ---------------------------------------------------------------------------

/**
 * FMXEventBus
 * Singleton UObject that holds all multicast delegate instances.
 * Accessed via the GameInstance subsystem (UMXEventBusSubsystem).
 * 
 * Usage pattern:
 *   UMXEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UMXEventBusSubsystem>();
 *   Bus->EventBus->OnRobotDied.AddDynamic(this, &UMyClass::HandleRobotDied);
 * 
 * Fire events:
 *   Bus->EventBus->OnRobotDied.Broadcast(RobotId, DeathEvent);
 */
UCLASS(BlueprintType)
class MICROEXODUS_API UMXEventBus : public UObject
{
    GENERATED_BODY()

public:

    /** Broadcast when a robot is rescued mid-level. Listened to by: Identity, Reports, Camera, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnRobotRescued OnRobotRescued;

    /** Broadcast when a robot dies. Listened to by: Identity, DEMS, Evolution, Camera, Reports, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnRobotDied OnRobotDied;

    /** Broadcast when a robot sacrifices itself. Listened to by: Identity, DEMS, Evolution, Camera, Reports, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnRobotSacrificed OnRobotSacrificed;

    /** Broadcast on near-miss detection. Listened to by: DEMS, Identity, Evolution, Camera. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnNearMiss OnNearMiss;

    /** Broadcast when a level is cleared. Listened to by: Roguelike, Reports, Camera, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnLevelComplete OnLevelComplete;

    /** Broadcast on run success. Listened to by: Reports, Persistence, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnRunComplete OnRunComplete;

    /** Broadcast on run failure. Listened to by: Reports, Persistence, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnRunFailed OnRunFailed;

    /** Broadcast when a hat is equipped. Listened to by: DEMS, Reports, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnHatEquipped OnHatEquipped;

    /** Broadcast when a hat is lost on death. Listened to by: Hats, DEMS, Reports, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnHatLost OnHatLost;

    /** Broadcast on robot level-up. Listened to by: Identity, Specialization, DEMS, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnLevelUp OnLevelUp;

    /** Broadcast when a spec is chosen. Listened to by: Identity, DEMS, Evolution, Reports, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnSpecChosen OnSpecChosen;

    /** Broadcast when a hat combo is discovered. Listened to by: Hats, DEMS, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnComboDiscovered OnComboDiscovered;

    /** Broadcast when a new hat is permanently unlocked. Listened to by: Hats, UI. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Events")
    FOnHatUnlocked OnHatUnlocked;
};
