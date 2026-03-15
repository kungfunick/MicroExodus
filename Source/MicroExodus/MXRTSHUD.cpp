// MXRTSHUD.cpp — Phase 2C: Selection + Control Group HUD
// Created: 2026-03-10

#include "MXRTSHUD.h"
#include "MXRTSPlayerController.h"
#include "MXSelectionManager.h"
#include "Engine/Canvas.h"

// ---------------------------------------------------------------------------
// DrawHUD — called every frame by the engine
// ---------------------------------------------------------------------------

void AMXRTSHUD::DrawHUD()
{
    Super::DrawHUD();

    DrawBoxSelectRect();
    DrawControlGroups();
}

// ---------------------------------------------------------------------------
// Box Select Rectangle
// ---------------------------------------------------------------------------

void AMXRTSHUD::DrawBoxSelectRect()
{
    AMXRTSPlayerController* PC = Cast<AMXRTSPlayerController>(GetOwningPlayerController());
    if (!PC) return;

    UMXSelectionManager* SM = PC->GetSelectionManager();
    if (!SM || !SM->IsBoxSelecting()) return;

    FVector2D Start = SM->GetBoxStart();
    FVector2D End = SM->GetBoxEnd();

    float X = FMath::Min(Start.X, End.X);
    float Y = FMath::Min(Start.Y, End.Y);
    float W = FMath::Abs(End.X - Start.X);
    float H = FMath::Abs(End.Y - Start.Y);

    // Skip if too small (just a click, not a drag).
    if (W < 3.0f && H < 3.0f) return;

    // Filled rectangle.
    DrawRect(BoxFillColor, X, Y, W, H);

    // Border lines (2px thick).
    float T = 2.0f;
    DrawRect(BoxBorderColor, X,         Y,         W, T);         // Top
    DrawRect(BoxBorderColor, X,         Y + H - T, W, T);         // Bottom
    DrawRect(BoxBorderColor, X,         Y,         T, H);         // Left
    DrawRect(BoxBorderColor, X + W - T, Y,         T, H);         // Right
}

// ---------------------------------------------------------------------------
// Control Group Indicators
// ---------------------------------------------------------------------------

void AMXRTSHUD::DrawControlGroups()
{
    AMXRTSPlayerController* PC = Cast<AMXRTSPlayerController>(GetOwningPlayerController());
    if (!PC) return;

    UMXSelectionManager* SM = PC->GetSelectionManager();
    if (!SM) return;

    float DrawX = 20.0f;
    float DrawY = 20.0f;

    // Show current selection count.
    int32 SelCount = SM->GetSelectedCount();
    if (SelCount > 0)
    {
        FString SelText = FString::Printf(TEXT("Selected: %d"), SelCount);
        DrawText(SelText, GroupTextColor, DrawX, DrawY, nullptr, GroupFontScale, false);
        DrawY += 30.0f;
    }

    // Show assigned control groups: [1] (3)  [2] (5) etc.
    FString GroupLine;
    for (int32 i = 1; i <= 9; ++i)
    {
        int32 Count = SM->GetControlGroupCount(i);
        if (Count > 0)
        {
            GroupLine += FString::Printf(TEXT("[%d] (%d)  "), i, Count);
        }
    }

    if (!GroupLine.IsEmpty())
    {
        DrawText(GroupLine, GroupTextColor, DrawX, DrawY, nullptr, GroupFontScale, false);
        DrawY += 30.0f;
    }

    // Hint text at bottom.
    if (Canvas)
    {
        FString HintText = TEXT("Shift+1-9: Save group | 1-9: Recall group");
        DrawText(HintText, FLinearColor(0.6f, 0.6f, 0.6f, 0.5f), DrawX, Canvas->SizeY - 40.0f,
                 nullptr, 1.0f, false);
    }
}
