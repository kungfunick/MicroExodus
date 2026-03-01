// MXRobotMeshComponent.cpp — Robot Assembly Module v1.0
// Agent 13: Robot Assembly

#include "MXRobotMeshComponent.h"
#include "MXRobotAssembler.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"

UMXRobotMeshComponent::UMXRobotMeshComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Start ticking at reduced rate — LOD checks don't need every frame.
    PrimaryComponentTick.TickInterval = 0.066f; // ~15 Hz

    PartMeshes.SetNum(MX_PART_SLOT_COUNT);
    PartMaterials.SetNum(MX_PART_SLOT_COUNT);
}

void UMXRobotMeshComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UMXRobotMeshComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // LOD is updated externally by the camera system for all robots in batch.
    // If not batch-updated, we skip per-robot camera distance checks here
    // to avoid 100 individual distance calculations per tick.
    // The swarm camera calls UpdateLOD on each robot after computing distances.
}

// ---------------------------------------------------------------------------
// Assembly
// ---------------------------------------------------------------------------

void UMXRobotMeshComponent::AssembleFromRecipe(
    const FMXAssemblyRecipe& Recipe,
    UMXRobotAssembler* Assembler)
{
    if (!GetOwner())
    {
        UE_LOG(LogTemp, Error, TEXT("MXRobotMeshComponent: No owning actor — cannot assemble."));
        return;
    }

    if (!Assembler)
    {
        UE_LOG(LogTemp, Error, TEXT("MXRobotMeshComponent: No assembler provided."));
        return;
    }

    // Clean up any previous assembly.
    if (bAssembled) Disassemble();

    CachedRecipe = Recipe;

    for (int32 SlotIdx = 0; SlotIdx < MX_PART_SLOT_COUNT; ++SlotIdx)
    {
        if (SlotIdx >= Recipe.SlotPartIds.Num()) continue;

        const FString& PartId = Recipe.SlotPartIds[SlotIdx];
        if (PartId.IsEmpty()) continue;

        FMXPartDefinition PartDef;
        if (!Assembler->GetPartDefinition(PartId, PartDef)) continue;

        // Create the StaticMeshComponent.
        UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(GetOwner(),
            *FString::Printf(TEXT("Part_%d_%s"), SlotIdx, *PartId));

        if (!MeshComp) continue;

        // Set mesh asset (soft reference — may need async load).
        if (PartDef.MeshAsset.IsValid())
        {
            UStaticMesh* LoadedMesh = PartDef.MeshAsset.LoadSynchronous();
            if (LoadedMesh)
            {
                MeshComp->SetStaticMesh(LoadedMesh);
            }
        }

        // Attach to the owning actor's root at the specified socket.
        USceneComponent* AttachParent = GetOwner()->GetRootComponent();
        if (AttachParent)
        {
            MeshComp->SetupAttachment(AttachParent, PartDef.SocketName);
        }
        MeshComp->RegisterComponent();

        // Apply offset and rotation from definition.
        MeshComp->SetRelativeLocation(PartDef.AttachOffset);
        MeshComp->SetRelativeRotation(PartDef.AttachRotation);
        MeshComp->SetRelativeScale3D(FVector(PartDef.Scale));

        // Configure for performance.
        MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MeshComp->SetCastShadow(true);
        MeshComp->bCastDynamicShadow = true;

        // Create per-part dynamic material.
        UMaterialInstanceDynamic* MID = CreatePartMaterial(MeshComp, SlotIdx);

        PartMeshes[SlotIdx] = MeshComp;
        PartMaterials[SlotIdx] = MID;
    }

    bAssembled = true;

    // Apply initial colors.
    SetChassisColor(Recipe.ChassisColor);
    SetEyeColor(Recipe.EyeColor);

    UE_LOG(LogTemp, Log,
        TEXT("MXRobotMeshComponent: Assembled robot %s — %d parts, locomotion=%d."),
        *Recipe.RobotId.ToString(),
        MX_PART_SLOT_COUNT,
        static_cast<int32>(Recipe.LocomotionType));
}

void UMXRobotMeshComponent::Disassemble()
{
    for (int32 i = 0; i < PartMeshes.Num(); ++i)
    {
        if (PartMeshes[i])
        {
            PartMeshes[i]->DestroyComponent();
            PartMeshes[i] = nullptr;
        }
        PartMaterials[i] = nullptr;
    }

    bAssembled = false;
    CurrentLOD = EMeshLODLevel::Full;
}

// ---------------------------------------------------------------------------
// LOD Management
// ---------------------------------------------------------------------------

void UMXRobotMeshComponent::UpdateLOD(float DistanceToCamera)
{
    EMeshLODLevel NewLOD;

    if (DistanceToCamera < LOD1Distance)
    {
        NewLOD = EMeshLODLevel::Full;
    }
    else if (DistanceToCamera < LOD2Distance)
    {
        NewLOD = EMeshLODLevel::Simplified;
    }
    else
    {
        NewLOD = EMeshLODLevel::Impostor;
    }

    if (NewLOD != CurrentLOD)
    {
        ApplyLODVisibility(NewLOD);
        CurrentLOD = NewLOD;

        // At impostor distance, disable tick entirely for performance.
        SetComponentTickEnabled(NewLOD != EMeshLODLevel::Impostor);

        OnLODChanged.Broadcast(CachedRecipe.RobotId, NewLOD);
    }
}

void UMXRobotMeshComponent::ForceLOD(EMeshLODLevel Level)
{
    ApplyLODVisibility(Level);
    CurrentLOD = Level;
    SetComponentTickEnabled(Level != EMeshLODLevel::Impostor);
}

void UMXRobotMeshComponent::ApplyLODVisibility(EMeshLODLevel NewLOD)
{
    switch (NewLOD)
    {
    case EMeshLODLevel::Full:
        // All parts visible.
        for (int32 i = 0; i < PartMeshes.Num(); ++i)
        {
            if (PartMeshes[i]) PartMeshes[i]->SetVisibility(true);
        }
        break;

    case EMeshLODLevel::Simplified:
        // Show body, locomotion, head. Hide arms.
        for (int32 i = 0; i < PartMeshes.Num(); ++i)
        {
            if (!PartMeshes[i]) continue;
            const EPartSlot Slot = static_cast<EPartSlot>(i);
            const bool bShow = (Slot == EPartSlot::Body ||
                                Slot == EPartSlot::Locomotion ||
                                Slot == EPartSlot::Head);
            PartMeshes[i]->SetVisibility(bShow);
        }
        break;

    case EMeshLODLevel::Impostor:
        // Hide all individual parts. The owning actor's LOD system handles
        // the impostor mesh (a single low-poly stand-in on the root component).
        for (int32 i = 0; i < PartMeshes.Num(); ++i)
        {
            if (PartMeshes[i]) PartMeshes[i]->SetVisibility(false);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// Damage Visuals
// ---------------------------------------------------------------------------

void UMXRobotMeshComponent::OnPartDamageEvent(const FMXDestructionEvent& Event)
{
    if (!bAssembled) return;

    const int32 SlotIdx = static_cast<int32>(Event.Slot);
    if (SlotIdx < 0 || SlotIdx >= PartMeshes.Num()) return;

    UpdatePartDamageVisuals(SlotIdx, Event.NewState);

    UE_LOG(LogTemp, Verbose,
        TEXT("MXRobotMeshComponent: Robot %s part [%d] visual update → %d"),
        *Event.RobotId.ToString(), SlotIdx, static_cast<int32>(Event.NewState));
}

void UMXRobotMeshComponent::OnPartDetachEvent(const FMXDestructionEvent& Event)
{
    if (!bAssembled) return;

    const int32 SlotIdx = static_cast<int32>(Event.Slot);
    if (SlotIdx < 0 || SlotIdx >= PartMeshes.Num()) return;

    // Skip body — it doesn't physically detach.
    if (Event.Slot == EPartSlot::Body)
    {
        UpdatePartDamageVisuals(SlotIdx, Event.NewState);
        return;
    }

    PhysicsDetachPart(SlotIdx, Event.ImpactDirection);
}

void UMXRobotMeshComponent::ApplyWearParameters(
    float BurnIntensity, float CrackIntensity,
    float WeldCoverage, float PatinaIntensity)
{
    for (int32 i = 0; i < PartMaterials.Num(); ++i)
    {
        UMaterialInstanceDynamic* MID = PartMaterials[i];
        if (!MID) continue;

        MID->SetScalarParameterValue(TEXT("BurnIntensity"), BurnIntensity);
        MID->SetScalarParameterValue(TEXT("CrackIntensity"), CrackIntensity);
        MID->SetScalarParameterValue(TEXT("WeldCoverage"), WeldCoverage);
        MID->SetScalarParameterValue(TEXT("PatinaIntensity"), PatinaIntensity);
    }
}

void UMXRobotMeshComponent::SetChassisColor(int32 ColorIndex)
{
    // Color palette — 12 distinct robot chassis colors.
    static const FLinearColor Palette[] = {
        FLinearColor(0.7f, 0.7f, 0.7f),   // 0  Silver
        FLinearColor(0.2f, 0.3f, 0.8f),   // 1  Blue
        FLinearColor(0.8f, 0.2f, 0.1f),   // 2  Red
        FLinearColor(0.1f, 0.7f, 0.2f),   // 3  Green
        FLinearColor(0.9f, 0.7f, 0.1f),   // 4  Yellow
        FLinearColor(0.5f, 0.2f, 0.7f),   // 5  Purple
        FLinearColor(0.1f, 0.6f, 0.7f),   // 6  Teal
        FLinearColor(0.9f, 0.5f, 0.1f),   // 7  Orange
        FLinearColor(0.3f, 0.3f, 0.3f),   // 8  Dark Grey
        FLinearColor(0.9f, 0.9f, 0.9f),   // 9  White
        FLinearColor(0.6f, 0.4f, 0.2f),   // 10 Bronze
        FLinearColor(0.1f, 0.1f, 0.1f),   // 11 Black
    };

    const int32 Idx = FMath::Clamp(ColorIndex, 0, 11);
    const FLinearColor& Color = Palette[Idx];

    for (UMaterialInstanceDynamic* MID : PartMaterials)
    {
        if (MID)
        {
            MID->SetVectorParameterValue(TEXT("ChassisColor"), Color);
        }
    }
}

void UMXRobotMeshComponent::SetEyeColor(int32 ColorIndex)
{
    // Eye emissive palette — 8 colors.
    static const FLinearColor EyePalette[] = {
        FLinearColor(0.0f, 0.8f, 1.0f),   // 0  Cyan (default)
        FLinearColor(1.0f, 0.3f, 0.0f),   // 1  Orange
        FLinearColor(0.0f, 1.0f, 0.3f),   // 2  Green
        FLinearColor(1.0f, 0.0f, 0.3f),   // 3  Red
        FLinearColor(0.8f, 0.0f, 1.0f),   // 4  Purple
        FLinearColor(1.0f, 1.0f, 0.0f),   // 5  Yellow
        FLinearColor(1.0f, 1.0f, 1.0f),   // 6  White
        FLinearColor(0.0f, 0.0f, 1.0f),   // 7  Blue
    };

    const int32 Idx = FMath::Clamp(ColorIndex, 0, 7);

    // Only the head part gets eye color.
    const int32 HeadIdx = static_cast<int32>(EPartSlot::Head);
    if (HeadIdx < PartMaterials.Num() && PartMaterials[HeadIdx])
    {
        PartMaterials[HeadIdx]->SetVectorParameterValue(TEXT("EyeEmissiveColor"), EyePalette[Idx]);
    }
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

UStaticMeshComponent* UMXRobotMeshComponent::GetPartMeshComponent(EPartSlot Slot) const
{
    const int32 Idx = static_cast<int32>(Slot);
    if (Idx >= 0 && Idx < PartMeshes.Num())
    {
        return PartMeshes[Idx];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Private: Material Helpers
// ---------------------------------------------------------------------------

UMaterialInstanceDynamic* UMXRobotMeshComponent::CreatePartMaterial(
    UStaticMeshComponent* MeshComp,
    int32 SlotIndex)
{
    if (!MeshComp) return nullptr;

    // Get the base material from the mesh (element 0).
    UMaterialInterface* BaseMat = MeshComp->GetMaterial(0);
    if (!BaseMat)
    {
        // If mesh has no material, use a default.
        // In production, this would reference M_Robot_Master.
        return nullptr;
    }

    UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, MeshComp);
    if (MID)
    {
        MeshComp->SetMaterial(0, MID);

        // Initialize all wear parameters to pristine.
        MID->SetScalarParameterValue(TEXT("BurnIntensity"), 0.0f);
        MID->SetScalarParameterValue(TEXT("CrackIntensity"), 0.0f);
        MID->SetScalarParameterValue(TEXT("WeldCoverage"), 0.0f);
        MID->SetScalarParameterValue(TEXT("PatinaIntensity"), 0.0f);
        MID->SetScalarParameterValue(TEXT("DamageState"), 0.0f);
    }

    return MID;
}

void UMXRobotMeshComponent::UpdatePartDamageVisuals(
    int32 SlotIndex,
    EPartDamageState DamageState)
{
    if (SlotIndex < 0 || SlotIndex >= PartMaterials.Num()) return;

    UMaterialInstanceDynamic* MID = PartMaterials[SlotIndex];
    if (!MID) return;

    // Map damage state to a 0–1 float for the material.
    const float DamageFloat = static_cast<float>(DamageState) /
        static_cast<float>(EPartDamageState::Destroyed);

    MID->SetScalarParameterValue(TEXT("DamageState"), DamageFloat);

    // Per-state visual effects.
    switch (DamageState)
    {
    case EPartDamageState::Scratched:
        MID->SetScalarParameterValue(TEXT("ScratchIntensity"), 0.4f);
        break;

    case EPartDamageState::Cracked:
        MID->SetScalarParameterValue(TEXT("ScratchIntensity"), 0.7f);
        MID->SetScalarParameterValue(TEXT("CrackIntensity"), 0.6f);
        // TODO: Spawn sparks particle component on this part.
        break;

    case EPartDamageState::Hanging:
        MID->SetScalarParameterValue(TEXT("CrackIntensity"), 1.0f);
        // Tilt the mesh slightly to show partial detachment.
        if (PartMeshes[SlotIndex])
        {
            FRotator Tilt = PartMeshes[SlotIndex]->GetRelativeRotation();
            Tilt.Pitch += 15.0f;
            Tilt.Roll  += 8.0f;
            PartMeshes[SlotIndex]->SetRelativeRotation(Tilt);
        }
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Private: Detachment Helpers
// ---------------------------------------------------------------------------

void UMXRobotMeshComponent::PhysicsDetachPart(int32 SlotIndex, const FVector& ImpactDirection)
{
    UStaticMeshComponent* MeshComp = PartMeshes[SlotIndex];
    if (!MeshComp) return;

    // Detach from the skeleton.
    MeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

    // Enable physics simulation.
    MeshComp->SetSimulatePhysics(true);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    MeshComp->SetCollisionResponseToAllChannels(ECR_Block);

    // Apply ejection impulse.
    FVector Impulse = ImpactDirection.GetSafeNormal() * DetachImpulse;
    // Add some upward kick.
    Impulse.Z += DetachImpulse * 0.5f;
    // Add random tumble.
    const FVector Tumble(
        FMath::RandRange(-100.0f, 100.0f),
        FMath::RandRange(-100.0f, 100.0f),
        FMath::RandRange(50.0f, 200.0f)
    );
    Impulse += Tumble;

    MeshComp->AddImpulse(Impulse, NAME_None, true);

    // Add angular impulse for tumbling.
    const FVector AngularImpulse(
        FMath::RandRange(-200.0f, 200.0f),
        FMath::RandRange(-200.0f, 200.0f),
        FMath::RandRange(-200.0f, 200.0f)
    );
    MeshComp->AddAngularImpulseInDegrees(AngularImpulse, NAME_None, true);

    UE_LOG(LogTemp, Log,
        TEXT("MXRobotMeshComponent: Part [%d] detached with impulse (%.0f, %.0f, %.0f)."),
        SlotIndex, Impulse.X, Impulse.Y, Impulse.Z);

    // Schedule cleanup after linger time.
    if (GetWorld())
    {
        FTimerHandle Handle;
        FTimerDelegate Delegate;
        Delegate.BindUFunction(this, FName(TEXT("DestroyLingeringPart_Internal")), SlotIndex);
        // Use a lambda-safe approach: capture SlotIndex.
        GetWorld()->GetTimerManager().SetTimer(
            Handle,
            [this, SlotIndex]() { DestroyLingeringPart(SlotIndex); },
            DetachedLingerTime,
            false);
    }
}

void UMXRobotMeshComponent::DestroyLingeringPart(int32 SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= PartMeshes.Num()) return;

    if (PartMeshes[SlotIndex])
    {
        PartMeshes[SlotIndex]->DestroyComponent();
        PartMeshes[SlotIndex] = nullptr;
    }
    PartMaterials[SlotIndex] = nullptr;

    UE_LOG(LogTemp, Verbose,
        TEXT("MXRobotMeshComponent: Lingering part [%d] destroyed after %.1fs."),
        SlotIndex, DetachedLingerTime);
}
