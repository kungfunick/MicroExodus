// MXHatManager.h — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Consumers: GameInstance (implements IMXHatProvider), UI, Reports, DEMS
// Change Log:
//   v1.0 — Initial implementation: hat inventory management, equip/unequip, run rewards, unlock tracking

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataTable.h"
#include "MXHatData.h"
#include "MXInterfaces.h"
#include "MXEvents.h"
#include "MXRunData.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXHatManager.generated.h"

class UMXHatStackEconomy;
class UMXComboDetector;
class UMXHatUnlockChecker;

/**
 * UMXHatManager
 * Central manager for the Micro Exodus hat collection system.
 *
 * Responsibilities:
 *   - Owns the FMXHatCollection (stacks + unlocked IDs + discovered combos)
 *   - Implements IMXHatProvider for cross-module read access
 *   - Drives equip/unequip cycle: consume from stack on equip, return on survival
 *   - Coordinates with MXHatStackEconomy for arithmetic, MXComboDetector at run end,
 *     and MXHatUnlockChecker at level/run boundaries
 *   - Fires all hat-related events onto the event bus
 *   - Loads DT_HatDefinitions at startup; definitions are immutable at runtime
 *
 * Usage:
 *   Created and owned by the GameInstance. Exposed via IMXHatProvider.
 *   Other modules obtain it via: GameInstance->GetSubsystem or interface cast.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXHatManager : public UObject, public IMXHatProvider
{
    GENERATED_BODY()

public:

    UMXHatManager();

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Initialize the manager. Call once from GameInstance::Init.
     * Loads DT_HatDefinitions, binds to the event bus, creates sub-components.
     * @param InEventBus   The shared event bus instance.
     * @param InRobotProviderObject  UObject implementing IMXRobotProvider.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    void Initialize(UMXEventBus* InEventBus, UObject* InRobotProviderObject);

    /**
     * Shut down gracefully — unbind all event listeners.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    void Shutdown();

    // -------------------------------------------------------------------------
    // IMXHatProvider implementation
    // -------------------------------------------------------------------------

    /** Returns the static definition for a hat ID. Returns default struct if not found. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    FMXHatDefinition GetHatData(int32 HatId) const;
    virtual FMXHatDefinition GetHatData_Implementation(int32 HatId) const override;

    /** Returns a copy of the player's full hat collection. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    FMXHatCollection GetHatCollection() const;
    virtual FMXHatCollection GetHatCollection_Implementation() const override;

    /** Consume one copy from the stack. Returns false if stack is empty. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    bool ConsumeHat(int32 HatId);
    virtual bool ConsumeHat_Implementation(int32 HatId) override;

    /** Return one copy to the stack (robot survived). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MX|Hats")
    void ReturnHat(int32 HatId);
    virtual void ReturnHat_Implementation(int32 HatId) override;

    // -------------------------------------------------------------------------
    // Equip / Unequip
    // -------------------------------------------------------------------------

    /**
     * Attempt to equip a hat on a robot. Consumes 1 stack copy, updates robot profile,
     * fires OnHatEquipped. Returns false if hat is unavailable or robot already has a hat.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    bool EquipHat(const FGuid& RobotId, int32 HatId);

    /**
     * Unequip a hat from a robot (robot survived). Returns hat copy to stack.
     * Clears current_hat_id on the robot profile.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    void UnequipHat(const FGuid& RobotId);

    // -------------------------------------------------------------------------
    // Unlock System
    // -------------------------------------------------------------------------

    /**
     * Permanently unlock a hat and add it to unlocked_hat_ids.
     * No-op if already unlocked. Fires OnHatUnlocked.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    void UnlockHat(int32 HatId);

    /** Returns true if HatId is in unlocked_hat_ids. */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    bool IsHatUnlocked(int32 HatId) const;

    /** Returns count of permanently unlocked hats. */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    int32 GetUnlockedCount() const;

    // -------------------------------------------------------------------------
    // Run Economy
    // -------------------------------------------------------------------------

    /**
     * Deposit run completion reward: increment stack by 1 for every unlocked hat.
     * Called on OnRunComplete and on OnRunFailed if FailureLevel >= 5.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    void DepositRunReward();

    // -------------------------------------------------------------------------
    // Paint Job Milestones
    // -------------------------------------------------------------------------

    /**
     * Check which paint job milestones have just been crossed based on current unlocked count.
     * Returns newly earned EPaintJob values (caller should apply to robot rosters as appropriate).
     * Milestones: 10=MatteBlack, 20=Chrome, 30=Camo, 40=NeonGlow, 50=Wooden,
     *             60=Marble, 70=PixelArt, 80=Holographic, 90=Invisible, 100=PureGold
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    TArray<EPaintJob> CheckPaintJobUnlocks();

    // -------------------------------------------------------------------------
    // Collection Queries
    // -------------------------------------------------------------------------

    /** Returns all hat IDs currently equipped on active robots (current_hat_id != -1). */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    TArray<int32> GetCurrentlyEquippedHatIds() const;

    /** Returns the stack count for a specific hat. 0 if not in collection. */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats")
    int32 GetStackCount(int32 HatId) const;

    // -------------------------------------------------------------------------
    // Persistence (called by Agent 10: Persistence)
    // -------------------------------------------------------------------------

    /** Serialize the hat collection to a JSON string for disk save. */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Persistence")
    FString SerializeCollection() const;

    /** Deserialize and restore the hat collection from a saved JSON string. */
    UFUNCTION(BlueprintCallable, Category = "MX|Hats|Persistence")
    bool DeserializeCollection(const FString& JsonString);

protected:

    // -------------------------------------------------------------------------
    // Event Handlers
    // -------------------------------------------------------------------------

    UFUNCTION()
    void HandleRobotDied(FGuid RobotId, FMXEventData DeathEvent);

    UFUNCTION()
    void HandleRunComplete(FMXRunData RunData);

    UFUNCTION()
    void HandleRunFailed(FMXRunData RunData, int32 FailureLevel);

    UFUNCTION()
    void HandleLevelComplete(int32 LevelNumber, const TArray<FGuid>& SurvivingRobots);

private:

    // -------------------------------------------------------------------------
    // Internal Helpers
    // -------------------------------------------------------------------------

    /** Load all hat definitions from DT_HatDefinitions DataTable into the local map. */
    void LoadHatDefinitions();

    /** Evaluate and fire all combo unlock checks at run end. */
    void RunComboCheck();

    /** Evaluate standard, hidden, and legendary unlock conditions at run end. */
    void RunEndUnlockChecks(const FMXRunData& RunData);

    /** Process a list of newly unlocked hat IDs: call UnlockHat, fire events. */
    void ProcessNewUnlocks(const TArray<int32>& NewHatIds);

    /** Retrieve IMXRobotProvider safely. Returns nullptr if not set. */
    IMXRobotProvider* GetRobotProvider() const;

    // -------------------------------------------------------------------------
    // Data
    // -------------------------------------------------------------------------

    /** Persistent hat collection — stacks, unlocked IDs, discovered combos. */
    UPROPERTY()
    FMXHatCollection Collection;

    /** Static hat definitions indexed by hat_id. Loaded once at startup. */
    TMap<int32, FMXHatDefinition> HatDefinitions;

    /** Current hat assignments: RobotId → HatId for robots currently in a run. */
    TMap<FGuid, int32> ActiveEquipments;

    /** Paint job milestones already awarded (tracks which ones have been fired). */
    TSet<EPaintJob> AwardedPaintJobs;

    /** Soft ref to DT_HatDefinitions DataTable asset. */
    UPROPERTY(EditDefaultsOnly, Category = "MX|Hats|Config")
    TSoftObjectPtr<UDataTable> HatDefinitionsTable;

    /** Cached event bus reference. */
    UPROPERTY()
    TObjectPtr<UMXEventBus> EventBus;

    /** Cached robot provider UObject (implements IMXRobotProvider). */
    UPROPERTY()
    TObjectPtr<UObject> RobotProviderObject;

    /** Sub-component: stack arithmetic. */
    UPROPERTY()
    TObjectPtr<UMXHatStackEconomy> StackEconomy;

    /** Sub-component: combo detection at run end. */
    UPROPERTY()
    TObjectPtr<UMXComboDetector> ComboDetector;

    /** Sub-component: unlock condition checking. */
    UPROPERTY()
    TObjectPtr<UMXHatUnlockChecker> UnlockChecker;

    /** Whether Initialize() has been called successfully. */
    bool bInitialized = false;
};
