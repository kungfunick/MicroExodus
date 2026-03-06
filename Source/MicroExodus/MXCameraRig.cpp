// MXCameraRig.cpp — Phase 2A: Camera Rig Actor
// Created: 2026-03-06

#include "MXCameraRig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"
#include "MXSwarmCamera.h"
#include "MXTiltShiftEffect.h"
#include "MXTimeDilation.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXCameraRig::AMXCameraRig()
{
    PrimaryActorTick.bCanEverTick = false; // MX components tick themselves.

    // ---- Root ----
    RigRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RigRoot"));
    RootComponent = RigRoot;

    // ---- Spring Arm ----
    // Overview angle for miniature diorama: -45° pitch, 500cm arm length.
    // UMXSwarmCamera will dynamically adjust TargetArmLength based on swarm count.
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RigRoot);
    SpringArm->TargetArmLength = 500.0f;
    SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;       // No collision — we're an overview camera.
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 5.0f;
    SpringArm->bEnableCameraRotationLag = false;
    SpringArm->bInheritPitch = true;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = false;

    // ---- Camera ----
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
    Camera->FieldOfView = 60.0f;

    // ---- Post Process ----
    // Unattached to spring arm — sits on the rig root with infinite extent.
    // UMXTiltShiftEffect auto-discovers this in its BeginPlay via FindComponentByClass.
    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
    PostProcess->SetupAttachment(RigRoot);
    PostProcess->bUnbound = true; // Infinite extent — affects entire scene.

    // ---- MX Swarm Camera ----
    SwarmCamera = CreateDefaultSubobject<UMXSwarmCamera>(TEXT("SwarmCamera"));
    // Wire the SpringArm reference that UMXSwarmCamera expects.
    SwarmCamera->SpringArm = SpringArm;
    // Note: UMXSwarmCamera::CameraActor expects an ACameraActor* (placed actor).
    // We use a UCameraComponent instead — CameraActor is left null.
    // The camera tracking works entirely through SpringArm, so this is fine.

    // ---- MX Tilt-Shift Effect ----
    // PostProcessComponent is auto-discovered in UMXTiltShiftEffect::BeginPlay()
    // via FindComponentByClass<UPostProcessComponent>() on the owning actor.
    TiltShiftEffect = CreateDefaultSubobject<UMXTiltShiftEffect>(TEXT("TiltShiftEffect"));

    // ---- MX Time Dilation ----
    TimeDilation = CreateDefaultSubobject<UMXTimeDilation>(TEXT("TimeDilation"));
    // Wire the TiltShiftEffect reference that UMXTimeDilation expects.
    TimeDilation->TiltShiftEffect = TiltShiftEffect;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXCameraRig::BeginPlay()
{
    Super::BeginPlay();

    // Verification logging.
    UE_LOG(LogTemp, Log, TEXT("[MXCameraRig] BeginPlay — wiring check:"));
    UE_LOG(LogTemp, Log, TEXT("  SpringArm:       %s"), SpringArm       ? TEXT("OK") : TEXT("MISSING"));
    UE_LOG(LogTemp, Log, TEXT("  Camera:          %s"), Camera          ? TEXT("OK") : TEXT("MISSING"));
    UE_LOG(LogTemp, Log, TEXT("  PostProcess:     %s"), PostProcess     ? TEXT("OK") : TEXT("MISSING"));
    UE_LOG(LogTemp, Log, TEXT("  SwarmCamera:     %s"), SwarmCamera     ? TEXT("OK") : TEXT("MISSING"));
    UE_LOG(LogTemp, Log, TEXT("  TiltShiftEffect: %s"), TiltShiftEffect ? TEXT("OK") : TEXT("MISSING"));
    UE_LOG(LogTemp, Log, TEXT("  TimeDilation:    %s"), TimeDilation    ? TEXT("OK") : TEXT("MISSING"));

    if (SwarmCamera)
    {
        UE_LOG(LogTemp, Log, TEXT("  SwarmCamera->SpringArm: %s"),
               SwarmCamera->SpringArm ? TEXT("WIRED") : TEXT("NULL — tracking will fail!"));
    }

    if (TiltShiftEffect)
    {
        UE_LOG(LogTemp, Log, TEXT("  TiltShiftEffect->PostProcessComponent: %s"),
               TiltShiftEffect->PostProcessComponent ? TEXT("WIRED") : TEXT("pending BeginPlay auto-discovery"));
    }

    if (TimeDilation)
    {
        UE_LOG(LogTemp, Log, TEXT("  TimeDilation->TiltShiftEffect: %s"),
               TimeDilation->TiltShiftEffect ? TEXT("WIRED") : TEXT("NULL"));
    }

    // TODO: Re-enable once spawn test is verified and camera tracking is confirmed working.
    // For now, leave PIE camera free so we can orbit and inspect the scene.
    // To test the camera rig view, press F8 in PIE to eject, then select the
    // CameraRig in the Outliner and click "Pilot" on it.
    //
    // if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    // {
    //     PC->SetViewTargetWithBlend(this, 0.5f);
    //     UE_LOG(LogTemp, Log, TEXT("[MXCameraRig] Set as active view target."));
    // }
    UE_LOG(LogTemp, Log, TEXT("[MXCameraRig] View target NOT set — free camera mode for debugging."));
}
