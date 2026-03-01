// MXRosterScreen.h — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI
// Full robot roster inspection and management screen.
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
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/WidgetSwitcher.h"
#include "Components/EditableTextBox.h"
#include "Components/Border.h"
#include "MXRobotProfile.h"
#include "MXEventData.h"
#include "MXTypes.h"
#include "MXRosterScreen.generated.h"

// ---------------------------------------------------------------------------
// Local enums
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ERosterSortMode : uint8
{
    Name        UMETA(DisplayName = "Name"),
    Level       UMETA(DisplayName = "Level"),
    XP          UMETA(DisplayName = "XP"),
    Age         UMETA(DisplayName = "Age"),
    Runs        UMETA(DisplayName = "Runs Survived"),
    Spec        UMETA(DisplayName = "Spec"),
    Hat         UMETA(DisplayName = "Hat")
};

UENUM(BlueprintType)
enum class ERosterFilterMode : uint8
{
    All         UMETA(DisplayName = "All"),
    Scout       UMETA(DisplayName = "Scout"),
    Guardian    UMETA(DisplayName = "Guardian"),
    Engineer    UMETA(DisplayName = "Engineer"),
    NoSpec      UMETA(DisplayName = "No Spec"),
    Hatted      UMETA(DisplayName = "Hatted"),
    Unhatted    UMETA(DisplayName = "No Hat")
};

// ---------------------------------------------------------------------------
// UMXRosterScreen
// ---------------------------------------------------------------------------

/**
 * UMXRosterScreen
 * Roster grid/list with per-robot detail panel.
 * Supports virtualized scrolling for up to 100 robots without frame drops.
 *
 * Bound widgets:
 *   RobotListBox       — ScrollBox containing robot card rows (virtualised)
 *   DetailPanel        — VerticalBox shown when a robot is selected
 *   RobotNameText      — Selected robot's name in detail view
 *   RobotLevelText     — "Level {N}" in detail view
 *   RobotSpecText      — Spec path string in detail view
 *   RobotPersonalityText — Combined personality description
 *   LifeLogScrollBox   — Scrollable life log event list
 *   SpecPathContainer  — VerticalBox visualising the spec tree path
 *   SortModeDropdown   — ComboBoxString for sort selection
 *   FilterModeDropdown — ComboBoxString for filter selection
 *   SearchBox          — EditableTextBox for name search
 *   RobotCountText     — "Showing {N} / {Total}"
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXRosterScreen : public UUserWidget
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Population & Refresh
    // -------------------------------------------------------------------------

    /**
     * Populate the roster from a full profile array.
     * Applies current sort and filter after loading.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void PopulateRoster(const TArray<FMXRobotProfile>& Robots);

    /**
     * Sort the currently displayed roster.
     * Does not re-fetch data — sorts the cached display list.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void SortRoster(ERosterSortMode SortMode);

    /**
     * Filter the displayed roster by role / hat state.
     * Does not re-fetch data — filters from the master cached list.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void FilterRoster(ERosterFilterMode FilterMode);

    /**
     * Text-based name search filter. Pass empty string to clear.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void SearchByName(const FString& SearchText);

    // -------------------------------------------------------------------------
    // Detail Panel
    // -------------------------------------------------------------------------

    /**
     * Open the detail panel for the specified robot.
     * Populates all detail sub-sections from the cached profile.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void ShowRobotDetail(const FGuid& RobotId);

    /** Close the detail panel and return to the grid. */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void CloseDetail();

    /** Populate the scrollable life log. */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void ShowLifeLog(const FGuid& RobotId, const TArray<FMXEventData>& Events);

    /** Visualise the robot's spec path in the tree container. */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void ShowSpecPath(const FMXRobotProfile& Robot);

    /** Display hat history strip (all previously worn hat IDs). */
    UFUNCTION(BlueprintCallable, Category = "MX|Roster")
    void ShowHatHistory(const TArray<int32>& HatIds);

    // -------------------------------------------------------------------------
    // BP Events — implement visual polish in Widget Blueprint
    // -------------------------------------------------------------------------

    /** Called when the detail panel opens for animation. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Roster")
    void OnDetailPanelOpen(const FMXRobotProfile& Robot);

    /** Called when a robot card is built — apply 3D portrait if available. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|Roster")
    void OnRobotCardBuilt(UWidget* CardWidget, const FMXRobotProfile& Robot);

protected:

    virtual void NativeConstruct() override;

    // -------------------------------------------------------------------------
    // Bound Widgets
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UScrollBox> RobotListBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> DetailPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> RobotNameText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> RobotLevelText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> RobotSpecText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> RobotPersonalityText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UScrollBox> LifeLogScrollBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> SpecPathContainer;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UScrollBox> HatHistoryScrollBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UComboBoxString> SortModeDropdown;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UComboBoxString> FilterModeDropdown;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UEditableTextBox> SearchBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> RobotCountText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> TitlesText;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /** Master list loaded via PopulateRoster — never filtered in place. */
    UPROPERTY()
    TArray<FMXRobotProfile> AllRobots;

    /** Currently displayed subset after sort/filter/search. */
    UPROPERTY()
    TArray<FMXRobotProfile> DisplayedRobots;

    ERosterSortMode   CurrentSort   = ERosterSortMode::Name;
    ERosterFilterMode CurrentFilter = ERosterFilterMode::All;
    FString           CurrentSearch;

    FGuid SelectedRobotId;

private:

    /** Rebuild RobotListBox from DisplayedRobots. */
    void RebuildRobotList();

    /** Apply CurrentFilter and CurrentSearch to AllRobots → DisplayedRobots, then sort. */
    void ApplyFilterAndSort();

    /** Build one robot card UWidget for the list. */
    UWidget* BuildRobotCard(const FMXRobotProfile& Robot);

    /** Resolve a spec path display string from a robot profile. */
    FString ResolveSpecString(const FMXRobotProfile& Robot) const;

    /** Callback: sort dropdown selection changed. */
    UFUNCTION()
    void OnSortDropdownChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    /** Callback: filter dropdown selection changed. */
    UFUNCTION()
    void OnFilterDropdownChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    /** Callback: search text changed. */
    UFUNCTION()
    void OnSearchTextChanged(const FText& Text);
};
