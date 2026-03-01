// MXMemorialWallUI.h — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI
// Permanent tribute wall for every robot lost across all runs.
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "MXRobotProfile.h"
#include "MXEventData.h"
#include "MXTypes.h"
#include "MXMemorialSerializer.h"
#include "MXMemorialWallUI.generated.h"

// FMXMemorialEntry is defined in MXMemorialSerializer.h

// ---------------------------------------------------------------------------
// Sort mode
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EMemorialSortMode : uint8
{
    DateOfDeath   UMETA(DisplayName = "Date of Death"),
    Level         UMETA(DisplayName = "Level"),
    RunsSurvived  UMETA(DisplayName = "Runs Survived"),
    Alphabetical  UMETA(DisplayName = "Alphabetical")
};

// ---------------------------------------------------------------------------
// UMXMemorialWallUI
// ---------------------------------------------------------------------------

/**
 * UMXMemorialWallUI
 * Scrollable wall of memorial tiles, one per fallen robot.
 * Sacrifice entries receive gold border treatment.
 * Supports sort, search, and expanded detail view.
 *
 * Bound widgets:
 *   MemorialTileBox     — WrapBox holding all tiles
 *   DetailPanel         — VerticalBox for selected entry
 *   MemorialNameText    — Name in detail view
 *   MemorialEulogyText  — Eulogy in detail view
 *   MemorialStatsText   — Lifetime stats in detail view
 *   TotalLostText       — "Robots Lost: {N}"
 *   SortDropdown        — ComboBoxString
 *   SearchBox           — EditableTextBox
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXMemorialWallUI : public UUserWidget
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Population
    // -------------------------------------------------------------------------

    /**
     * Build all memorial tiles from the persistence entry list.
     * Applies current sort and search after loading.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Memorial")
    void PopulateWall(const TArray<FMXMemorialEntry>& Entries);

    /**
     * Open the full memorial detail view for a specific robot.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Memorial")
    void ShowMemorialDetail(const FGuid& RobotId);

    /** Close the detail panel. */
    UFUNCTION(BlueprintCallable, Category = "MX|Memorial")
    void CloseDetail();

    /** Re-sort the displayed tiles. */
    UFUNCTION(BlueprintCallable, Category = "MX|Memorial")
    void SortMemorial(EMemorialSortMode SortMode);

    /** Filter tiles by name search. Pass empty string to clear. */
    UFUNCTION(BlueprintCallable, Category = "MX|Memorial")
    void SearchByName(const FString& SearchText);

    /** Returns total robots ever lost. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Memorial")
    int32 GetTotalLost() const { return AllEntries.Num(); }

    // -------------------------------------------------------------------------
    // BP Events
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Memorial")
    void OnDetailPanelOpened(const FMXMemorialEntry& Entry);

    /** Called when a tile is built — attach evolution portrait from BP. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Memorial")
    void OnMemorialTileBuilt(UWidget* TileWidget, const FMXMemorialEntry& Entry);

protected:

    virtual void NativeConstruct() override;

    // -------------------------------------------------------------------------
    // Bound Widgets
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UWrapBox> MemorialTileBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> DetailPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> MemorialNameText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> MemorialEulogyText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> MemorialStatsText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> TotalLostText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UComboBoxString> SortDropdown;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UEditableTextBox> SearchBox;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    UPROPERTY()
    TArray<FMXMemorialEntry> AllEntries;

    UPROPERTY()
    TArray<FMXMemorialEntry> DisplayedEntries;

    EMemorialSortMode CurrentSort = EMemorialSortMode::DateOfDeath;
    FString           CurrentSearch;
    FGuid             SelectedRobotId;

private:

    void ApplyFilterAndSort();
    void RebuildWall();
    UWidget* BuildMemorialTile(const FMXMemorialEntry& Entry);
    FString  BuildStatsString(const FMXRobotProfile& Profile) const;

    UFUNCTION()
    void OnSortDropdownChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnSearchTextChanged(const FText& Text);
};
