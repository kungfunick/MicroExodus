// MXHatPhysics.cpp — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Change Log:
//   v1.0 — Initial implementation

#include "MXHatPhysics.h"
#include "MXConstants.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

// ---------------------------------------------------------------------------
// Static constants
// ---------------------------------------------------------------------------

const FName UMXHatPhysics::HAT_SOCKET_NAME    = TEXT("HatSocket");
const FName UMXHatPhysics::DISSOLVE_PARAM_NAME = TEXT("DissolveAmount");

// ---------------------------------------------------------------------------
// Attach Hat
// ---------------------------------------------------------------------------

void UMXHatPhysics::AttachHat(AActor* RobotActor, const FGuid& RobotId, int32 HatId, const FSoftObjectPath& MeshPath)
{
    if (!RobotActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMXHatPhysics::AttachHat — RobotActor is null."));
        return;
    }

    // Don't attach if already tracked
    if (FindInstance(RobotId))
    {
        UE_LOG(LogTemp, Warning, TEXT("UMXHatPhysics::AttachHat — robot %s already has a hat instance."),
            *RobotId.ToString());
        return;
    }

    // Create the mesh component on the robot actor
    UStaticMeshComponent* HatMesh = NewObject<UStaticMeshComponent>(RobotActor);
    HatMesh->RegisterComponent();

    // Set small relative scale for 2cm hat on a 15cm robot
    HatMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.2f));

    // Attach to head socket
    HatMesh->AttachToComponent(
        RobotActor->GetRootComponent(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        HAT_SOCKET_NAME);

    // Disable physics initially — wobble is simulated manually
    HatMesh->SetSimulatePhysics(false);
    HatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Async load the mesh asset
    if (MeshPath.IsValid())
    {
        FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
        StreamableManager.RequestAsyncLoad(MeshPath, [WeakMesh = TWeakObjectPtr<UStaticMeshComponent>(HatMesh), MeshPath]()
        {
            if (UStaticMeshComponent* Comp = WeakMesh.Get())
            {
                UStaticMesh* LoadedMesh = Cast<UStaticMesh>(MeshPath.ResolveObject());
                if (LoadedMesh)
                {
                    Comp->SetStaticMesh(LoadedMesh);
                }
            }
        });
    }

    // Register the instance
    FMXHatPhysicsInstance Instance;
    Instance.RobotId          = RobotId;
    Instance.HatId            = HatId;
    Instance.HatMeshComponent = HatMesh;
    Instance.WobbleAngle      = FMath::RandRange(-3.0f, 3.0f); // Small initial offset
    Instance.LingerDuration   = MX::HAT_DEATH_LINGER_TIME;

    ActiveInstances.Add(Instance);
}

// ---------------------------------------------------------------------------
// Detach Hat (Death / Sacrifice)
// ---------------------------------------------------------------------------

void UMXHatPhysics::DetachHat(const FGuid& RobotId, bool bIsSacrifice)
{
    FMXHatPhysicsInstance* Instance = FindInstance(RobotId);
    if (!Instance)
    {
        return;
    }

    Instance->bDying       = true;
    Instance->bIsSacrifice = bIsSacrifice;
    Instance->DyingElapsed = bIsSacrifice ? -SACRIFICE_PAUSE : 0.0f;

    // Detach from socket but keep component alive in world for linger
    if (Instance->HatMeshComponent)
    {
        Instance->HatMeshComponent->DetachFromComponent(
            FDetachmentTransformRules::KeepWorldTransform);

        // Enable physics so it falls naturally
        Instance->HatMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Instance->HatMeshComponent->SetSimulatePhysics(true);

        // Apply a tiny outward impulse for a satisfying pop on death
        const FVector Impulse = FVector(
            FMath::RandRange(-50.0f, 50.0f),
            FMath::RandRange(-50.0f, 50.0f),
            FMath::RandRange(30.0f, 80.0f));
        Instance->HatMeshComponent->AddImpulse(Impulse, NAME_None, true);
    }
}

// ---------------------------------------------------------------------------
// Tick — Update All Hat Physics
// ---------------------------------------------------------------------------

void UMXHatPhysics::UpdateHatPhysics(float DeltaTime)
{
    for (FMXHatPhysicsInstance& Instance : ActiveInstances)
    {
        if (!Instance.bDying)
        {
            TickWobble(Instance, DeltaTime);
        }
        else
        {
            TickDeath(Instance, DeltaTime);
        }
    }

    // Remove instances flagged for cleanup
    ActiveInstances.RemoveAll([](const FMXHatPhysicsInstance& Inst)
    {
        return Inst.bReadyForCleanup;
    });
}

// ---------------------------------------------------------------------------
// Set Wobble Intensity
// ---------------------------------------------------------------------------

void UMXHatPhysics::SetWobbleIntensity(float Intensity)
{
    WobbleIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// Clear All
// ---------------------------------------------------------------------------

void UMXHatPhysics::ClearAllHats()
{
    for (FMXHatPhysicsInstance& Instance : ActiveInstances)
    {
        if (Instance.HatMeshComponent)
        {
            Instance.HatMeshComponent->DestroyComponent();
        }
    }
    ActiveInstances.Empty();
}

// ---------------------------------------------------------------------------
// FindInstance
// ---------------------------------------------------------------------------

FMXHatPhysicsInstance* UMXHatPhysics::FindInstance(const FGuid& RobotId)
{
    for (FMXHatPhysicsInstance& Instance : ActiveInstances)
    {
        if (Instance.RobotId == RobotId)
        {
            return &Instance;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Private — Wobble Simulation
// ---------------------------------------------------------------------------

void UMXHatPhysics::TickWobble(FMXHatPhysicsInstance& Instance, float DeltaTime)
{
    if (!Instance.HatMeshComponent) return;

    // Spring stiffness scales with wobble intensity
    const float Stiffness = FMath::Lerp(WOBBLE_STIFFNESS_MIN, WOBBLE_STIFFNESS_MAX, WobbleIntensity);

    // Spring-damper: F = -k*x - c*v
    const float SpringForce  = -Stiffness * Instance.WobbleAngle;
    const float DampingForce = -WOBBLE_DAMPING * Instance.WobbleVelocity;
    const float Acceleration = SpringForce + DampingForce;

    Instance.WobbleVelocity += Acceleration * DeltaTime;
    Instance.WobbleAngle    += Instance.WobbleVelocity * DeltaTime;

    // Clamp angle to prevent wild oscillation
    Instance.WobbleAngle = FMath::Clamp(Instance.WobbleAngle, -WOBBLE_MAX_ANGLE, WOBBLE_MAX_ANGLE);

    // Apply wobble as relative roll offset on the hat component
    FRotator CurrentRelativeRot = Instance.HatMeshComponent->GetRelativeRotation();
    CurrentRelativeRot.Roll = Instance.WobbleAngle;
    Instance.HatMeshComponent->SetRelativeRotation(CurrentRelativeRot);
}

// ---------------------------------------------------------------------------
// Private — Death-Fall Simulation
// ---------------------------------------------------------------------------

bool UMXHatPhysics::TickDeath(FMXHatPhysicsInstance& Instance, float DeltaTime)
{
    // DyingElapsed starts negative for sacrifice to implement pause
    Instance.DyingElapsed += DeltaTime;

    if (Instance.DyingElapsed < 0.0f)
    {
        // Still in sacrifice pause — hat hasn't detached yet (physics already enabled above)
        return false;
    }

    // Once linger time has elapsed, start dissolving
    if (Instance.DyingElapsed >= Instance.LingerDuration)
    {
        Instance.DissolveProgress += DeltaTime * DISSOLVE_RATE;
        Instance.DissolveProgress  = FMath::Min(Instance.DissolveProgress, 1.0f);
        ApplyDissolveMaterial(Instance, Instance.DissolveProgress);

        if (Instance.DissolveProgress >= 1.0f)
        {
            // Dissolve complete — destroy mesh component and flag for cleanup
            if (Instance.HatMeshComponent)
            {
                Instance.HatMeshComponent->DestroyComponent();
                Instance.HatMeshComponent = nullptr;
            }
            Instance.bReadyForCleanup = true;
        }
    }

    return Instance.bReadyForCleanup;
}

// ---------------------------------------------------------------------------
// Private — Dissolve Material
// ---------------------------------------------------------------------------

void UMXHatPhysics::ApplyDissolveMaterial(FMXHatPhysicsInstance& Instance, float NormalizedDissolve)
{
    if (!Instance.HatMeshComponent) return;

    UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(
        Instance.HatMeshComponent->GetMaterial(0));

    if (!DynMat)
    {
        // Create a dynamic instance if one doesn't exist
        UMaterialInterface* BaseMat = Instance.HatMeshComponent->GetMaterial(0);
        if (BaseMat)
        {
            DynMat = UMaterialInstanceDynamic::Create(BaseMat, Instance.HatMeshComponent);
            Instance.HatMeshComponent->SetMaterial(0, DynMat);
        }
    }

    if (DynMat)
    {
        DynMat->SetScalarParameterValue(DISSOLVE_PARAM_NAME, NormalizedDissolve);
    }
}
