// MXTierSystem.h — Roguelike Module v1.0
// Agent 6: Roguelike
// Manages difficulty tier unlocking, multipliers, and per-tier gameplay effects.
// Only the Roguelike module uses FMXTierEffects — it is NOT in Shared.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MXTypes.h"
#include "MXConstants.h"
#include "MXTierSystem.generated.h"

// ---------------------------------------------------------------------------
// FMXTierEffects
// Module-internal struct. Describes the gameplay modifications active on a given tier.
// NOT placed in Shared headers — only Roguelike reads/writes tier effects.
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMXTierEffects
{
    GENERATED_BODY()

    /** Multiplier applied to hazard movement and trigger speed (1.0 = normal). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HazardSpeedMultiplier = 1.0f;

    /** Fraction by which rescue spawn counts are reduced (0.0 = full count, 1.0 = no spawns). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnReduction = 0.0f;

    /** Multiplier applied to navigable path widths in layout generation (1.0 = normal, 0.5 = half-width). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PathWidthMultiplier = 1.0f;

    /** If true, fog-of-war is active — robots outside a short radius are not visible. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFogOfWar = false;

    /** If true, hazard types are randomised each room rather than following the layout definition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRandomHazards = false;

    /** If true, any robot below Level 15 is instantly destroyed by any hazard contact. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOneHitSubLevel15 = false;

    /** If true, the run ends immediately on the first permanent robot loss of any kind. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bPermadeathOnFirstLoss = false;
};

// ---------------------------------------------------------------------------
// UMXTierSystem
// ---------------------------------------------------------------------------

/**
 * UMXTierSystem
 * Manages which difficulty tiers the player has unlocked and exposes tier effects
 * to MXRunManager at run-start. Persisted state (unlocked tier flags) is serialised
 * by MXRunManager via IMXPersistable.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXTierSystem : public UObject
{
    GENERATED_BODY()

public:

    UMXTierSystem();

    // -------------------------------------------------------------------------
    // Tier Queries
    // -------------------------------------------------------------------------

    /**
     * Returns true if the specified tier has been permanently unlocked by the player.
     * Standard (Tier 0) is always unlocked by default.
     * @param Tier  The tier to check.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    bool IsTierUnlocked(ETier Tier) const;

    /**
     * Returns the XP and score multiplier for the given tier.
     * Values sourced from MXConstants::TIER_MULTIPLIERS[].
     * @param Tier  The tier to query.
     * @return      Multiplier (1.0 for Standard, up to 10.0 for Legendary).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    float GetTierMultiplier(ETier Tier) const;

    /**
     * Returns the full set of gameplay effects active on the given tier.
     * @param Tier  The tier to query.
     * @return      FMXTierEffects struct populated for that tier.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    FMXTierEffects GetTierEffects(ETier Tier) const;

    /**
     * Returns a human-readable display name for the given tier.
     * @param Tier  The tier to name.
     * @return      String like "Standard", "Hardened", etc.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    FString GetTierDisplayName(ETier Tier) const;

    // -------------------------------------------------------------------------
    // Tier Progression
    // -------------------------------------------------------------------------

    /**
     * Permanently unlocks a tier. Called by MXRunManager after a qualifying run completion.
     * Idempotent — unlocking an already-unlocked tier is a no-op.
     * @param Tier  The tier to unlock.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    void UnlockTier(ETier Tier);

    /**
     * Checks whether the conditions to unlock the next tier are met and unlocks it if so.
     * Called at the end of every successful run by MXRunManager.
     * @param CompletedTier         The tier just completed.
     * @param SurvivingRobotCount   Number of robots alive at run end (needed for Legendary unlock).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    void EvaluateAndUnlockNextTier(ETier CompletedTier, int32 SurvivingRobotCount);

    /**
     * Returns a bitmask (as uint8) of all currently unlocked tiers for persistence.
     * Bit 0 = Standard, Bit 1 = Hardened, etc.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    uint8 GetUnlockedTiersMask() const;

    /**
     * Restores unlocked tier state from a previously serialised bitmask.
     * Called by MXRunManager during Deserialize.
     * @param Mask  Bitmask as returned by GetUnlockedTiersMask.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Tiers")
    void SetUnlockedTiersMask(uint8 Mask);

private:

    /** Tracks which tiers are permanently unlocked. Index = ETier cast to int32. */
    UPROPERTY()
    TArray<bool> UnlockedTiers;

    /** Builds the static FMXTierEffects definition for each ETier value. */
    FMXTierEffects BuildTierEffects(ETier Tier) const;
};
