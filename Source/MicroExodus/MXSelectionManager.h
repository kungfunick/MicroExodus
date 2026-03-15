// MXSelectionManager.h — Phase 2C: Selection System
// Created: 2026-03-09
// Handles single-click, box-select, shift-multi-select, and Ctrl+number control groups.
// Attached as ActorComponent to AMXRTSPlayerController.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MXSelectionManager.generated.h"

// Forward declarations
class AMXRobotActor;

// ---------------------------------------------------------------------------
// Delegates
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, const TArray<AMXRobotActor*>&, SelectedRobots);

// ---------------------------------------------------------------------------
// UMXSelectionManager
// ---------------------------------------------------------------------------

/**
 * UMXSelectionManager
 * Manages robot selection state for the RTS camera controller.
 *
 * Features:
 *   - Left-click: select single robot (raycast)
 *   - Left-click drag: box select (screen-space rectangle)
 *   - Shift+click: add/remove from selection
 *   - Ctrl+1-9: save control group
 *   - 1-9: recall control group
 *   - Double-click: select all robots (future: all of same type)
 *
 * Selection state is stored as weak pointers to AMXRobotActor.
 * Each selected robot is notified via SetSelected(true/false).
 */
UCLASS(ClassGroup=(MicroExodus), BlueprintType, Blueprintable,
       meta=(BlueprintSpawnableComponent))
class MICROEXODUS_API UMXSelectionManager : public UActorComponent
{
    GENERATED_BODY()

public:

    UMXSelectionManager();

    // -------------------------------------------------------------------------
    // Selection Actions
    // -------------------------------------------------------------------------

    /**
     * Attempt to select a robot under the mouse cursor via line trace.
     * @param bAdditive  If true (Shift held), add to existing selection instead of replacing.
     * @return           The robot that was clicked, or nullptr if ground/empty.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    AMXRobotActor* TrySelectAtCursor(bool bAdditive);

    /**
     * Begin a box selection drag. Call when left mouse is pressed.
     * @param ScreenPosition  Mouse position in screen space at press time.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void BeginBoxSelect(FVector2D ScreenPosition);

    /**
     * Update the box selection rectangle. Call each tick while dragging.
     * @param ScreenPosition  Current mouse position in screen space.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void UpdateBoxSelect(FVector2D ScreenPosition);

    /**
     * Finalize box selection. Selects all robots within the screen-space rectangle.
     * @param bAdditive  If true, add to existing selection.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void EndBoxSelect(bool bAdditive);

    /**
     * Clear all selected robots.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void ClearSelection();

    /**
     * Select all robots in the level.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void SelectAll();

    // -------------------------------------------------------------------------
    // Control Groups
    // -------------------------------------------------------------------------

    /**
     * Save current selection to a control group (1-9).
     * @param GroupIndex  Group number (1-9).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void SaveControlGroup(int32 GroupIndex);

    /**
     * Recall a control group (1-9), replacing current selection.
     * @param GroupIndex  Group number (1-9).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void RecallControlGroup(int32 GroupIndex);

    // -------------------------------------------------------------------------
    // Hover
    // -------------------------------------------------------------------------

    /**
     * Update hover state — call each tick to raycast under cursor and set hover.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Selection")
    void UpdateHover();

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Selection")
    TArray<AMXRobotActor*> GetSelectedRobots() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Selection")
    int32 GetSelectedCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Selection")
    bool IsBoxSelecting() const { return bIsBoxSelecting; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Selection")
    FVector2D GetBoxStart() const { return BoxStartScreen; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Selection")
    FVector2D GetBoxEnd() const { return BoxEndScreen; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Selection")
    bool HasSelection() const { return SelectedRobots.Num() > 0; }

    /** Get the number of valid robots in a control group (1-9). Returns 0 if unassigned. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Selection")
    int32 GetControlGroupCount(int32 GroupIndex) const;

    // -------------------------------------------------------------------------
    // Delegates
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "MX|Selection")
    FOnSelectionChanged OnSelectionChanged;

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    /** Minimum drag distance (pixels) before a click becomes a box select. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Selection|Config")
    float BoxSelectThreshold = 5.0f;

    /** Trace channel for robot selection raycasts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Selection|Config")
    TEnumAsByte<ECollisionChannel> SelectionTraceChannel = ECC_Pawn;

private:

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    UPROPERTY()
    TArray<TWeakObjectPtr<AMXRobotActor>> SelectedRobots;

    /** Control groups: index 1-9 → array of robot weak ptrs. */
    TMap<int32, TArray<TWeakObjectPtr<AMXRobotActor>>> ControlGroups;

    /** Currently hovered robot (for name display). */
    UPROPERTY()
    TWeakObjectPtr<AMXRobotActor> HoveredRobot;

    /** Box select state. */
    bool bIsBoxSelecting = false;
    FVector2D BoxStartScreen = FVector2D::ZeroVector;
    FVector2D BoxEndScreen = FVector2D::ZeroVector;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /** Get the owning PlayerController. */
    APlayerController* GetPC() const;

    /** Line trace from cursor, return hit robot (or nullptr). */
    AMXRobotActor* RaycastForRobot() const;

    /** Add a robot to selection (notifies the actor). */
    void AddToSelection(AMXRobotActor* Robot);

    /** Remove a robot from selection (notifies the actor). */
    void RemoveFromSelection(AMXRobotActor* Robot);

    /** Notify all robots of their selection state and broadcast delegate. */
    void BroadcastSelectionChanged();

    /** Prune any invalid (destroyed) weak pointers from selection. */
    void PruneInvalid();

    /** Check if a robot actor's screen-space bounds overlap a rectangle. */
    bool IsActorInScreenRect(AMXRobotActor* Robot, FVector2D RectMin, FVector2D RectMax) const;
};
