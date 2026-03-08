// MXRTSPlayerController.cpp — Phase 2B: RTS Camera Controller
// Created: 2026-03-08
// v3 — axis bindings for WASD, pivot rotation, double-click snap

#include "MXRTSPlayerController.h"
#include "MXCameraRig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXRTSPlayerController::AMXRTSPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Find the CameraRig in the level.
    for (TActorIterator<AMXCameraRig> It(GetWorld()); It; ++It)
    {
        CameraRig = *It;
        break;
    }

    if (CameraRig)
    {
        SetViewTargetWithBlend(CameraRig, 0.2f);

        if (CameraRig->SpringArm)
        {
            CurrentZoom = CameraRig->SpringArm->TargetArmLength;
            TargetZoom = CurrentZoom;
        }

        UE_LOG(LogTemp, Log, TEXT("[MXRTSController] CameraRig found. Zoom: %.0f"), CurrentZoom);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MXRTSController] No AMXCameraRig found in level!"));
    }

    // Game + UI: cursor visible, all input works.
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
}

// ---------------------------------------------------------------------------
// SetupInputComponent
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent) return;

    // Axes — bound to DefaultInput.ini mappings.
    InputComponent->BindAxis("ZoomAxis", this, &AMXRTSPlayerController::OnZoomAxis);
    InputComponent->BindAxis("MoveForward", this, &AMXRTSPlayerController::OnMoveForward);
    InputComponent->BindAxis("MoveRight", this, &AMXRTSPlayerController::OnMoveRight);
    InputComponent->BindAxis("MouseX", this, &AMXRTSPlayerController::OnMouseX);
    InputComponent->BindAxis("MouseY", this, &AMXRTSPlayerController::OnMouseY);

    // Actions.
    InputComponent->BindAction("RightMouse", IE_Pressed, this, &AMXRTSPlayerController::OnRightMousePressed);
    InputComponent->BindAction("RightMouse", IE_Released, this, &AMXRTSPlayerController::OnRightMouseReleased);
    InputComponent->BindAction("MiddleMouse", IE_Pressed, this, &AMXRTSPlayerController::OnMiddleMousePressed);
    InputComponent->BindAction("MiddleMouse", IE_Released, this, &AMXRTSPlayerController::OnMiddleMouseReleased);
    InputComponent->BindAction("LeftMouse", IE_Pressed, this, &AMXRTSPlayerController::OnLeftMousePressed);
    InputComponent->BindAction("LeftMouse", IE_Released, this, &AMXRTSPlayerController::OnLeftMouseReleased);

    UE_LOG(LogTemp, Log, TEXT("[MXRTSController] Input bindings complete."));
}

// ---------------------------------------------------------------------------
// Input Callbacks
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::OnZoomAxis(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;

    // Zoom toward/away from cursor position for natural feel.
    float ZoomDelta = -Value * ZoomSpeed * FMath::Max(CurrentZoom / 300.0f, 1.0f);
    float NewZoom = FMath::Clamp(TargetZoom + ZoomDelta, MinZoom, MaxZoom);

    // Move the rig toward the cursor world position proportionally to zoom change.
    if (CameraRig)
    {
        FVector CursorWorld;
        if (GetCursorWorldPosition(CursorWorld))
        {
            float ZoomFraction = (TargetZoom - NewZoom) / TargetZoom;
            FVector RigPos = CameraRig->GetActorLocation();
            FVector Direction = CursorWorld - RigPos;
            Direction.Z = 0.0f; // Stay on the plane.
            CameraRig->SetActorLocation(RigPos + Direction * ZoomFraction * 0.3f);
        }
    }

    TargetZoom = NewZoom;
}

void AMXRTSPlayerController::OnMoveForward(float Value) { MoveForwardInput = Value; }
void AMXRTSPlayerController::OnMoveRight(float Value)   { MoveRightInput = Value; }
void AMXRTSPlayerController::OnMouseX(float Value)       { MouseDeltaX = Value; }
void AMXRTSPlayerController::OnMouseY(float Value)       { MouseDeltaY = Value; }

void AMXRTSPlayerController::OnRightMousePressed()
{
    bRotating = true;
    bHasRotationPivot = false;

    // Capture the world position under the cursor as the rotation pivot.
    FVector CursorWorld;
    if (GetCursorWorldPosition(CursorWorld))
    {
        RotationPivot = CursorWorld;
        bHasRotationPivot = true;
    }
}

void AMXRTSPlayerController::OnRightMouseReleased()
{
    bRotating = false;
    bHasRotationPivot = false;
}

void AMXRTSPlayerController::OnMiddleMousePressed()  { bDragPanning = true; }
void AMXRTSPlayerController::OnMiddleMouseReleased() { bDragPanning = false; }

void AMXRTSPlayerController::OnLeftMousePressed()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    bLeftMouseDown = true;
    bLeftDragging = false;

    // Capture screen position for drag threshold check.
    float MX, MY;
    if (GetMousePosition(MX, MY))
    {
        LeftDragScreenStart = FVector2D(MX, MY);
    }

    // Capture world position as the grab anchor.
    GetCursorWorldPosition(LeftDragWorldAnchor);

    // Double-click detection.
    if (CurrentTime - LastLeftClickTime < DoubleClickTime)
    {
        // Double-click — snap camera to cursor world position.
        FVector CursorWorld;
        if (GetCursorWorldPosition(CursorWorld))
        {
            SnapTarget = FVector(CursorWorld.X, CursorWorld.Y, CameraRig ? CameraRig->GetActorLocation().Z : 0.0f);
            bSnapping = true;
        }
        LastLeftClickTime = -1.0f; // Reset to prevent triple-click.
    }
    else
    {
        LastLeftClickTime = CurrentTime;
    }
}

void AMXRTSPlayerController::OnLeftMouseReleased()
{
    bLeftMouseDown = false;

    if (bLeftDragging)
    {
        // Was a drag — just stop dragging.
        bLeftDragging = false;
    }
    else
    {
        // Was a click (didn't exceed drag threshold).
        // Reserved for Phase 2C: selection system.
    }
}

// ---------------------------------------------------------------------------
// PlayerTick
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!CameraRig) return;

    HandleZoom(DeltaTime);
    HandleRotation(DeltaTime);
    HandleDragPan(DeltaTime);
    HandleLeftDragPan(DeltaTime);
    HandleKeyboardPan(DeltaTime);
    HandleEdgePan(DeltaTime);
    HandleSnapMove(DeltaTime);
}

// ---------------------------------------------------------------------------
// HandleZoom
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleZoom(float DeltaTime)
{
    if (!CameraRig || !CameraRig->SpringArm) return;
    CurrentZoom = FMath::FInterpTo(CurrentZoom, TargetZoom, DeltaTime, ZoomInterpSpeed);
    CameraRig->SpringArm->TargetArmLength = CurrentZoom;
}

// ---------------------------------------------------------------------------
// HandleRotation — rotate around the cursor pivot point
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleRotation(float DeltaTime)
{
    if (!CameraRig || !bRotating) return;
    if (FMath::IsNearlyZero(MouseDeltaX)) return;

    float YawDelta = MouseDeltaX * RotateSpeed;

    if (bHasRotationPivot)
    {
        // Rotate the rig around the pivot point (where the cursor was when RMB was pressed).
        FVector RigPos = CameraRig->GetActorLocation();
        FVector Offset = RigPos - RotationPivot;
        Offset.Z = 0.0f;

        // Rotate the offset vector around Z.
        float Rad = FMath::DegreesToRadians(YawDelta);
        float CosA = FMath::Cos(Rad);
        float SinA = FMath::Sin(Rad);
        FVector RotatedOffset;
        RotatedOffset.X = Offset.X * CosA - Offset.Y * SinA;
        RotatedOffset.Y = Offset.X * SinA + Offset.Y * CosA;
        RotatedOffset.Z = RigPos.Z;

        FVector NewPos = FVector(RotationPivot.X + RotatedOffset.X,
                                  RotationPivot.Y + RotatedOffset.Y,
                                  RigPos.Z);
        CameraRig->SetActorLocation(NewPos);
    }

    // Always rotate the rig's yaw.
    FRotator Rot = CameraRig->GetActorRotation();
    Rot.Yaw += YawDelta;
    CameraRig->SetActorRotation(Rot);
}

// ---------------------------------------------------------------------------
// HandleKeyboardPan — WASD / Arrows via axis bindings
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleKeyboardPan(float DeltaTime)
{
    if (!CameraRig) return;
    if (FMath::IsNearlyZero(MoveForwardInput) && FMath::IsNearlyZero(MoveRightInput)) return;

    FVector Forward, Right;
    GetPlanarDirections(Forward, Right);

    FVector PanDir = Forward * MoveForwardInput + Right * MoveRightInput;
    PanDir.Normalize();

    float SpeedScale = FMath::Max(CurrentZoom / 300.0f, 1.0f);
    CameraRig->SetActorLocation(
        CameraRig->GetActorLocation() + PanDir * PanSpeed * SpeedScale * DeltaTime);

    // Cancel snap if player manually pans.
    bSnapping = false;
}

// ---------------------------------------------------------------------------
// HandleEdgePan
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleEdgePan(float DeltaTime)
{
    if (!CameraRig || !bEdgePanEnabled) return;
    if (bRotating || bDragPanning || bLeftDragging) return;

    int32 VPX, VPY;
    GetViewportSize(VPX, VPY);
    if (VPX <= 0 || VPY <= 0) return;

    float MX, MY;
    if (!GetMousePosition(MX, MY)) return;
    if (MX <= 1.0f && MY <= 1.0f) return;

    float TX = VPX * EdgePanThreshold;
    float TY = VPY * EdgePanThreshold;

    FVector PanDir = FVector::ZeroVector;
    FVector Forward, Right;
    GetPlanarDirections(Forward, Right);

    if (MX < TX)             PanDir -= Right;
    else if (MX > VPX - TX)  PanDir += Right;
    if (MY < TY)             PanDir += Forward;
    else if (MY > VPY - TY)  PanDir -= Forward;

    if (!PanDir.IsNearlyZero())
    {
        PanDir.Normalize();
        float SpeedScale = FMath::Max(CurrentZoom / 300.0f, 1.0f);
        CameraRig->SetActorLocation(
            CameraRig->GetActorLocation() + PanDir * EdgePanSpeed * SpeedScale * DeltaTime);
        bSnapping = false;
    }
}

// ---------------------------------------------------------------------------
// HandleDragPan — middle mouse grab-pan
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleDragPan(float DeltaTime)
{
    if (!CameraRig || !bDragPanning) return;
    if (FMath::IsNearlyZero(MouseDeltaX) && FMath::IsNearlyZero(MouseDeltaY)) return;

    FVector Forward, Right;
    GetPlanarDirections(Forward, Right);

    float SpeedScale = FMath::Max(CurrentZoom / 300.0f, 0.5f);
    FVector Offset = (-Right * MouseDeltaX + Forward * MouseDeltaY) * DragPanSpeed * SpeedScale;
    CameraRig->SetActorLocation(CameraRig->GetActorLocation() + Offset);
    bSnapping = false;
}

// ---------------------------------------------------------------------------
// HandleLeftDragPan — left-click grab ground and slide
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleLeftDragPan(float DeltaTime)
{
    if (!CameraRig || !bLeftMouseDown) return;

    // Check if we should start dragging (past the pixel threshold).
    if (!bLeftDragging)
    {
        float MX, MY;
        if (!GetMousePosition(MX, MY)) return;

        float PixelDist = FVector2D::Distance(LeftDragScreenStart, FVector2D(MX, MY));
        if (PixelDist < LeftDragThreshold) return;

        // Crossed the threshold — commit to dragging (not a click).
        bLeftDragging = true;
        bSnapping = false;
        LastLeftClickTime = -1.0f; // Cancel double-click if we started dragging.
    }

    // We're dragging: move the camera so the original grab point stays under the cursor.
    // Get where the cursor currently points in the world.
    FVector CurrentCursorWorld;
    if (!GetCursorWorldPosition(CurrentCursorWorld)) return;

    // The difference between where the anchor was and where it is now tells us how to move.
    FVector Delta = LeftDragWorldAnchor - CurrentCursorWorld;
    Delta.Z = 0.0f;

    if (!Delta.IsNearlyZero(0.5f))
    {
        CameraRig->SetActorLocation(CameraRig->GetActorLocation() + Delta);
    }
}

// ---------------------------------------------------------------------------
// HandleSnapMove — smooth move to double-clicked position
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::HandleSnapMove(float DeltaTime)
{
    if (!CameraRig || !bSnapping) return;

    FVector Current = CameraRig->GetActorLocation();
    FVector NewPos = FMath::VInterpTo(Current, SnapTarget, DeltaTime, SnapMoveSpeed);
    CameraRig->SetActorLocation(NewPos);

    // Stop snapping when close enough.
    if (FVector::Dist2D(NewPos, SnapTarget) < 5.0f)
    {
        bSnapping = false;
    }
}

// ---------------------------------------------------------------------------
// GetPlanarDirections
// ---------------------------------------------------------------------------

void AMXRTSPlayerController::GetPlanarDirections(FVector& OutForward, FVector& OutRight) const
{
    if (!CameraRig)
    {
        OutForward = FVector::ForwardVector;
        OutRight = FVector::RightVector;
        return;
    }

    FRotator YawOnly(0.0f, CameraRig->GetActorRotation().Yaw, 0.0f);
    OutForward = YawOnly.Vector();
    OutRight = FRotationMatrix(YawOnly).GetScaledAxis(EAxis::Y);
    OutForward.Z = 0.0f;
    OutRight.Z = 0.0f;
    OutForward.Normalize();
    OutRight.Normalize();
}

// ---------------------------------------------------------------------------
// GetCursorWorldPosition — trace from cursor to the ground plane
// ---------------------------------------------------------------------------

bool AMXRTSPlayerController::GetCursorWorldPosition(FVector& OutWorldPos) const
{
    FVector WorldOrigin, WorldDirection;
    if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
    {
        return false;
    }

    // Trace against a horizontal plane at Z=0 (the floor).
    // Ray: P = Origin + t * Direction
    // Plane: Z = 0 → t = -Origin.Z / Direction.Z
    if (FMath::IsNearlyZero(WorldDirection.Z))
    {
        return false; // Ray is parallel to floor.
    }

    float T = -WorldOrigin.Z / WorldDirection.Z;
    if (T < 0.0f)
    {
        return false; // Intersection is behind the camera.
    }

    OutWorldPos = WorldOrigin + WorldDirection * T;
    return true;
}
