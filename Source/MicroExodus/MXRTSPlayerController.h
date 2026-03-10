// MXRTSPlayerController.h — Phase 2C-Move Update
// Originally created: Phase 2B
// Updated: 2026-03-09 — Integrated UMXSelectionManager, right-click move, box select HUD
//
// RTS-style camera controller with selection and click-to-move.
// All input via raw polling in PlayerTick (no Enhanced Input dependency).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MXRTSPlayerController.generated.h"

// Forward declarations
class AMXCameraRig;
class AMXRobotActor;
class UMXSelectionManager;
class USpringArmComponent;

// ---------------------------------------------------------------------------
// AMXRTSPlayerController
// ---------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXRTSPlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    AMXRTSPlayerController();

protected:

    virtual void BeginPlay() override;

public:

    virtual void PlayerTick(float DeltaTime) override;

    // -------------------------------------------------------------------------
    // Selection Manager Access
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|RTS")
    UMXSelectionManager* GetSelectionManager() const { return SelectionManager; }

    // -------------------------------------------------------------------------
    // Camera Config
    // -------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float ZoomSpeed = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float MinZoom = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float MaxZoom = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float ZoomInterpSpeed = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float PanSpeed = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float RotateSpeed = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float DragPanSpeed = 2.0f;

    /** Default zoom level (arm length in cm) used by reset view (Home key). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Camera")
    float DefaultZoom = 800.0f;

    // -------------------------------------------------------------------------
    // Selection Config
    // -------------------------------------------------------------------------

    /** Color for the box select rectangle drawn on HUD. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Selection")
    FLinearColor BoxSelectColor = FLinearColor(0.0f, 0.8f, 1.0f, 0.3f);

    /** Border color for box select rectangle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Selection")
    FLinearColor BoxSelectBorderColor = FLinearColor(0.0f, 0.8f, 1.0f, 0.8f);

    // -------------------------------------------------------------------------
    // Move Command Config
    // -------------------------------------------------------------------------

    /** Formation spread when moving multiple robots to a click. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|Movement")
    float FormationSpacing = 40.0f;

    // -------------------------------------------------------------------------
    // Double-Click Config
    // -------------------------------------------------------------------------

    /** Max time between clicks to count as double-click (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|DoubleClick")
    float DoubleClickTime = 0.3f;

    /** Max pixel drift between clicks to count as double-click. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|DoubleClick")
    float DoubleClickRadius = 20.0f;

    /** Zoom level when double-clicking a robot (arm length in cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|RTS|DoubleClick")
    float DoubleClickRobotZoom = 150.0f;

protected:

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|RTS|Components")
    TObjectPtr<UMXSelectionManager> SelectionManager;

private:

    // -------------------------------------------------------------------------
    // Camera State
    // -------------------------------------------------------------------------

    UPROPERTY()
    TObjectPtr<AMXCameraRig> CameraRig;

    UPROPERTY()
    TObjectPtr<USpringArmComponent> CachedSpringArm;

    float TargetZoom = 800.0f;

    /** Saved initial camera position for reset view. */
    FVector InitialCameraPos = FVector::ZeroVector;

    // -------------------------------------------------------------------------
    // Input State
    // -------------------------------------------------------------------------

    /** Left mouse state for selection. */
    bool bLeftMouseDown = false;
    bool bLeftMouseJustPressed = false;
    bool bLeftMouseJustReleased = false;
    FVector2D LeftMouseDownPos = FVector2D::ZeroVector;
    bool bIsDraggingBoxSelect = false;

    /** Right mouse state for rotation. */
    bool bRightMouseDown = false;
    FVector2D PreviousMousePos = FVector2D::ZeroVector;

    /** Middle mouse state for drag pan. */
    bool bMiddleMouseDown = false;
    FVector2D MiddleMouseDownPos = FVector2D::ZeroVector;

    /** Double-click tracking. */
    float LastLeftClickTime = -1.0f;
    FVector2D LastLeftClickPos = FVector2D::ZeroVector;

    // -------------------------------------------------------------------------
    // Camera Input Handlers
    // -------------------------------------------------------------------------

    void HandleZoom(float DeltaTime);
    void HandleRotation(float DeltaTime);
    void HandleKeyboardPan(float DeltaTime);
    void HandleDragPan(float DeltaTime);
    void HandleResetView();

    // -------------------------------------------------------------------------
    // Selection Input Handlers
    // -------------------------------------------------------------------------

    void HandleLeftMouseInput(float DeltaTime);
    void HandleRightClickMove();
    void HandleControlGroups();
    void HandleSelectAll();
    void HandleDoubleClick(const FVector2D& ClickPos);

    // -------------------------------------------------------------------------
    // Movement Commands
    // -------------------------------------------------------------------------

    /**
     * Issue a move command to all selected robots.
     * Robots spread in a circle formation around the click point.
     */
    void IssueMoveCommand(FVector WorldTarget);

    /**
     * Raycast from cursor to the ground plane and return the hit location.
     * @param OutLocation  The world-space hit point.
     * @return             True if a ground hit was found.
     */
    bool GetGroundHitUnderCursor(FVector& OutLocation) const;

    /**
     * Raycast from cursor and return the robot actor under it (if any).
     * @return  The AMXRobotActor under the cursor, or nullptr.
     */
    AMXRobotActor* GetRobotUnderCursor() const;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /** Get camera-relative forward/right vectors projected onto XY plane. */
    void GetPlanarDirections(FVector& OutForward, FVector& OutRight) const;

    /** Find the camera rig in the level. */
    void FindCameraRig();
};
