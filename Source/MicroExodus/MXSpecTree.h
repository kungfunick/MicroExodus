// MXSpecTree.h — Specialization Module Version 1.0
// Created: 2026-02-22
// Agent 5: Specialization
// Consumers: UI, Identity (via events), Evolution, Reports
// Change Log:
//   v1.0 — Initial definition

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "MXTypes.h"
#include "MXRobotProfile.h"
#include "MXSpecData.h"
#include "MXInterfaces.h"
#include "MXEvents.h"
#include "MXSpecTree.generated.h"

// ---------------------------------------------------------------------------
// FMXSpecChoice
// Represents a single option presented to the player during a spec selection event.
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXSpecChoice
{
    GENERATED_BODY()

    /** Which tier this choice belongs to. 1 = Role (Lv5), 2 = Tier2 (Lv10), 3 = Tier3 (Lv20). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ChoiceTier = 0;

    /** Valid when ChoiceTier == 1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERobotRole RoleOption = ERobotRole::None;

    /** Valid when ChoiceTier == 2. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier2Spec Tier2Option = ETier2Spec::None;

    /** Valid when ChoiceTier == 3. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier3Spec Tier3Option = ETier3Spec::None;

    /** Short player-facing name shown in the selection UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    /** Full ability description shown in the selection UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    /** Text description of the visual change this spec applies to the robot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString VisualPreview;
};

// ---------------------------------------------------------------------------
// UMXSpecTree
// Manages the skill tree structure, validates and applies spec choices,
// and fires OnSpecChosen events to the event bus.
// ---------------------------------------------------------------------------

/**
 * UMXSpecTree
 * Authoritative manager for the robot specialization system.
 * Loads DT_SpecTree at startup, provides choice menus when level thresholds are crossed,
 * commits choices to the robot profile via IMXRobotProvider, and fires OnSpecChosen.
 *
 * Level thresholds (GDD-canonical):
 *   Level 5  → Role selection   (Scout / Guardian / Engineer)
 *   Level 10 → Tier 2 selection (2 options per Role)
 *   Level 20 → Tier 3 selection (2 options per Tier 2)
 *   Level 35 → Mastery title    (auto-assigned, no player choice)
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXSpecTree : public UObject,
                                     public IMXPersistable
{
    GENERATED_BODY()

public:

    UMXSpecTree();

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Must be called once after construction.
     * Loads DT_SpecTree, binds to the event bus OnLevelUp delegate.
     * @param InSpecTreeTable   DataTable asset loaded from Content/DataTables/DT_SpecTree.
     * @param InRobotProvider   IMXRobotProvider implementation (RobotManager).
     * @param InEventBus        The singleton event bus for binding and broadcasting.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    void Initialize(UDataTable* InSpecTreeTable,
                    TScriptInterface<IMXRobotProvider> InRobotProvider,
                    UMXEventBus* InEventBus);

    // -------------------------------------------------------------------------
    // Choice Query
    // -------------------------------------------------------------------------

    /**
     * Given a robot's current level and existing spec path, returns the available options.
     * Returns an empty array if no choice is pending or if the robot has already chosen at
     * the current tier.
     * @param Robot     The full robot profile to evaluate.
     * @return          Array of FMXSpecChoice (2–3 entries when a choice is pending).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    TArray<FMXSpecChoice> GetAvailableChoices(const FMXRobotProfile& Robot) const;

    /**
     * Returns true if the robot has a pending unresolved spec choice based on level and path.
     * @param Robot     The robot profile to check.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    bool IsChoicePending(const FMXRobotProfile& Robot) const;

    // -------------------------------------------------------------------------
    // Choice Application
    // -------------------------------------------------------------------------

    /**
     * Commits a Role (Tier 1) choice for a robot that has reached Level 5.
     * Validates that the robot is at the correct state before applying.
     * Fires OnSpecChosen on the event bus.
     * @param RobotId   The robot to update.
     * @param Role      The chosen role.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    void ApplyRoleChoice(const FGuid& RobotId, ERobotRole Role);

    /**
     * Commits a Tier 2 choice for a robot that has reached Level 10.
     * Validates that the choice is consistent with the robot's role.
     * Fires OnSpecChosen on the event bus.
     * @param RobotId   The robot to update.
     * @param Spec      The chosen Tier 2 spec.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    void ApplyTier2Choice(const FGuid& RobotId, ETier2Spec Spec);

    /**
     * Commits a Tier 3 choice for a robot that has reached Level 20.
     * Validates that the choice is consistent with the robot's Tier 2 spec.
     * Fires OnSpecChosen on the event bus.
     * @param RobotId   The robot to update.
     * @param Spec      The chosen Tier 3 spec.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    void ApplyTier3Choice(const FGuid& RobotId, ETier3Spec Spec);

    // -------------------------------------------------------------------------
    // Mastery Resolution
    // -------------------------------------------------------------------------

    /**
     * Deterministically maps a full specialization path to the correct mastery title.
     * Called automatically when a robot reaches Level 35 with a complete path.
     * @param Role      The robot's chosen Role.
     * @param Tier2     The robot's chosen Tier 2 spec.
     * @param Tier3     The robot's chosen Tier 3 spec.
     * @return          The matching EMasteryTitle, or EMasteryTitle::None if path is invalid.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    static EMasteryTitle ResolveMasteryTitle(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3);

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /**
     * Returns true if the given Role choice is valid for the robot's current state.
     * (Robot must be Lv5+, role must be None currently.)
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    bool ValidateRoleChoice(const FMXRobotProfile& Robot, ERobotRole Role) const;

    /**
     * Returns true if the given Tier 2 choice is valid (robot Lv10+, role set, choice matches role).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    bool ValidateTier2Choice(const FMXRobotProfile& Robot, ETier2Spec Spec) const;

    /**
     * Returns true if the given Tier 3 choice is valid (robot Lv20+, tier2 set, choice matches tier2).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    bool ValidateTier3Choice(const FMXRobotProfile& Robot, ETier3Spec Spec) const;

    // -------------------------------------------------------------------------
    // Description Queries (for UI)
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    FString GetRoleDescription(ERobotRole Role) const;

    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    FString GetTier2Description(ETier2Spec Spec) const;

    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    FString GetTier3Description(ETier3Spec Spec) const;

    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    FString GetMasteryDescription(EMasteryTitle Title) const;

    // -------------------------------------------------------------------------
    // Event Bus Handlers
    // -------------------------------------------------------------------------

    /**
     * Bound to OnLevelUp. Checks whether the new level triggers a spec choice or mastery award.
     * If so, fires UI notification (implementation notifies via delegate or UI subsystem).
     */
    UFUNCTION()
    void OnLevelUp(FGuid RobotId, int32 NewLevel);

    /**
     * Bound to OnRunComplete / OnRunFailed. No spec state changes needed at run boundaries
     * (per-run charges are handled by MXRoleComponent), but kept for future hooks.
     */
    UFUNCTION()
    void OnRunComplete(FMXRunData RunData);

    UFUNCTION()
    void OnRunFailed(FMXRunData RunData, int32 FailureLevel);

    // -------------------------------------------------------------------------
    // IMXPersistable
    // -------------------------------------------------------------------------

    virtual void MXSerialize(FArchive& Ar) override;
    virtual void MXDeserialize(FArchive& Ar) override;

    // -------------------------------------------------------------------------
    // Level Thresholds (GDD-canonical)
    // -------------------------------------------------------------------------

    static constexpr int32 ROLE_UNLOCK_LEVEL     = 5;
    static constexpr int32 TIER2_UNLOCK_LEVEL    = 10;
    static constexpr int32 TIER3_UNLOCK_LEVEL    = 20;
    static constexpr int32 MASTERY_UNLOCK_LEVEL  = 35;

private:

    // -------------------------------------------------------------------------
    // Internal Helpers
    // -------------------------------------------------------------------------

    /** Returns all valid Tier 2 specs for a given role. */
    TArray<ETier2Spec> GetTier2ForRole(ERobotRole Role) const;

    /** Returns all valid Tier 3 specs for a given Tier 2 spec. */
    TArray<ETier3Spec> GetTier3ForTier2(ETier2Spec Tier2) const;

    /** Looks up the FMXSpecDefinition row for a given spec combination. */
    const FMXSpecDefinition* FindSpecRow(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3) const;

    /** Applies mastery title to a robot profile when Lv35 is reached with a complete path. */
    void TryAwardMastery(const FGuid& RobotId);

    /** Broadcasts OnSpecChosen with the robot's current (possibly partial) path. */
    void BroadcastSpecChosen(const FGuid& RobotId, const FMXRobotProfile& Profile);

    // -------------------------------------------------------------------------
    // Dependencies (injected via Initialize)
    // -------------------------------------------------------------------------

    UPROPERTY()
    UDataTable* SpecTreeTable = nullptr;

    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

    UPROPERTY()
    UMXEventBus* EventBus = nullptr;
};
