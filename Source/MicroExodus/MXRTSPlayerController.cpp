// MXRTSPlayerController.cpp — Phase 2C-Move Update
// Originally created: Phase 2B
// Updated: 2026-03-09

#include "MXRTSPlayerController.h"
#include "MXSelectionManager.h"
#include "MXRobotActor.h"
#include "MXCameraRig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXRTSPlayerController::AMXRTSPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    // Create selection manager as a default subobject.
    SelectionManager = CreateDefaultSubobject<UMXSelectionManager>(TEXT("SelectionManager"));
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FindCameraRig();

    if (CameraRig)
    {
        // Set view target to the camera rig.
        SetViewTargetWithBlend(CameraRig, 0.5f);
    }
}

// ---------------------------------------------------------------------------
// PlayerTick — Main input loop
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    // ---- Camera ----
    HandleZoom(DeltaTime);
    HandleRotation(DeltaTime);
    HandleKeyboardPan(DeltaTime);
    HandleDragPan(DeltaTime);
    HandleDoubleClickCenter();

    // ---- Selection ----
    HandleLeftMouseInput(DeltaTime);
    HandleRightClickMove();
    HandleControlGroups();
    HandleSelectAll();

    // ---- Hover (every tick) ----
    if (SelectionManager && !bIsDraggingBoxSelect)
    {
        SelectionManager->UpdateHover();
    }
}

// ---------------------------------------------------------------------------
// Camera: Zoom
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleZoom(float DeltaTime)
{
    if (!CachedSpringArm) return;

    // Detect scroll wheel via axis value.
    float ScrollDelta = 0.0f;
    if (IsInputKeyDown(EKeys::MouseScrollUp))
    {
        ScrollDelta = -1.0f;
    }
    else if (IsInputKeyDown(EKeys::MouseScrollDown))
    {
        ScrollDelta = 1.0f;
    }

    if (!FMath::IsNearlyZero(ScrollDelta))
    {
        // Speed scales with current zoom level for natural feel.
        float ZoomFactor = ZoomSpeed * (CachedSpringArm->TargetArmLength / 500.0f);
        TargetZoom = FMath::Clamp(TargetZoom + ScrollDelta * ZoomFactor, MinZoom, MaxZoom);
    }

    // Smooth zoom interpolation.
    CachedSpringArm->TargetArmLength = FMath::FInterpTo(
        CachedSpringArm->TargetArmLength, TargetZoom, DeltaTime, ZoomInterpSpeed);
}

// ---------------------------------------------------------------------------
// Camera: Rotation (Right-click drag)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleRotation(float DeltaTime)
{
    if (!CameraRig) return;

    bool bRightDown = IsInputKeyDown(EKeys::RightMouseButton);

    if (bRightDown && !bRightMouseDown)
    {
        // Just pressed.
        bRightMouseDown = true;
        GetMousePosition(PreviousMousePos.X, PreviousMousePos.Y);
    }
    else if (!bRightDown && bRightMouseDown)
    {
        // Just released.
        bRightMouseDown = false;
    }

    if (bRightMouseDown)
    {
        FVector2D CurrentMouse;
        GetMousePosition(CurrentMouse.X, CurrentMouse.Y);
        float DeltaX = CurrentMouse.X - PreviousMousePos.X;

        FRotator RigRot = CameraRig->GetActorRotation();
        RigRot.Yaw += DeltaX * RotateSpeed;
        CameraRig->SetActorRotation(RigRot);

        PreviousMousePos = CurrentMouse;
    }
}

// ---------------------------------------------------------------------------
// Camera: Keyboard Pan (WASD / Arrows)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleKeyboardPan(float DeltaTime)
{
    if (!CameraRig) return;

    FVector Forward, Right;
    GetPlanarDirections(Forward, Right);

    FVector PanDirection = FVector::ZeroVector;

    if (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up))    PanDirection += Forward;
    if (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down))  PanDirection -= Forward;
    if (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) PanDirection += Right;
    if (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left))  PanDirection -= Right;

    if (!PanDirection.IsNearlyZero())
    {
        PanDirection.Normalize();
        // Pan speed scales with zoom for consistent screen-space speed.
        float ZoomScale = CachedSpringArm ? (CachedSpringArm->TargetArmLength / 800.0f) : 1.0f;
        CameraRig->AddActorWorldOffset(PanDirection * PanSpeed * ZoomScale * DeltaTime);
    }
}

// ---------------------------------------------------------------------------
// Camera: Double-Click Center — snap camera to ground under cursor
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleDoubleClickCenter()
{
    if (!CameraRig) return;

    // Detect double-click on left mouse (via timing).
    static float LastClickTime = 0.0f;
    static bool bWasLeftDown = false;

    bool bLeftDown = IsInputKeyDown(EKeys::LeftMouseButton);

    if (bLeftDown && !bWasLeftDown)
    {
        float Now = GetWorld()->GetTimeSeconds();
        float TimeSinceLast = Now - LastClickTime;

        if (TimeSinceLast < 0.3f && TimeSinceLast > 0.01f)
        {
            // Double-click detected — center camera on ground hit.
            FVector GroundHit;
            if (GetGroundHitUnderCursor(GroundHit))
            {
                // Keep the camera's current Z, just move XY.
                FVector NewPos = CameraRig->GetActorLocation();
                NewPos.X = GroundHit.X;
                NewPos.Y = GroundHit.Y;
                CameraRig->SetActorLocation(NewPos);

                UE_LOG(LogTemp, Log,
                    TEXT("MXRTSPlayerController: Double-click centered camera at (%.0f, %.0f)."),
                    GroundHit.X, GroundHit.Y);
            }
        }

        LastClickTime = Now;
    }

    bWasLeftDown = bLeftDown;
}

// ---------------------------------------------------------------------------
// Camera: Drag Pan (Middle-click)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleDragPan(float DeltaTime)
{
    if (!CameraRig) return;

    bool bMiddleDown = IsInputKeyDown(EKeys::MiddleMouseButton);

    if (bMiddleDown && !bMiddleMouseDown)
    {
        bMiddleMouseDown = true;
        GetMousePosition(MiddleMouseDownPos.X, MiddleMouseDownPos.Y);
    }
    else if (!bMiddleDown && bMiddleMouseDown)
    {
        bMiddleMouseDown = false;
    }

    if (bMiddleMouseDown)
    {
        FVector2D CurrentMouse;
        GetMousePosition(CurrentMouse.X, CurrentMouse.Y);
        FVector2D Delta = CurrentMouse - MiddleMouseDownPos;

        FVector Forward, Right;
        GetPlanarDirections(Forward, Right);

        // Inverted for grab-and-drag feel.
        FVector PanOffset = (-Right * Delta.X + Forward * Delta.Y) * DragPanSpeed;
        CameraRig->AddActorWorldOffset(PanOffset * DeltaTime);

        MiddleMouseDownPos = CurrentMouse;
    }
}

// ---------------------------------------------------------------------------
// Selection: Left Mouse Input
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleLeftMouseInput(float DeltaTime)
{
    if (!SelectionManager) return;

    bool bLeftDown = IsInputKeyDown(EKeys::LeftMouseButton);
    bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

    // Don't process selection while right-click rotating.
    if (bRightMouseDown) return;

    // --- Just Pressed ---
    if (bLeftDown && !bLeftMouseDown)
    {
        bLeftMouseDown = true;
        bLeftMouseJustPressed = true;
        bIsDraggingBoxSelect = false;

        FVector2D MousePos;
        GetMousePosition(MousePos.X, MousePos.Y);
        LeftMouseDownPos = MousePos;

        SelectionManager->BeginBoxSelect(MousePos);
    }

    // --- Held ---
    if (bLeftDown && bLeftMouseDown)
    {
        FVector2D MousePos;
        GetMousePosition(MousePos.X, MousePos.Y);

        float DragDist = FVector2D::Distance(LeftMouseDownPos, MousePos);
        if (DragDist > SelectionManager->BoxSelectThreshold)
        {
            bIsDraggingBoxSelect = true;
        }

        SelectionManager->UpdateBoxSelect(MousePos);
        bLeftMouseJustPressed = false;
    }

    // --- Just Released ---
    if (!bLeftDown && bLeftMouseDown)
    {
        bLeftMouseDown = false;

        if (bIsDraggingBoxSelect)
        {
            // Finalize box select.
            SelectionManager->EndBoxSelect(bShift);
        }
        else
        {
            // Single click select.
            SelectionManager->TrySelectAtCursor(bShift);
        }

        bIsDraggingBoxSelect = false;
    }
}

// ---------------------------------------------------------------------------
// Movement: Right-Click Move Command
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleRightClickMove()
{
    if (!SelectionManager) return;
    if (!SelectionManager->HasSelection()) return;

    // Detect right-click release (not drag-rotate).
    // We use a simple approach: if right mouse was down briefly without much drag, it's a move command.
    // For simplicity, we'll check for a dedicated move key instead.
    // Actually: right-click is also used for rotation. We need to distinguish.
    // Solution: short right-click = move command. Long right-click = rotation.
    // We track the right mouse press time.

    static float RightClickStartTime = 0.0f;
    static bool bRightWasDown = false;
    static FVector2D RightClickStartPos = FVector2D::ZeroVector;

    bool bRightDown = IsInputKeyDown(EKeys::RightMouseButton);

    if (bRightDown && !bRightWasDown)
    {
        // Just pressed.
        RightClickStartTime = GetWorld()->GetTimeSeconds();
        GetMousePosition(RightClickStartPos.X, RightClickStartPos.Y);
        bRightWasDown = true;
    }

    if (!bRightDown && bRightWasDown)
    {
        // Just released.
        bRightWasDown = false;

        float HeldDuration = GetWorld()->GetTimeSeconds() - RightClickStartTime;
        FVector2D ReleasePos;
        GetMousePosition(ReleasePos.X, ReleasePos.Y);
        float DragDist = FVector2D::Distance(RightClickStartPos, ReleasePos);

        // Short click with minimal drag = move command.
        if (HeldDuration < 0.25f && DragDist < 10.0f)
        {
            FVector GroundHit;
            if (GetGroundHitUnderCursor(GroundHit))
            {
                IssueMoveCommand(GroundHit);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Selection: Control Groups (Ctrl+1-9 to save, 1-9 to recall)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleControlGroups()
{
    if (!SelectionManager) return;

    bool bCtrl = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

    // Check number keys 1-9.
    for (int32 i = 1; i <= 9; ++i)
    {
        FKey NumberKey = FKey(*FString::Printf(TEXT("%d"), i));
        if (WasInputKeyJustPressed(NumberKey))
        {
            if (bCtrl)
            {
                SelectionManager->SaveControlGroup(i);
            }
            else
            {
                SelectionManager->RecallControlGroup(i);
            }
            break; // Only handle one per tick.
        }
    }
}

// ---------------------------------------------------------------------------
// Selection: Select All (Ctrl+A)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleSelectAll()
{
    if (!SelectionManager) return;

    bool bCtrl = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    if (bCtrl && WasInputKeyJustPressed(EKeys::A))
    {
        SelectionManager->SelectAll();
    }
}

// ---------------------------------------------------------------------------
// Movement: Issue Move Command
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::IssueMoveCommand(FVector WorldTarget)
{
    TArray<AMXRobotActor*> Selected = SelectionManager->GetSelectedRobots();
    if (Selected.Num() == 0) return;

    if (Selected.Num() == 1)
    {
        // Single robot — move directly to click.
        Selected[0]->MoveToLocation(WorldTarget);
    }
    else
    {
        // Multiple robots — spread in a circle formation.
        const float Radius = FormationSpacing * FMath::Sqrt((float)Selected.Num());
        const float AngleStep = 360.0f / (float)Selected.Num();

        for (int32 i = 0; i < Selected.Num(); ++i)
        {
            float AngleDeg = AngleStep * i;
            float AngleRad = FMath::DegreesToRadians(AngleDeg);

            FVector Offset(
                FMath::Cos(AngleRad) * Radius,
                FMath::Sin(AngleRad) * Radius,
                0.0f
            );

            Selected[i]->MoveToLocation(WorldTarget + Offset);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXRTSPlayerController: Move command issued to %d robots at (%.0f, %.0f, %.0f)."),
           Selected.Num(), WorldTarget.X, WorldTarget.Y, WorldTarget.Z);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool AMXRTSPlayerController::GetGroundHitUnderCursor(FVector& OutLocation) const
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_WorldStatic, true, Hit);

    if (Hit.bBlockingHit)
    {
        OutLocation = Hit.ImpactPoint;
        return true;
    }

    return false;
}

void AMXRTSPlayerController::GetPlanarDirections(FVector& OutForward, FVector& OutRight) const
{
    FRotator ControlRot = GetControlRotation();
    ControlRot.Pitch = 0.0f;
    ControlRot.Roll = 0.0f;

    OutForward = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
    OutRight = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);
}

void AMXRTSPlayerController::FindCameraRig()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (TActorIterator<AMXCameraRig> It(World); It; ++It)
    {
        CameraRig = *It;
        break;
    }

    if (CameraRig)
    {
        // Cache the spring arm for zoom control.
        CachedSpringArm = CameraRig->FindComponentByClass<USpringArmComponent>();
        if (CachedSpringArm)
        {
            TargetZoom = CachedSpringArm->TargetArmLength;
        }

        UE_LOG(LogTemp, Log, TEXT("MXRTSPlayerController: Found CameraRig, SpringArm cached."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRTSPlayerController: No AMXCameraRig found in level!"));
    }
}
