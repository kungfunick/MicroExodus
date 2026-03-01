// MXRobotAssemblyData.h — Robot Assembly Module v1.0
// Created: 2026-03-01
// Agent 13: Robot Assembly
// All structs and enums for the modular robot mesh system.
// Module-local — not added to Shared contracts.
//
// Design philosophy:
//   Every robot is assembled from 5 part slots. All variants within a slot
//   occupy the same bounding volume and attach to standardised sockets on a
//   shared base skeleton. Assembly is DETERMINISTIC from the robot's GUID —
//   the same robot always looks the same, no storage required.
//
//   For 100 simultaneous robots: parts are StaticMeshComponents on sockets.
//   The base skeleton handles locomotion animation. Distant robots (<LOD1)
//   collapse to a single merged proxy mesh.
//
// Change Log:
//   v1.0 — Initial definition

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MXTypes.h"
#include "MXRobotAssemblyData.generated.h"

// ---------------------------------------------------------------------------
// EPartSlot — the five modular attachment points
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EPartSlot : uint8
{
    Locomotion  UMETA(DisplayName = "Locomotion"),  // Tracks, wheels, legs, hover pads
    Body        UMETA(DisplayName = "Body"),         // Central chassis / torso
    ArmLeft     UMETA(DisplayName = "Arm Left"),     // Left tool / manipulator
    ArmRight    UMETA(DisplayName = "Arm Right"),    // Right tool / manipulator
    Head        UMETA(DisplayName = "Head"),          // Sensor dome, visor, camera eye

    Auto        UMETA(DisplayName = "Auto")  // Used as sentinel for auto-targeting
};

/** Number of real part slots (excluding the Auto sentinel). */
static constexpr int32 MX_PART_SLOT_COUNT = 5;

// ---------------------------------------------------------------------------
// EPartDamageState — per-part visual damage progression
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EPartDamageState : uint8
{
    Pristine    UMETA(DisplayName = "Pristine"),     // No visible damage
    Scratched   UMETA(DisplayName = "Scratched"),    // Minor cosmetic damage
    Cracked     UMETA(DisplayName = "Cracked"),      // Visible fractures, sparks
    Hanging     UMETA(DisplayName = "Hanging"),      // Partially detached, dangling
    Detached    UMETA(DisplayName = "Detached"),     // Fully separated, physics active
    Destroyed   UMETA(DisplayName = "Destroyed"),    // Gone — replaced with stub/sparks
};

// ---------------------------------------------------------------------------
// ELocomotionClass — locomotion archetypes (determines animation set)
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ELocomotionClass : uint8
{
    Treads      UMETA(DisplayName = "Treads"),       // Tank treads — smooth, stable
    Wheels      UMETA(DisplayName = "Wheels"),       // Fast, wobbly on terrain
    Bipedal     UMETA(DisplayName = "Bipedal"),      // Two legs — humanoid gait
    Spider      UMETA(DisplayName = "Spider"),       // Four+ legs — creepy, stable
    Hover       UMETA(DisplayName = "Hover"),        // Hover pads — floaty movement
    Unicycle    UMETA(DisplayName = "Unicycle"),      // Single wheel — comical, tippy
};

// ---------------------------------------------------------------------------
// FMXPartDefinition — one variant of one slot
// ---------------------------------------------------------------------------

/**
 * FMXPartDefinition
 * Describes a single mesh variant that can occupy a part slot.
 * Loaded from DT_RobotParts DataTable.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXPartDefinition : public FTableRowBase
{
    GENERATED_BODY()

    /** Unique identifier for this part variant. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartId;

    /** Which slot this part occupies. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPartSlot Slot = EPartSlot::Body;

    /** Human-readable display name (e.g., "Tank Treads", "Dome Head"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    /**
     * Soft reference to the StaticMesh asset for this part.
     * Loaded asynchronously at assembly time.
     * Path format: /Game/Robots/Parts/SM_Loco_Treads.SM_Loco_Treads
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UStaticMesh> MeshAsset;

    /**
     * Socket name on the base skeleton where this part attaches.
     * All variants within a slot use the SAME socket.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SocketName;

    /** Local offset from the socket origin (for fine-tuning alignment). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector AttachOffset = FVector::ZeroVector;

    /** Local rotation from the socket (degrees). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator AttachRotation = FRotator::ZeroRotator;

    /** For locomotion parts: which animation class this maps to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ELocomotionClass LocomotionClass = ELocomotionClass::Treads;

    /** Mass of this part in kg. Affects detachment physics impulse. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Mass = 5.0f;

    /** How many damage hits this part absorbs before progressing to the next damage state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HitPoints = 2;

    /**
     * Visual scale applied to this part. 1.0 = designed size.
     * Allows slight size variation per variant within the bounding standard.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 1.0f;

    /** Selection weight (higher = more common in random assembly). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Weight = 1.0f;

    /** If true, this part has a mirrored version for the opposite arm slot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bMirrorable = false;
};

// ---------------------------------------------------------------------------
// FMXPartState — runtime state of one assembled part
// ---------------------------------------------------------------------------

/**
 * FMXPartState
 * Tracks the current condition of a single part on a specific robot.
 * Lives in the assembly component during gameplay — not persisted.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXPartState
{
    GENERATED_BODY()

    /** Which slot this state is for. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPartSlot Slot = EPartSlot::Body;

    /** The part definition ID that was assembled into this slot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartId;

    /** Current damage progression. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPartDamageState DamageState = EPartDamageState::Pristine;

    /** Remaining hit points before next damage state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentHP = 2;

    /** Maximum HP from the part definition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxHP = 2;

    /**
     * Accumulated wear parameter (0.0–1.0).
     * Drives material parameter overrides per-part. Fed from FMXVisualEvolutionState.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WearAmount = 0.0f;
};

// ---------------------------------------------------------------------------
// FMXAssemblyRecipe — the complete part selection for one robot
// ---------------------------------------------------------------------------

/**
 * FMXAssemblyRecipe
 * Describes which part variant fills each slot for one robot.
 * Deterministically generated from the robot's GUID — not stored in save data.
 * Can be regenerated at any time from the same GUID.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXAssemblyRecipe
{
    GENERATED_BODY()

    /** The robot this recipe was generated for. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RobotId;

    /** Part ID selected for each slot. Index matches EPartSlot (0=Locomotion..4=Head). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> SlotPartIds;

    /** The locomotion class determined by the locomotion part (for animation selection). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ELocomotionClass LocomotionType = ELocomotionClass::Treads;

    /** Chassis color index from the robot profile (passed through for material setup). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ChassisColor = 0;

    /** Eye color index from the robot profile. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EyeColor = 0;
};

// ---------------------------------------------------------------------------
// FMXDestructionEvent — describes a single part destruction moment
// ---------------------------------------------------------------------------

/**
 * FMXDestructionEvent
 * Emitted when a part transitions damage state.
 * Used by DEMS for flavor text and by Camera for reactions.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXDestructionEvent
{
    GENERATED_BODY()

    /** Robot that was damaged. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RobotId;

    /** Which part was affected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPartSlot Slot = EPartSlot::Body;

    /** The part definition ID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartId;

    /** New damage state after this event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPartDamageState NewState = EPartDamageState::Scratched;

    /** World-space position of the impact. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ImpactLocation = FVector::ZeroVector;

    /** Direction of the impact force (normalized). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ImpactDirection = FVector::ForwardVector;

    /** The hazard element that caused this damage. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EHazardElement DamageElement = EHazardElement::Mechanical;
};

// ---------------------------------------------------------------------------
// FMXDestructionProfile — full destruction state for one robot
// ---------------------------------------------------------------------------

/**
 * FMXDestructionProfile
 * Tracks all part states for a single robot during a run.
 * Created at level start, discarded at level end (destruction resets per level).
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXDestructionProfile
{
    GENERATED_BODY()

    /** The robot this destruction state belongs to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RobotId;

    /** Per-part damage states. Index matches EPartSlot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMXPartState> Parts;

    /** Total parts still attached (Pristine through Cracked). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AttachedPartCount = 5;

    /** Whether this robot has lost any parts this level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasLostParts = false;
};
