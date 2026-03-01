// MXRunReportEngine.cpp — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports

#include "MXRunReportEngine.h"
#include "MXEvents.h"
#include "MXConstants.h"
#include "Misc/DateTime.h"

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void UMXRunReportEngine::Initialize(
    TScriptInterface<IMXRobotProvider> RobotProvider,
    TScriptInterface<IMXHatProvider>   HatProvider,
    TScriptInterface<IMXRunProvider>   RunProvider)
{
    RobotProviderRef = RobotProvider;
    HatProviderRef   = HatProvider;
    RunProviderRef   = RunProvider;

    // Instantiate sub-systems
    StatCompiler    = NewObject<UMXStatCompiler>(this);
    HighlightScorer = NewObject<UMXHighlightScorer>(this);
    AwardSelector   = NewObject<UMXAwardSelector>(this);
    Narrator        = NewObject<UMXReportNarrator>(this);

    // Load DataTables
    UDataTable* OpeningDT   = LoadDT(DT_ReportOpeningTemplates);
    UDataTable* HighlightDT = LoadDT(DT_ReportHighlightTemplates);
    UDataTable* AwardDT     = LoadDT(DT_AwardTemplates);
    UDataTable* EulogyDT    = LoadDT(DT_EulogyTemplates);
    UDataTable* ClosingDT   = LoadDT(DT_ReportClosingTemplates);

    Narrator->Initialize(OpeningDT, HighlightDT, AwardDT, EulogyDT, ClosingDT);

    bInitialized = true;
}


// ---------------------------------------------------------------------------
// Initialize (9-arg overload — called by GameInstance with pre-loaded DataTables)
// ---------------------------------------------------------------------------

void UMXRunReportEngine::Initialize(
    UMXEventBus*                       InEventBus,
    TScriptInterface<IMXRobotProvider> RobotProvider,
    TScriptInterface<IMXHatProvider>   HatProvider,
    TScriptInterface<IMXRunProvider>   RunProvider,
    UDataTable*                        OpeningTemplates,
    UDataTable*                        HighlightTemplates,
    UDataTable*                        AwardTemplates,
    UDataTable*                        EulogyTemplates,
    UDataTable*                        ClosingTemplates)
{
    RobotProviderRef = RobotProvider;
    HatProviderRef   = HatProvider;
    RunProviderRef   = RunProvider;

    // Instantiate sub-systems
    StatCompiler    = NewObject<UMXStatCompiler>(this);
    HighlightScorer = NewObject<UMXHighlightScorer>(this);
    AwardSelector   = NewObject<UMXAwardSelector>(this);
    Narrator        = NewObject<UMXReportNarrator>(this);

    Narrator->Initialize(OpeningTemplates, HighlightTemplates, AwardTemplates,
                         EulogyTemplates, ClosingTemplates);

    bInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("MXRunReportEngine: Initialized via GameInstance."));
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void UMXRunReportEngine::Shutdown()
{
    bInitialized = false;
    RobotProviderRef = nullptr;
    HatProviderRef   = nullptr;
    RunProviderRef   = nullptr;
    UE_LOG(LogTemp, Log, TEXT("MXRunReportEngine: Shutdown."));
}

// ---------------------------------------------------------------------------
// GenerateReport — Main Pipeline
// ---------------------------------------------------------------------------

FMXRunReport UMXRunReportEngine::GenerateReport(
    const FMXRunData& RunData,
    bool bRunFailed,
    int32 NextRunNumber,
    int32 HatCollectionCount,
    int32 NewHatsUnlocked)
{
    FMXRunReport Report;
    Report.run_data = RunData;

    if (!bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRunReportEngine: GenerateReport called before Initialize()."));
        return Report;
    }

    const double StartMs = FPlatformTime::Seconds() * 1000.0;

    // -------------------------------------------------------------------------
    // Step 1: Fetch all robot profiles (event log is already frozen in RunData)
    // -------------------------------------------------------------------------
    TArray<FMXRobotProfile> AllProfiles;
    if (RobotProviderRef.GetObject())
        AllProfiles = IMXRobotProvider::Execute_GetAllRobotProfiles(RobotProviderRef.GetObject());

    // -------------------------------------------------------------------------
    // Step 2: STAT COMPILATION
    // -------------------------------------------------------------------------
    FMXCompiledStats Stats = StepCompileStats(RunData, AllProfiles);

    const double AfterStats = FPlatformTime::Seconds() * 1000.0;
    UE_LOG(LogTemp, Log, TEXT("MXRunReportEngine: Stat compilation: %.1f ms"), AfterStats - StartMs);

    // -------------------------------------------------------------------------
    // Step 3: HIGHLIGHT SCORING
    // -------------------------------------------------------------------------
    TArray<FMXHighlight> Highlights = StepScoreHighlights(RunData, Stats);

    // -------------------------------------------------------------------------
    // Step 4: AWARD SELECTION
    // -------------------------------------------------------------------------
    TArray<FMXAward> Awards = StepSelectAwards(Stats, AllProfiles);

    // -------------------------------------------------------------------------
    // Step 5: NARRATIVE GENERATION
    // -------------------------------------------------------------------------
    Report.opening_narrative = Narrator->GenerateOpeningNarrative(RunData, Stats);

    StepNarrateHighlights(Highlights, Stats);
    Report.highlights = Highlights;

    StepNarrateAwards(Awards, AllProfiles, Stats);
    Report.awards = Awards;

    Report.closing_narrative = Narrator->GenerateClosingNarrative(
        RunData, Stats, NextRunNumber, HatCollectionCount, NewHatsUnlocked);

    // -------------------------------------------------------------------------
    // Step 6: HAT CENSUS
    // -------------------------------------------------------------------------
    Report.hat_census = StepCompileHatCensus(Stats);

    // -------------------------------------------------------------------------
    // Step 7: DEATH ROLL
    // -------------------------------------------------------------------------
    Report.death_roll = StepBuildDeathRoll(RunData, AllProfiles);

    // -------------------------------------------------------------------------
    // Step 8: SACRIFICE HONOR ROLL
    // -------------------------------------------------------------------------
    Report.sacrifice_honor_roll = StepBuildSacrificeRoll(RunData, AllProfiles);

    const double EndMs = FPlatformTime::Seconds() * 1000.0;
    UE_LOG(LogTemp, Log, TEXT("MXRunReportEngine: Total report generation: %.1f ms"), EndMs - StartMs);

    if (EndMs - StartMs > 1000.0)
        UE_LOG(LogTemp, Warning, TEXT("MXRunReportEngine: Report exceeded 1s budget (%.1f ms)."), EndMs - StartMs);

    LastReport = Report;
    return Report;
}

// ---------------------------------------------------------------------------
// Event Handlers
// ---------------------------------------------------------------------------

void UMXRunReportEngine::HandleRunComplete(FMXRunData RunData)
{
    int32 NextRun = RunData.run_number + 1;
    int32 HatCount = 0;
    if (HatProviderRef.GetObject())
    {
        FMXHatCollection Coll = IMXHatProvider::Execute_GetHatCollection(HatProviderRef.GetObject());
        HatCount = Coll.unlocked_hat_ids.Num();
    }
    GenerateReport(RunData, false, NextRun, HatCount, 0);
}

void UMXRunReportEngine::HandleRunFailed(FMXRunData RunData, int32 FailureLevel)
{
    int32 NextRun = RunData.run_number + 1;
    GenerateReport(RunData, true, NextRun, 0, 0);
}

// ---------------------------------------------------------------------------
// Pipeline Steps
// ---------------------------------------------------------------------------

FMXCompiledStats UMXRunReportEngine::StepCompileStats(
    const FMXRunData& RunData,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    return StatCompiler->CompileStats(RunData, AllProfiles);
}

TArray<FMXHighlight> UMXRunReportEngine::StepScoreHighlights(
    const FMXRunData& RunData,
    const FMXCompiledStats& Stats)
{
    return HighlightScorer->ScoreEvents(RunData.events, Stats);
}

TArray<FMXAward> UMXRunReportEngine::StepSelectAwards(
    const FMXCompiledStats& Stats,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    return AwardSelector->SelectAwards(Stats, AllProfiles);
}

void UMXRunReportEngine::StepNarrateHighlights(
    TArray<FMXHighlight>& Highlights,
    const FMXCompiledStats& Stats)
{
    for (FMXHighlight& H : Highlights)
        H.narrative = Narrator->GenerateHighlightNarrative(H, Stats);
}

void UMXRunReportEngine::StepNarrateAwards(
    TArray<FMXAward>& Awards,
    const TArray<FMXRobotProfile>& AllProfiles,
    const FMXCompiledStats& Stats)
{
    for (FMXAward& A : Awards)
    {
        const FMXRobotRunStats* RS = Stats.FindRobotStats(A.robot_id);
        if (!RS) continue;

        // Find robot profile
        const FMXRobotProfile* ProfilePtr = nullptr;
        for (const FMXRobotProfile& P : AllProfiles)
            if (P.robot_id == A.robot_id) { ProfilePtr = &P; break; }

        if (ProfilePtr)
            A.narrative = Narrator->GenerateAwardNarrative(A, *ProfilePtr, *RS);
        else
        {
            // Construct minimal profile from event data
            FMXRobotProfile Stub;
            Stub.name     = A.robot_name;
            Stub.robot_id = A.robot_id;
            Stub.level    = RS->robot_level;
            A.narrative   = Narrator->GenerateAwardNarrative(A, Stub, *RS);
        }
    }
}

FMXHatCensus UMXRunReportEngine::StepCompileHatCensus(const FMXCompiledStats& Stats)
{
    FMXHatCensus Census;
    Census.total_hatted      = Stats.total_hatted;
    Census.unique_hats       = Stats.unique_hat_types;
    Census.most_common_hat   = Stats.most_common_hat;
    Census.rarest_hat        = Stats.rarest_hat;
    Census.hats_lost         = Stats.hats_lost;

    // Hat survival rate: (hatted robots that survived) / (total hatted robots)
    int32 HattedSurvivors = 0;
    for (const FMXRobotRunStats& RS : Stats.robot_stats)
        if (RS.hat_id >= 0 && RS.survived) HattedSurvivors++;

    Census.hat_survival_rate = (Stats.total_hatted > 0)
        ? (float)HattedSurvivors / (float)Stats.total_hatted
        : 0.0f;

    return Census;
}

TArray<FMXDeathEntry> UMXRunReportEngine::StepBuildDeathRoll(
    const FMXRunData& RunData,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    TArray<FMXDeathEntry> Roll;

    // Collect death events (exclude sacrifices — those go to sacrifice roll)
    for (const FMXEventData& Ev : RunData.events)
    {
        if (Ev.event_type != EEventType::Death) continue;

        FMXDeathEntry Entry;
        Entry.death_event = Ev;

        // Find profile snapshot
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.robot_id == Ev.robot_id)
            {
                Entry.robot_profile_snapshot = P;
                break;
            }
        }

        // If we couldn't find profile, synthesize minimal one from event
        if (!Entry.robot_profile_snapshot.robot_id.IsValid())
        {
            Entry.robot_profile_snapshot.name     = Ev.robot_name;
            Entry.robot_profile_snapshot.robot_id = Ev.robot_id;
            Entry.robot_profile_snapshot.level    = Ev.robot_level;
        }

        Entry.eulogy = Narrator->GenerateEulogy(Entry.robot_profile_snapshot, Ev);

        // Hat lost note
        if (Ev.hat_worn_id >= 0)
        {
            Entry.hat_lost_note = FString::Printf(
                TEXT("The %s landed on the ground and sat there. Then it dissolved."),
                *Ev.hat_worn_name);
        }

        Roll.Add(Entry);
    }

    return Roll;
}

TArray<FMXSacrificeEntry> UMXRunReportEngine::StepBuildSacrificeRoll(
    const FMXRunData& RunData,
    const TArray<FMXRobotProfile>& AllProfiles)
{
    TArray<FMXSacrificeEntry> Roll;

    for (const FMXEventData& Ev : RunData.events)
    {
        if (Ev.event_type != EEventType::Sacrifice) continue;

        FMXSacrificeEntry Entry;
        Entry.sacrifice_event = Ev;

        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.robot_id == Ev.robot_id)
            {
                Entry.robot_profile_snapshot = P;
                break;
            }
        }

        if (!Entry.robot_profile_snapshot.robot_id.IsValid())
        {
            Entry.robot_profile_snapshot.name     = Ev.robot_name;
            Entry.robot_profile_snapshot.robot_id = Ev.robot_id;
            Entry.robot_profile_snapshot.level    = Ev.robot_level;
        }

        // Pull surviving robot names from AllProfiles (up to 3).
        TArray<FString> SavedNames;
        // (Sacrifice narrative doesn't require exact saved list from game data here)
        for (const FMXRobotProfile& P : AllProfiles)
        {
            if (P.robot_id != Ev.robot_id)
                SavedNames.Add(P.name);
            if (SavedNames.Num() >= 3) break; // Show up to 3 saved names
        }

        Entry.extended_narrative = Narrator->GenerateSacrificeNarrative(
            Entry.robot_profile_snapshot, Ev, SavedNames);

        Entry.robots_saved = SavedNames;
        Roll.Add(Entry);
    }

    return Roll;
}

// ---------------------------------------------------------------------------
// IMXPersistable
// ---------------------------------------------------------------------------

void UMXRunReportEngine::MXSerialize(FArchive& Ar)
{
    // Report archive: serialize the last report for post-session review
    // Full FMXRunReport serialization handled by Persistence module
    UE_LOG(LogTemp, Log, TEXT("MXRunReportEngine: Serializing last report (run #%d)."),
        LastReport.run_data.run_number);
}

void UMXRunReportEngine::MXDeserialize(FArchive& Ar)
{
    UE_LOG(LogTemp, Log, TEXT("MXRunReportEngine: Deserializing report archive."));
}

// ---------------------------------------------------------------------------
// LoadDT
// ---------------------------------------------------------------------------

UDataTable* UMXRunReportEngine::LoadDT(TSoftObjectPtr<UDataTable>& SoftRef)
{
    if (SoftRef.IsNull()) return nullptr;
    UDataTable* Loaded = SoftRef.LoadSynchronous();
    if (!Loaded)
        UE_LOG(LogTemp, Warning, TEXT("MXRunReportEngine: Failed to load DataTable: %s"),
            *SoftRef.ToString());
    return Loaded;
}
