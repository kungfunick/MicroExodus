// MXReportNarrator.cpp — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports

#include "MXReportNarrator.h"
#include "MXConstants.h"

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void UMXReportNarrator::Initialize(
    UDataTable* OpeningTemplates,
    UDataTable* HighlightTemplates,
    UDataTable* AwardTemplates,
    UDataTable* EulogyTemplates,
    UDataTable* ClosingTemplates)
{
    OpeningDT   = OpeningTemplates;
    HighlightDT = HighlightTemplates;
    AwardDT     = AwardTemplates;
    EulogyDT    = EulogyTemplates;
    ClosingDT   = ClosingTemplates;
}

// ---------------------------------------------------------------------------
// GenerateOpeningNarrative
// ---------------------------------------------------------------------------

FString UMXReportNarrator::GenerateOpeningNarrative(
    const FMXRunData& RunData,
    const FMXCompiledStats& Stats)
{
    if (!OpeningDT) return TEXT("Run complete.");

    FString Condition = ResolveOpeningCondition(Stats);
    const FMXOpeningTemplateRow* Row = SelectTemplate<FMXOpeningTemplateRow>(
        OpeningDT, UsedOpeningRows, Condition);

    if (!Row) return TEXT("Run complete.");

    const int32 FailureLevel = (Stats.is_failed && !Stats.level_stats.IsEmpty())
        ? Stats.level_stats.Last().level_number : 0;

    TMap<FString, FString> Tokens;
    Tokens.Add(TEXT("run_number"),      FString::FromInt(RunData.run_number));
    Tokens.Add(TEXT("deployed"),        FString::FromInt(Stats.robots_deployed));
    Tokens.Add(TEXT("survived"),        FString::FromInt(Stats.robots_survived));
    Tokens.Add(TEXT("rescued"),         FString::FromInt(Stats.total_rescues));
    Tokens.Add(TEXT("hat_percent"),     FString::Printf(TEXT("%.0f%%"), Stats.hat_percent));
    Tokens.Add(TEXT("sacrifice_count"), FString::FromInt(Stats.total_sacrifices));
    Tokens.Add(TEXT("failure_level"),   FString::FromInt(FailureLevel));
    Tokens.Add(TEXT("lost"),            FString::FromInt(Stats.total_losses));
    Tokens.Add(TEXT("levels_completed"),FString::FromInt(Stats.levels_completed));

    return FillTokens(Row->TemplateText, Tokens);
}

// ---------------------------------------------------------------------------
// GenerateHighlightNarrative
// ---------------------------------------------------------------------------

FString UMXReportNarrator::GenerateHighlightNarrative(
    const FMXHighlight& Highlight,
    const FMXCompiledStats& Stats)
{
    if (!HighlightDT) return Highlight.event.message_text;

    const FMXEventData& Ev = Highlight.event;
    FString TypeKey = ResolveHighlightType(Ev);

    // Filter: require hat only if event has one; require spec only if event has one
    bool bHasHat  = Ev.hat_worn_id >= 0;
    bool bHasSpec = !Ev.robot_spec.IsEmpty() && Ev.robot_spec != TEXT("None");

    const FMXHighlightTemplateRow* Row = nullptr;
    if (HighlightDT)
    {
        TArray<FName> RowNames = HighlightDT->GetRowNames();
        // Try to find a matching, unused row
        TArray<FName> Candidates;
        for (FName RN : RowNames)
        {
            if (UsedHighlightRows.Contains(RN)) continue;
            const FMXHighlightTemplateRow* R = HighlightDT->FindRow<FMXHighlightTemplateRow>(RN, TEXT(""));
            if (!R) continue;
            if (R->EventType != TypeKey && R->EventType != TEXT("any")) continue;
            if (R->bRequiresHat  && !bHasHat)  continue;
            if (R->bRequiresSpec && !bHasSpec)  continue;
            Candidates.Add(RN);
        }
        if (Candidates.IsEmpty())
        {
            // Fallback: allow used rows
            for (FName RN : RowNames)
            {
                const FMXHighlightTemplateRow* R = HighlightDT->FindRow<FMXHighlightTemplateRow>(RN, TEXT(""));
                if (!R) continue;
                if (R->EventType != TypeKey && R->EventType != TEXT("any")) continue;
                Candidates.Add(RN);
            }
        }
        if (!Candidates.IsEmpty())
        {
            FName Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
            Row = HighlightDT->FindRow<FMXHighlightTemplateRow>(Chosen, TEXT(""));
            UsedHighlightRows.Add(Chosen);
        }
    }

    if (!Row) return Ev.message_text;

    // Gather per-level death count for mass casualty templates
    int32 DeathCount = 0;
    for (const FMXLevelRunStats& LS : Stats.level_stats)
        if (LS.level_number == Ev.level_number) { DeathCount = LS.losses; break; }

    TMap<FString, FString> Tokens;
    Tokens.Add(TEXT("name"),           Ev.robot_name);
    Tokens.Add(TEXT("robot_level"),    FString::FromInt(Ev.robot_level));
    Tokens.Add(TEXT("hat"),            bHasHat ? Ev.hat_worn_name : TEXT("(no hat)"));
    Tokens.Add(TEXT("spec"),           bHasSpec ? Ev.robot_spec : TEXT("Unspecialized"));
    Tokens.Add(TEXT("verb"),           Ev.verb);
    Tokens.Add(TEXT("source"),         Ev.source);
    Tokens.Add(TEXT("level_num"),      FString::FromInt(Ev.level_number));
    Tokens.Add(TEXT("death_count"),    FString::FromInt(DeathCount));
    Tokens.Add(TEXT("spec_action"),    TEXT("used their ability at the critical moment"));
    Tokens.Add(TEXT("idle_levels"),    TEXT("several"));
    Tokens.Add(TEXT("record_type"),    TEXT("near-misses survived"));
    Tokens.Add(TEXT("record_value"),   FString::FromInt(Stats.longest_near_miss_streak));
    Tokens.Add(TEXT("near_miss_count"),FString::FromInt(Stats.longest_near_miss_streak));

    return FillTokens(Row->TemplateText, Tokens);
}

// ---------------------------------------------------------------------------
// GenerateAwardNarrative
// ---------------------------------------------------------------------------

FString UMXReportNarrator::GenerateAwardNarrative(
    const FMXAward& Award,
    const FMXRobotProfile& Profile,
    const FMXRobotRunStats& RobotStats)
{
    if (!AwardDT) return FString::Printf(TEXT("%s wins %s."),
        *Award.robot_name, *Award.stat_value);

    // Find rows matching the award category
    FString CategoryName = UEnum::GetDisplayValueAsText(Award.category).ToString();
    TArray<FName> RowNames = AwardDT->GetRowNames();
    TArray<FName> Candidates;
    for (FName RN : RowNames)
    {
        if (UsedAwardRows.Contains(RN)) continue;
        const FMXAwardTemplateRow* R = AwardDT->FindRow<FMXAwardTemplateRow>(RN, TEXT(""));
        if (!R) continue;
        if (R->AwardCategory == CategoryName) Candidates.Add(RN);
    }
    if (Candidates.IsEmpty())
    {
        // Fallback to any row matching category, including used
        for (FName RN : RowNames)
        {
            const FMXAwardTemplateRow* R = AwardDT->FindRow<FMXAwardTemplateRow>(RN, TEXT(""));
            if (R && R->AwardCategory == CategoryName) Candidates.Add(RN);
        }
    }
    if (Candidates.IsEmpty())
        return FString::Printf(TEXT("%s: %s."), *Award.robot_name, *Award.stat_value);

    FName Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
    const FMXAwardTemplateRow* Row = AwardDT->FindRow<FMXAwardTemplateRow>(Chosen, TEXT(""));
    UsedAwardRows.Add(Chosen);

    FString SpecLabel = FormatSpecLabel(Profile);
    int32   IdleTime  = FMath::RoundToInt(RobotStats.idle_time_seconds / 60.0f); // in minutes

    TMap<FString, FString> Tokens;
    Tokens.Add(TEXT("name"),         Award.robot_name);
    Tokens.Add(TEXT("spec"),         SpecLabel.IsEmpty() ? TEXT("Unspecialized") : SpecLabel);
    Tokens.Add(TEXT("stat_summary"), Award.stat_value);
    Tokens.Add(TEXT("count"),        Award.stat_value);
    Tokens.Add(TEXT("idle_time"),    FString::Printf(TEXT("%dm"), IdleTime));
    Tokens.Add(TEXT("idle_levels"),  FString::FromInt(RobotStats.active_levels));
    Tokens.Add(TEXT("hat"),          RobotStats.hat_id >= 0 ? RobotStats.hat_name : TEXT("nothing"));
    Tokens.Add(TEXT("rarity"),       TEXT("exceptional"));
    Tokens.Add(TEXT("verb"),         TEXT("was lost"));
    Tokens.Add(TEXT("level_num"),    TEXT("an unknown level"));
    Tokens.Add(TEXT("remaining"),    TEXT("unknown"));

    return FillTokens(Row->TemplateText, Tokens);
}

// ---------------------------------------------------------------------------
// GenerateEulogy
// ---------------------------------------------------------------------------

FString UMXReportNarrator::GenerateEulogy(
    const FMXRobotProfile& Profile,
    const FMXEventData& DeathEvent)
{
    if (!EulogyDT)
    {
        return FString::Printf(TEXT("%s (Lvl %d) is gone."), *Profile.name, Profile.level);
    }

    FString Bracket = ResolveEulogyBracket(Profile, DeathEvent);

    TArray<FName> RowNames = EulogyDT->GetRowNames();
    TArray<FName> Candidates;
    for (FName RN : RowNames)
    {
        if (UsedEulogyRows.Contains(RN)) continue;
        const FMXEulogyTemplateRow* R = EulogyDT->FindRow<FMXEulogyTemplateRow>(RN, TEXT(""));
        if (R && R->LevelBracket == Bracket) Candidates.Add(RN);
    }
    if (Candidates.IsEmpty())
    {
        for (FName RN : RowNames)
        {
            const FMXEulogyTemplateRow* R = EulogyDT->FindRow<FMXEulogyTemplateRow>(RN, TEXT(""));
            if (R && R->LevelBracket == Bracket) Candidates.Add(RN);
        }
    }
    if (Candidates.IsEmpty()) return FString::Printf(
        TEXT("%s didn't make it."), *Profile.name);

    FName Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
    const FMXEulogyTemplateRow* Row = EulogyDT->FindRow<FMXEulogyTemplateRow>(Chosen, TEXT(""));
    UsedEulogyRows.Add(Chosen);

    FString SpecLabel   = FormatSpecLabel(Profile);
    FString LikeText    = Profile.likes.IsEmpty() ? TEXT("") :
        FString::Printf(TEXT("They liked %s."), *Profile.likes[0]);
    FString ActiveAge   = FormatActiveAge(Profile.active_age_seconds);
    FString HatName     = (DeathEvent.hat_worn_id >= 0) ? DeathEvent.hat_worn_name : TEXT("nothing");

    TMap<FString, FString> Tokens;
    Tokens.Add(TEXT("name"),          Profile.name);
    Tokens.Add(TEXT("spec"),          SpecLabel.IsEmpty() ? TEXT("Unspecialized") : SpecLabel);
    Tokens.Add(TEXT("robot_level"),   FString::FromInt(Profile.level));
    Tokens.Add(TEXT("runs_survived"), FString::FromInt(Profile.runs_survived));
    Tokens.Add(TEXT("near_misses"),   FString::FromInt(Profile.near_misses));
    Tokens.Add(TEXT("active_age"),    ActiveAge);
    Tokens.Add(TEXT("hat"),           HatName);
    Tokens.Add(TEXT("quirk"),         Profile.quirk);
    Tokens.Add(TEXT("like_text"),     LikeText);

    return FillTokens(Row->TemplateText, Tokens);
}

// ---------------------------------------------------------------------------
// GenerateSacrificeNarrative
// ---------------------------------------------------------------------------

FString UMXReportNarrator::GenerateSacrificeNarrative(
    const FMXRobotProfile& Profile,
    const FMXEventData& SacrificeEvent,
    const TArray<FString>& SavedNames)
{
    FString SavedList;
    for (int32 i = 0; i < SavedNames.Num(); ++i)
    {
        if (i > 0) SavedList += (i == SavedNames.Num() - 1) ? TEXT(" and ") : TEXT(", ");
        SavedList += SavedNames[i];
    }
    if (SavedList.IsEmpty()) SavedList = TEXT("those who remained");

    // Sacrifice gets the eulogy template with the "sacrifice" bracket, extended
    FString Core = GenerateEulogy(Profile, SacrificeEvent);

    // Append saved-names postscript with the sardonic narrator voice
    return FString::Printf(
        TEXT("%s\n\n%s walked onto the plate on Level %d and did not walk off. "
             "%s made it through. That number is not abstract."),
        *Core,
        *Profile.name,
        SacrificeEvent.level_number,
        *SavedList
    );
}

// ---------------------------------------------------------------------------
// GenerateHatCensusCommentary
// ---------------------------------------------------------------------------

TArray<FString> UMXReportNarrator::GenerateHatCensusCommentary(
    const FMXHatCensus& Census,
    const FMXCompiledStats& Stats)
{
    TArray<FString> Lines;

    // Line 1: raw hat coverage stat
    Lines.Add(FString::Printf(
        TEXT("%.0f%% of the deployed swarm were hatted. %s"),
        Stats.hat_percent,
        Stats.hat_percent >= 80.0f  ? TEXT("The hat lobby is strong.") :
        Stats.hat_percent >= 50.0f  ? TEXT("A respectable showing.") :
        Stats.hat_percent >= 25.0f  ? TEXT("Room for improvement.") :
                                      TEXT("Embarrassing, honestly.")));

    // Line 2: unique types
    if (Census.unique_hats > 0)
    {
        Lines.Add(FString::Printf(
            TEXT("%d unique hat %s made an appearance. %s"),
            Census.unique_hats,
            Census.unique_hats == 1 ? TEXT("type") : TEXT("types"),
            Census.unique_hats >= 10 ? TEXT("The runway was crowded.") :
            Census.unique_hats >= 5  ? TEXT("Decent variety.") :
                                       TEXT("Not a fashion-forward run.")));
    }

    // Line 3: losses
    if (Census.hats_lost > 0)
    {
        Lines.Add(FString::Printf(
            TEXT("%d %s lost. They will not be spoken of lightly."),
            Census.hats_lost,
            Census.hats_lost == 1 ? TEXT("hat was") : TEXT("hats were")));
    }
    else
    {
        Lines.Add(TEXT("No hats were lost. The collection is intact. Against all odds."));
    }

    // Line 4: rarest survivor
    if (!Census.rarest_hat.IsEmpty() && Census.hat_survival_rate > 0.0f)
    {
        Lines.Add(FString::Printf(
            TEXT("The %s survived. Rare things sometimes do."),
            *Census.rarest_hat));
    }

    return Lines;
}

// ---------------------------------------------------------------------------
// GenerateClosingNarrative
// ---------------------------------------------------------------------------

FString UMXReportNarrator::GenerateClosingNarrative(
    const FMXRunData& RunData,
    const FMXCompiledStats& Stats,
    int32 NextRunNumber,
    int32 HatCollectionCount,
    int32 NewHatsUnlockedThisRun)
{
    if (!ClosingDT) return TEXT("Next run begins soon.");

    FString Condition;
    if (Stats.is_perfect_run)
        Condition = TEXT("perfect");
    else if (Stats.is_failed)
        Condition = TEXT("failed");
    else if (RunData.run_number == 1)
        Condition = TEXT("first_run");
    else if (NewHatsUnlockedThisRun > 0)
        Condition = TEXT("hat_milestone");
    else if (Stats.total_losses == 0 || Stats.survival_rate >= 0.9f)
        Condition = TEXT("success_low_loss");
    else
        Condition = TEXT("success_heavy_loss");

    TArray<FName> RowNames = ClosingDT->GetRowNames();
    TArray<FName> Candidates;
    for (FName RN : RowNames)
    {
        if (UsedClosingRows.Contains(RN)) continue;
        const FMXClosingTemplateRow* R = ClosingDT->FindRow<FMXClosingTemplateRow>(RN, TEXT(""));
        if (!R) continue;
        if (R->Condition == Condition || R->Condition == TEXT("any")) Candidates.Add(RN);
    }
    if (Candidates.IsEmpty())
    {
        for (FName RN : RowNames)
        {
            const FMXClosingTemplateRow* R = ClosingDT->FindRow<FMXClosingTemplateRow>(RN, TEXT(""));
            if (R && (R->Condition == Condition || R->Condition == TEXT("any"))) Candidates.Add(RN);
        }
    }
    if (Candidates.IsEmpty()) return TEXT("Run archived. Next run begins.");

    FName Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
    const FMXClosingTemplateRow* Row = ClosingDT->FindRow<FMXClosingTemplateRow>(Chosen, TEXT(""));
    UsedClosingRows.Add(Chosen);

    TMap<FString, FString> Tokens;
    Tokens.Add(TEXT("survived"),          FString::FromInt(Stats.robots_survived));
    Tokens.Add(TEXT("lost"),              FString::FromInt(Stats.total_losses + Stats.total_sacrifices));
    Tokens.Add(TEXT("deployed"),          FString::FromInt(Stats.robots_deployed));
    Tokens.Add(TEXT("levels_completed"),  FString::FromInt(Stats.levels_completed));
    Tokens.Add(TEXT("lost_all"),          FString::FromInt(Stats.total_losses + Stats.total_sacrifices));
    Tokens.Add(TEXT("next_run"),          FString::FromInt(NextRunNumber));
    Tokens.Add(TEXT("surviving"),         FString::FromInt(Stats.robots_survived));
    Tokens.Add(TEXT("new_hats"),          FString::FromInt(NewHatsUnlockedThisRun));
    Tokens.Add(TEXT("collection_count"),  FString::FromInt(HatCollectionCount));

    return FillTokens(Row->TemplateText, Tokens);
}

// ---------------------------------------------------------------------------
// FillTokens
// ---------------------------------------------------------------------------

FString UMXReportNarrator::FillTokens(
    const FString& Template,
    const TMap<FString, FString>& TokenMap) const
{
    FString Result = Template;
    for (const auto& Pair : TokenMap)
    {
        FString Token = FString::Printf(TEXT("{%s}"), *Pair.Key);
        Result.ReplaceInline(*Token, *Pair.Value, ESearchCase::IgnoreCase);
    }
    return Result;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

FString UMXReportNarrator::ResolveOpeningCondition(const FMXCompiledStats& Stats) const
{
    if (Stats.is_perfect_run)        return TEXT("perfect");
    if (Stats.is_failed)             return TEXT("failed");
    if (Stats.is_bloodbath)          return TEXT("bloodbath");
    if (Stats.is_sacrifice_heavy)    return TEXT("sacrifice_heavy");
    if (Stats.is_hat_heavy)          return TEXT("hat_heavy");
    if (Stats.is_high_survival)      return TEXT("high_survival");
    return TEXT("any");
}

FString UMXReportNarrator::ResolveHighlightType(const FMXEventData& Event) const
{
    switch (Event.event_type)
    {
    case EEventType::Death:
        if (Event.severity == ESeverity::Dramatic) return TEXT("dramatic_death");
        return TEXT("death");

    case EEventType::Sacrifice:      return TEXT("sacrifice");
    case EEventType::NearMiss:       return TEXT("near_miss");
    case EEventType::Rescue:         return TEXT("rescue");
    case EEventType::LevelUp:        return TEXT("specialist_clutch");
    default:
        if (Event.hazard_element == EHazardElement::Comedy) return TEXT("hat_comedy");
        return TEXT("any");
    }
}

FString UMXReportNarrator::ResolveEulogyBracket(
    const FMXRobotProfile& Profile,
    const FMXEventData& DeathEvent) const
{
    if (DeathEvent.event_type == EEventType::Sacrifice) return TEXT("sacrifice");
    if (DeathEvent.hat_worn_id >= 0)                    return TEXT("hatted");
    if (Profile.level >= 20)                            return TEXT("veteran");
    if (Profile.level >= 5)                             return TEXT("mid_level");
    return TEXT("low_level");
}

FString UMXReportNarrator::FormatActiveAge(float Seconds) const
{
    const int32 TotalMins  = FMath::FloorToInt(Seconds / 60.0f);
    const int32 Hours      = TotalMins / 60;
    const int32 Mins       = TotalMins % 60;
    const int32 Days       = Hours / 24;
    const int32 RemHours   = Hours % 24;

    if (Days > 0)
        return FString::Printf(TEXT("%dd %dh of active service"), Days, RemHours);
    if (Hours > 0)
        return FString::Printf(TEXT("%dh %dm of active service"), Hours, Mins);
    return FString::Printf(TEXT("%dm of active service"), FMath::Max(1, Mins));
}

FString UMXReportNarrator::FormatSpecLabel(const FMXRobotProfile& Profile) const
{
    // Prefer mastery title, then tier3, then tier2, then role
    if (Profile.mastery_title != EMasteryTitle::None)
    {
        // Shorten the mastery title display name
        return UEnum::GetDisplayValueAsText(Profile.mastery_title).ToString();
    }
    if (Profile.tier3_spec != ETier3Spec::None)
        return UEnum::GetDisplayValueAsText(Profile.tier3_spec).ToString();
    if (Profile.tier2_spec != ETier2Spec::None)
        return UEnum::GetDisplayValueAsText(Profile.tier2_spec).ToString();
    if (Profile.role != ERobotRole::None)
        return UEnum::GetDisplayValueAsText(Profile.role).ToString();
    return TEXT("");
}

// ---------------------------------------------------------------------------
// SelectTemplate (template method — implementation in header via explicit instantiation)
// ---------------------------------------------------------------------------
// Note: Because this is a template function, we explicitly instantiate for each row type.
// For brevity the selection logic is inlined in the public Generate*() methods above
// using the same pattern. The SelectTemplate<T> signature is used by other callers.

template<typename TRow>
const TRow* UMXReportNarrator::SelectTemplate(
    UDataTable* DT,
    TSet<FName>& UsedRows,
    const FString& ConditionKey,
    TFunctionRef<bool(const TRow&)> ExtraFilter)
{
    if (!DT) return nullptr;

    TArray<FName> RowNames = DT->GetRowNames();
    TArray<FName> Matching;

    // First pass: unused rows matching condition
    for (FName RN : RowNames)
    {
        if (UsedRows.Contains(RN)) continue;
        const TRow* R = DT->FindRow<TRow>(RN, TEXT(""));
        if (!R) continue;
        if (!ExtraFilter(*R)) continue;
        Matching.Add(RN);
    }

    // Fallback: any unused matching row with "any" — already handled by ExtraFilter
    // Second fallback: allow used rows
    if (Matching.IsEmpty())
    {
        for (FName RN : RowNames)
        {
            const TRow* R = DT->FindRow<TRow>(RN, TEXT(""));
            if (!R) continue;
            if (!ExtraFilter(*R)) continue;
            Matching.Add(RN);
        }
    }

    if (Matching.IsEmpty()) return nullptr;

    FName Chosen = Matching[FMath::RandRange(0, Matching.Num() - 1)];
    UsedRows.Add(Chosen);
    return DT->FindRow<TRow>(Chosen, TEXT(""));
}

// Explicit instantiations
template const FMXOpeningTemplateRow* UMXReportNarrator::SelectTemplate<FMXOpeningTemplateRow>(
    UDataTable*, TSet<FName>&, const FString&, TFunctionRef<bool(const FMXOpeningTemplateRow&)>);
template const FMXClosingTemplateRow* UMXReportNarrator::SelectTemplate<FMXClosingTemplateRow>(
    UDataTable*, TSet<FName>&, const FString&, TFunctionRef<bool(const FMXClosingTemplateRow&)>);
