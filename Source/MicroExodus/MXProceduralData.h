// MXProceduralData.h — Procedural Generation Module v1.0
// Created: 2026-03-01
// Agent 12: Procedural Generation
// Consumers: Roguelike (RunManager, SpawnManager), Swarm (hazard spawning), Camera, UI
//
// All structs describing procedurally generated level content.
// These are MODULE-LOCAL — not added to Shared contracts.
// Other modules consume them through the UMXProceduralGen query interface.
//
// Change Log:
//   v1.0 — Initial definition

#pragma once

#include "CoreMinimal.h"
#include "MXTypes.h"
#include "MXProceduralData.generated.h"

// ---------------------------------------------------------------------------
// ERoomType
// ---------------------------------------------------------------------------

/**
 * The architectural archetype of a room within a generated level.
 * Controls room dimensions, hazard slot count, and connectivity rules.
 */
UENUM(BlueprintType)
enum class ERoomType : uint8
{
    Corridor        UMETA(DisplayName = "Corridor"),        // Long, narrow. 1–2 hazards. Transit room.
    Arena           UMETA(DisplayName = "Arena"),            // Large open space. 3–5 hazards. Major encounters.
    Bottleneck      UMETA(DisplayName = "Bottleneck"),       // Narrow chokepoint. 1–2 hazards. Forces single-file.
    Sanctuary       UMETA(DisplayName = "Sanctuary"),        // Safe zone. 0 hazards. Rescue spawns here.
    Gauntlet        UMETA(DisplayName = "Gauntlet"),         // Dense hazard sequence. 4–6 hazards. High lethality.
    Junction        UMETA(DisplayName = "Junction"),         // Multi-exit hub. 1–2 hazards. Path splits.
    GateRoom        UMETA(DisplayName = "Gate Room"),        // Contains a sacrifice gate. 0–1 hazards.
    Entrance        UMETA(DisplayName = "Entrance"),         // Level start. 0 hazards. Staging area.
    Exit            UMETA(DisplayName = "Exit"),             // Level end. 0–1 hazards. Goal room.
};

// ---------------------------------------------------------------------------
// FMXRoomDef
// ---------------------------------------------------------------------------

/**
 * FMXRoomDef
 * Definition of a single room in a procedurally generated level.
 * Describes the room's archetype, dimensions, and capacity — NOT its contents.
 * Hazards and rescue spawns are placed separately by the HazardPlacer.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRoomDef
{
    GENERATED_BODY()

    /** Index of this room within the level's room array. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RoomIndex = 0;

    /** Architectural archetype. Controls dimension ranges and hazard slot count. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERoomType RoomType = ERoomType::Corridor;

    /** Room bounding dimensions in Unreal units (cm). X = width, Y = length. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Size = FVector2D(800.0f, 1200.0f);

    /** World-space offset of this room's origin relative to the level origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector WorldOffset = FVector::ZeroVector;

    /** Maximum number of hazards this room can host (based on room type and size). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxHazardSlots = 2;

    /** Indices of rooms this room connects to (directed graph edges). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> ConnectedRoomIndices;

    /** Whether this room lies on the critical path from entrance to exit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOnCriticalPath = false;
};

// ---------------------------------------------------------------------------
// FMXHazardPlacement
// ---------------------------------------------------------------------------

/**
 * FMXHazardPlacement
 * A single hazard instance placed within a generated level.
 * Maps to a DT_HazardFields row type and a hazard subclass.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXHazardPlacement
{
    GENERATED_BODY()

    /** Hazard type key matching DT_HazardFields.HazardType (e.g., "FireVent", "Crusher"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString HazardType;

    /** Index of the room this hazard is placed in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RoomIndex = 0;

    /** Position relative to the room's origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector LocalPosition = FVector::ZeroVector;

    /** Rotation within the room. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;

    /** Speed multiplier applied to this hazard (from tier effects). 1.0 = normal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpeedMultiplier = 1.0f;

    /** Kill radius override. 0 = use hazard class default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float KillRadiusOverride = 0.0f;

    /** Whether this is a cycling (periodic) hazard instance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCycling = true;

    /** Phase offset in seconds so adjacent hazards don't sync up. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CycleOffset = 0.0f;
};

// ---------------------------------------------------------------------------
// FMXRescueSpawn
// ---------------------------------------------------------------------------

/**
 * FMXRescueSpawn
 * A rescue robot spawn point within a generated level.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRescueSpawn
{
    GENERATED_BODY()

    /** Index of the room containing this spawn point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RoomIndex = 0;

    /** Position relative to the room's origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector LocalPosition = FVector::ZeroVector;

    /** True if a hazard is within warning distance of this spawn — robot needs rescuing quickly. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bGuarded = false;
};

// ---------------------------------------------------------------------------
// FMXLevelConditions
// ---------------------------------------------------------------------------

/**
 * FMXLevelConditions
 * Environmental and pacing parameters for a single level.
 * Controls the "feel" of the level — its dominant element theme, density, and atmosphere.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXLevelConditions
{
    GENERATED_BODY()

    /** Dominant hazard element for this level. 60–80% of hazards will be this element. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHazardElement PrimaryElement = EHazardElement::Mechanical;

    /** Secondary element. 20–40% of hazards will be this element. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHazardElement SecondaryElement = EHazardElement::Fire;

    /**
     * Hazard density multiplier for this level.
     * 0.5 = sparse (early levels), 1.0 = standard, 2.0 = dense (late levels / gauntlets).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HazardDensity = 1.0f;

    /** Whether reduced visibility (fog-of-war) is active on this level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bReducedVisibility = false;

    /**
     * Ambient threat level (0.0–1.0). Controls pacing hints for camera and audio.
     * 0.0 = calm. 0.5 = tense. 1.0 = relentless.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AmbientThreatLevel = 0.5f;

    /**
     * Target duration for this level in seconds.
     * Shorter = more frantic. Longer = more methodical.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TargetDurationSeconds = 300.0f;
};

// ---------------------------------------------------------------------------
// FMXLevelLayout
// ---------------------------------------------------------------------------

/**
 * FMXLevelLayout
 * Complete procedurally generated content for a single level within a run.
 * Everything needed to instantiate the level's rooms, hazards, and rescue spawns.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXLevelLayout
{
    GENERATED_BODY()

    /** 1-indexed level number within the run (1–20). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LevelNumber = 1;

    /** Per-level seed derived from the master seed. Enables replaying individual levels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 LevelSeed = 0;

    /** All rooms in this level, ordered by index. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXRoomDef> Rooms;

    /** All hazard instances placed in this level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXHazardPlacement> Hazards;

    /** Rescue robot spawn points for this level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXRescueSpawn> RescueSpawns;

    /** Environmental conditions and pacing for this level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMXLevelConditions Conditions;

    /**
     * Room indices forming the shortest path from Entrance to Exit.
     * Players MUST traverse these rooms; side rooms are optional.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> CriticalPath;

    /** Room index containing the sacrifice gate, or -1 if none this level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GateRoomIndex = -1;

    /** Total hazard count (convenience — equals Hazards.Num()). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalHazardCount = 0;

    /** Total rescue spawn count (convenience — equals RescueSpawns.Num()). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalRescueCount = 0;
};

// ---------------------------------------------------------------------------
// FMXRunLayout
// ---------------------------------------------------------------------------

/**
 * FMXRunLayout
 * Complete procedurally generated content for an entire 20-level run.
 * Generated once at run start from a master seed. Deterministic: same seed = same run.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRunLayout
{
    GENERATED_BODY()

    /** Master seed from which all per-level seeds are derived. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 MasterSeed = 0;

    /** Difficulty tier this layout was generated for. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETier Tier = ETier::Standard;

    /** Human-readable modifier names active for this run (copied from RunManager). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> ActiveModifiers;

    /** All 20 level layouts, indexed by (LevelNumber - 1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXLevelLayout> Levels;

    /** Total hazards across all levels (summary stat). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalHazards = 0;

    /** Total rescue spawns across all levels (summary stat). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalRescues = 0;

    /** Total rooms across all levels (summary stat). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalRooms = 0;
};
