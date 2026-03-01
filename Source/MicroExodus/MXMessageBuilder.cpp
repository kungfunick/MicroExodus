// MXMessageBuilder.cpp — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS

#include "MXMessageBuilder.h"
#include "MXConstants.h"

UMXMessageBuilder::UMXMessageBuilder()
    : TemplateSelector(nullptr)
    , DedupBuffer(nullptr)
{
}

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void UMXMessageBuilder::Initialize(UMXTemplateSelector* InTemplateSelector, UMXDeduplicationBuffer* InDedupBuffer)
{
    checkf(InTemplateSelector, TEXT("[DEMS] MXMessageBuilder: TemplateSelector must not be null."));
    checkf(InDedupBuffer,      TEXT("[DEMS] MXMessageBuilder: DedupBuffer must not be null."));

    TemplateSelector = InTemplateSelector;
    DedupBuffer      = InDedupBuffer;

    UE_LOG(LogTemp, Log, TEXT("[DEMS] MXMessageBuilder initialized."));
}

// ---------------------------------------------------------------------------
// ResetDedupBuffer
// ---------------------------------------------------------------------------

void UMXMessageBuilder::ResetDedupBuffer()
{
    if (DedupBuffer)
    {
        DedupBuffer->Clear();
    }
}

// ---------------------------------------------------------------------------
// BuildEventMessage — main entry point
// ---------------------------------------------------------------------------

FMXEventData UMXMessageBuilder::BuildEventMessage(
    EEventType EventType,
    const FMXObjectEventFields& HazardFields,
    const FMXRobotProfile& Robot,
    int32 RunNumber,
    int32 LevelNumber,
    int32 SavedCount)
{
    // --- Variety roll with dedup ---
    FMXVarietyRollResult Roll = PerformVarietyRollWithDedup(HazardFields);

    // --- Resolve hat name (empty if no hat) ---
    const bool bHasHat  = Robot.current_hat_id >= 0;
    const bool bHasSpec = Robot.tier2_spec != ETier2Spec::None;
    FString HatName;   // Will be populated by MXEventMessageComponent before calling us.
                       // For now we use the hat_worn_name from whatever is passed in.
                       // (The component sets hat_worn_name on the EventData post-build.)

    // --- Template selection ---
    const FString SelectedTemplate = TemplateSelector->SelectTemplate(EventType, bHasHat, bHasSpec, HazardFields.severity);

    // --- Token fill ---
    const FString MessageText = FillTokens(
        SelectedTemplate, Robot, Roll,
        HazardFields.preposition,
        RunNumber, LevelNumber, SavedCount, HatName);

    // --- Push roll to dedup buffer ---
    DedupBuffer->Push(Roll.Verb, Roll.Source, Roll.Flavor);

    // --- Assemble FMXEventData ---
    FMXEventData EventData;
    EventData.timestamp      = FDateTime::Now();
    EventData.run_number     = RunNumber;
    EventData.level_number   = LevelNumber;
    EventData.event_type     = EventType;

    EventData.robot_id       = Robot.robot_id;
    EventData.robot_name     = Robot.name;
    EventData.robot_level    = Robot.level;
    EventData.robot_spec     = ResolveSpecLabel(Robot);

    EventData.hat_worn_id    = Robot.current_hat_id;
    // hat_worn_name is filled by MXEventMessageComponent after the hat lookup.

    EventData.hazard_element = HazardFields.element;
    EventData.damage_type    = HazardFields.damage_type;

    EventData.verb           = Roll.Verb;
    EventData.source         = Roll.Source;
    EventData.flavor         = Roll.Flavor;
    EventData.preposition    = HazardFields.preposition;
    EventData.severity       = HazardFields.severity;
    EventData.camera_behavior= HazardFields.camera_behavior;
    EventData.visual_mark    = HazardFields.visual_mark;

    EventData.message_text   = MessageText;

    return EventData;
}

// ---------------------------------------------------------------------------
// PerformVarietyRoll
// ---------------------------------------------------------------------------

FMXVarietyRollResult UMXMessageBuilder::PerformVarietyRoll(const FMXObjectEventFields& Fields) const
{
    FMXVarietyRollResult Result;

    // --- Verb ---
    const bool bHasVerbAlts = Fields.verb_past_alt.Num() > 0;
    if (bHasVerbAlts && FMath::RandRange(0, 99) < ALT_PICK_CHANCE)
    {
        Result.Verb = Fields.verb_past_alt[FMath::RandRange(0, Fields.verb_past_alt.Num() - 1)];
    }
    else
    {
        Result.Verb = Fields.verb_past;
    }

    // --- Source noun ---
    const bool bHasSourceAlts = Fields.source_noun_alt.Num() > 0;
    if (bHasSourceAlts && FMath::RandRange(0, 99) < ALT_PICK_CHANCE)
    {
        Result.Source = Fields.source_noun_alt[FMath::RandRange(0, Fields.source_noun_alt.Num() - 1)];
    }
    else
    {
        Result.Source = Fields.source_noun;
    }

    // --- Flavor suffix ---
    if (Fields.flavor_suffix.Num() > 0 && FMath::RandRange(0, 99) < FLAVOR_PICK_CHANCE)
    {
        Result.Flavor = Fields.flavor_suffix[FMath::RandRange(0, Fields.flavor_suffix.Num() - 1)];
    }
    else
    {
        Result.Flavor = FString();
    }

    return Result;
}

// ---------------------------------------------------------------------------
// PerformVarietyRollWithDedup
// ---------------------------------------------------------------------------

FMXVarietyRollResult UMXMessageBuilder::PerformVarietyRollWithDedup(const FMXObjectEventFields& Fields)
{
    FMXVarietyRollResult Roll;
    for (int32 Attempt = 0; Attempt < MAX_DEDUP_RETRIES; ++Attempt)
    {
        Roll = PerformVarietyRoll(Fields);
        if (!DedupBuffer->IsDuplicate(Roll.Verb, Roll.Source, Roll.Flavor))
        {
            return Roll; // Clean roll — use it.
        }
        // Duplicate found — try again. On the last attempt we fall through and use it anyway.
    }
    // All retries exhausted (small hazard pool edge case). Use last roll.
    return Roll;
}

// ---------------------------------------------------------------------------
// FillTokens
// ---------------------------------------------------------------------------

FString UMXMessageBuilder::FillTokens(
    const FString& Template,
    const FMXRobotProfile& Robot,
    const FMXVarietyRollResult& Roll,
    const FString& Preposition,
    int32 RunNumber,
    int32 LevelNumber,
    int32 SavedCount,
    const FString& HatName) const
{
    FString Result = Template;

    // Helper lambda — replace every occurrence of a token.
    auto Replace = [&Result](const TCHAR* Token, const FString& Value)
    {
        Result = Result.Replace(Token, *Value, ESearchCase::CaseSensitive);
    };

    // Core robot tokens
    Replace(TEXT("{name}"),         Robot.name);
    Replace(TEXT("{robot_level}"),  FString::FromInt(Robot.level));
    Replace(TEXT("{spec}"),         ResolveSpecLabel(Robot));
    Replace(TEXT("{mastery}"),      ResolveMasteryLabel(Robot));
    Replace(TEXT("{title}"),        Robot.displayed_title.IsEmpty() ? TEXT("No Title") : *Robot.displayed_title);
    Replace(TEXT("{quirk}"),        Robot.quirk);
    Replace(TEXT("{like}"),         ResolveRandomLike(Robot));
    Replace(TEXT("{dislike}"),      ResolveRandomDislike(Robot));
    Replace(TEXT("{runs_survived}"),FString::FromInt(Robot.runs_survived));
    Replace(TEXT("{active_age}"),   FormatActiveAge(Robot.active_age_seconds));
    Replace(TEXT("{near_miss_count}"), FString::FromInt(Robot.near_misses));

    // Hat tokens
    Replace(TEXT("{hat}"),          HatName.IsEmpty() ? TEXT("some hat") : *HatName);
    Replace(TEXT("{hat_detail}"),   HatName.IsEmpty() ? TEXT("It was unremarkable.") : *FString::Printf(TEXT("The %s went with them."), *HatName));

    // Hazard tokens
    Replace(TEXT("{verb}"),         Roll.Verb);
    Replace(TEXT("{source}"),       Roll.Source);
    Replace(TEXT("{preposition}"),  Preposition);

    // Flavor — if flavor is empty, remove the token and any trailing space before it.
    if (Roll.Flavor.IsEmpty())
    {
        Result = Result.Replace(TEXT(" {flavor}"), TEXT(""), ESearchCase::CaseSensitive);
        Result = Result.Replace(TEXT("{flavor}"),  TEXT(""), ESearchCase::CaseSensitive);
    }
    else
    {
        Replace(TEXT("{flavor}"), Roll.Flavor);
    }

    // Context tokens
    Replace(TEXT("{run_number}"),   FString::FromInt(RunNumber));
    Replace(TEXT("{level_number}"), FString::FromInt(LevelNumber));
    Replace(TEXT("{saved_count}"),  FString::FromInt(SavedCount));

    // Roster count placeholder (resolved as saved_count in sacrifice context)
    Replace(TEXT("{robot_count}"),  FString::FromInt(SavedCount));

    return Result;
}

// ---------------------------------------------------------------------------
// ResolveSpecLabel
// ---------------------------------------------------------------------------

FString UMXMessageBuilder::ResolveSpecLabel(const FMXRobotProfile& Robot) const
{
    // Mastery title takes precedence over all lower spec labels.
    if (Robot.mastery_title != EMasteryTitle::None)
    {
        return ResolveMasteryLabel(Robot);
    }

    // Tier 3
    const UEnum* Tier3Enum = StaticEnum<ETier3Spec>();
    if (Robot.tier3_spec != ETier3Spec::None && Tier3Enum)
    {
        return Tier3Enum->GetDisplayNameTextByValue((int64)Robot.tier3_spec).ToString();
    }

    // Tier 2
    const UEnum* Tier2Enum = StaticEnum<ETier2Spec>();
    if (Robot.tier2_spec != ETier2Spec::None && Tier2Enum)
    {
        return Tier2Enum->GetDisplayNameTextByValue((int64)Robot.tier2_spec).ToString();
    }

    // Role
    const UEnum* RoleEnum = StaticEnum<ERobotRole>();
    if (Robot.role != ERobotRole::None && RoleEnum)
    {
        return RoleEnum->GetDisplayNameTextByValue((int64)Robot.role).ToString();
    }

    return TEXT("Unspecialized");
}

// ---------------------------------------------------------------------------
// ResolveMasteryLabel
// ---------------------------------------------------------------------------

FString UMXMessageBuilder::ResolveMasteryLabel(const FMXRobotProfile& Robot) const
{
    const UEnum* MasteryEnum = StaticEnum<EMasteryTitle>();
    if (Robot.mastery_title != EMasteryTitle::None && MasteryEnum)
    {
        return MasteryEnum->GetDisplayNameTextByValue((int64)Robot.mastery_title).ToString();
    }
    return FString();
}

// ---------------------------------------------------------------------------
// ResolveRandomLike / Dislike
// ---------------------------------------------------------------------------

FString UMXMessageBuilder::ResolveRandomLike(const FMXRobotProfile& Robot) const
{
    if (Robot.likes.Num() == 0) return TEXT("orderly systems");
    return Robot.likes[FMath::RandRange(0, Robot.likes.Num() - 1)];
}

FString UMXMessageBuilder::ResolveRandomDislike(const FMXRobotProfile& Robot) const
{
    if (Robot.dislikes.Num() == 0) return TEXT("unexpected outcomes");
    return Robot.dislikes[FMath::RandRange(0, Robot.dislikes.Num() - 1)];
}

// ---------------------------------------------------------------------------
// FormatActiveAge
// ---------------------------------------------------------------------------

FString UMXMessageBuilder::FormatActiveAge(float ActiveAgeSeconds) const
{
    const int32 TotalSeconds = FMath::FloorToInt(ActiveAgeSeconds);
    const int32 Hours        = TotalSeconds / 3600;
    const int32 Minutes      = (TotalSeconds % 3600) / 60;
    const int32 Secs         = TotalSeconds % 60;

    if (Hours > 0)
    {
        return FString::Printf(TEXT("%dh %dm active"), Hours, Minutes);
    }
    if (Minutes > 0)
    {
        return FString::Printf(TEXT("%dm %ds active"), Minutes, Secs);
    }
    return FString::Printf(TEXT("%ds active"), Secs);
}
