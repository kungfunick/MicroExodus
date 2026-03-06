// MXRobotActor.cpp — Phase 2A: Robot Actor for Spawn Test
// Created: 2026-03-06

#include "MXRobotActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMesh.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXRobotActor::AMXRobotActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // ---- Capsule ----
    // Default ACharacter capsule is 34 radius, 88 half-height (for ~180cm human).
    // We'll scale the whole actor in BeginPlay, so keep standard capsule defaults.
    // After 0.20 scale: effective radius ~6.8cm, half-height ~17.6cm → ~35cm tall robot.

    // NOTE: Mesh alignment (Z offset, Yaw rotation) is done in BeginPlay, NOT here.
    // Blueprint child classes serialize component transforms that override constructor values.

    // ---- Floating Name Text ----
    // Positioned well above the mesh head. The capsule half-height is 88, so the
    // mannequin head is at roughly Z=80 in actor-local space.
    // Z=130 puts the text about 50 units above the head (unscaled).
    // At 0.20 actor scale, that's ~26cm world above origin, ~10cm above head.
    NameTextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameText"));
    NameTextComponent->SetupAttachment(RootComponent);
    NameTextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
    NameTextComponent->SetHorizontalAlignment(EHTA_Center);
    NameTextComponent->SetVerticalAlignment(EVRTA_TextCenter);
    NameTextComponent->SetWorldSize(80.0f);  // At 0.20 scale → effective ~16 world units
    NameTextComponent->SetTextRenderColor(FColor::Cyan);
    NameTextComponent->SetText(FText::FromString(TEXT("???")));

    // Don't let the text cast shadows.
    NameTextComponent->SetCastShadow(false);

    // ---- Movement ----
    // For the spawn test, robots are static. Disable gravity so they don't
    // fall through floors that lack collision (basic Plane mesh has none).
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DefaultLandMovementMode = MOVE_Walking;
        MoveComp->GravityScale = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// BindToProfile
// ---------------------------------------------------------------------------

void AMXRobotActor::BindToProfile(const FGuid& InRobotId, const FString& InRobotName)
{
    RobotId = InRobotId;
    RobotName = InRobotName;

    if (NameTextComponent)
    {
        NameTextComponent->SetText(FText::FromString(RobotName));
    }

    UE_LOG(LogTemp, Log, TEXT("[MXRobotActor] Bound to '%s' (ID: %s)"),
           *RobotName, *RobotId.ToString());
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXRobotActor::BeginPlay()
{
    Super::BeginPlay();

    // ---- Apply uniform scale ----
    SetActorScale3D(FVector(RobotScale));

    // ---- Mesh Alignment ----
    // Must be done here, NOT in the constructor. Blueprint child classes serialize
    // component transforms that override constructor values. BeginPlay runs after
    // Blueprint deserialization, so these offsets actually stick.
    // Z = -CapsuleHalfHeight (88) so feet sit at capsule bottom instead of pelvis at center.
    // Yaw = -90° so mesh forward aligns with capsule forward.
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
        MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    }

    // ---- Optional mesh assignment from soft reference ----
    if (!SkeletalMeshAsset.IsNull())
    {
        USkeletalMesh* LoadedMesh = SkeletalMeshAsset.LoadSynchronous();
        if (LoadedMesh && GetMesh())
        {
            GetMesh()->SetSkeletalMesh(LoadedMesh);
            UE_LOG(LogTemp, Log, TEXT("[MXRobotActor] Skeletal mesh set from soft reference: %s"),
                   *LoadedMesh->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[MXRobotActor] Failed to load SkeletalMeshAsset."));
        }
    }

    // If no mesh is set at all, log a reminder.
    if (GetMesh() && !GetMesh()->GetSkeletalMeshAsset())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[MXRobotActor] No skeletal mesh assigned. Set SkeletalMeshAsset or use a Blueprint child with a mesh."));
    }

    UE_LOG(LogTemp, Log, TEXT("[MXRobotActor] BeginPlay — Scale: %.2f, Name: '%s'"),
           RobotScale, *RobotName);
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AMXRobotActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Billboard the name text toward the active camera.
    UpdateNameBillboard();
}

// ---------------------------------------------------------------------------
// UpdateNameBillboard
// ---------------------------------------------------------------------------

void AMXRobotActor::UpdateNameBillboard()
{
    if (!NameTextComponent) return;

    APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!CamMgr) return;

    FVector CamLocation = CamMgr->GetCameraLocation();
    FVector TextLocation = NameTextComponent->GetComponentLocation();

    // UTextRenderComponent renders text readable from its local -X direction.
    // So we point +X AWAY from the camera, making the front face toward it.
    FVector Direction = TextLocation - CamLocation;
    Direction.Z = 0.0f; // Keep text upright — only rotate on Yaw.

    if (!Direction.IsNearlyZero())
    {
        FRotator LookAtRotation = Direction.Rotation();
        NameTextComponent->SetWorldRotation(LookAtRotation);
    }
}
