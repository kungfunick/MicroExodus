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

    // Enable both game and UI input so keyboard works with cursor visible.
    // Without this, bShowMouseCursor + no pawn defaults to UI-only mode,
    // blocking keyboard events from reaching IsInputKeyDown.
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    FindCameraRig();

    if (CameraRig)
    {
        // Set view target to the camera rig.
        SetViewTargetWithBlend(CameraRig, 0.5f);
        // Save initial position for reset view.
        InitialCameraPos = CameraRig->GetActorLocation();
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
    HandleResetView();

    // ---- Selection + Movement (all on LMB) ----
    HandleLeftMouseInput(DeltaTime);
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

    // Use the analog axis value — the ONLY reliable scroll wheel detection in UE5.
    // IsInputKeyDown and WasInputKeyJustPressed both miss scroll events because
    // the scroll wheel fires as a sub-frame impulse, not a sustained key state.
    float ScrollDelta = GetInputAnalogKeyState(EKeys::MouseWheelAxis);

    if (!FMath::IsNearlyZero(ScrollDelta))
    {
        // Negate so scroll-up = zoom in (shorter arm). Speed scales with zoom level.
        float ZoomFactor = ZoomSpeed * (CachedSpringArm->TargetArmLength / 500.0f);
        TargetZoom = FMath::Clamp(TargetZoom - ScrollDelta * ZoomFactor, MinZoom, MaxZoom);
    }

    // Smooth zoom interpolation.
    CachedSpringArm->TargetArmLength = FMath::FInterpTo(
        CachedSpringArm->TargetArmLength, TargetZoom, DeltaTime, ZoomInterpSpeed);
}

// ---------------------------------------------------------------------------
// Camera: Rotation (Middle-click drag — yaw + pitch)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleRotation(float DeltaTime)
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
        float DeltaX = CurrentMouse.X - MiddleMouseDownPos.X;
        float DeltaY = CurrentMouse.Y - MiddleMouseDownPos.Y;

        // Yaw: rotate the camera rig actor.
        FRotator RigRot = CameraRig->GetActorRotation();
        RigRot.Yaw += DeltaX * RotateSpeed;
        CameraRig->SetActorRotation(RigRot);

        // Pitch: adjust the spring arm's relative pitch, clamped.
        if (CachedSpringArm)
        {
            FRotator ArmRot = CachedSpringArm->GetRelativeRotation();
            ArmRot.Pitch = FMath::Clamp(ArmRot.Pitch - DeltaY * RotateSpeed,
                                         PitchMin, PitchMax);
            CachedSpringArm->SetRelativeRotation(ArmRot);
        }

        MiddleMouseDownPos = CurrentMouse;
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
// Camera: Tablecloth Drag Pan (Right-click drag)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleDragPan(float DeltaTime)
{
    if (!CameraRig) return;

    bool bRightDown = IsInputKeyDown(EKeys::RightMouseButton);

    if (bRightDown && !bRightMouseDown)
    {
        bRightMouseDown = true;
        GetMousePosition(PreviousMousePos.X, PreviousMousePos.Y);
    }
    else if (!bRightDown && bRightMouseDown)
    {
        bRightMouseDown = false;
    }

    if (bRightMouseDown)
    {
        FVector2D CurrentMouse;
        GetMousePosition(CurrentMouse.X, CurrentMouse.Y);
        FVector2D Delta = CurrentMouse - PreviousMousePos;

        FVector Forward, Right;
        GetPlanarDirections(Forward, Right);

        // Inverted for grab-and-drag (tablecloth) feel.
        // Delta is already per-frame pixel movement — do NOT multiply by DeltaTime.
        float ZoomScale = CachedSpringArm ? (CachedSpringArm->TargetArmLength / 800.0f) : 1.0f;
        FVector PanOffset = (-Right * Delta.X + Forward * Delta.Y) * DragPanSpeed * ZoomScale;
        CameraRig->AddActorWorldOffset(PanOffset);

        PreviousMousePos = CurrentMouse;
    }
}

// ---------------------------------------------------------------------------
// Camera: Reset View (Home key)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleResetView()
{
    if (!CameraRig) return;

    if (WasInputKeyJustPressed(EKeys::Home))
    {
        // Reset position to initial (floor center).
        CameraRig->SetActorLocation(InitialCameraPos);

        // Reset yaw to 0.
        FRotator RigRot = CameraRig->GetActorRotation();
        RigRot.Yaw = 0.0f;
        CameraRig->SetActorRotation(RigRot);

        // Reset pitch to default.
        if (CachedSpringArm)
        {
            FRotator ArmRot = CachedSpringArm->GetRelativeRotation();
            ArmRot.Pitch = DefaultPitch;
            CachedSpringArm->SetRelativeRotation(ArmRot);
        }

        // Reset zoom to default.
        TargetZoom = DefaultZoom;

        UE_LOG(LogTemp, Log, TEXT("MXRTSPlayerController: View reset to default."));
    }
}

// ---------------------------------------------------------------------------
// Selection + Movement: Left Mouse Input
// LMB click robot = select. LMB click ground (with selection) = move.
// LMB drag = box select. Shift = additive select. Double-click = zoom.
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleLeftMouseInput(float DeltaTime)
{
    if (!SelectionManager) return;

    bool bLeftDown = IsInputKeyDown(EKeys::LeftMouseButton);
    bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

    // --- Just Pressed ---
    if (bLeftDown && !bLeftMouseDown)
    {
        bLeftMouseDown = true;
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
    }

    // --- Just Released ---
    if (!bLeftDown && bLeftMouseDown)
    {
        bLeftMouseDown = false;

        FVector2D ReleasePos;
        GetMousePosition(ReleasePos.X, ReleasePos.Y);

        if (bIsDraggingBoxSelect)
        {
            // Finalize box select.
            SelectionManager->EndBoxSelect(bShift);
        }
        else
        {
            // Single click — check for double-click first.
            float Now = GetWorld()->GetTimeSeconds();
            float TimeSinceLastClick = Now - LastLeftClickTime;
            float DistFromLastClick = FVector2D::Distance(ReleasePos, LastLeftClickPos);

            if (TimeSinceLastClick < DoubleClickTime && DistFromLastClick < DoubleClickRadius)
            {
                // Double-click.
                HandleDoubleClick(ReleasePos);
                LastLeftClickTime = -1.0f;
            }
            else
            {
                // Single click: check what's under cursor.
                AMXRobotActor* HitRobot = GetRobotUnderCursor();

                if (HitRobot)
                {
                    // Clicked a robot — select it.
                    SelectionManager->TrySelectAtCursor(bShift);
                }
                else if (SelectionManager->HasSelection())
                {
                    // Clicked ground with active selection — move command.
                    FVector GroundHit;
                    if (GetGroundHitUnderCursor(GroundHit))
                    {
                        IssueMoveCommand(GroundHit);
                    }
                }
                // Clicked empty ground with no selection — do nothing.
            }

            LastLeftClickTime = Now;
            LastLeftClickPos = ReleasePos;
        }

        bIsDraggingBoxSelect = false;
    }
}

// ---------------------------------------------------------------------------
// Selection: Control Groups (Shift+1-9 to save, 1-9 to recall)
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleControlGroups()
{
    if (!SelectionManager) return;

    bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

    // Proper EKeys for number row. FKey("1") != EKeys::One in UE5.
    static const FKey NumberKeys[] = {
        EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
        EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
    };

    for (int32 i = 0; i < 9; ++i)
    {
        if (WasInputKeyJustPressed(NumberKeys[i]))
        {
            int32 GroupIndex = i + 1;
            if (bShift)
            {
                SelectionManager->SaveControlGroup(GroupIndex);
                UE_LOG(LogTemp, Log, TEXT("MXRTSPlayerController: Shift+%d — saved group."), GroupIndex);
            }
            else
            {
                SelectionManager->RecallControlGroup(GroupIndex);
                UE_LOG(LogTemp, Log, TEXT("MXRTSPlayerController: %d — recalled group."), GroupIndex);
            }
            break;
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
    // Use the camera rig's yaw so WASD/drag follow camera orientation.
    // GetControlRotation() never updates when we rotate the rig actor directly.
    FRotator CamYaw = FRotator::ZeroRotator;
    if (CameraRig)
    {
        CamYaw.Yaw = CameraRig->GetActorRotation().Yaw;
    }

    OutForward = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::X);
    OutRight = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::Y);
}

// ---------------------------------------------------------------------------
// Double-Click: Zoom to robot or center on ground
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleDoubleClick(const FVector2D& ClickPos)
{
    // Check if a robot is under the cursor.
    AMXRobotActor* Robot = GetRobotUnderCursor();

    if (Robot)
    {
        // Double-click on robot: center camera on robot and zoom in.
        if (CameraRig)
        {
            FVector RobotPos = Robot->GetActorLocation();
            RobotPos.Z = CameraRig->GetActorLocation().Z;
            CameraRig->SetActorLocation(RobotPos);
        }
        if (CachedSpringArm)
        {
            TargetZoom = DoubleClickRobotZoom;
        }
        // Also select the robot.
        if (SelectionManager)
        {
            SelectionManager->TrySelectAtCursor(false);
        }

        UE_LOG(LogTemp, Log, TEXT("MXRTSPlayerController: Double-click zoom to robot '%s'."),
               *Robot->RobotName);
    }
    else
    {
        // Double-click on ground: center camera on that point (no zoom change).
        FVector GroundHit;
        if (GetGroundHitUnderCursor(GroundHit) && CameraRig)
        {
            FVector NewPos = GroundHit;
            NewPos.Z = CameraRig->GetActorLocation().Z;
            CameraRig->SetActorLocation(NewPos);

            UE_LOG(LogTemp, Log,
                TEXT("MXRTSPlayerController: Double-click center at (%.0f, %.0f)."),
                GroundHit.X, GroundHit.Y);
        }
    }
}

AMXRobotActor* AMXRTSPlayerController::GetRobotUnderCursor() const
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Pawn, false, Hit);

    if (Hit.bBlockingHit && Hit.GetActor())
    {
        return Cast<AMXRobotActor>(Hit.GetActor());
    }
    return nullptr;
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
