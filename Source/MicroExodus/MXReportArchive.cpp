// MXReportArchive.cpp — Persistence Module Version 1.1
// Change Log:
//   v1.1 — Fixed field names: active_modifiers, FMXScoreBreakdown struct,
//           hat_census on FMXRunReport, FMXHighlight/FMXAward corrected fields.

#include "MXReportArchive.h"

void UMXReportArchive::SerializeReports(FArchive& Ar, const TArray<FMXRunReport>& Reports)
{
    TArray<FMXRunReport> WorkingCopy = Reports;
    CompressOldReports(WorkingCopy);

    int32 Count = WorkingCopy.Num();
    Ar << Count;
    for (const FMXRunReport& Report : WorkingCopy)
        SerializeReport(Ar, Report);

    UE_LOG(LogTemp, Log, TEXT("MXReportArchive: Serialized %d run reports."), Count);
}

TArray<FMXRunReport> UMXReportArchive::DeserializeReports(FArchive& Ar)
{
    TArray<FMXRunReport> Result;
    int32 Count = 0;
    Ar << Count;
    if (Count < 0 || Count > 10000) { UE_LOG(LogTemp, Error, TEXT("MXReportArchive: invalid count %d"), Count); return Result; }
    Result.Reserve(Count);
    for (int32 i = 0; i < Count; ++i) Result.Add(DeserializeReport(Ar));
    UE_LOG(LogTemp, Log, TEXT("MXReportArchive: Deserialized %d run reports."), Count);
    return Result;
}

void UMXReportArchive::CompressOldReports(TArray<FMXRunReport>& Reports, int32 KeepFullCount)
{
    const int32 CompressCount = Reports.Num() - KeepFullCount;
    if (CompressCount <= 0) return;
    int32 Compressed = 0;
    for (int32 i = 0; i < CompressCount; ++i)
    {
        if (Reports[i].run_data.events.Num() > 0) { Reports[i].run_data.events.Empty(); ++Compressed; }
    }
    if (Compressed > 0) UE_LOG(LogTemp, Log, TEXT("MXReportArchive: pruned events from %d old reports."), Compressed);
}

FMXRunReport* UMXReportArchive::GetReportByRunNumber(TArray<FMXRunReport>& Reports, int32 RunNumber)
{
    for (FMXRunReport& R : Reports) { if (R.run_data.run_number == RunNumber) return &R; }
    return nullptr;
}

TArray<int32> UMXReportArchive::GetReportsForRobot(const TArray<FMXRunReport>& Reports, const FGuid& RobotId)
{
    TArray<int32> Nums;
    for (const FMXRunReport& R : Reports) { if (ReportMentionsRobot(R, RobotId)) Nums.AddUnique(R.run_data.run_number); }
    Nums.Sort();
    return Nums;
}

void UMXReportArchive::SerializeAllTimeRecords(FArchive& Ar, const FMXAllTimeRecords& Records)
{
    uint8 V = 1; Ar << V;
    float  BestSR     = Records.BestSurvivalRate;          Ar << BestSR;
    int32  BestSRRun  = Records.BestSurvivalRunNumber;     Ar << BestSRRun;
    int32  HiScore    = Records.HighestScore;              Ar << HiScore;
    int32  HiRun      = Records.HighestScoreRunNumber;     Ar << HiRun;
    int32  MostHats   = Records.MostHatsDeployed;          Ar << MostHats;
    int32  MostNM     = Records.MostNearMissesSingleRobot; Ar << MostNM;
    FString MostNMNm  = Records.MostNearMissesRobotName;   Ar << MostNMNm;
    int32  MostDths   = Records.MostDeathsSingleRun;       Ar << MostDths;
    int32  LongStk    = Records.LongestPerfectStreak;      Ar << LongStk;
    int32  CurStk     = Records.CurrentPerfectStreak;      Ar << CurStk;
    int32  TotLost    = Records.TotalRobotsLostAllTime;    Ar << TotLost;
    int32  TotResc    = Records.TotalRobotsRescuedAllTime; Ar << TotResc;
    int32  TotHats    = Records.TotalHatsLostAllTime;      Ar << TotHats;
}

FMXAllTimeRecords UMXReportArchive::DeserializeAllTimeRecords(FArchive& Ar)
{
    FMXAllTimeRecords R;
    uint8 V = 0; Ar << V;
    Ar << R.BestSurvivalRate;    Ar << R.BestSurvivalRunNumber;
    Ar << R.HighestScore;        Ar << R.HighestScoreRunNumber;
    Ar << R.MostHatsDeployed;    Ar << R.MostNearMissesSingleRobot;
    Ar << R.MostNearMissesRobotName;
    Ar << R.MostDeathsSingleRun;
    Ar << R.LongestPerfectStreak; Ar << R.CurrentPerfectStreak;
    Ar << R.TotalRobotsLostAllTime; Ar << R.TotalRobotsRescuedAllTime;
    Ar << R.TotalHatsLostAllTime;
    return R;
}

// ---------------------------------------------------------------------------
// Private: Single Report Serialization
// ---------------------------------------------------------------------------

void UMXReportArchive::SerializeReport(FArchive& Ar, const FMXRunReport& Report)
{
    const int64 SizePos = Ar.Tell();
    uint32 BlockSize = 0; Ar << BlockSize;
    const int64 DataStart = Ar.Tell();

    // run_data — mutable copy so FArchive << works on all fields
    FMXRunData RD = Report.run_data;
    Ar << RD.run_number;
    uint8 Tier = (uint8)RD.tier;       Ar << Tier;
    uint8 Out  = (uint8)RD.outcome;    Ar << Out;
    Ar << RD.start_time; Ar << RD.end_time;
    Ar << RD.levels_completed; Ar << RD.robots_deployed;
    Ar << RD.robots_survived;  Ar << RD.robots_lost;
    Ar << RD.robots_rescued;   Ar << RD.score;

    // Modifiers: TArray<FString> active_modifiers
    Ar << RD.active_modifiers;

    // Score breakdown: flat struct
    Ar << RD.score_breakdown.rescue_bonus;
    Ar << RD.score_breakdown.survival_bonus;
    Ar << RD.score_breakdown.loss_penalty;
    Ar << RD.score_breakdown.time_bonus;
    Ar << RD.score_breakdown.multiplier;
    Ar << RD.score_breakdown.final_score;

    // Events (minimal snapshot — may be empty if compressed)
    int32 EvCnt = RD.events.Num(); Ar << EvCnt;
    for (FMXEventData& Ev : RD.events)
    {
        uint8 ET = (uint8)Ev.event_type; Ar << ET;
        Ar << Ev.robot_id; Ar << Ev.robot_name;
        Ar << Ev.message_text; Ar << Ev.run_number; Ar << Ev.level_number;
    }

    // Narrative
    FString Op = Report.opening_narrative; Ar << Op;
    FString Cl = Report.closing_narrative; Ar << Cl;

    // Hat census (on FMXRunReport, not FMXRunData)
    // FMXHatCensus fields: total_hatted, unique_hats, most_common_hat, rarest_hat, hats_lost, hat_survival_rate
    FMXHatCensus HC = Report.hat_census;
    Ar << HC.total_hatted; Ar << HC.unique_hats;
    Ar << HC.most_common_hat; Ar << HC.rarest_hat;
    Ar << HC.hats_lost; Ar << HC.hat_survival_rate;

    // Highlights: FMXHighlight has score, event (FMXEventData), narrative
    int32 HlCnt = Report.highlights.Num(); Ar << HlCnt;
    for (const FMXHighlight& Hl : Report.highlights)
    {
        float HlScore = Hl.score;             Ar << HlScore;
        FString HlNarr = Hl.narrative;        Ar << HlNarr;
        uint8 HlET = (uint8)Hl.event.event_type; Ar << HlET;
        FGuid HlRid = Hl.event.robot_id;      Ar << HlRid;
        FString HlMsg = Hl.event.message_text; Ar << HlMsg;
    }

    // Awards: FMXAward has category, robot_id, robot_name, narrative, stat_value
    int32 AwCnt = Report.awards.Num(); Ar << AwCnt;
    for (const FMXAward& Aw : Report.awards)
    {
        uint8 AwCat   = (uint8)Aw.category; Ar << AwCat;
        FGuid AwRid   = Aw.robot_id;        Ar << AwRid;
        FString AwNm  = Aw.robot_name;      Ar << AwNm;
        FString AwNarr= Aw.narrative;       Ar << AwNarr;
        FString AwStat= Aw.stat_value;      Ar << AwStat;
    }

    // Death roll
    int32 DrCnt = Report.death_roll.Num(); Ar << DrCnt;
    for (const FMXDeathEntry& De : Report.death_roll)
    {
        FString DeNm = De.robot_profile_snapshot.name;     Ar << DeNm;
        FGuid   DeId = De.robot_profile_snapshot.robot_id; Ar << DeId;
        int32   DeLv = De.robot_profile_snapshot.level;    Ar << DeLv;
        FString DeEu = De.eulogy;                          Ar << DeEu;
        FString DeHt = De.hat_lost_note;                   Ar << DeHt;
        FString DeMg = De.death_event.message_text;        Ar << DeMg;
    }

    // Sacrifice honor roll
    int32 SrCnt = Report.sacrifice_honor_roll.Num(); Ar << SrCnt;
    for (const FMXSacrificeEntry& Sr : Report.sacrifice_honor_roll)
    {
        FString SrNm = Sr.robot_profile_snapshot.name;     Ar << SrNm;
        FGuid   SrId = Sr.robot_profile_snapshot.robot_id; Ar << SrId;
        int32   SrLv = Sr.robot_profile_snapshot.level;    Ar << SrLv;
        FString SrNr = Sr.extended_narrative;               Ar << SrNr;
        TArray<FString> SrSv = Sr.robots_saved;             Ar << SrSv;
    }

    // Patch block size
    const int64 DataEnd = Ar.Tell();
    BlockSize = (uint32)(DataEnd - DataStart);
    Ar.Seek(SizePos); Ar << BlockSize; Ar.Seek(DataEnd);
}

FMXRunReport UMXReportArchive::DeserializeReport(FArchive& Ar)
{
    FMXRunReport Report;
    uint32 BlockSize = 0; Ar << BlockSize;
    const int64 DataStart = Ar.Tell();
    const int64 DataEnd   = DataStart + (int64)BlockSize;

    Ar << Report.run_data.run_number;
    uint8 Tier = 0; Ar << Tier; Report.run_data.tier = (ETier)Tier;
    uint8 Out  = 0; Ar << Out;  Report.run_data.outcome = (ERunOutcome)Out;
    Ar << Report.run_data.start_time; Ar << Report.run_data.end_time;
    Ar << Report.run_data.levels_completed; Ar << Report.run_data.robots_deployed;
    Ar << Report.run_data.robots_survived;  Ar << Report.run_data.robots_lost;
    Ar << Report.run_data.robots_rescued;   Ar << Report.run_data.score;

    Ar << Report.run_data.active_modifiers;

    Ar << Report.run_data.score_breakdown.rescue_bonus;
    Ar << Report.run_data.score_breakdown.survival_bonus;
    Ar << Report.run_data.score_breakdown.loss_penalty;
    Ar << Report.run_data.score_breakdown.time_bonus;
    Ar << Report.run_data.score_breakdown.multiplier;
    Ar << Report.run_data.score_breakdown.final_score;

    int32 EvCnt = 0; Ar << EvCnt;
    for (int32 i = 0; i < EvCnt && !Ar.IsError(); ++i)
    {
        FMXEventData Ev;
        uint8 ET = 0; Ar << ET; Ev.event_type = (EEventType)ET;
        Ar << Ev.robot_id; Ar << Ev.robot_name;
        Ar << Ev.message_text; Ar << Ev.run_number; Ar << Ev.level_number;
        Report.run_data.events.Add(Ev);
    }

    Ar << Report.opening_narrative;
    Ar << Report.closing_narrative;

    Ar << Report.hat_census.total_hatted; Ar << Report.hat_census.unique_hats;
    Ar << Report.hat_census.most_common_hat; Ar << Report.hat_census.rarest_hat;
    Ar << Report.hat_census.hats_lost; Ar << Report.hat_census.hat_survival_rate;

    int32 HlCnt = 0; Ar << HlCnt;
    for (int32 i = 0; i < HlCnt && !Ar.IsError(); ++i)
    {
        FMXHighlight Hl;
        Ar << Hl.score; Ar << Hl.narrative;
        uint8 ET = 0; Ar << ET; Hl.event.event_type = (EEventType)ET;
        Ar << Hl.event.robot_id; Ar << Hl.event.message_text;
        Report.highlights.Add(Hl);
    }

    int32 AwCnt = 0; Ar << AwCnt;
    for (int32 i = 0; i < AwCnt && !Ar.IsError(); ++i)
    {
        FMXAward Aw;
        uint8 Cat = 0; Ar << Cat; Aw.category = (EAwardCategory)Cat;
        Ar << Aw.robot_id; Ar << Aw.robot_name; Ar << Aw.narrative; Ar << Aw.stat_value;
        Report.awards.Add(Aw);
    }

    int32 DrCnt = 0; Ar << DrCnt;
    for (int32 i = 0; i < DrCnt && !Ar.IsError(); ++i)
    {
        FMXDeathEntry De;
        Ar << De.robot_profile_snapshot.name;
        Ar << De.robot_profile_snapshot.robot_id;
        Ar << De.robot_profile_snapshot.level;
        Ar << De.eulogy; Ar << De.hat_lost_note;
        Ar << De.death_event.message_text;
        Report.death_roll.Add(De);
    }

    int32 SrCnt = 0; Ar << SrCnt;
    for (int32 i = 0; i < SrCnt && !Ar.IsError(); ++i)
    {
        FMXSacrificeEntry Sr;
        Ar << Sr.robot_profile_snapshot.name;
        Ar << Sr.robot_profile_snapshot.robot_id;
        Ar << Sr.robot_profile_snapshot.level;
        Ar << Sr.extended_narrative; Ar << Sr.robots_saved;
        Report.sacrifice_honor_roll.Add(Sr);
    }

    if (Ar.Tell() < DataEnd) Ar.Seek(DataEnd);
    return Report;
}

bool UMXReportArchive::ReportMentionsRobot(const FMXRunReport& Report, const FGuid& RobotId) const
{
    for (const FMXDeathEntry& De : Report.death_roll)
        if (De.robot_profile_snapshot.robot_id == RobotId) return true;
    for (const FMXSacrificeEntry& Sr : Report.sacrifice_honor_roll)
        if (Sr.robot_profile_snapshot.robot_id == RobotId) return true;
    for (const FMXEventData& Ev : Report.run_data.events)
        if (Ev.robot_id == RobotId || Ev.related_robot_id == RobotId) return true;
    for (const FMXHighlight& Hl : Report.highlights)
        if (Hl.event.robot_id == RobotId) return true;
    return false;
}
