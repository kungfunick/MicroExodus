// MXRobotFactoryData.h — Robot Factory Module v1.0
// Created: 2026-03-01
// Agent 14: Robot Factory
// All data structs for the lobby/factory pre-level system.
//
// Concepts:
//   The Robot Factory is the metagame layer between runs/levels.
//   Before each level, players enter the lobby where they pick robots
//   from a pool of survivors, rescued robots, and fresh factory recruits.
//   The mission cap is always 5, but the pool size varies by difficulty.
//
//   Robot Code: Every robot has a shareable alphanumeric code derived
//   deterministically from its GUID. Entering a code at an upgraded
//   factory replicates that exact robot (same name, appearance, personality,
//   modular parts). The replica starts fresh at Level 1 with no XP.
//   Players need the matching cosmetic parts unlocked to replicate.
//
// Change Log:
//   v1.0 — Initial definition

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXRobotAssemblyData.h"
#include "MXRobotFactoryData.generated.h"

// ---------------------------------------------------------------------------
// Enums (all declared before structs that reference them)
// ---------------------------------------------------------------------------

/** Persistent upgrade categories for the robot factory. */
UENUM(BlueprintType)
enum class EFactoryUpgrade : uint8
{
    RecruitQuality      UMETA(DisplayName = "Recruit Quality"),
    RecruitDiversity    UMETA(DisplayName = "Recruit Diversity"),
    BatchSize           UMETA(DisplayName = "Batch Size"),
    SalvageEfficiency   UMETA(DisplayName = "Salvage Efficiency"),
    CodeTerminal        UMETA(DisplayName = "Code Terminal"),
    CosmeticWorkshop    UMETA(DisplayName = "Cosmetic Workshop"),
    RepairBay           UMETA(DisplayName = "Repair Bay"),
    EliteForge          UMETA(DisplayName = "Elite Forge"),
};

/** Where a lobby recruit candidate came from. */
UENUM(BlueprintType)
enum class ERecruitSource : uint8
{
    Survivor        UMETA(DisplayName = "Survivor"),
    Rescued         UMETA(DisplayName = "Rescued"),
    FreshRecruit    UMETA(DisplayName = "Fresh Recruit"),
    CodeReplica     UMETA(DisplayName = "Code Replica"),
};

// ---------------------------------------------------------------------------
// FMXFactoryUpgradeLevel
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXFactoryUpgradeLevel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFactoryUpgrade Upgrade = EFactoryUpgrade::RecruitQuality;

    /** Current level (0 = not purchased). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxLevel = 5;

    /** Cost to upgrade to next level (salvage parts). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NextUpgradeCost = 10;
};

// ---------------------------------------------------------------------------
// FMXFactoryState — complete persistent factory state
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXFactoryState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXFactoryUpgradeLevel> Upgrades;

    /** Salvage parts currency. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SalvageParts = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LifetimeSalvageParts = 0;

    /** Unlocked cosmetic part IDs (for Cosmetic Workshop). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> UnlockedCosmeticParts;

    /** Codes entered (prevents duplicate replications). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> RedeemedCodes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LifetimeRobotsProduced = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LifetimeCodesRedeemed = 0;
};

// ---------------------------------------------------------------------------
// FMXRecruitCandidate — a robot available for selection in the lobby
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRecruitCandidate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RobotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecruitSource Source = ERecruitSource::FreshRecruit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERobotRole Role = ERobotRole::None;

    /** The shareable robot code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RobotCode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFromCode = false;

    /** Assembly recipe preview (for lobby UI to show modular parts). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXAssemblyRecipe Recipe;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSelected = false;
};

// ---------------------------------------------------------------------------
// FMXSalvageDrop — a part found during a level run
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXSalvageDrop
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPartSlot Slot = EPartSlot::Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ScrapValue = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNewUnlock = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FoundOnLevel = 0;
};

// ---------------------------------------------------------------------------
// FMXLobbyPool — the complete candidate pool for one level's lobby
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXLobbyPool
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXRecruitCandidate> Candidates;

    /** Max robots selectable for this level's mission (always 5). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxSelectable = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FreshRecruitCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LevelNumber = 1;

    /** Salvage drops from previous level (displayed in lobby). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXSalvageDrop> PendingSalvage;
};
