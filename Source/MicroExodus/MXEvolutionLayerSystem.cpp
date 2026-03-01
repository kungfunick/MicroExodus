// MXEvolutionLayerSystem.cpp — Evolution Module v1.0
// Created: 2026-02-22
// Agent 4: Evolution
// Change Log:
//   v1.0 — Initial implementation.
//           Event bus subscription pattern mirrors DEMS and Identity module conventions.

#include "MXEvolutionLayerSystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/MeshComponent.h"

// ---------------------------------------------------------------------------
// TODO: Replace with GetGameInstance()->GetSubsystem<UMXEventBusSubsystem>()
//       once UMXEventBusSubsystem is implemented. For now, we forward-declare
//       and guard all bus calls behind validity checks.
// ---------------------------------------------------------------------------

UMXEvolutionLayerSystem::UMXEvolutionLayerSystem()
{
    PrimaryComponentTick.bCanEverTick = false;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

// =========================================================================
// UActorComponent Lifecycle
// =========================================================================

void UMXEvolutionLayerSystem::BeginPlay()
{
    Super::BeginPlay();

    // Pull the initial profile from the RobotManager so CurrentState is never stale.
    RefreshCachedProfile();

    // Create the per-robot material instance now that we know the paint job.
    if (EnsureMID())
    {
        // Run a full recalculation to populate CurrentState and push parameters.
        RecalculateEvolution();
    }

    // Wire up event bus listeners.
    SubscribeToEventBus();
}

void UMXEvolutionLayerSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnsubscribeFromEventBus();
    Super::EndPlay(EndPlayReason);
}

// =========================================================================
// IMXEvolutionTarget — ApplyVisualMark
// =========================================================================

void UMXEvolutionLayerSystem::ApplyVisualMark_Implementation(const FString& MarkType, float Intensity)
{
    if (Intensity <= 0.0f)
    {
        return;
    }

    // Map mark type → encounter stat increment. The canonical stat lives in CachedProfile
    // (a local mirror). RecalculateEvolution will re-read from RobotManager on next full
    // refresh; the immediate increment ensures the visual responds to the event right now.
    // Note: The authoritative increment happens in UMXRobotManager::IncrementElementEncounter
    // triggered by the same swarm physics event that fires the DEMS visual_mark. We do NOT
    // double-count — this path only drives the immediate visual update via the cached profile.
    // A clean architecture would have this be a read-only pull, but since we can't guarantee
    // RobotManager has flushed its stat before DEMS calls us, we mirror the increment here
    // and accept a possible off-by-one that corrects on the next full refresh.

    if (MarkType == TEXT("burn_mark"))
    {
        CachedProfile.fire_encounters++;
    }
    else if (MarkType == TEXT("impact_crack"))
    {
        CachedProfile.crush_encounters++;
    }
    else if (MarkType == TEXT("fall_scar"))
    {
        CachedProfile.fall_encounters++;
    }
    else if (MarkType == TEXT("frost_residue"))
    {
        CachedProfile.ice_encounters++;
    }
    else if (MarkType == TEXT("elec_burn"))
    {
        CachedProfile.emp_encounters++;
    }
    else if (MarkType == TEXT("weld_repair"))
    {
        CachedProfile.near_misses++;
    }
    // Unknown mark types pass through to flash-only (no stat change).

    // Recompute visuals from the updated cached profile.
    RecalculateEvolution();

    // Trigger the brief flash glow at the mark location.
    if (IsValid(ActiveMID))
    {
        UMXWearShader::TriggerVisualMarkFlash(ActiveMID, this, MarkType, 0.5f);
    }
}

// =========================================================================
// Core Pipeline
// =========================================================================

void UMXEvolutionLayerSystem::RecalculateEvolution()
{
    // 1. Pull latest stats from the authoritative RobotManager.
    RefreshCachedProfile();

    // 2. Pure math — compute the full visual state from the profile.
    CurrentState = UMXEvolutionCalculator::ComputeFullState(CachedProfile);

    // 3. Push material parameters.
    if (EnsureMID())
    {
        UMXWearShader::SetWearParameters(ActiveMID, CurrentState);
    }

    // 4. Regenerate and apply decals.
    if (IsValid(GetOwner()))
    {
        TArray<FMXDecalPlacement> Placements =
            UMXDecalManager::GenerateDecalPlacements(CurrentState.decal_seed, CurrentState);
        UMXDecalManager::ApplyDecals(GetOwner(), Placements);
    }

    // 5. Notify Blueprint listeners (UI, debug overlay, etc.).
    OnEvolutionUpdated.Broadcast();

    UE_LOG(LogTemp, Verbose, TEXT("MXEvolutionLayerSystem: Evolution recalculated for '%s' (%s)."),
           *CachedProfile.name,
           *UMXEvolutionCalculator::GetEvolutionSummary(CurrentState));
}

void UMXEvolutionLayerSystem::ApplyEvolutionToActor(
    AActor*                        RobotActor,
    const FMXVisualEvolutionState& State,
    EPaintJob                      PaintJob)
{
    if (!IsValid(RobotActor))
    {
        return;
    }

    // If applying to our own actor, use ActiveMID. Otherwise create a transient one.
    UMaterialInstanceDynamic* TargetMID = nullptr;
    if (RobotActor == GetOwner())
    {
        TargetMID = ActiveMID;
    }
    else
    {
        // For preview actors — find their first mesh and create a transient MID.
        UMeshComponent* PreviewMesh = RobotActor->FindComponentByClass<UMeshComponent>();
        if (IsValid(PreviewMesh) && IsValid(BaseMaterial))
        {
            TargetMID = UMXWearShader::CreateMaterialInstance(RobotActor, BaseMaterial, PaintJob);
            if (IsValid(TargetMID))
            {
                PreviewMesh->SetMaterial(0, TargetMID);
            }
        }
    }

    if (IsValid(TargetMID))
    {
        UMXWearShader::SetPaintJobBase(TargetMID, PaintJob);
        UMXWearShader::SetWearParameters(TargetMID, State);
    }

    const TArray<FMXDecalPlacement> Placements =
        UMXDecalManager::GenerateDecalPlacements(State.decal_seed, State);
    UMXDecalManager::ApplyDecals(RobotActor, Placements);
}

// =========================================================================
// Static Batch Utility
// =========================================================================

/*static*/ void UMXEvolutionLayerSystem::RecalculateAllRobots(UObject* WorldCtx)
{
    if (!IsValid(WorldCtx))
    {
        return;
    }

    UWorld* World = WorldCtx->GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    int32 UpdatedCount = 0;

    // Iterate all actors with an Evolution component.
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
        {
            continue;
        }

        UMXEvolutionLayerSystem* EvoComp = Actor->FindComponentByClass<UMXEvolutionLayerSystem>();
        if (IsValid(EvoComp))
        {
            EvoComp->RecalculateEvolution();
            ++UpdatedCount;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXEvolutionLayerSystem: Batch recalculation complete — %d robots updated."),
           UpdatedCount);
}

// =========================================================================
// Event Bus Subscriptions
// =========================================================================

void UMXEvolutionLayerSystem::SubscribeToEventBus()
{
    // TODO: Retrieve UMXEventBusSubsystem once the subsystem is registered.
    //       Pattern:
    //         UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    //         UMXEventBusSubsystem* Bus = GI ? GI->GetSubsystem<UMXEventBusSubsystem>() : nullptr;
    //         if (Bus && Bus->EventBus)
    //         {
    //             Bus->EventBus->OnLevelUp.AddDynamic(this, &UMXEvolutionLayerSystem::HandleLevelUp);
    //             Bus->EventBus->OnSpecChosen.AddDynamic(this, &UMXEvolutionLayerSystem::HandleSpecChosen);
    //             Bus->EventBus->OnRunComplete.AddDynamic(this, &UMXEvolutionLayerSystem::HandleRunComplete);
    //             Bus->EventBus->OnNearMiss.AddDynamic(this, &UMXEvolutionLayerSystem::HandleNearMiss);
    //             bEventBusSubscribed = true;
    //         }

    UE_LOG(LogTemp, Log,
           TEXT("MXEvolutionLayerSystem: Event bus subscription pending UMXEventBusSubsystem registration."));
}

void UMXEvolutionLayerSystem::UnsubscribeFromEventBus()
{
    if (!bEventBusSubscribed)
    {
        return;
    }

    // TODO: Mirror the AddDynamic calls above with RemoveDynamic.
    //         Bus->EventBus->OnLevelUp.RemoveDynamic(this, &UMXEvolutionLayerSystem::HandleLevelUp);
    //         Bus->EventBus->OnSpecChosen.RemoveDynamic(this, &UMXEvolutionLayerSystem::HandleSpecChosen);
    //         Bus->EventBus->OnRunComplete.RemoveDynamic(this, &UMXEvolutionLayerSystem::HandleRunComplete);
    //         Bus->EventBus->OnNearMiss.RemoveDynamic(this, &UMXEvolutionLayerSystem::HandleNearMiss);

    bEventBusSubscribed = false;
}

// =========================================================================
// Event Handlers
// =========================================================================

void UMXEvolutionLayerSystem::HandleLevelUp(FGuid LeveledRobotId, int32 NewLevel)
{
    // Only care about our own robot.
    if (LeveledRobotId != RobotId)
    {
        return;
    }

    // Update cached level directly — avoids a round-trip to RobotManager for a single field.
    CachedProfile.level = NewLevel;

    // Antenna stage and eye brightness are driven by level; recalculate.
    RecalculateEvolution();

    UE_LOG(LogTemp, Log, TEXT("MXEvolutionLayerSystem: '%s' leveled up to %d — evolution recalculated."),
           *CachedProfile.name, NewLevel);
}

void UMXEvolutionLayerSystem::HandleSpecChosen(
    FGuid      SpecRobotId,
    ERobotRole Role,
    ETier2Spec Tier2,
    ETier3Spec Tier3)
{
    if (SpecRobotId != RobotId)
    {
        return;
    }

    // Update spec fields on cached profile.
    CachedProfile.role       = Role;
    CachedProfile.tier2_spec = Tier2;
    CachedProfile.tier3_spec = Tier3;

    // Aura stage is driven by spec; recalculate.
    RecalculateEvolution();

    UE_LOG(LogTemp, Log, TEXT("MXEvolutionLayerSystem: '%s' spec updated — aura recalculated."),
           *CachedProfile.name);
}

void UMXEvolutionLayerSystem::HandleRunComplete(FMXRunData RunData)
{
    // All robots get a post-run recalculation — armor stage and patina both update on run end.
    // We do a full refresh from RobotManager (via RefreshCachedProfile inside RecalculateEvolution)
    // so that runs_survived and any other stat increments from the run are picked up.
    RecalculateEvolution();
}

void UMXEvolutionLayerSystem::HandleNearMiss(FGuid NearMissRobotId, FMXObjectEventFields HazardFields)
{
    if (NearMissRobotId != RobotId)
    {
        return;
    }

    // Weld coverage is driven by near_misses. Increment the cached value immediately
    // so the visual responds without waiting for the next full RobotManager flush.
    CachedProfile.near_misses++;

    RecalculateEvolution();
}

// =========================================================================
// Internal Helpers
// =========================================================================

void UMXEvolutionLayerSystem::RefreshCachedProfile()
{
    IMXRobotProvider* Provider = GetRobotProvider();
    if (!Provider)
    {
        // Can't connect to RobotManager yet — keep whatever we have.
        return;
    }

    const FMXRobotProfile Fresh = Provider->Execute_GetRobotProfile(
        Cast<UObject>(Provider), RobotId);

    // Only update if we got a valid response (non-zero guid means the robot exists).
    if (Fresh.robot_id.IsValid())
    {
        // Preserve the decal_seed from the fresh profile — it is the authoritative value.
        CachedProfile = Fresh;
    }
}

IMXRobotProvider* UMXEvolutionLayerSystem::GetRobotProvider() const
{
    // TODO: Retrieve from UMXEventBusSubsystem or a dedicated RobotManagerSubsystem.
    //       Pattern:
    //         UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
    //         return GI ? GI->GetSubsystem<UMXRobotManagerSubsystem>() : nullptr;
    return nullptr;
}

bool UMXEvolutionLayerSystem::EnsureMID()
{
    if (IsValid(ActiveMID))
    {
        return true;
    }

    // Attempt to create the MID from the configured BaseMaterial.
    if (!IsValid(BaseMaterial) || !IsValid(GetOwner()))
    {
        UE_LOG(LogTemp, Warning,
               TEXT("MXEvolutionLayerSystem: Cannot create MID — BaseMaterial or Owner is invalid. "
                    "Set BaseMaterial in the robot Blueprint's Evolution component panel."));
        return false;
    }

    ActiveMID = UMXWearShader::CreateMaterialInstance(
        GetOwner(),
        BaseMaterial,
        CachedProfile.paint_job
    );

    if (!IsValid(ActiveMID))
    {
        return false;
    }

    // Apply the new MID to the configured mesh component.
    if (IsValid(TargetMesh))
    {
        TargetMesh->SetMaterial(0, ActiveMID);
    }
    else
    {
        // Fallback: apply to all materials on the first mesh found on the owner.
        UMeshComponent* FirstMesh = GetOwner()->FindComponentByClass<UMeshComponent>();
        if (IsValid(FirstMesh))
        {
            for (int32 i = 0; i < FirstMesh->GetNumMaterials(); ++i)
            {
                FirstMesh->SetMaterial(i, ActiveMID);
            }
        }
    }

    return true;
}
