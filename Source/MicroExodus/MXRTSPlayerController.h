// MXRTSPlayerController.h — Phase 2B: RTS Camera Controller
// Created: 2026-03-08
// v3 — axis bindings for WASD, pivot rotation, double-click snap

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MXRTSPlayerController.generated.h"

class AMXCameraRig;

/**
 * AMXRTSPlayerController
 * RTS-style camera controller.
 *
 * Input (configured in DefaultInput.ini, visible in Project Settings → Input):
 *   Scroll Wheel      → Zoom in/out toward cursor
 *   Right Mouse Drag  → Rotate camera around point under cursor
 *   WASD / Arrows     → Pan camera (axis bindings: MoveForward, MoveRight)
 *   Edge of Screen    → Auto-pan when cursor near edges
 *   Middle Mouse Drag → Grab-pan camera
 *   Double Left Click → Snap camera to clicked world position
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXRTSPlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    AMXRTSPlayerController();

    // =========================================================================
    // Configuration
    // =========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Zoom")
    float ZoomSpeed = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Zoom")
    float MinZoom = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Zoom")
    float MaxZoom = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Zoom")
    float ZoomInterpSpeed = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Pan")
    float PanSpeed = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Pan")
    float EdgePanSpeed = 1000.0f;

    /** Fraction of screen from edge that triggers pan (0.05 = 5%). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Pan", meta=(ClampMin="0.01", ClampMax="0.15"))
    float EdgePanThreshold = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Pan")
    bool bEdgePanEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Rotate")
    float RotateSpeed = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Pan")
    float DragPanSpeed = 3.0f;

    /** Time window for double-click detection (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Click")
    float DoubleClickTime = 0.3f;

    /** Speed at which the camera moves to a double-clicked position. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Click")
    float SnapMoveSpeed = 10.0f;

    // =========================================================================
    // Camera Rig Reference
    // =========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "MX|RTS")
    TObjectPtr<AMXCameraRig> CameraRig;

protected:

    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

private:

    // ---- Input Callbacks ----
    void OnZoomAxis(float Value);
    void OnMoveForward(float Value);
    void OnMoveRight(float Value);
    void OnMouseX(float Value);
    void OnMouseY(float Value);
    void OnRightMousePressed();
    void OnRightMouseReleased();
    void OnMiddleMousePressed();
    void OnMiddleMouseReleased();
    void OnLeftMousePressed();
    void OnLeftMouseReleased();

    // ---- Tick Handlers ----
    void HandleZoom(float DeltaTime);
    void HandleRotation(float DeltaTime);
    void HandleKeyboardPan(float DeltaTime);
    void HandleEdgePan(float DeltaTime);
    void HandleDragPan(float DeltaTime);
    void HandleLeftDragPan(float DeltaTime);
    void HandleSnapMove(float DeltaTime);

    // ---- Helpers ----
    void GetPlanarDirections(FVector& OutForward, FVector& OutRight) const;
    bool GetCursorWorldPosition(FVector& OutWorldPos) const;

    // ---- State ----
    float TargetZoom = 500.0f;
    float CurrentZoom = 500.0f;

    float MoveForwardInput = 0.0f;
    float MoveRightInput = 0.0f;
    float MouseDeltaX = 0.0f;
    float MouseDeltaY = 0.0f;

    bool bRotating = false;
    bool bDragPanning = false;

    // Pivot rotation: world position under cursor when right-click started.
    FVector RotationPivot = FVector::ZeroVector;
    bool bHasRotationPivot = false;

    // Double-click detection.
    float LastLeftClickTime = -1.0f;

    // Left-click drag-to-pan: grab the ground and slide it.
    bool bLeftMouseDown = false;
    bool bLeftDragging = false;
    FVector LeftDragWorldAnchor = FVector::ZeroVector;  // World pos where the grab started.
    FVector2D LeftDragScreenStart = FVector2D::ZeroVector;
    /** Pixel distance the mouse must move before a left-click becomes a drag (prevents accidental drags). */
    static constexpr float LeftDragThreshold = 5.0f;

    // Snap-move target.
    bool bSnapping = false;
    FVector SnapTarget = FVector::ZeroVector;
};
