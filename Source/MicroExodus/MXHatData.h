// MXHatData.h â€” Contract Version 1.0
// Created: 2026-02-17
// Agent 0: Contracts
// Consumers: Hats, Reports, Persistence, UI, DEMS
// Change Log:
//   v1.0 â€” Initial definition

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXHatData.generated.h"

/**
 * FMXHatDefinition
 * Static definition of a single hat type. Loaded from DT_HatDefinitions DataTable.
 * One row per hat ID (0â€“99). Immutable during runtime.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXHatDefinition : public FTableRowBase
{
    GENERATED_BODY()

    /** Unique hat identifier (0â€“99). Used as the primary key across all hat systems. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 hat_id = -1;

    /** Display name shown in the UI and referenced by DEMS. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString name;

    /** Rarity tier. Controls drop weight, visual badge, and award priority. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHatRarity rarity = EHatRarity::Common;

    /** Soft reference to the hat mesh asset. Loaded on demand when a robot equips this hat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FSoftObjectPath mesh_path;

    /** Human-readable description of how this hat is unlocked. Displayed in the hat collection UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString unlock_condition;

    /** If true, this hat does not appear in the collection UI until unlocked. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool is_hidden = false;

    /** If true, this hat is a combo hat â€” it only appears by combining two other hats. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool is_combo = false;

    /**
     * For combo hats: the hat IDs that must be combined to discover this hat.
     * Typically 2 IDs. The ComboDetector checks active hat assignments against this list.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> combo_requirements;

    /** If true, this hat can only be found on a specific level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool is_level_specific = false;

    /** The level number (1â€“20) where this hat can be found, if is_level_specific is true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 level_specific_level = -1;

    /**
     * For level-specific hats: a hat that must already be equipped on the entering robot
     * to trigger the special spawn. -1 means no prerequisite.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 level_specific_hat_required = -1;
};

/**
 * FMXHatStack
 * Represents how many copies of a single hat the player currently holds in their collection.
 * Count caps at MAX_HAT_STACK (100). Used by the HatStackEconomy.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXHatStack
{
    GENERATED_BODY()

    /** The hat type this stack refers to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 hat_id = -1;

    /** Number of copies currently in the collection (0â€“100). 0 = depleted (robot is wearing the last one). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 count = 0;
};

/**
 * FMXHatCollection
 * The player's full hat inventory. Persisted by the Persistence module.
 * Managed exclusively by the Hats module (HatManager + HatStackEconomy).
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXHatCollection
{
    GENERATED_BODY()

    /** All hat stacks currently in the collection. One entry per hat_id encountered. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXHatStack> stacks;

    /** Hat IDs whose unlock condition has been permanently satisfied. Used by the collection UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> unlocked_hat_ids;

    /** Combo hat IDs that have been discovered and added to the collection at least once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> discovered_combo_ids;
};
