// MXMemorialWallUI.cpp — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI

#include "MXMemorialWallUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"

// ---------------------------------------------------------------------------
// NativeConstruct
// ---------------------------------------------------------------------------

void UMXMemorialWallUI::NativeConstruct()
{
    Super::NativeConstruct();

    if (SortDropdown)
    {
        SortDropdown->ClearOptions();
        SortDropdown->AddOption(TEXT("Date of Death"));
        SortDropdown->AddOption(TEXT("Level"));
        SortDropdown->AddOption(TEXT("Runs Survived"));
        SortDropdown->AddOption(TEXT("Alphabetical"));
        SortDropdown->OnSelectionChanged.AddDynamic(this, &UMXMemorialWallUI::OnSortDropdownChanged);
    }

    if (SearchBox)
    {
        SearchBox->OnTextChanged.AddDynamic(this, &UMXMemorialWallUI::OnSearchTextChanged);
    }

    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// ---------------------------------------------------------------------------
// PopulateWall
// ---------------------------------------------------------------------------

void UMXMemorialWallUI::PopulateWall(const TArray<FMXMemorialEntry>& Entries)
{
    AllEntries = Entries;
    ApplyFilterAndSort();
    RebuildWall();

    if (TotalLostText)
    {
        TotalLostText->SetText(FText::FromString(
            FString::Printf(TEXT("Robots Lost: %d"), AllEntries.Num())));
    }
}

// ---------------------------------------------------------------------------
// ShowMemorialDetail
// ---------------------------------------------------------------------------

void UMXMemorialWallUI::ShowMemorialDetail(const FGuid& RobotId)
{
    const FMXMemorialEntry* Found = nullptr;
    for (const FMXMemorialEntry& E : AllEntries)
    {
        if (E.ProfileSnapshot.robot_id == RobotId)
        {
            Found = &E;
            break;
        }
    }

    if (!Found) return;

    SelectedRobotId = RobotId;

    if (MemorialNameText)
    {
        MemorialNameText->SetText(FText::FromString(
            FString::Printf(TEXT("%s  Lv.%d"),
                *Found->ProfileSnapshot.name,
                Found->ProfileSnapshot.level)));

        // Gold tint for sacrifices
        MemorialNameText->SetColorAndOpacity(FSlateColor(Found->bWasSacrifice
            ? FLinearColor(1.0f, 0.8f, 0.1f, 1.0f)
            : FLinearColor::White));
    }

    if (MemorialEulogyText)
    {
        MemorialEulogyText->SetText(FText::FromString(Found->Eulogy));
    }

    if (MemorialStatsText)
    {
        MemorialStatsText->SetText(FText::FromString(BuildStatsString(Found->ProfileSnapshot)));
    }

    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    OnDetailPanelOpened(*Found);
}

void UMXMemorialWallUI::CloseDetail()
{
    SelectedRobotId.Invalidate();
    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// ---------------------------------------------------------------------------
// Sort / Search
// ---------------------------------------------------------------------------

void UMXMemorialWallUI::SortMemorial(EMemorialSortMode SortMode)
{
    CurrentSort = SortMode;
    ApplyFilterAndSort();
    RebuildWall();
}

void UMXMemorialWallUI::SearchByName(const FString& SearchText)
{
    CurrentSearch = SearchText;
    ApplyFilterAndSort();
    RebuildWall();
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void UMXMemorialWallUI::ApplyFilterAndSort()
{
    DisplayedEntries.Empty();

    for (const FMXMemorialEntry& Entry : AllEntries)
    {
        if (!CurrentSearch.IsEmpty())
        {
            if (!Entry.ProfileSnapshot.name.Contains(CurrentSearch, ESearchCase::IgnoreCase))
                continue;
        }
        DisplayedEntries.Add(Entry);
    }

    switch (CurrentSort)
    {
    case EMemorialSortMode::DateOfDeath:
        DisplayedEntries.Sort([](const FMXMemorialEntry& A, const FMXMemorialEntry& B)
        {
            return A.DeathEvent.timestamp > B.DeathEvent.timestamp;
        });
        break;

    case EMemorialSortMode::Level:
        DisplayedEntries.Sort([](const FMXMemorialEntry& A, const FMXMemorialEntry& B)
        {
            return A.ProfileSnapshot.level > B.ProfileSnapshot.level;
        });
        break;

    case EMemorialSortMode::RunsSurvived:
        DisplayedEntries.Sort([](const FMXMemorialEntry& A, const FMXMemorialEntry& B)
        {
            return A.ProfileSnapshot.runs_survived > B.ProfileSnapshot.runs_survived;
        });
        break;

    case EMemorialSortMode::Alphabetical:
        DisplayedEntries.Sort([](const FMXMemorialEntry& A, const FMXMemorialEntry& B)
        {
            return A.ProfileSnapshot.name < B.ProfileSnapshot.name;
        });
        break;

    default: break;
    }
}

void UMXMemorialWallUI::RebuildWall()
{
    if (!MemorialTileBox) return;

    MemorialTileBox->ClearChildren();

    for (const FMXMemorialEntry& Entry : DisplayedEntries)
    {
        UWidget* Tile = BuildMemorialTile(Entry);
        if (Tile)
        {
            MemorialTileBox->AddChild(Tile);
        }
    }
}

UWidget* UMXMemorialWallUI::BuildMemorialTile(const FMXMemorialEntry& Entry)
{
    if (!WidgetTree) return nullptr;

    UBorder* Tile = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (!Tile) return nullptr;

    // Gold for sacrifice, dark for standard loss
    Tile->SetBrushColor(Entry.bWasSacrifice
        ? FLinearColor(0.2f, 0.14f, 0.0f, 1.0f)
        : FLinearColor(0.1f, 0.1f, 0.12f, 0.95f));

    UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (Inner)
    {
        auto AddLine = [&](const FString& Text, FLinearColor Color = FLinearColor::White)
        {
            UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            if (T)
            {
                T->SetText(FText::FromString(Text));
                T->SetColorAndOpacity(FSlateColor(Color));
                Inner->AddChild(T);
            }
        };

        // Name + level
        AddLine(FString::Printf(TEXT("%s  Lv.%d"),
            *Entry.ProfileSnapshot.name,
            Entry.ProfileSnapshot.level),
            Entry.bWasSacrifice
                ? FLinearColor(1.0f, 0.85f, 0.2f, 1.0f)
                : FLinearColor(0.9f, 0.9f, 0.9f, 1.0f));

        // Spec summary
        if (Entry.ProfileSnapshot.role != ERobotRole::None)
        {
            const UEnum* RoleEnum = StaticEnum<ERobotRole>();
            const FString RoleName = RoleEnum
                ? RoleEnum->GetNameStringByValue((int64)Entry.ProfileSnapshot.role)
                : TEXT("Unknown");
            AddLine(RoleName, FLinearColor(0.6f, 0.7f, 0.9f, 1.0f));
        }

        // Hat indicator
        if (Entry.ProfileSnapshot.current_hat_id >= 0)
        {
            AddLine(TEXT("[HAT]"), FLinearColor(1.0f, 0.6f, 0.1f, 1.0f));
        }

        // Death cause (1-line DEMS message)
        if (!Entry.DeathEvent.message_text.IsEmpty())
        {
            // Truncate to ~60 chars for tile readability
            FString Cause = Entry.DeathEvent.message_text;
            if (Cause.Len() > 60) Cause = Cause.Left(57) + TEXT("...");
            AddLine(Cause, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
        }

        Tile->SetContent(Inner);
    }

    // Notify BP for portrait attachment
    OnMemorialTileBuilt(Tile, Entry);

    return Tile;
}

FString UMXMemorialWallUI::BuildStatsString(const FMXRobotProfile& Profile) const
{
    return FString::Printf(
        TEXT("Runs Survived: %d\n")
        TEXT("Levels Completed: %d\n")
        TEXT("Near Misses: %d\n")
        TEXT("Rescues Witnessed: %d\n")
        TEXT("Deaths Witnessed: %d\n")
        TEXT("Sacrifices Witnessed: %d"),
        Profile.runs_survived,
        Profile.levels_completed,
        Profile.near_misses,
        Profile.rescues_witnessed,
        Profile.deaths_witnessed,
        Profile.sacrifices_witnessed);
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void UMXMemorialWallUI::OnSortDropdownChanged(FString SelectedItem, ESelectInfo::Type)
{
    if      (SelectedItem == TEXT("Date of Death"))   SortMemorial(EMemorialSortMode::DateOfDeath);
    else if (SelectedItem == TEXT("Level"))           SortMemorial(EMemorialSortMode::Level);
    else if (SelectedItem == TEXT("Runs Survived"))   SortMemorial(EMemorialSortMode::RunsSurvived);
    else if (SelectedItem == TEXT("Alphabetical"))    SortMemorial(EMemorialSortMode::Alphabetical);
}

void UMXMemorialWallUI::OnSearchTextChanged(const FText& Text)
{
    SearchByName(Text.ToString());
}
