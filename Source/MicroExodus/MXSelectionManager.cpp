// MXSelectionManager.cpp — Phase 2C: Selection System
// Created: 2026-03-09

#include "MXSelectionManager.h"
#include "MXRobotActor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXSelectionManager::UMXSelectionManager()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// ---------------------------------------------------------------------------
// Selection Actions
// ---------------------------------------------------------------------------

AMXRobotActor* UMXSelectionManager::TrySelectAtCursor(bool bAdditive)
{
    PruneInvalid();

    AMXRobotActor* HitRobot = RaycastForRobot();

    if (HitRobot)
    {
        if (bAdditive)
        {
            // Toggle: if already selected, deselect; otherwise add.
            bool bAlreadySelected = false;
            for (const auto& Weak : SelectedRobots)
            {
                if (Weak.Get() == HitRobot)
                {
                    bAlreadySelected = true;
                    break;
                }
            }

            if (bAlreadySelected)
            {
                RemoveFromSelection(HitRobot);
            }
            else
            {
                AddToSelection(HitRobot);
            }
        }
        else
        {
            // Replace selection with this single robot.
            ClearSelection();
            AddToSelection(HitRobot);
        }
    }
    else if (!bAdditive)
    {
        // Clicked empty space without Shift — deselect all.
        ClearSelection();
    }

    BroadcastSelectionChanged();
    return HitRobot;
}

void UMXSelectionManager::BeginBoxSelect(FVector2D ScreenPosition)
{
    BoxStartScreen = ScreenPosition;
    BoxEndScreen = ScreenPosition;
    bIsBoxSelecting = true;
}

void UMXSelectionManager::UpdateBoxSelect(FVector2D ScreenPosition)
{
    if (!bIsBoxSelecting) return;
    BoxEndScreen = ScreenPosition;
}

void UMXSelectionManager::EndBoxSelect(bool bAdditive)
{
    if (!bIsBoxSelecting) return;
    bIsBoxSelecting = false;

    // Check if the drag distance was enough to count as box select.
    const float DragDist = FVector2D::Distance(BoxStartScreen, BoxEndScreen);
    if (DragDist < BoxSelectThreshold)
    {
        // Too small — treat as a click, not a drag.
        return;
    }

    PruneInvalid();

    // Compute screen-space rectangle (min/max).
    FVector2D RectMin(
        FMath::Min(BoxStartScreen.X, BoxEndScreen.X),
        FMath::Min(BoxStartScreen.Y, BoxEndScreen.Y));
    FVector2D RectMax(
        FMath::Max(BoxStartScreen.X, BoxEndScreen.X),
        FMath::Max(BoxStartScreen.Y, BoxEndScreen.Y));

    if (!bAdditive)
    {
        ClearSelection();
    }

    // Iterate all robot actors and check if they fall within the box.
    UWorld* World = GetWorld();
    if (!World) return;

    for (TActorIterator<AMXRobotActor> It(World); It; ++It)
    {
        AMXRobotActor* Robot = *It;
        if (Robot && IsActorInScreenRect(Robot, RectMin, RectMax))
        {
            AddToSelection(Robot);
        }
    }

    BroadcastSelectionChanged();
}

void UMXSelectionManager::ClearSelection()
{
    for (auto& Weak : SelectedRobots)
    {
        if (AMXRobotActor* Robot = Weak.Get())
        {
            Robot->SetSelected(false);
        }
    }
    SelectedRobots.Empty();
}

void UMXSelectionManager::SelectAll()
{
    ClearSelection();

    UWorld* World = GetWorld();
    if (!World) return;

    for (TActorIterator<AMXRobotActor> It(World); It; ++It)
    {
        AMXRobotActor* Robot = *It;
        if (Robot)
        {
            AddToSelection(Robot);
        }
    }

    BroadcastSelectionChanged();
}

// ---------------------------------------------------------------------------
// Control Groups
// ---------------------------------------------------------------------------

void UMXSelectionManager::SaveControlGroup(int32 GroupIndex)
{
    if (GroupIndex < 1 || GroupIndex > 9) return;

    PruneInvalid();
    ControlGroups.FindOrAdd(GroupIndex) = SelectedRobots;

    UE_LOG(LogTemp, Log, TEXT("MXSelectionManager: Saved control group %d (%d robots)."),
           GroupIndex, SelectedRobots.Num());
}

void UMXSelectionManager::RecallControlGroup(int32 GroupIndex)
{
    if (GroupIndex < 1 || GroupIndex > 9) return;

    TArray<TWeakObjectPtr<AMXRobotActor>>* Group = ControlGroups.Find(GroupIndex);
    if (!Group || Group->Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("MXSelectionManager: Control group %d is empty."), GroupIndex);
        return;
    }

    ClearSelection();

    for (auto& Weak : *Group)
    {
        if (AMXRobotActor* Robot = Weak.Get())
        {
            AddToSelection(Robot);
        }
    }

    BroadcastSelectionChanged();

    UE_LOG(LogTemp, Log, TEXT("MXSelectionManager: Recalled control group %d (%d robots)."),
           GroupIndex, SelectedRobots.Num());
}

// ---------------------------------------------------------------------------
// Hover
// ---------------------------------------------------------------------------

void UMXSelectionManager::UpdateHover()
{
    AMXRobotActor* NewHover = RaycastForRobot();

    // Clear old hover.
    if (AMXRobotActor* OldHover = HoveredRobot.Get())
    {
        if (OldHover != NewHover)
        {
            OldHover->SetHovered(false);
        }
    }

    // Set new hover.
    if (NewHover)
    {
        NewHover->SetHovered(true);
    }

    HoveredRobot = NewHover;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

TArray<AMXRobotActor*> UMXSelectionManager::GetSelectedRobots() const
{
    TArray<AMXRobotActor*> Result;
    for (const auto& Weak : SelectedRobots)
    {
        if (AMXRobotActor* Robot = Weak.Get())
        {
            Result.Add(Robot);
        }
    }
    return Result;
}

int32 UMXSelectionManager::GetSelectedCount() const
{
    int32 Count = 0;
    for (const auto& Weak : SelectedRobots)
    {
        if (Weak.IsValid()) ++Count;
    }
    return Count;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

APlayerController* UMXSelectionManager::GetPC() const
{
    return Cast<APlayerController>(GetOwner());
}

AMXRobotActor* UMXSelectionManager::RaycastForRobot() const
{
    APlayerController* PC = GetPC();
    if (!PC) return nullptr;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(SelectionTraceChannel, false, Hit);

    if (Hit.bBlockingHit && Hit.GetActor())
    {
        return Cast<AMXRobotActor>(Hit.GetActor());
    }

    return nullptr;
}

void UMXSelectionManager::AddToSelection(AMXRobotActor* Robot)
{
    if (!Robot) return;

    // Check if already selected.
    for (const auto& Weak : SelectedRobots)
    {
        if (Weak.Get() == Robot) return;
    }

    SelectedRobots.Add(Robot);
    Robot->SetSelected(true);
}

void UMXSelectionManager::RemoveFromSelection(AMXRobotActor* Robot)
{
    if (!Robot) return;

    SelectedRobots.RemoveAll([Robot](const TWeakObjectPtr<AMXRobotActor>& Weak)
    {
        return Weak.Get() == Robot;
    });

    Robot->SetSelected(false);
}

void UMXSelectionManager::BroadcastSelectionChanged()
{
    TArray<AMXRobotActor*> Current = GetSelectedRobots();
    OnSelectionChanged.Broadcast(Current);
}

void UMXSelectionManager::PruneInvalid()
{
    SelectedRobots.RemoveAll([](const TWeakObjectPtr<AMXRobotActor>& Weak)
    {
        return !Weak.IsValid();
    });
}

bool UMXSelectionManager::IsActorInScreenRect(AMXRobotActor* Robot, FVector2D RectMin, FVector2D RectMax) const
{
    APlayerController* PC = GetPC();
    if (!PC || !Robot) return false;

    FVector2D ScreenPos;
    if (PC->ProjectWorldLocationToScreen(Robot->GetActorLocation(), ScreenPos, true))
    {
        return (ScreenPos.X >= RectMin.X && ScreenPos.X <= RectMax.X &&
                ScreenPos.Y >= RectMin.Y && ScreenPos.Y <= RectMax.Y);
    }

    return false;
}
