// MXModifierRegistry.h — Roguelike Module v1.0
// Agent 6: Roguelike
// Manages run modifier definitions loaded from DT_TierModifiers.csv.
// Tracks which modifiers are unlocked and combines their effects into FMXModifierEffects.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MXTypes.h"
#include "MXModifierRegistry.generated.h"

// ---------------------------------------------------------------------------
// FMXModifierRow — DataTable row struct for DT_TierModifiers.csv
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMXModifierRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Unique string key matching the modifier names used in FMXRunData::active_modifiers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ModifierName;

    /** Human-readable description shown on the run-setup screen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    /** Additional XP bonus multiplier stacked on top of the tier multiplier (0.25 = +25%). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float XPBonus = 0.0f;

    /** Human-readable description of the condition required to unlock this modifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString UnlockCondition;

    /** Robot movement speed multiplier (1.0 = no change). Applies to Overcharge. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpeedMult = 1.0f;

    /** Maximum roster size override for this run (-1 = no override). Applies to Skeleton Crew. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRobots = -1;

    /** If true, any hazard contact destroys the robot instantly. Applies to Glass Cannon. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool OneHit = false;

    /** If true, no rescue robots spawn during the run. Applies to Iron Swarm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool NoRescues = false;

    /** If true, hazard types are randomised every room. Applies to Chaos Mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool RandomHazards = false;

    /** Visibility range multiplier (1.0 = full, 0.5 = half). Applies to Graveyard Shift. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ReducedVisibility = 1.0f;

    /** If true, specialization stat bonuses are suppressed. Applies to Naked Run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool NoSpecBonuses = false;

    /** If true, all robots can wear hats regardless of slot limits. Applies to Hat Parade. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool UnlimitedHats = false;

    /** If true, equipped hats provide minor stat bonuses. Applies to Fashionista. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool HatBonuses = false;
};

// ---------------------------------------------------------------------------
// FMXModifierEffects — Aggregated effect of all active modifiers for a run
// Module-internal struct. NOT in Shared headers.
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FMXModifierEffects
{
    GENERATED_BODY()

    /** Combined speed multiplier from all active modifiers (1.0 = no change). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpeedMultiplier = 1.0f;

    /** If >= 0, caps the maximum number of robots for this run (Skeleton Crew). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRobotOverride = -1;

    /** If true, any hazard contact is lethal regardless of robot stats (Glass Cannon). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOneHitKill = false;

    /** If true, rescue robots do not spawn anywhere in the run (Iron Swarm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNoRescues = false;

    /** If true, hazard types are randomised every room (Chaos Mode). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRandomHazards = false;

    /** GUID of the escort robot that must survive or the run fails (Legendary Escort). Invalid = not active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid EscortTargetRobot;

    /** Visibility range multiplier (1.0 = full visibility, 0.5 = half range). Graveyard Shift. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float VisibilityMultiplier = 1.0f;

    /** If true, all specialization stat bonuses are suppressed (Naked Run). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNoSpecBonuses = false;

    /** If true, robots can wear hats without slot restrictions (Hat Parade). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUnlimitedHatSlots = false;

    /** If true, equipped hats provide minor stat bonuses (Fashionista). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHatStatBonuses = false;

    /** Sum of all active modifier XP bonus values. Added to the tier multiplier during scoring. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CombinedXPBonus = 0.0f;
};

// ---------------------------------------------------------------------------
// UMXModifierRegistry
// ---------------------------------------------------------------------------

/**
 * UMXModifierRegistry
 * Loads modifier definitions from DT_TierModifiers and tracks which modifiers
 * have been unlocked through play. Provides combined effect resolution for the
 * active modifier list chosen at run-start.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXModifierRegistry : public UObject
{
    GENERATED_BODY()

public:

    UMXModifierRegistry();

    // -------------------------------------------------------------------------
    // Initialisation
    // -------------------------------------------------------------------------

    /**
     * Loads modifier definitions from the provided DataTable asset.
     * Must be called once during game startup before any modifier queries.
     * @param DataTable  The DT_TierModifiers DataTable asset.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    void InitialiseFromDataTable(UDataTable* DataTable);

    // -------------------------------------------------------------------------
    // Modifier Queries
    // -------------------------------------------------------------------------

    /**
     * Returns true if the specified modifier has been unlocked by the player.
     * @param ModifierName  The modifier key string (e.g., "Overcharge", "SkeletonCrew").
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    bool IsModifierUnlocked(const FString& ModifierName) const;

    /**
     * Returns the XP bonus fraction for a single modifier (e.g., 0.25 for +25%).
     * Returns 0.0 if the modifier is not found.
     * @param ModifierName  The modifier key string.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    float GetModifierXPBonus(const FString& ModifierName) const;

    /**
     * Aggregates the effects of all supplied active modifiers into a single FMXModifierEffects struct.
     * Called by MXRunManager during StartRun to apply modifiers for the duration of the run.
     * @param ActiveModifiers  Array of modifier key strings chosen at run-setup.
     * @return                 Combined effects struct.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    FMXModifierEffects GetActiveModifierEffects(const TArray<FString>& ActiveModifiers) const;

    /**
     * Returns the human-readable description of the named modifier.
     * Returns an empty string if the modifier is not found.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    FString GetModifierDescription(const FString& ModifierName) const;

    /**
     * Returns all modifier names currently loaded from the DataTable.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    TArray<FString> GetAllModifierNames() const;

    /**
     * Returns only the modifier names that are currently unlocked.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    TArray<FString> GetUnlockedModifierNames() const;

    // -------------------------------------------------------------------------
    // Modifier Progression
    // -------------------------------------------------------------------------

    /**
     * Permanently unlocks a modifier. Called by MXRunManager when a qualifying condition is met.
     * Idempotent — unlocking an already-unlocked modifier is a no-op.
     * @param ModifierName  The modifier key to unlock.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    void UnlockModifier(const FString& ModifierName);

    /**
     * Evaluates post-run state to determine if any new modifiers should be unlocked.
     * Called by MXRunManager at run end.
     * @param CompletedRunData  The finalised run data for the just-completed run.
     * @param TotalRunCount     All-time run count (including this run).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    void EvaluateUnlocks(const struct FMXRunData& CompletedRunData, int32 TotalRunCount);

    // -------------------------------------------------------------------------
    // Persistence Helpers
    // -------------------------------------------------------------------------

    /**
     * Returns a copy of the set of unlocked modifier name strings for persistence.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    TSet<FString> GetUnlockedModifierSet() const;

    /**
     * Restores unlocked modifier state from a persisted set.
     * @param RestoredSet  The set as returned by GetUnlockedModifierSet.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Modifiers")
    void SetUnlockedModifierSet(const TSet<FString>& RestoredSet);

private:

    /** All modifier definitions loaded from DT_TierModifiers, keyed by ModifierName. */
    UPROPERTY()
    TMap<FString, FMXModifierRow> ModifierDefinitions;

    /** Set of modifier names the player has permanently unlocked. */
    UPROPERTY()
    TSet<FString> UnlockedModifiers;

    /** Returns a pointer to the row for ModifierName, or nullptr if not found. */
    const FMXModifierRow* FindModifier(const FString& ModifierName) const;
};
