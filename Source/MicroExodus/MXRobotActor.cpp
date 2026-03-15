// MXRobotActor.cpp — Phase 2C-Move Update
// Originally created: Phase 2A
// Updated: 2026-03-09

#include "MXRobotActor.h"
#include "MXRobotProfile.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "MXAnimBridge.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXRobotActor::AMXRobotActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // ---- Capsule sizing for miniaturised robot ----
    // Default mannequin is ~180cm. At 0.20 scale = ~36cm.
    // Capsule: radius 14cm, half-height 20cm — slightly oversized for easy click-selection.
    // NOTE: We do NOT scale the actor — only the mesh is scaled in BeginPlay.
    GetCapsuleComponent()->InitCapsuleSize(14.0f, 20.0f);

    // Ensure capsule blocks Pawn traces (for cursor selection raycast).
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

    // Mesh Z offset: align mesh feet with capsule bottom.
    // Capsule bottom = center - half_height = 0. Mesh origin (UE5 mannequin) is at feet.
    // So mesh at relative Z = -HalfHeight places feet at capsule bottom.
    if (GetMesh())
    {
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
    }

    // ---- Character Movement ----
    UCharacterMovementComponent* CMC = GetCharacterMovement();
    if (CMC)
    {
        CMC->MaxWalkSpeed = 150.0f;
        CMC->bOrientRotationToMovement = true;
        CMC->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
        CMC->GravityScale = 1.0f;
        CMC->MaxAcceleration = 800.0f;
        CMC->BrakingDecelerationWalking = 400.0f;
        CMC->bUseControllerDesiredRotation = false;
        // CRITICAL: Robots are NOT possessed by a controller.
        // Without this, CMC skips its entire tick and AddMovementInput does nothing.
        CMC->bRunPhysicsWithNoController = true;
    }

    // ---- Name Text Component ----
    NameTextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameText"));
    if (NameTextComponent)
    {
        NameTextComponent->SetupAttachment(GetRootComponent());
        // Capsule center at Z=20 (half-height 20). Robot head at ~Z=36 (mesh 0.2 scale).
        // Place name above head: relative Z = head_world(36) - capsule_center(20) + margin(6) = 22.
        NameTextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 22.0f));
        NameTextComponent->SetHorizontalAlignment(EHTA_Center);
        NameTextComponent->SetVerticalAlignment(EVRTA_TextBottom);
        NameTextComponent->SetWorldSize(4.0f);
        NameTextComponent->SetTextRenderColor(FColor(0, 220, 60, 255)); // Green
        NameTextComponent->SetVisibility(false);
    }

    // ---- Selection Ring ----
    SelectionRingComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
    if (SelectionRingComponent)
    {
        SelectionRingComponent->SetupAttachment(GetRootComponent());
        // Ring at feet = capsule bottom = relative Z = -HalfHeight = -20.
        SelectionRingComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
        // Bigger ring for visibility: 40cm diameter, paper-thin.
        SelectionRingComponent->SetRelativeScale3D(FVector(0.40f, 0.40f, 0.01f));
        SelectionRingComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SelectionRingComponent->SetVisibility(false); // Hidden until selected.
        SelectionRingComponent->SetCastShadow(false);

        // Try to load the engine cylinder mesh for a ring effect.
        UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        if (CylinderMesh)
        {
            SelectionRingComponent->SetStaticMesh(CylinderMesh);
        }
    }

    AnimBridge = CreateDefaultSubobject<UMXAnimBridge>(TEXT("AnimBridge"));
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXRobotActor::BeginPlay()
{
    Super::BeginPlay();

    // Apply robot scale to mesh only. Done in BeginPlay to avoid Blueprint serialisation override.
    // IMPORTANT: Do NOT use SetActorScale3D — that would double-miniaturise the capsule
    // (already sized at 8/18 in the constructor) and break the CMC's ground detection.
    if (GetMesh())
    {
        GetMesh()->SetRelativeScale3D(FVector(RobotScale));
    }

    // If a skeletal mesh asset is set, load and apply it.
    if (!SkeletalMeshAsset.IsNull())
    {
        USkeletalMesh* LoadedMesh = SkeletalMeshAsset.LoadSynchronous();
        if (LoadedMesh && GetMesh())
        {
            GetMesh()->SetSkeletalMesh(LoadedMesh);
        }
    }

    // Update CMC speed from config.
    UCharacterMovementComponent* CMC = GetCharacterMovement();
    if (CMC)
    {
        CMC->MaxWalkSpeed = MoveSpeed;
    }

    // Create selection ring material.
    if (SelectionRingComponent)
    {
        UMaterial* BaseMat = LoadObject<UMaterial>(nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (BaseMat)
        {
            UMaterialInstanceDynamic* RingMat = UMaterialInstanceDynamic::Create(BaseMat, this);
            if (RingMat)
            {
                RingMat->SetVectorParameterValue(TEXT("Color"), SelectionColor);
                SelectionRingComponent->SetMaterial(0, RingMat);
            }
        }
    }

    // Ensure initial visibility states are correct.
    UpdateNameVisibility();
    UpdateSelectionRing();
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AMXRobotActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TickMovement(DeltaTime);
    TickNameBillboard();
}

// ---------------------------------------------------------------------------
// Profile Binding
// ---------------------------------------------------------------------------

void AMXRobotActor::BindToProfile(const FGuid& InRobotId, const FString& InRobotName)
{
    RobotId = InRobotId;
    RobotName = InRobotName;

    if (NameTextComponent)
    {
        NameTextComponent->SetText(FText::FromString(RobotName));
    }

    UE_LOG(LogTemp, Log, TEXT("MXRobotActor: Bound to '%s' (ID: %s)"),
           *RobotName, *RobotId.ToString());
}

void AMXRobotActor::BindToFullProfile(const FMXRobotProfile& Profile)
{
    // Core identity.
    RobotId = Profile.robot_id;
    RobotName = Profile.name;

    // Generated profile details — visible in editor Details panel.
    PersonalityDescription = Profile.description;
    Quirk = Profile.quirk;
    Likes = Profile.likes;
    Dislikes = Profile.dislikes;
    RobotRole = Profile.role;
    Level = Profile.level;
    DisplayedTitle = Profile.displayed_title;
    ChassisColor = Profile.chassis_color;
    EyeColor = Profile.eye_color;

    // Update visual display.
    if (NameTextComponent)
    {
        NameTextComponent->SetText(FText::FromString(RobotName));
    }

    UE_LOG(LogTemp, Log, TEXT("MXRobotActor: Full bind to '%s' — %s, Lv%d, Role=%d (ID: %s)"),
           *RobotName, *PersonalityDescription, Level,
           static_cast<int32>(RobotRole), *RobotId.ToString());
}

// ---------------------------------------------------------------------------
// Selection & Hover
// ---------------------------------------------------------------------------

void AMXRobotActor::SetSelected(bool bNewSelected)
{
    if (bSelected == bNewSelected) return;
    bSelected = bNewSelected;

    UpdateNameVisibility();
    UpdateSelectionRing();
}

void AMXRobotActor::SetHovered(bool bNewHovered)
{
    if (bHovered == bNewHovered) return;
    bHovered = bNewHovered;

    UpdateNameVisibility();
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

void AMXRobotActor::MoveToLocation(FVector Target)
{
    MoveTargetLocation = Target;
    // Keep the target at the same Z as the robot (XY plane movement only).
    MoveTargetLocation.Z = GetActorLocation().Z;
    bHasMoveTarget = true;
}

void AMXRobotActor::StopMoving()
{
    bHasMoveTarget = false;
    MoveTargetLocation = FVector::ZeroVector;
}

// ---------------------------------------------------------------------------
// Internal: Movement Tick
// ---------------------------------------------------------------------------

void AMXRobotActor::TickMovement(float DeltaTime)
{
    if (!bHasMoveTarget) return;

    const FVector CurrentLocation = GetActorLocation();
    const FVector ToTarget = MoveTargetLocation - CurrentLocation;
    const float DistXY = FVector2D(ToTarget.X, ToTarget.Y).Size();

    if (DistXY <= StopDistance)
    {
        // Arrived.
        StopMoving();
        return;
    }

    // Compute movement direction on XY plane.
    FVector Direction = ToTarget.GetSafeNormal2D();

    // Use AddMovementInput — CharacterMovementComponent handles the physics.
    AddMovementInput(Direction, 1.0f);

    // Smooth rotation toward movement direction.
    if (!Direction.IsNearlyZero())
    {
        FRotator TargetRot = Direction.Rotation();
        FRotator CurrentRot = GetActorRotation();
        FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, RotationInterpSpeed);
        SetActorRotation(NewRot);
    }
}

// ---------------------------------------------------------------------------
// Internal: Name Visibility
// ---------------------------------------------------------------------------

void AMXRobotActor::UpdateNameVisibility()
{
    if (!NameTextComponent) return;

    // Show name if selected OR hovered.
    const bool bShowName = bSelected || bHovered;
    NameTextComponent->SetVisibility(bShowName);

    // Color depends on state.
    if (bSelected)
    {
        NameTextComponent->SetTextRenderColor(SelectedNameColor);
    }
    else if (bHovered)
    {
        NameTextComponent->SetTextRenderColor(HoveredNameColor);
    }
}

// ---------------------------------------------------------------------------
// Internal: Selection Ring
// ---------------------------------------------------------------------------

void AMXRobotActor::UpdateSelectionRing()
{
    if (!SelectionRingComponent) return;
    SelectionRingComponent->SetVisibility(bSelected);
}

// ---------------------------------------------------------------------------
// Internal: Name Billboard
// ---------------------------------------------------------------------------

void AMXRobotActor::TickNameBillboard()
{
    if (!NameTextComponent || !NameTextComponent->IsVisible()) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    // Face the text toward the camera.
    FVector ToCamera = CameraLocation - NameTextComponent->GetComponentLocation();
    if (!ToCamera.IsNearlyZero())
    {
        FRotator LookAtRot = ToCamera.Rotation();
        NameTextComponent->SetWorldRotation(LookAtRot);
    }

    // Scale WorldSize so name appears constant on screen regardless of zoom.
    // Reference: WorldSize 4.0 looks right at ~500cm camera distance.
    float Dist = ToCamera.Size();
    float ScaledSize = FMath::Clamp(Dist * 0.008f, 2.0f, 20.0f);
    NameTextComponent->SetWorldSize(ScaledSize);
}
