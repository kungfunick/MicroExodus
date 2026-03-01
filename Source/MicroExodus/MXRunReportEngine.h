// MXRunReportEngine.h — Reports Module, Version 1.0
// Created: 2026-02-22
// Agent 9: Reports
// Consumers: Persistence (receives FMXRunReport), UI (displays FMXRunReport)
// Change Log:
//   v1.0 — Master pipeline orchestrator for run report generation

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "MXInterfaces.h"
#include "MXRunData.h"
#include "MXStatCompiler.h"
#include "MXHighlightScorer.h"
#include "MXAwardSelector.h"
#include "MXReportNarrator.h"
#include "MXEvents.h"
#include "MXRunReportEngine.generated.h"

/**
 * UMXRunReportEngine
 * The master orchestrator for run report generation.
 *
 * Pipeline (invoked once at run end via OnRunComplete / OnRunFailed):
 *   1. EVENT LOG FREEZE  — RunData is finalized; we hold a const reference.
 *   2. STAT COMPILATION  — MXStatCompiler single-pass aggregation.
 *   3. HIGHLIGHT SCORING — MXHighlightScorer ranks events, extracts top 10.
 *   4. AWARD SELECTION   — MXAwardSelector evaluates all 16 categories.
 *   5. NARRATIVE FILL    — MXReportNarrator generates all text sections.
 *   6. HAT CENSUS        — FMXHatCensus populated from compiled stats.
 *   7. DEATH ROLL        — Eulogies written for each fallen robot.
 *   8. SACRIFICE ROLL    — Extended narratives for sacrificed robots.
 *   9. PACKAGE           — Assembled into FMXRunReport and returned.
 *
 * Performance targets: <500ms stat compilation, <200ms template filling, <1s total.
 *
 * Implements IMXPersistable for report archive serialization.
 */
UCLASS()
class UMXRunReportEngine : public UObject, public IMXPersistable
{
    GENERATED_BODY()

public:
    // ---------------------------------------------------------------------------
    // Lifecycle
    // ---------------------------------------------------------------------------

    /**
     * Initialize the engine and all sub-systems. Load DataTables. Bind to event bus.
     * Must be called once after construction (typically from GameInstance or Roguelike subsystem).
     *
     * @param RobotProvider   IMXRobotProvider implementation (Identity module).
     * @param HatProvider     IMXHatProvider implementation (Hats module).
     * @param RunProvider     IMXRunProvider implementation (Roguelike module).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Reports")
    void Initialize(
        TScriptInterface<IMXRobotProvider> RobotProvider,
        TScriptInterface<IMXHatProvider>   HatProvider,
        TScriptInterface<IMXRunProvider>   RunProvider
    );

    /** Extended initialize called by GameInstance — accepts EventBus and pre-loaded DataTables directly. */
    void Initialize(
        UMXEventBus*                       InEventBus,
        TScriptInterface<IMXRobotProvider> RobotProvider,
        TScriptInterface<IMXHatProvider>   HatProvider,
        TScriptInterface<IMXRunProvider>   RunProvider,
        UDataTable*                        OpeningTemplates,
        UDataTable*                        HighlightTemplates,
        UDataTable*                        AwardTemplates,
        UDataTable*                        EulogyTemplates,
        UDataTable*                        ClosingTemplates
    );

    /** Unbind delegates and release references. Called by GameInstance on shutdown. */
    UFUNCTION(BlueprintCallable, Category = "MX|Reports")
    void Shutdown();

    // ---------------------------------------------------------------------------
    // Main Entry Point
    // ---------------------------------------------------------------------------

    /**
     * Generate the complete run report from a finalized FMXRunData.
     * This is the only public generation method — called by the OnRunComplete / OnRunFailed handlers.
     *
     * @param RunData           Finalized run record (frozen at run end, never mutated here).
     * @param bRunFailed        True if the run ended in failure; affects narrative tone.
     * @param NextRunNumber     Used for closing narrative forward-tease.
     * @param HatCollectionCount Total hats in the player's collection after this run.
     * @param NewHatsUnlocked   Number of new hat types unlocked this run.
     * @return                  Complete FMXRunReport ready for UI display and persistence.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Reports")
    FMXRunReport GenerateReport(
        const FMXRunData& RunData,
        bool bRunFailed,
        int32 NextRunNumber,
        int32 HatCollectionCount,
        int32 NewHatsUnlocked
    );

    // ---------------------------------------------------------------------------
    // IMXPersistable
    // ---------------------------------------------------------------------------

    virtual void MXSerialize(FArchive& Ar) override;
    virtual void MXDeserialize(FArchive& Ar) override;

    // ---------------------------------------------------------------------------
    // Event Handlers (bound during Initialize)
    // ---------------------------------------------------------------------------

    UFUNCTION()
    void HandleRunComplete(FMXRunData RunData);

    UFUNCTION()
    void HandleRunFailed(FMXRunData RunData, int32 FailureLevel);

    // ---------------------------------------------------------------------------
    // Last Generated Report (cache for UI)
    // ---------------------------------------------------------------------------

    /** Returns the most recently generated report. Valid after first GenerateReport() call. */
    UFUNCTION(BlueprintCallable, Category = "MX|Reports")
    const FMXRunReport& GetLastReport() const { return LastReport; }

private:
    // ---------------------------------------------------------------------------
    // Sub-Systems
    // ---------------------------------------------------------------------------

    UPROPERTY() UMXStatCompiler*    StatCompiler   = nullptr;
    UPROPERTY() UMXHighlightScorer* HighlightScorer = nullptr;
    UPROPERTY() UMXAwardSelector*   AwardSelector   = nullptr;
    UPROPERTY() UMXReportNarrator*  Narrator        = nullptr;

    // ---------------------------------------------------------------------------
    // Provider References (not owned — injected at Initialize)
    // ---------------------------------------------------------------------------

    UPROPERTY() TScriptInterface<IMXRobotProvider> RobotProviderRef;
    UPROPERTY() TScriptInterface<IMXHatProvider>   HatProviderRef;
    UPROPERTY() TScriptInterface<IMXRunProvider>   RunProviderRef;

    // ---------------------------------------------------------------------------
    // DataTable Assets
    // ---------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "DataTables")
    TSoftObjectPtr<UDataTable> DT_ReportOpeningTemplates;

    UPROPERTY(EditAnywhere, Category = "DataTables")
    TSoftObjectPtr<UDataTable> DT_ReportHighlightTemplates;

    UPROPERTY(EditAnywhere, Category = "DataTables")
    TSoftObjectPtr<UDataTable> DT_AwardTemplates;

    UPROPERTY(EditAnywhere, Category = "DataTables")
    TSoftObjectPtr<UDataTable> DT_EulogyTemplates;

    UPROPERTY(EditAnywhere, Category = "DataTables")
    TSoftObjectPtr<UDataTable> DT_ReportClosingTemplates;

    // ---------------------------------------------------------------------------
    // State
    // ---------------------------------------------------------------------------

    FMXRunReport LastReport;

    bool bInitialized = false;

    // ---------------------------------------------------------------------------
    // Pipeline Steps (called in order by GenerateReport)
    // ---------------------------------------------------------------------------

    /** Step 2: Compile stats from the frozen event log. */
    FMXCompiledStats StepCompileStats(
        const FMXRunData& RunData,
        const TArray<FMXRobotProfile>& AllProfiles
    );

    /** Step 3: Score events and produce sorted highlight array. */
    TArray<FMXHighlight> StepScoreHighlights(
        const FMXRunData& RunData,
        const FMXCompiledStats& Stats
    );

    /** Step 4: Evaluate awards. */
    TArray<FMXAward> StepSelectAwards(
        const FMXCompiledStats& Stats,
        const TArray<FMXRobotProfile>& AllProfiles
    );

    /** Step 5: Fill highlight narratives in-place. */
    void StepNarrateHighlights(
        TArray<FMXHighlight>& Highlights,
        const FMXCompiledStats& Stats
    );

    /** Step 5b: Fill award narratives in-place. */
    void StepNarrateAwards(
        TArray<FMXAward>& Awards,
        const TArray<FMXRobotProfile>& AllProfiles,
        const FMXCompiledStats& Stats
    );

    /** Step 6: Compile hat census from stats. */
    FMXHatCensus StepCompileHatCensus(const FMXCompiledStats& Stats);

    /** Step 7: Build death roll — eulogies for all fallen robots. */
    TArray<FMXDeathEntry> StepBuildDeathRoll(
        const FMXRunData& RunData,
        const TArray<FMXRobotProfile>& AllProfiles
    );

    /** Step 8: Build sacrifice honor roll. */
    TArray<FMXSacrificeEntry> StepBuildSacrificeRoll(
        const FMXRunData& RunData,
        const TArray<FMXRobotProfile>& AllProfiles
    );

    /** Load a soft DataTable reference, returning nullptr if not loaded. */
    UDataTable* LoadDT(TSoftObjectPtr<UDataTable>& SoftRef);
};
