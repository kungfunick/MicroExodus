// MXHatCollectionUI.h — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI
// The 100-hat collection showcase screen — 10×10 grid with detail panel,
// combo log, and paint job gallery.
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
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "MXHatData.h"
#include "MXTypes.h"
#include "MXHatCollectionUI.generated.h"

// ---------------------------------------------------------------------------
// UMXHatCollectionUI
// ---------------------------------------------------------------------------

/**
 * UMXHatCollectionUI
 * Full hat collection browser. Renders 100 cells (locked/unlocked/hidden)
 * in a grid, expands to a detail panel on tap, and shows combo/paint logs.
 *
 * Bound widgets:
 *   HatGrid           — WrapBox or UniformGridPanel with 100 cells
 *   ProgressText      — "{N}/100 hats discovered"
 *   ProgressBar       — Numeric unlock progress
 *   DetailPanel       — VerticalBox for selected hat details
 *   HatNameText       — Hat name in detail view
 *   HatRarityText     — Rarity label
 *   HatStackText      — "Stack: {N}"
 *   HatUnlockText     — Unlock condition hint
 *   ComboLogBox       — ScrollBox listing discovered combos
 *   PaintJobGallery   — HorizontalBox of paint job colour swatches
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXHatCollectionUI : public UUserWidget
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Population
    // -------------------------------------------------------------------------

    /**
     * Build all 100 hat cells from the master definition list and current collection.
     * Locks hidden hats, renders silhouettes for undiscovered hats.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|HatCollection")
    void PopulateCollection(const TArray<FMXHatDefinition>& AllHats,
                            const FMXHatCollection& Collection);

    /**
     * Refresh a single cell's stack count without rebuilding the entire grid.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|HatCollection")
    void UpdateStackCount(int32 HatId, int32 NewCount);

    // -------------------------------------------------------------------------
    // Detail Panel
    // -------------------------------------------------------------------------

    /**
     * Open the detail panel for a specific hat.
     * Shows full preview, unlock condition, stack count, and combo/level requirements.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|HatCollection")
    void ShowHatDetail(int32 HatId);

    /** Close the detail panel. */
    UFUNCTION(BlueprintCallable, Category = "MX|HatCollection")
    void CloseDetail();

    // -------------------------------------------------------------------------
    // Combo & Paint Logs
    // -------------------------------------------------------------------------

    /**
     * Rebuild the combo discovery log from an array of discovered combo hat IDs.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|HatCollection")
    void ShowComboLog(const TArray<int32>& DiscoveredComboIds);

    /**
     * Rebuild the paint job gallery from unlocked paint job entries.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|HatCollection")
    void ShowPaintJobGallery(const TArray<EPaintJob>& UnlockedPaintJobs);

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    /** Returns the progress string: "{N}/100 hats discovered". */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|HatCollection")
    FString GetProgressText() const;

    // -------------------------------------------------------------------------
    // BP Events
    // -------------------------------------------------------------------------

    /** Called when the detail panel opens — animate in BP. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|HatCollection")
    void OnDetailPanelOpened(int32 HatId);

    /** Called when a cell is built — attach 3D hat mesh preview from BP. */
    UFUNCTION(BlueprintImplementableEvent, Category = "MX|HatCollection")
    void OnHatCellBuilt(UWidget* CellWidget, int32 HatId, bool bUnlocked);

protected:

    virtual void NativeConstruct() override;

    // -------------------------------------------------------------------------
    // Bound Widgets
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UWrapBox> HatGrid;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> ProgressText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UProgressBar> CollectionProgressBar;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UVerticalBox> DetailPanel;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> HatNameText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> HatRarityText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> HatStackText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UTextBlock> HatUnlockText;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UScrollBox> ComboLogBox;

    UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
    TObjectPtr<UHorizontalBox> PaintJobGallery;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /** All hat definitions from the last PopulateCollection call. */
    UPROPERTY()
    TArray<FMXHatDefinition> CachedDefinitions;

    /** Collection snapshot from last PopulateCollection. */
    FMXHatCollection CachedCollection;

    /** Currently selected hat ID (-1 = none). */
    int32 SelectedHatId = -1;

    /** Total unlocked count (excluding hidden undiscovered). */
    int32 UnlockedCount = 0;

private:

    /** Build one hat cell UWidget. */
    UWidget* BuildHatCell(const FMXHatDefinition& Def, const FMXHatCollection& Collection);

    /** Get stack count from collection for a hat ID. */
    int32 GetStackCount(int32 HatId, const FMXHatCollection& Collection) const;

    /** True if the hat has been unlocked (exists in unlocked_hat_ids). */
    bool IsUnlocked(int32 HatId, const FMXHatCollection& Collection) const;

    /** Colour tint per rarity for cell borders. */
    FLinearColor GetRarityColor(EHatRarity Rarity) const;

    /** String label for a paint job enum. */
    FString GetPaintJobLabel(EPaintJob PaintJob) const;
};
