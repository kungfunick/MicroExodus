// MXRTSHUD.h — Phase 2C: Selection + Control Group HUD
// Created: 2026-03-10
// Draws box select rectangle and control group indicators.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MXRTSHUD.generated.h"

class AMXRTSPlayerController;
class UMXSelectionManager;

UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXRTSHUD : public AHUD
{
    GENERATED_BODY()

public:

    virtual void DrawHUD() override;

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    /** Fill color for box select rectangle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|BoxSelect")
    FLinearColor BoxFillColor = FLinearColor(0.0f, 0.85f, 0.25f, 0.15f);

    /** Border color for box select rectangle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|BoxSelect")
    FLinearColor BoxBorderColor = FLinearColor(0.0f, 0.85f, 0.25f, 0.8f);

    /** Color for control group text. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|ControlGroups")
    FLinearColor GroupTextColor = FLinearColor(0.0f, 0.85f, 0.25f, 1.0f);

    /** Font scale for control group indicators. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|HUD|ControlGroups")
    float GroupFontScale = 1.5f;

private:

    void DrawBoxSelectRect();
    void DrawControlGroups();
};
