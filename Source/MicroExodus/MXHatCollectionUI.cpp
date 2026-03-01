// MXHatCollectionUI.cpp — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI

#include "MXHatCollectionUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/WrapBox.h"

// ---------------------------------------------------------------------------
// NativeConstruct
// ---------------------------------------------------------------------------

void UMXHatCollectionUI::NativeConstruct()
{
    Super::NativeConstruct();

    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// ---------------------------------------------------------------------------
// PopulateCollection
// ---------------------------------------------------------------------------

void UMXHatCollectionUI::PopulateCollection(const TArray<FMXHatDefinition>& AllHats,
                                             const FMXHatCollection& Collection)
{
    CachedDefinitions = AllHats;
    CachedCollection  = Collection;

    // Count unlocked
    UnlockedCount = Collection.unlocked_hat_ids.Num();

    if (!HatGrid) return;
    HatGrid->ClearChildren();

    for (const FMXHatDefinition& Def : AllHats)
    {
        UWidget* Cell = BuildHatCell(Def, Collection);
        if (Cell)
        {
            HatGrid->AddChild(Cell);
        }
    }

    // Update progress
    if (ProgressText)
    {
        ProgressText->SetText(FText::FromString(GetProgressText()));
    }

    if (CollectionProgressBar)
    {
        CollectionProgressBar->SetPercent(
            AllHats.Num() > 0 ? (float)UnlockedCount / (float)AllHats.Num() : 0.0f);
    }
}

void UMXHatCollectionUI::UpdateStackCount(int32 HatId, int32 NewCount)
{
    // Update the cached collection stack and refresh just the count text in detail
    for (FMXHatStack& Stack : CachedCollection.stacks)
    {
        if (Stack.hat_id == HatId)
        {
            Stack.count = NewCount;
            break;
        }
    }

    // If this hat is currently in the detail panel, refresh the stack text
    if (SelectedHatId == HatId && HatStackText)
    {
        HatStackText->SetText(FText::FromString(
            FString::Printf(TEXT("Stack: %d"), NewCount)));
    }
}

// ---------------------------------------------------------------------------
// Detail Panel
// ---------------------------------------------------------------------------

void UMXHatCollectionUI::ShowHatDetail(int32 HatId)
{
    // Find definition
    const FMXHatDefinition* Def = nullptr;
    for (const FMXHatDefinition& D : CachedDefinitions)
    {
        if (D.hat_id == HatId)
        {
            Def = &D;
            break;
        }
    }

    if (!Def) return;

    SelectedHatId = HatId;
    const bool bUnlocked = IsUnlocked(HatId, CachedCollection);

    if (HatNameText)
    {
        // Hidden hats show ??? until discovered
        HatNameText->SetText(FText::FromString(
            (!bUnlocked && Def->is_hidden) ? TEXT("???") : Def->name));
    }

    if (HatRarityText)
    {
        const UEnum* RarityEnum = StaticEnum<EHatRarity>();
        const FString RarityStr = RarityEnum
            ? RarityEnum->GetNameStringByValue((int64)Def->rarity)
            : TEXT("Unknown");
        HatRarityText->SetText(FText::FromString(RarityStr));
        HatRarityText->SetColorAndOpacity(FSlateColor(GetRarityColor(Def->rarity)));
    }

    if (HatStackText)
    {
        const int32 Count = GetStackCount(HatId, CachedCollection);
        HatStackText->SetText(FText::FromString(
            FString::Printf(TEXT("Stack: %d"), Count)));
    }

    if (HatUnlockText)
    {
        FString HintStr;
        if (!bUnlocked && Def->is_hidden)
        {
            HintStr = TEXT("???");
        }
        else if (Def->is_combo)
        {
            HintStr = TEXT("Requires a specific combination of hats...");
        }
        else
        {
            HintStr = Def->unlock_condition.IsEmpty()
                ? TEXT("Available from the start")
                : Def->unlock_condition;
        }
        HatUnlockText->SetText(FText::FromString(HintStr));
    }

    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    OnDetailPanelOpened(HatId);
}

void UMXHatCollectionUI::CloseDetail()
{
    SelectedHatId = -1;
    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// ---------------------------------------------------------------------------
// Combo & Paint Logs
// ---------------------------------------------------------------------------

void UMXHatCollectionUI::ShowComboLog(const TArray<int32>& DiscoveredComboIds)
{
    if (!ComboLogBox || !WidgetTree) return;

    ComboLogBox->ClearChildren();

    if (DiscoveredComboIds.IsEmpty())
    {
        UTextBlock* NoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (NoneText)
        {
            NoneText->SetText(FText::FromString(TEXT("No combos discovered yet.")));
            ComboLogBox->AddChild(NoneText);
        }
        return;
    }

    for (int32 ComboId : DiscoveredComboIds)
    {
        // Find hat name
        FString ComboName = FString::Printf(TEXT("Combo Hat #%d"), ComboId);
        for (const FMXHatDefinition& Def : CachedDefinitions)
        {
            if (Def.hat_id == ComboId)
            {
                ComboName = Def.name;
                break;
            }
        }

        UTextBlock* ComboEntry = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (ComboEntry)
        {
            ComboEntry->SetText(FText::FromString(FString::Printf(TEXT("• %s"), *ComboName)));
            ComboLogBox->AddChild(ComboEntry);
        }
    }
}

void UMXHatCollectionUI::ShowPaintJobGallery(const TArray<EPaintJob>& UnlockedPaintJobs)
{
    if (!PaintJobGallery || !WidgetTree) return;

    PaintJobGallery->ClearChildren();

    for (EPaintJob PaintJob : UnlockedPaintJobs)
    {
        UBorder* Swatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        if (!Swatch) continue;

        // Placeholder colours — real paint job swatches assigned in BP from static mesh materials
        Swatch->SetBrushColor(FLinearColor(0.3f, 0.3f, 0.35f, 1.0f));

        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (Label)
        {
            Label->SetText(FText::FromString(GetPaintJobLabel(PaintJob)));
            Swatch->SetContent(Label);
        }

        PaintJobGallery->AddChild(Swatch);
    }
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

FString UMXHatCollectionUI::GetProgressText() const
{
    return FString::Printf(TEXT("%d / 100 hats discovered"), UnlockedCount);
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

UWidget* UMXHatCollectionUI::BuildHatCell(const FMXHatDefinition& Def,
                                           const FMXHatCollection& Collection)
{
    if (!WidgetTree) return nullptr;

    UBorder* Cell = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (!Cell) return nullptr;

    const bool bUnlocked  = IsUnlocked(Def.hat_id, Collection);
    const bool bHidden    = Def.is_hidden && !bUnlocked;
    const int32 StackCount = GetStackCount(Def.hat_id, Collection);

    // Border colour: rarity tint if unlocked, dark silhouette if locked
    Cell->SetBrushColor(bUnlocked
        ? GetRarityColor(Def.rarity)
        : FLinearColor(0.08f, 0.08f, 0.1f, 0.9f));

    UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (Inner)
    {
        // Hat name (or silhouette)
        UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (NameText)
        {
            NameText->SetText(FText::FromString(bHidden ? TEXT("???") : Def.name));
            Inner->AddChild(NameText);
        }

        // Stack count (only if unlocked)
        if (bUnlocked)
        {
            UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            if (CountText)
            {
                CountText->SetText(FText::FromString(
                    FString::Printf(TEXT("×%d"), StackCount)));
                Inner->AddChild(CountText);
            }
        }

        Cell->SetContent(Inner);
    }

    // Notify BP to attach 3D mesh preview
    OnHatCellBuilt(Cell, Def.hat_id, bUnlocked);

    return Cell;
}

int32 UMXHatCollectionUI::GetStackCount(int32 HatId, const FMXHatCollection& Collection) const
{
    for (const FMXHatStack& Stack : Collection.stacks)
    {
        if (Stack.hat_id == HatId)
        {
            return Stack.count;
        }
    }
    return 0;
}

bool UMXHatCollectionUI::IsUnlocked(int32 HatId, const FMXHatCollection& Collection) const
{
    return Collection.unlocked_hat_ids.Contains(HatId);
}

FLinearColor UMXHatCollectionUI::GetRarityColor(EHatRarity Rarity) const
{
    switch (Rarity)
    {
    case EHatRarity::Common:    return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
    case EHatRarity::Uncommon:  return FLinearColor(0.1f, 0.7f, 0.2f, 1.0f);
    case EHatRarity::Rare:      return FLinearColor(0.1f, 0.3f, 0.9f, 1.0f);
    case EHatRarity::Epic:      return FLinearColor(0.5f, 0.1f, 0.9f, 1.0f);
    case EHatRarity::Legendary: return FLinearColor(1.0f, 0.6f, 0.0f, 1.0f);
    case EHatRarity::Mythic:    return FLinearColor(0.9f, 0.1f, 0.5f, 1.0f);
    default:                    return FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
    }
}

FString UMXHatCollectionUI::GetPaintJobLabel(EPaintJob PaintJob) const
{
    const UEnum* Enum = StaticEnum<EPaintJob>();
    return Enum ? Enum->GetNameStringByValue((int64)PaintJob) : TEXT("Unknown");
}
