// MXPartDestructionManager.h — Robot Assembly Module v1.0
// Created: 2026-03-01
// Agent 13: Robot Assembly
// Per-part damage progression and destruction orchestration.
// Takes damage hits from the hazard system and advances individual part
// damage states through the Pristine → Destroyed pipeline.
//
// Integrates with:
//   - DEMS event bus: fires OnPartDamaged/OnPartDetached for narrative
//   - Evolution system: destruction feeds back into visual wear
//   - Camera system: detachment events trigger dramatic reactions
//
// Manages destruction state for ALL robots in the active swarm.
// State resets each level (robots "repair" between levels — visual wear persists).
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXRobotAssemblyData.h"
#include "MXPartDestructionManager.generated.h"

// Forward declarations
class UMXRobotAssembler;
class UMXEventBus;

// ---------------------------------------------------------------------------
// Delegates — module-local events for destruction moments
// ---------------------------------------------------------------------------

/** Fired when a part takes damage and progresses to a new damage state. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnPartDamaged,
    const FMXDestructionEvent&, Event);

/** Fired when a part reaches Detached or Destroyed — triggers physics separation. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnPartDetached,
    const FMXDestructionEvent&, Event);

/** Fired when a robot loses ALL non-body parts. The sad skeleton remains. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnRobotStripped,
    const FGuid&, RobotId);

// ---------------------------------------------------------------------------
// UMXPartDestructionManager
// ---------------------------------------------------------------------------

/**
 * UMXPartDestructionManager
 *
 * Centralized damage state tracker for all robot parts in the active run.
 *
 * Damage model:
 *   1. A hazard hit calls ApplyDamage(RobotId, Slot, Element, ImpactLocation, Direction)
 *   2. The targeted part loses HP
 *   3. When HP reaches 0, the part advances to the next EPartDamageState
 *   4. State transitions fire delegates for mesh component reactions:
 *        Pristine  → Scratched  : surface decal appears
 *        Scratched → Cracked    : sparks particle emitter starts
 *        Cracked   → Hanging    : part mesh tilts, hinge constraint
 *        Hanging   → Detached   : physics impulse, part flies off
 *        Detached  → Destroyed  : mesh fades/despawns after linger time
 *
 *   The Body slot is special: it cannot be detached.
 *   If Body reaches Destroyed, the robot is dead (forwarded to DEMS).
 *
 * Part targeting heuristic:
 *   When a hazard hit doesn't specify a slot, the manager picks one:
 *   - Outermost non-destroyed parts first (arms > head > locomotion > body)
 *   - Random among eligible parts weighted by exposure
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXPartDestructionManager : public UObject
{
    GENERATED_BODY()

public:

    UMXPartDestructionManager();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Set up the assembler reference for looking up part definitions.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    void SetAssembler(UMXRobotAssembler* InAssembler);

    /**
     * Initialise destruction profiles for all robots in the current level.
     * Creates fresh FMXDestructionProfile for each robot based on their assembly recipe.
     * Call at level start, after recipes are generated.
     * @param RobotIds  All active robot GUIDs for this level.
     * @param Recipes   Matching assembly recipes (same order as RobotIds).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    void InitialiseForLevel(const TArray<FGuid>& RobotIds, const TArray<FMXAssemblyRecipe>& Recipes);

    /**
     * Reset all destruction state (call between levels).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    void ResetForNewLevel();

    // -------------------------------------------------------------------------
    // Damage Application
    // -------------------------------------------------------------------------

    /**
     * Apply a single damage hit to a robot.
     * If Slot is not specified, the manager auto-targets using the exposure heuristic.
     * @param RobotId         The robot being damaged.
     * @param DamageElement   The hazard element causing damage (for DEMS and visual mark).
     * @param ImpactLocation  World-space hit point.
     * @param ImpactDirection Normalized direction of force.
     * @param TargetSlot      Specific slot to hit, or EPartSlot::Auto for auto-target.
     * @param DamageAmount    HP to remove (default 1).
     * @return                The destruction event generated, or default if no damage applied.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    FMXDestructionEvent ApplyDamage(
        const FGuid&    RobotId,
        EHazardElement  DamageElement,
        FVector         ImpactLocation,
        FVector         ImpactDirection,
        EPartSlot       TargetSlot = EPartSlot::Auto,
        int32           DamageAmount = 1);

    /**
     * Instantly destroy all parts on a robot (total destruction / death).
     * Fires a cascade of detachment events for dramatic effect.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    void DestroyAllParts(const FGuid& RobotId, EHazardElement DamageElement, FVector ImpactLocation);

    /**
     * Apply element-specific destruction pattern:
     *   Fire   → burns from outside in (arms/head first)
     *   Crush  → collapses from top down (head → body)
     *   Fall   → locomotion breaks first, then body
     *   EMP    → all parts simultaneously flicker, head pops off
     *   Ice    → parts freeze and shatter (all detach with delay cascade)
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    TArray<FMXDestructionEvent> ApplyElementalDestruction(
        const FGuid&    RobotId,
        EHazardElement  DamageElement,
        FVector         ImpactLocation,
        FVector         ImpactDirection);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /**
     * Get the current destruction profile for a robot.
     * @param RobotId   The robot to query.
     * @param OutProfile  The destruction profile if found.
     * @return           True if the robot has an active profile.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    bool GetDestructionProfile(const FGuid& RobotId, FMXDestructionProfile& OutProfile) const;

    /**
     * Get the current damage state of a specific part.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Destruction")
    EPartDamageState GetPartDamageState(const FGuid& RobotId, EPartSlot Slot) const;

    /**
     * Check if a robot has lost any parts this level.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Destruction")
    bool HasLostParts(const FGuid& RobotId) const;

    /**
     * Get the count of robots that have taken any damage this level.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Destruction")
    int32 GetDamagedRobotCount() const;

    // -------------------------------------------------------------------------
    // Delegates
    // -------------------------------------------------------------------------

    /** Fires on any damage state transition (including minor scratches). */
    UPROPERTY(BlueprintAssignable, Category = "MX|Destruction")
    FOnPartDamaged OnPartDamaged;

    /** Fires when a part fully detaches or is destroyed. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Destruction")
    FOnPartDetached OnPartDetached;

    /** Fires when a robot has been reduced to just its body (all extremities gone). */
    UPROPERTY(BlueprintAssignable, Category = "MX|Destruction")
    FOnRobotStripped OnRobotStripped;

private:

    /** Reference to the assembler for part definition lookups. */
    UPROPERTY()
    TObjectPtr<UMXRobotAssembler> Assembler;

    /** Active destruction profiles indexed by RobotId. */
    TMap<FGuid, FMXDestructionProfile> Profiles;

    /**
     * Auto-target a part slot based on current damage state and exposure.
     * Prefers outermost undamaged parts.
     */
    EPartSlot AutoTargetSlot(const FMXDestructionProfile& Profile, FRandomStream& Stream) const;

    /**
     * Advance a part's damage state and fire the appropriate delegate.
     * @return  The generated destruction event.
     */
    FMXDestructionEvent AdvanceDamageState(
        FMXDestructionProfile& Profile,
        EPartSlot              Slot,
        EHazardElement         DamageElement,
        const FVector&         ImpactLocation,
        const FVector&         ImpactDirection);

    /** Check if a robot has been stripped (only Body remaining). */
    void CheckStrippedState(const FGuid& RobotId, const FMXDestructionProfile& Profile);
};
