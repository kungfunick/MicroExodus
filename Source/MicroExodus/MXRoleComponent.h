// MXRoleComponent.h — Specialization Module Version 1.0
// Created: 2026-02-22
// Agent 5: Specialization
// Consumers: Swarm (speed/weight), Hazard (lethal absorption, dodge), XPDistributor (xp modifier)
// Change Log:
//   v1.0 — Initial definition

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXTypes.h"
#include "MXRobotProfile.h"
#include "MXSpecData.h"
#include "MXInterfaces.h"
#include "MXRoleComponent.generated.h"

/**
 * UMXRoleComponent
 * UActorComponent attached to each robot actor in the scene.
 * Reads the robot's spec path from its profile and caches the computed FMXSpecBonus.
 * Exposes per-frame accessor methods consumed by Swarm, Hazard, and XP systems.
 *
 * Per-run charges (lethal hit absorptions, revives) are tracked here and reset each run.
 * The Naked Run modifier disables all bonus output without destroying the component.
 *
 * Attach this component to robot Blueprint actors in the editor, then call
 * InitializeFromProfile() after the robot is spawned and assigned its ID.
 */
UCLASS(ClassGroup = (MicroExodus), meta = (BlueprintSpawnableComponent))
class MICROEXODUS_API UMXRoleComponent : public UActorComponent,
                                          public IMXPersistable
{
    GENERATED_BODY()

public:

    UMXRoleComponent();

    // -------------------------------------------------------------------------
    // Initialization
    // -------------------------------------------------------------------------

    /**
     * Reads the robot's full spec path from its FMXRobotProfile and computes the FMXSpecBonus.
     * Also initializes per-run charge counters (lethal hits remaining, revives remaining).
     * Call this immediately after the robot actor is spawned and its profile is available.
     *
     * @param Robot     The robot's full persistent profile (read-only snapshot).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    void InitializeFromProfile(const FMXRobotProfile& Robot);

    // -------------------------------------------------------------------------
    // Bonus Accessors
    // -------------------------------------------------------------------------

    /**
     * Returns the full computed FMXSpecBonus for this robot.
     * Returns a default (all 1.0 / zero) bonus if bonuses are inactive (Naked Run).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    FMXSpecBonus GetSpecBonus() const;

    /**
     * Returns this robot's speed modifier (multiplicative, applied to swarm base speed).
     * Returns 1.0 if bonuses are inactive.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    float GetSpeedModifier() const;

    /**
     * Returns this robot's weight multiplier (for SwarmGate weight plate calculations).
     * Returns 1.0 if bonuses are inactive.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    float GetWeightModifier() const;

    /**
     * Returns this robot's XP modifier (for XPDistributor to scale awards).
     * Returns 1.0 if bonuses are inactive.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    float GetXPModifier() const;

    /**
     * Returns the hazard reveal radius for Lookout-branch robots (in world units).
     * Returns 0.0 if bonuses are inactive or robot is not Lookout.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    float GetHazardRevealRadius() const;

    /**
     * Returns the auto-dodge chance for Lookout-branch robots (0.0–1.0).
     * Returns 0.0 if bonuses are inactive or robot is not Lookout.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    float GetAutoDodgeChance() const;

    // -------------------------------------------------------------------------
    // Per-Run Charge Management — Lethal Hits
    // -------------------------------------------------------------------------

    /**
     * Returns the number of lethal hit absorptions remaining for this run.
     * Returns 0 if bonuses are inactive.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    int32 GetLethalHitsRemaining() const;

    /**
     * Attempts to absorb a lethal hit. Decrements the counter if charges remain.
     * @return true if the hit was absorbed (robot survives), false if no charges remain (robot dies).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    bool AbsorbLethalHit();

    // -------------------------------------------------------------------------
    // Per-Run Charge Management — Revives
    // -------------------------------------------------------------------------

    /**
     * Returns the number of robot revives remaining for this run.
     * Returns 0 if bonuses are inactive.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    int32 GetRevivesRemaining() const;

    /**
     * Attempts to use a revive charge. Decrements the counter if charges remain.
     * @return true if a revive was used successfully, false if no charges remain.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    bool UseRevive();

    // -------------------------------------------------------------------------
    // Run Boundary Management
    // -------------------------------------------------------------------------

    /**
     * Resets per-run charges (lethal hits and revives) back to their maximum values.
     * Must be called at the start of each new run for robots that persist between runs.
     * The Roguelike module (RunManager) is responsible for calling this on all active robots.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    void ResetPerRunCharges();

    // -------------------------------------------------------------------------
    // Naked Run Modifier
    // -------------------------------------------------------------------------

    /**
     * Enables or disables all spec bonuses. When inactive, all accessors return baseline values.
     * Called by the Roguelike module when the "Naked Run" tier modifier is active.
     *
     * @param bActive   true = bonuses enabled (normal play), false = all bonuses suppressed.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    void SetBonusesActive(bool bActive);

    /**
     * Returns whether spec bonuses are currently active for this robot.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    bool AreBonusesActive() const { return bBonusesActive; }

    // -------------------------------------------------------------------------
    // Read-Only Profile Info
    // -------------------------------------------------------------------------

    /** Returns the robot's role (for UI display and ability gating). */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    ERobotRole GetRole() const { return CachedRole; }

    /** Returns the robot's Tier 2 spec. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    ETier2Spec GetTier2Spec() const { return CachedTier2; }

    /** Returns the robot's Tier 3 spec. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    ETier3Spec GetTier3Spec() const { return CachedTier3; }

    /** Returns the robot's mastery title. */
    UFUNCTION(BlueprintCallable, Category = "MX|Specialization")
    EMasteryTitle GetMasteryTitle() const { return CachedMastery; }

    // -------------------------------------------------------------------------
    // IMXPersistable — serializes per-run charge state for mid-run save support
    // -------------------------------------------------------------------------

    virtual void MXSerialize(FArchive& Ar) override;
    virtual void MXDeserialize(FArchive& Ar) override;

protected:

    virtual void BeginPlay() override;

private:

    // -------------------------------------------------------------------------
    // Cached Spec Path (populated from FMXRobotProfile at Init time)
    // -------------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    ERobotRole    CachedRole    = ERobotRole::None;

    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    ETier2Spec    CachedTier2   = ETier2Spec::None;

    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    ETier3Spec    CachedTier3   = ETier3Spec::None;

    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    EMasteryTitle CachedMastery = EMasteryTitle::None;

    // -------------------------------------------------------------------------
    // Computed Bonus (cached — recomputed on Init or spec change)
    // -------------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    FMXSpecBonus CachedBonus;

    // -------------------------------------------------------------------------
    // Per-Run Charges
    // -------------------------------------------------------------------------

    /** Remaining lethal hit absorptions for the current run. */
    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    int32 LethalHitsRemaining = 0;

    /** Remaining robot revives for the current run. */
    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    int32 RevivesRemaining = 0;

    // -------------------------------------------------------------------------
    // Naked Run Flag
    // -------------------------------------------------------------------------

    /** When false, all GetSpecBonus / accessor calls return baseline values. */
    UPROPERTY(VisibleAnywhere, Category = "MX|Debug")
    bool bBonusesActive = true;
};
