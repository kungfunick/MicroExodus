// MXRobotMeshComponent.h — Robot Assembly Module v1.0
// Created: 2026-03-01
// Agent 13: Robot Assembly
// Per-robot ActorComponent that physically assembles modular mesh parts on
// the owning actor's skeleton. Handles LOD switching for 100-robot performance,
// per-part material instances for wear/damage, and detachment physics.
//
// Lives alongside UMXEvolutionLayerSystem on the robot Blueprint actor.
// Evolution feeds wear parameters → this component applies them per-part.
//
// Performance strategy for 100 robots (500 mesh components):
//   LOD0 (< 2000 cm)  : Full 5-part modular assembly. Per-part materials.
//   LOD1 (2000–5000)   : Simplified 3-part (body+loco+head merged proxy).
//   LOD2 (> 5000 cm)   : Single billboard/impostor mesh. Minimal cost.
//
//   Tick is disabled on LOD2 robots. Material updates batch at 15Hz, not per-frame.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXRobotAssemblyData.h"
#include "MXRobotMeshComponent.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMXRobotAssembler;
class UMXPartDestructionManager;

// ---------------------------------------------------------------------------
// EMeshLODLevel — LOD tiers for the modular assembly
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EMeshLODLevel : uint8
{
    Full        UMETA(DisplayName = "Full"),        // All 5 parts active.
    Simplified  UMETA(DisplayName = "Simplified"),  // 3 merged parts.
    Impostor    UMETA(DisplayName = "Impostor"),     // Single proxy mesh.
};

// ---------------------------------------------------------------------------
// Delegate: visual state change
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnMeshLODChanged,
    const FGuid&, RobotId,
    EMeshLODLevel, NewLOD);

// ---------------------------------------------------------------------------
// UMXRobotMeshComponent
// ---------------------------------------------------------------------------

/**
 * UMXRobotMeshComponent
 *
 * The visual backbone of every robot in the game.
 *
 * Lifecycle:
 *   1. BeginPlay — waits for AssembleFromRecipe call.
 *   2. AssembleFromRecipe — creates StaticMeshComponents on sockets.
 *   3. Each tick (LOD0/1 only) — updates LOD, processes pending damage visuals.
 *   4. OnPartDamaged/OnPartDetached — reacts to destruction events.
 *   5. Teardown — clears all part components.
 *
 * Each part slot gets:
 *   - A UStaticMeshComponent attached to the skeleton socket
 *   - A UMaterialInstanceDynamic for per-part wear/color/damage
 *   - Optional particle emitter (sparks for Cracked state)
 *   - Optional physics body (activated on Detached state)
 */
UCLASS(ClassGroup=(MicroExodus), meta=(BlueprintSpawnableComponent), BlueprintType)
class MICROEXODUS_API UMXRobotMeshComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXRobotMeshComponent();

    // -------------------------------------------------------------------------
    // Assembly
    // -------------------------------------------------------------------------

    /**
     * Assemble this robot's visual representation from a recipe.
     * Creates StaticMeshComponents for each part and attaches them to skeleton sockets.
     *
     * @param Recipe      The assembly recipe describing which parts go where.
     * @param Assembler   Reference to the assembler for part definition lookups.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void AssembleFromRecipe(const FMXAssemblyRecipe& Recipe, UMXRobotAssembler* Assembler);

    /**
     * Tear down all mesh components. Called when a robot is removed from the swarm
     * or at end of level for cleanup.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void Disassemble();

    /**
     * Check if this robot has been assembled.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Assembly|Mesh")
    bool IsAssembled() const { return bAssembled; }

    // -------------------------------------------------------------------------
    // LOD Management
    // -------------------------------------------------------------------------

    /**
     * Update LOD based on distance to the camera.
     * Called from Tick or externally by the swarm camera.
     * @param DistanceToCamera  Distance in cm from this robot to the active camera.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void UpdateLOD(float DistanceToCamera);

    /**
     * Force a specific LOD level (for debugging or cutscenes).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void ForceLOD(EMeshLODLevel Level);

    /**
     * Get the current LOD level.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Assembly|Mesh")
    EMeshLODLevel GetCurrentLOD() const { return CurrentLOD; }

    // -------------------------------------------------------------------------
    // Damage Visuals
    // -------------------------------------------------------------------------

    /**
     * React to a part damage event. Updates the part's material and optionally
     * spawns sparks/effects.
     * @param Event  The destruction event to react to.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void OnPartDamageEvent(const FMXDestructionEvent& Event);

    /**
     * React to a part detachment. Enables physics on the mesh component and
     * applies an impulse in the impact direction.
     * @param Event  The detachment event.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void OnPartDetachEvent(const FMXDestructionEvent& Event);

    /**
     * Apply wear parameters from the evolution system to all part materials.
     * Called by UMXEvolutionLayerSystem after RecalculateEvolution.
     * @param BurnIntensity     Fire damage wear (0–1).
     * @param CrackIntensity    Mechanical damage wear (0–1).
     * @param WeldCoverage      Repair seam visibility (0–1).
     * @param PatinaIntensity   Age patina (0–1).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void ApplyWearParameters(float BurnIntensity, float CrackIntensity,
                             float WeldCoverage, float PatinaIntensity);

    /**
     * Set the chassis color on all part materials.
     * @param ColorIndex  Index into the color palette (0–11).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void SetChassisColor(int32 ColorIndex);

    /**
     * Set the eye color (applies to head part only).
     * @param ColorIndex  Index into the eye color palette (0–7).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    void SetEyeColor(int32 ColorIndex);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /**
     * Get the StaticMeshComponent for a specific part slot.
     * Returns nullptr if the slot is empty or the robot isn't assembled.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Assembly|Mesh")
    UStaticMeshComponent* GetPartMeshComponent(EPartSlot Slot) const;

    /**
     * Get the assembly recipe this robot was built from.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Assembly|Mesh")
    const FMXAssemblyRecipe& GetRecipe() const { return CachedRecipe; }

    /**
     * Get the locomotion class (for animation selection).
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Assembly|Mesh")
    ELocomotionClass GetLocomotionClass() const { return CachedRecipe.LocomotionType; }

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** Distance threshold for LOD0 → LOD1 transition (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Assembly|LOD")
    float LOD1Distance = 2000.0f;

    /** Distance threshold for LOD1 → LOD2 transition (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Assembly|LOD")
    float LOD2Distance = 5000.0f;

    /** How long a detached part lingers before being destroyed (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Assembly|Destruction")
    float DetachedLingerTime = 3.0f;

    /** Impulse magnitude applied to detaching parts (cm/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Assembly|Destruction")
    float DetachImpulse = 500.0f;

    // -------------------------------------------------------------------------
    // Delegates
    // -------------------------------------------------------------------------

    /** Fires when the LOD level changes. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Assembly|Mesh")
    FOnMeshLODChanged OnLODChanged;

protected:

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /** Whether AssembleFromRecipe has been called. */
    bool bAssembled = false;

    /** Current LOD tier. */
    EMeshLODLevel CurrentLOD = EMeshLODLevel::Full;

    /** Cached recipe for queries. */
    FMXAssemblyRecipe CachedRecipe;

    /** StaticMeshComponents per slot. Index matches EPartSlot. */
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> PartMeshes;

    /** Dynamic material instances per slot. Index matches EPartSlot. */
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceDynamic>> PartMaterials;

    // -------------------------------------------------------------------------
    // LOD Helpers
    // -------------------------------------------------------------------------

    /** Show/hide part mesh components based on LOD tier. */
    void ApplyLODVisibility(EMeshLODLevel NewLOD);

    // -------------------------------------------------------------------------
    // Material Helpers
    // -------------------------------------------------------------------------

    /** Create a dynamic material instance for a part mesh component. */
    UMaterialInstanceDynamic* CreatePartMaterial(UStaticMeshComponent* MeshComp, int32 SlotIndex);

    /** Apply damage state visuals to a part's material. */
    void UpdatePartDamageVisuals(int32 SlotIndex, EPartDamageState DamageState);

    // -------------------------------------------------------------------------
    // Detachment Helpers
    // -------------------------------------------------------------------------

    /** Detach a part mesh, enable physics, apply impulse. */
    void PhysicsDetachPart(int32 SlotIndex, const FVector& ImpactDirection);

    /** Timer callback to destroy a lingering detached part mesh. */
    void DestroyLingeringPart(int32 SlotIndex);
};
