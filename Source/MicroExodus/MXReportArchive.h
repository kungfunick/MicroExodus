// MXReportArchive.h — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Consumers: MXSaveManager (internal to Persistence module)
// Change Log:
//   v1.0 — Initial implementation: run report archive with old-report compression and all-time records

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXRunData.h"
#include "MXEventData.h"
#include "MXReportArchive.generated.h"

// ---------------------------------------------------------------------------
// All-Time Records
// ---------------------------------------------------------------------------

/**
 * FMXAllTimeRecords
 * Campaign-wide records accumulated across all runs.
 * Serialized alongside the report archive.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXAllTimeRecords
{
    GENERATED_BODY()

    /** Best run-end survival rate (robots_survived / robots_deployed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BestSurvivalRate = 0.0f;

    /** Run number that set BestSurvivalRate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BestSurvivalRunNumber = 0;

    /** Highest score achieved in a single run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HighestScore = 0;

    /** Run number that set HighestScore. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HighestScoreRunNumber = 0;

    /** Most hats deployed simultaneously in a single run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MostHatsDeployed = 0;

    /** Most near-misses accumulated by a single robot in any run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MostNearMissesSingleRobot = 0;

    /** Name of the robot that holds MostNearMissesSingleRobot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MostNearMissesRobotName;

    /** Most robot deaths in a single run (not counting sacrifices). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MostDeathsSingleRun = 0;

    /** Longest streak of consecutive runs with zero losses. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LongestPerfectStreak = 0;

    /** Current ongoing streak of consecutive runs with zero losses. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentPerfectStreak = 0;

    /** Total robots lost across all runs, all time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalRobotsLostAllTime = 0;

    /** Total robots rescued across all runs, all time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalRobotsRescuedAllTime = 0;

    /** Total hats lost to robot deaths across all runs, all time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalHatsLostAllTime = 0;
};

// ---------------------------------------------------------------------------
// UMXReportArchive
// ---------------------------------------------------------------------------

/**
 * UMXReportArchive
 * Serializes the run report archive and all-time records for disk persistence.
 *
 * Compression policy:
 *   - The most recent KeepFullCount reports are stored in full (all event arrays intact).
 *   - Reports older than KeepFullCount have their raw run_data.events array pruned.
 *   - All narrative text (opening, highlights, awards, death roll, sacrifice roll, closing)
 *     is ALWAYS preserved regardless of age. The player can always re-read any past run's story.
 *
 * Reports are indexed by run_number for random access (O(n) linear scan over the archive;
 * at 100 runs this is negligible).
 */
UCLASS()
class MICROEXODUS_API UMXReportArchive : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Report Array Serialization
    // -------------------------------------------------------------------------

    /**
     * Serialize the full run report archive to the archive.
     * Applies compression to old reports before writing.
     *
     * @param Ar       FArchive opened for writing.
     * @param Reports  Full array of run reports, newest last.
     */
    void SerializeReports(FArchive& Ar, const TArray<FMXRunReport>& Reports);

    /**
     * Deserialize the run report archive from the archive.
     *
     * @param Ar  FArchive opened for reading.
     * @return    Array of run reports in the order they were written.
     */
    TArray<FMXRunReport> DeserializeReports(FArchive& Ar);

    // -------------------------------------------------------------------------
    // Compression
    // -------------------------------------------------------------------------

    /**
     * Prune raw event arrays from old reports to reduce disk footprint.
     * Reports with index < (Reports.Num() - KeepFullCount) are compressed.
     * Narrative text, stats, highlights, awards, death roll, sacrifice roll, and closing
     * are all preserved.
     *
     * @param Reports        The report array to compress in-place.
     * @param KeepFullCount  Number of most-recent reports to leave uncompressed (default: 50).
     */
    void CompressOldReports(TArray<FMXRunReport>& Reports, int32 KeepFullCount = 50);

    // -------------------------------------------------------------------------
    // Lookup Helpers
    // -------------------------------------------------------------------------

    /**
     * Find a run report by run number. O(n) scan.
     *
     * @param Reports    The archive to search.
     * @param RunNumber  The 1-indexed run number to locate.
     * @return           Pointer to the matching report, or nullptr if not found.
     */
    FMXRunReport* GetReportByRunNumber(TArray<FMXRunReport>& Reports, int32 RunNumber);

    /**
     * Return run numbers of every report that mentions a given robot.
     * A robot "appears" in a report if it is in the death roll, sacrifice roll, or
     * the run_data.events array (for compressed reports, only narrative sections are checked).
     *
     * @param Reports  The archive to search.
     * @param RobotId  The robot GUID to look up.
     * @return         Sorted array of run numbers.
     */
    TArray<int32> GetReportsForRobot(const TArray<FMXRunReport>& Reports, const FGuid& RobotId);

    // -------------------------------------------------------------------------
    // All-Time Records Serialization
    // -------------------------------------------------------------------------

    /**
     * Serialize the all-time records struct.
     *
     * @param Ar       FArchive opened for writing.
     * @param Records  The records to write.
     */
    void SerializeAllTimeRecords(FArchive& Ar, const FMXAllTimeRecords& Records);

    /**
     * Deserialize the all-time records struct.
     *
     * @param Ar  FArchive opened for reading.
     * @return    Reconstructed FMXAllTimeRecords.
     */
    FMXAllTimeRecords DeserializeAllTimeRecords(FArchive& Ar);

private:

    /** Serialize a single FMXRunReport into the archive. */
    void SerializeReport(FArchive& Ar, const FMXRunReport& Report);

    /** Deserialize a single FMXRunReport from the archive. */
    FMXRunReport DeserializeReport(FArchive& Ar);

    /** True if a robot GUID appears anywhere in the report's narrative sections. */
    bool ReportMentionsRobot(const FMXRunReport& Report, const FGuid& RobotId) const;
};
