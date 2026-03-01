// MXEvolutionLayerSystem.h — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Consumers: Robot actor Blueprint (via component), Game mode (batch update on run end)
// Change Log:
//   v1.0 — Initial implementation. Coordinates Calculator → WearShader → DecalManager pipeline.
//           Subscribes to event bus: OnLevelUp, OnSpecChosen, OnRunComplete, OnNearMiss.
//           Implements IMXEvolutionTarget::ApplyVisualMark for DEMS event-driven mark injection.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXEvolutionData.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "MXEvents.h"
#include "MXInterfaces.h"
#include "MXEvolutionCalculator.h"
#include "MXWearShader.h"
#include "MXDecalManager.h"
#include "MXEvolutionLayerSystem.generated.h"

/**
 * UMXEvolutionLayerSystem
 *
 * ActorComponent that lives on each robot Blueprint actor. It is the single entry point
 * for the full evolution pipeline:
 *
 *   Robot stats change (level-up, run end, near-miss, DEMS visual mark)
 *       → RecalculateEvolution()
 *           → UMXEvolutionCalculator::ComputeFullState() — pure math
 *           → UMXWearShader::SetWearParameters()         — material parameters
 *           → UMXDecalManager::GenerateDecalPlacements() — seeded scatter
 *           → UMXDecalManager::ApplyDecals()             — component placement
 *
 * DEMS flash path:
 *   ApplyVisualMark() [IMXEvolutionTarget]
 *       → Increments the relevant stat on the cached profile
 *       → Calls RecalculateEvolution()
 *       → Calls UMXWearShader::TriggerVisualMarkFlash()   — brief glow
 *
 * The component caches a local copy of the robot's FMXRobotProfile. It is refreshed on
 * BeginPlay via IMXRobotProvider and after each event-driven update. The canonical copy
 * always lives in UMXRobotManager — this is a display-side cache only.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMXOnEvolutionUpdated);

UCLASS(ClassGroup=(MicroExodus), meta=(BlueprintSpawnableComponent), BlueprintType)
class MICROEXODUS_API UMXEvolutionLayerSystem : public UActorComponent, public IMXEvolutionTarget
{
    GENERATED_BODY()

public:

    UMXEvolutionLayerSystem();

    // =========================================================================
    // UActorComponent overrides
    // =========================================================================

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // =========================================================================
    // IMXEvolutionTarget — DEMS event-driven mark injection
    // =========================================================================

    /**
     * ApplyVisualMark_Implementation
     * Called by the DEMS MessageDispatcher after a significant robot event.
     * Translates the mark type string into a stat increment on the cached profile,
     * triggers a full recalculation, then calls WearShader::TriggerVisualMarkFlash.
     *
     * @param MarkType   "burn_mark", "impact_crack", "weld_repair", "frost_residue",
     *                   "elec_burn", "fall_scar". Unmapped strings trigger flash only.
     * @param Intensity  Strength of the mark [0, 1]. Used for flash magnitude.
     */
    virtual void ApplyVisualMark_Implementation(const FString& MarkType, float Intensity) override;

    // =========================================================================
    // Core Pipeline
    // =========================================================================

    /**
     * RecalculateEvolution
     * Recomputes all FMXVisualEvolutionState fields from CachedProfile, then pushes
     * results to the material instance (via WearShader) and decals (via DecalManager).
     * Call this after any stat change on CachedProfile.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution")
    void RecalculateEvolution();

    /**
     * ApplyEvolutionToActor
     * Pushes a pre-computed FMXVisualEvolutionState to the actor's material and decals.
     * Useful when the caller already has a computed state and wants to skip recalculation.
     *
     * @param RobotActor  Target actor (can differ from GetOwner for preview purposes).
     * @param State       The state to apply.
     * @param PaintJob    Paint job to apply with the state.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution")
    void ApplyEvolutionToActor(
        AActor*                        RobotActor,
        const FMXVisualEvolutionState& State,
        EPaintJob                      PaintJob
    );

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * RobotId
     * Set by the robot actor on spawn to identify which profile this component manages.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Evolution|Config")
    FGuid RobotId;

    /**
     * BaseMaterial
     * Reference to M_RobotMaster. Set in the robot Blueprint's component panel.
     * Required for CreateMaterialInstance to succeed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Evolution|Config")
    TObjectPtr<UMaterialInterface> BaseMaterial;

    /**
     * TargetMesh
     * The skeletal or static mesh component on the robot actor that receives the MID.
     * Set in the robot Blueprint's component panel.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Evolution|Config")
    TObjectPtr<UMeshComponent> TargetMesh;

    // =========================================================================
    // Runtime State (readable by Blueprint for UI / debug)
    // =========================================================================

    /** The most recently computed visual evolution state. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Evolution|State")
    FMXVisualEvolutionState CurrentState;

    /** The active per-robot material instance. Created in BeginPlay. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Evolution|State")
    TObjectPtr<UMaterialInstanceDynamic> ActiveMID;

    // =========================================================================
    // Convenience Delegates (Blueprint-bindable)
    // =========================================================================

    /** Broadcast after every successful RecalculateEvolution call. */
    UPROPERTY(BlueprintAssignable, Category = "MX|Evolution|Events")
    FMXOnEvolutionUpdated OnEvolutionUpdated;

    // =========================================================================
    // Static Batch Utility
    // =========================================================================

    /**
     * RecalculateAllRobots
     * Batch recalculation called after a run ends. Iterates all active robot actors
     * in the world and triggers RecalculateEvolution on each Evolution component.
     * Intended to be called by the game mode or run manager after OnRunComplete.
     *
     * @param WorldCtx  Any UObject with a valid World reference.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Evolution")
    static void RecalculateAllRobots(UObject* WorldCtx);

private:

    // =========================================================================
    // Event Bus Subscriptions
    // =========================================================================

    void SubscribeToEventBus();
    void UnsubscribeFromEventBus();

    UFUNCTION()
    void HandleLevelUp(FGuid LeveledRobotId, int32 NewLevel);

    UFUNCTION()
    void HandleSpecChosen(FGuid SpecRobotId, ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3);

    UFUNCTION()
    void HandleRunComplete(FMXRunData RunData);

    UFUNCTION()
    void HandleNearMiss(FGuid NearMissRobotId, FMXObjectEventFields HazardFields);

    // =========================================================================
    // Internal Helpers
    // =========================================================================

    /** Fetches the latest profile from IMXRobotProvider and stores it in CachedProfile. */
    void RefreshCachedProfile();

    /** Returns the IMXRobotProvider from the GameInstance subsystem, or nullptr. */
    IMXRobotProvider* GetRobotProvider() const;

    /** Ensures ActiveMID exists, creating it if needed. Returns true if ready. */
    bool EnsureMID();

    // =========================================================================
    // Private State
    // =========================================================================

    /** Local copy of the robot's profile. Refreshed from RobotManager before recalculation. */
    FMXRobotProfile CachedProfile;

    /** Whether we have successfully subscribed to the event bus. */
    bool bEventBusSubscribed = false;
};
