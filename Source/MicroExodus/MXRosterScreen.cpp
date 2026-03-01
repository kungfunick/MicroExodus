// MXRosterScreen.cpp — UI Module v1.0
// Created: 2026-02-22
// Agent 11: UI

#include "MXRosterScreen.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"

// ---------------------------------------------------------------------------
// NativeConstruct
// ---------------------------------------------------------------------------

void UMXRosterScreen::NativeConstruct()
{
    Super::NativeConstruct();

    // Wire sort/filter dropdowns
    if (SortModeDropdown)
    {
        SortModeDropdown->ClearOptions();
        SortModeDropdown->AddOption(TEXT("Name"));
        SortModeDropdown->AddOption(TEXT("Level"));
        SortModeDropdown->AddOption(TEXT("XP"));
        SortModeDropdown->AddOption(TEXT("Age"));
        SortModeDropdown->AddOption(TEXT("Runs"));
        SortModeDropdown->AddOption(TEXT("Spec"));
        SortModeDropdown->AddOption(TEXT("Hat"));
        SortModeDropdown->OnSelectionChanged.AddDynamic(this, &UMXRosterScreen::OnSortDropdownChanged);
    }

    if (FilterModeDropdown)
    {
        FilterModeDropdown->ClearOptions();
        FilterModeDropdown->AddOption(TEXT("All"));
        FilterModeDropdown->AddOption(TEXT("Scout"));
        FilterModeDropdown->AddOption(TEXT("Guardian"));
        FilterModeDropdown->AddOption(TEXT("Engineer"));
        FilterModeDropdown->AddOption(TEXT("No Spec"));
        FilterModeDropdown->AddOption(TEXT("Hatted"));
        FilterModeDropdown->AddOption(TEXT("No Hat"));
        FilterModeDropdown->OnSelectionChanged.AddDynamic(this, &UMXRosterScreen::OnFilterDropdownChanged);
    }

    if (SearchBox)
    {
        SearchBox->OnTextChanged.AddDynamic(this, &UMXRosterScreen::OnSearchTextChanged);
    }

    // Start with detail panel hidden
    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// ---------------------------------------------------------------------------
// Population
// ---------------------------------------------------------------------------

void UMXRosterScreen::PopulateRoster(const TArray<FMXRobotProfile>& Robots)
{
    AllRobots = Robots;
    ApplyFilterAndSort();
    RebuildRobotList();
}

void UMXRosterScreen::SortRoster(ERosterSortMode SortMode)
{
    CurrentSort = SortMode;
    ApplyFilterAndSort();
    RebuildRobotList();
}

void UMXRosterScreen::FilterRoster(ERosterFilterMode FilterMode)
{
    CurrentFilter = FilterMode;
    ApplyFilterAndSort();
    RebuildRobotList();
}

void UMXRosterScreen::SearchByName(const FString& SearchText)
{
    CurrentSearch = SearchText;
    ApplyFilterAndSort();
    RebuildRobotList();
}

// ---------------------------------------------------------------------------
// Detail Panel
// ---------------------------------------------------------------------------

void UMXRosterScreen::ShowRobotDetail(const FGuid& RobotId)
{
    // Find profile in cache
    const FMXRobotProfile* FoundProfile = nullptr;
    for (const FMXRobotProfile& P : AllRobots)
    {
        if (P.robot_id == RobotId)
        {
            FoundProfile = &P;
            break;
        }
    }

    if (!FoundProfile) return;

    SelectedRobotId = RobotId;

    if (RobotNameText)
    {
        RobotNameText->SetText(FText::FromString(FoundProfile->name));
    }

    if (RobotLevelText)
    {
        RobotLevelText->SetText(FText::FromString(
            FString::Printf(TEXT("Level %d"), FoundProfile->level)));
    }

    if (RobotSpecText)
    {
        RobotSpecText->SetText(FText::FromString(ResolveSpecString(*FoundProfile)));
    }

    if (RobotPersonalityText)
    {
        FString PersonalityStr = FoundProfile->description;
        if (!FoundProfile->likes.IsEmpty())
        {
            PersonalityStr += FString::Printf(TEXT("\nLikes: %s"), *FoundProfile->likes[0]);
        }
        if (!FoundProfile->dislikes.IsEmpty())
        {
            PersonalityStr += FString::Printf(TEXT("\nDislikes: %s"), *FoundProfile->dislikes[0]);
        }
        if (!FoundProfile->quirk.IsEmpty())
        {
            PersonalityStr += FString::Printf(TEXT("\nQuirk: %s"), *FoundProfile->quirk);
        }
        RobotPersonalityText->SetText(FText::FromString(PersonalityStr));
    }

    if (TitlesText)
    {
        FString TitleList = FString::Join(FoundProfile->earned_titles, TEXT(", "));
        TitlesText->SetText(FText::FromString(
            TitleList.IsEmpty() ? TEXT("No titles earned") : TitleList));
    }

    ShowSpecPath(*FoundProfile);
    ShowHatHistory(FoundProfile->hat_history);

    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    OnDetailPanelOpen(*FoundProfile);
}

void UMXRosterScreen::CloseDetail()
{
    SelectedRobotId.Invalidate();

    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMXRosterScreen::ShowLifeLog(const FGuid& RobotId, const TArray<FMXEventData>& Events)
{
    if (!LifeLogScrollBox || !WidgetTree) return;

    LifeLogScrollBox->ClearChildren();

    // Build one row per event (oldest to newest)
    for (const FMXEventData& Ev : Events)
    {
        UTextBlock* EntryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!EntryText) continue;

        const FString Line = FString::Printf(
            TEXT("[L%d] %s"), Ev.level_number, *Ev.message_text);
        EntryText->SetText(FText::FromString(Line));
        LifeLogScrollBox->AddChild(EntryText);
    }
}

void UMXRosterScreen::ShowSpecPath(const FMXRobotProfile& Robot)
{
    if (!SpecPathContainer || !WidgetTree) return;

    SpecPathContainer->ClearChildren();

    auto AddSpecNode = [&](const FString& Label, bool bChosen)
    {
        UBorder* Node = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        if (!Node) return;

        Node->SetBrushColor(bChosen
            ? FLinearColor(0.2f, 0.6f, 1.0f, 1.0f)
            : FLinearColor(0.15f, 0.15f, 0.15f, 0.6f));

        UTextBlock* NodeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (NodeText)
        {
            NodeText->SetText(FText::FromString(Label));
            Node->SetContent(NodeText);
        }
        SpecPathContainer->AddChild(Node);
    };

    // Role / tier nodes — display what the robot has chosen
    if (Robot.role != ERobotRole::None)
    {
        const UEnum* RoleEnum = StaticEnum<ERobotRole>();
        AddSpecNode(RoleEnum ? RoleEnum->GetNameStringByValue((int64)Robot.role) : TEXT("Role"), true);
    }

    if (Robot.tier2_spec != ETier2Spec::None)
    {
        const UEnum* T2Enum = StaticEnum<ETier2Spec>();
        AddSpecNode(T2Enum ? T2Enum->GetNameStringByValue((int64)Robot.tier2_spec) : TEXT("Tier 2"), true);
    }

    if (Robot.tier3_spec != ETier3Spec::None)
    {
        const UEnum* T3Enum = StaticEnum<ETier3Spec>();
        AddSpecNode(T3Enum ? T3Enum->GetNameStringByValue((int64)Robot.tier3_spec) : TEXT("Tier 3"), true);
    }

    if (Robot.mastery_title != EMasteryTitle::None)
    {
        const UEnum* MEnum = StaticEnum<EMasteryTitle>();
        AddSpecNode(MEnum ? MEnum->GetNameStringByValue((int64)Robot.mastery_title) : TEXT("Mastery"), true);
    }

    if (Robot.role == ERobotRole::None)
    {
        UTextBlock* NoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (NoneText)
        {
            NoneText->SetText(FText::FromString(TEXT("Unspecialized")));
            SpecPathContainer->AddChild(NoneText);
        }
    }
}

void UMXRosterScreen::ShowHatHistory(const TArray<int32>& HatIds)
{
    if (!HatHistoryScrollBox || !WidgetTree) return;

    HatHistoryScrollBox->ClearChildren();

    if (HatIds.IsEmpty())
    {
        UTextBlock* NoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (NoneText)
        {
            NoneText->SetText(FText::FromString(TEXT("No hats worn")));
            HatHistoryScrollBox->AddChild(NoneText);
        }
        return;
    }

    for (int32 HatId : HatIds)
    {
        UTextBlock* HatEntry = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (HatEntry)
        {
            HatEntry->SetText(FText::FromString(FString::Printf(TEXT("Hat #%d"), HatId)));
            HatHistoryScrollBox->AddChild(HatEntry);
        }
    }
}

// ---------------------------------------------------------------------------
// Private: filter/sort/rebuild
// ---------------------------------------------------------------------------

void UMXRosterScreen::ApplyFilterAndSort()
{
    // --- Filter ---
    DisplayedRobots.Empty();
    for (const FMXRobotProfile& Robot : AllRobots)
    {
        // Name search
        if (!CurrentSearch.IsEmpty())
        {
            if (!Robot.name.Contains(CurrentSearch, ESearchCase::IgnoreCase))
                continue;
        }

        // Role / hat filter
        bool bPassFilter = false;
        switch (CurrentFilter)
        {
        case ERosterFilterMode::All:       bPassFilter = true; break;
        case ERosterFilterMode::Scout:     bPassFilter = (Robot.role == ERobotRole::Scout); break;
        case ERosterFilterMode::Guardian:  bPassFilter = (Robot.role == ERobotRole::Guardian); break;
        case ERosterFilterMode::Engineer:  bPassFilter = (Robot.role == ERobotRole::Engineer); break;
        case ERosterFilterMode::NoSpec:    bPassFilter = (Robot.role == ERobotRole::None); break;
        case ERosterFilterMode::Hatted:    bPassFilter = (Robot.current_hat_id >= 0); break;
        case ERosterFilterMode::Unhatted:  bPassFilter = (Robot.current_hat_id < 0); break;
        default:                           bPassFilter = true; break;
        }

        if (bPassFilter)
        {
            DisplayedRobots.Add(Robot);
        }
    }

    // --- Sort ---
    switch (CurrentSort)
    {
    case ERosterSortMode::Name:
        DisplayedRobots.Sort([](const FMXRobotProfile& A, const FMXRobotProfile& B)
            { return A.name < B.name; });
        break;

    case ERosterSortMode::Level:
        DisplayedRobots.Sort([](const FMXRobotProfile& A, const FMXRobotProfile& B)
            { return A.level > B.level; });
        break;

    case ERosterSortMode::XP:
        DisplayedRobots.Sort([](const FMXRobotProfile& A, const FMXRobotProfile& B)
            { return A.total_xp > B.total_xp; });
        break;

    case ERosterSortMode::Age:
        DisplayedRobots.Sort([](const FMXRobotProfile& A, const FMXRobotProfile& B)
            { return A.active_age_seconds > B.active_age_seconds; });
        break;

    case ERosterSortMode::Runs:
        DisplayedRobots.Sort([](const FMXRobotProfile& A, const FMXRobotProfile& B)
            { return A.runs_survived > B.runs_survived; });
        break;

    case ERosterSortMode::Spec:
        DisplayedRobots.Sort([](const FMXRobotProfile& A, const FMXRobotProfile& B)
            { return (int32)A.role > (int32)B.role; });
        break;

    case ERosterSortMode::Hat:
        DisplayedRobots.Sort([](const FMXRobotProfile& A, const FMXRobotProfile& B)
            { return A.current_hat_id > B.current_hat_id; });
        break;

    default: break;
    }
}

void UMXRosterScreen::RebuildRobotList()
{
    if (!RobotListBox) return;

    RobotListBox->ClearChildren();

    for (const FMXRobotProfile& Robot : DisplayedRobots)
    {
        UWidget* Card = BuildRobotCard(Robot);
        if (Card)
        {
            RobotListBox->AddChild(Card);
        }
    }

    // Update count text
    if (RobotCountText)
    {
        RobotCountText->SetText(FText::FromString(
            FString::Printf(TEXT("Showing %d / %d"),
                DisplayedRobots.Num(), AllRobots.Num())));
    }
}

UWidget* UMXRosterScreen::BuildRobotCard(const FMXRobotProfile& Robot)
{
    if (!WidgetTree) return nullptr;

    UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    if (!Card) return nullptr;

    // Subtle background tint by role
    FLinearColor CardColor = FLinearColor(0.1f, 0.1f, 0.12f, 0.95f);
    if (Robot.role == ERobotRole::Scout)    CardColor = FLinearColor(0.07f, 0.12f, 0.18f, 0.95f);
    if (Robot.role == ERobotRole::Guardian) CardColor = FLinearColor(0.15f, 0.07f, 0.07f, 0.95f);
    if (Robot.role == ERobotRole::Engineer) CardColor = FLinearColor(0.1f,  0.14f, 0.08f, 0.95f);
    Card->SetBrushColor(CardColor);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!Row) return Card;

    // Name + level
    UTextBlock* NameLevel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (NameLevel)
    {
        NameLevel->SetText(FText::FromString(
            FString::Printf(TEXT("%s  Lv.%d"), *Robot.name, Robot.level)));
        Row->AddChild(NameLevel);
    }

    // Spec summary
    UTextBlock* SpecText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (SpecText)
    {
        SpecText->SetText(FText::FromString(ResolveSpecString(Robot)));
        Row->AddChild(SpecText);
    }

    // Hat indicator
    if (Robot.current_hat_id >= 0)
    {
        UTextBlock* HatTag = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (HatTag)
        {
            HatTag->SetText(FText::FromString(TEXT("[HAT]")));
            Row->AddChild(HatTag);
        }
    }

    Card->SetContent(Row);

    // Tap handler — open detail
    UButton* TapTarget = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    if (TapTarget)
    {
        const FGuid RobotId = Robot.robot_id;
        TapTarget->OnClicked.AddDynamic(this, &UMXRosterScreen::CloseDetail); // placeholder; BP wires real nav
    }

    // Notify BP to attach 3D portrait
    OnRobotCardBuilt(Card, Robot);

    return Card;
}

FString UMXRosterScreen::ResolveSpecString(const FMXRobotProfile& Robot) const
{
    if (Robot.role == ERobotRole::None)
    {
        return TEXT("Unspecialized");
    }

    const UEnum* RoleEnum = StaticEnum<ERobotRole>();
    FString RoleName = RoleEnum
        ? RoleEnum->GetNameStringByValue((int64)Robot.role)
        : TEXT("Role");

    if (Robot.tier2_spec != ETier2Spec::None)
    {
        const UEnum* T2Enum = StaticEnum<ETier2Spec>();
        FString T2Name = T2Enum
            ? T2Enum->GetNameStringByValue((int64)Robot.tier2_spec)
            : TEXT("T2");
        return FString::Printf(TEXT("%s / %s"), *RoleName, *T2Name);
    }

    return RoleName;
}

// ---------------------------------------------------------------------------
// Dropdown / search callbacks
// ---------------------------------------------------------------------------

void UMXRosterScreen::OnSortDropdownChanged(FString SelectedItem, ESelectInfo::Type)
{
    if (SelectedItem == TEXT("Name"))       SortRoster(ERosterSortMode::Name);
    else if (SelectedItem == TEXT("Level")) SortRoster(ERosterSortMode::Level);
    else if (SelectedItem == TEXT("XP"))    SortRoster(ERosterSortMode::XP);
    else if (SelectedItem == TEXT("Age"))   SortRoster(ERosterSortMode::Age);
    else if (SelectedItem == TEXT("Runs"))  SortRoster(ERosterSortMode::Runs);
    else if (SelectedItem == TEXT("Spec"))  SortRoster(ERosterSortMode::Spec);
    else if (SelectedItem == TEXT("Hat"))   SortRoster(ERosterSortMode::Hat);
}

void UMXRosterScreen::OnFilterDropdownChanged(FString SelectedItem, ESelectInfo::Type)
{
    if      (SelectedItem == TEXT("All"))       FilterRoster(ERosterFilterMode::All);
    else if (SelectedItem == TEXT("Scout"))     FilterRoster(ERosterFilterMode::Scout);
    else if (SelectedItem == TEXT("Guardian"))  FilterRoster(ERosterFilterMode::Guardian);
    else if (SelectedItem == TEXT("Engineer"))  FilterRoster(ERosterFilterMode::Engineer);
    else if (SelectedItem == TEXT("No Spec"))   FilterRoster(ERosterFilterMode::NoSpec);
    else if (SelectedItem == TEXT("Hatted"))    FilterRoster(ERosterFilterMode::Hatted);
    else if (SelectedItem == TEXT("No Hat"))    FilterRoster(ERosterFilterMode::Unhatted);
}

void UMXRosterScreen::OnSearchTextChanged(const FText& Text)
{
    SearchByName(Text.ToString());
}
