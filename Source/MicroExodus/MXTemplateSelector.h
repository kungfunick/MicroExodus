// MXTemplateSelector.h — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS
// Loads and manages the four template DataTables, then selects an appropriate
// template row given the event type, robot state, and narrative severity.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "MXTypes.h"
#include "MXTemplateSelector.generated.h"

// ---------------------------------------------------------------------------
// FMXTemplateRow
// One row in any of the four *Templates DataTables.
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXTemplateRow : public FTableRowBase
{
    GENERATED_BODY()

    /** The full template string with token placeholders (e.g., {name}, {verb}). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString TemplateText;

    /** If true, this template is only eligible when the robot is currently wearing a hat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresHat = false;

    /** If true, this template is only eligible when the robot has a specialization (Tier 2+). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresSpec = false;

    /**
     * The minimum narrative severity needed for this template to be eligible.
     * Stored as FString ("Minor" / "Major" / "Dramatic") to match CSV import format.
     * Compared at runtime against the event's ESeverity.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MinSeverity = TEXT("Minor");
};

// ---------------------------------------------------------------------------
// UMXTemplateSelector
// Manages all four template pools and provides random-weighted selection.
// ---------------------------------------------------------------------------

UCLASS(BlueprintType)
class MICROEXODUS_API UMXTemplateSelector : public UObject
{
    GENERATED_BODY()

public:

    UMXTemplateSelector();

    /**
     * Load all four template DataTables from the Content/DataTables directory.
     * Must be called once at subsystem initialization before any events fire.
     * Logs an error if any table fails to load; selection will fall back to hardcoded defaults.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Templates")
    void LoadTemplates();

    /**
     * Select one template string from the appropriate pool, filtered to the
     * robot's current state (hat/spec) and the event's severity.
     *
     * Priority ordering:
     *   1. bRequiresHat && bRequiresSpec (both)
     *   2. bRequiresHat only
     *   3. bRequiresSpec only
     *   4. Base templates (no requirements)
     * Falls through to the next tier if the preferred tier has no eligible rows.
     *
     * @param EventType   The class of event (Death, Rescue, Sacrifice, NearMiss).
     * @param bHasHat     True if the robot is currently wearing a hat.
     * @param bHasSpec    True if the robot has a Tier 2 or higher specialization.
     * @param Severity    The event's narrative weight.
     * @return            A template string ready for token substitution.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Templates")
    FString SelectTemplate(EEventType EventType, bool bHasHat, bool bHasSpec, ESeverity Severity) const;

    /**
     * Returns how many templates are loaded for a given event type.
     * Useful for validation and tests.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Templates")
    int32 GetTemplateCount(EEventType EventType) const;

private:

    /** Converts the MinSeverity FString from the CSV row to an ESeverity enum. */
    ESeverity ParseSeverity(const FString& SeverityStr) const;

    /**
     * Internal selection helper. Given a filtered list of eligible rows,
     * returns a random element. Returns an empty string if the list is empty.
     */
    FString PickRandom(const TArray<FMXTemplateRow*>& Eligible) const;

    /**
     * Filter the pool for a given event type to rows that satisfy severity, hat, and spec filters.
     * @param Pool      The full row list for an event type.
     * @param bHasHat   Whether the robot has a hat.
     * @param bHasSpec  Whether the robot has a spec.
     * @param Severity  The minimum severity threshold.
     * @param bReqHat   Filter: only include rows with bRequiresHat == bReqHat.
     * @param bReqSpec  Filter: only include rows with bRequiresSpec == bReqSpec.
     */
    TArray<FMXTemplateRow*> FilterPool(
        const TArray<FMXTemplateRow*>& Pool,
        bool bHasHat,
        bool bHasSpec,
        ESeverity Severity,
        bool bReqHat,
        bool bReqSpec) const;

    // Template pools keyed by EEventType. Populated by LoadTemplates().
    TMap<EEventType, TArray<FMXTemplateRow*>> TemplatePools;

    // Soft object paths to the four DataTables — resolved at LoadTemplates() time.
    static const TCHAR* DeathTablePath;
    static const TCHAR* RescueTablePath;
    static const TCHAR* SacrificeTablePath;
    static const TCHAR* NearMissTablePath;

    // Holds strong references to keep tables alive after load.
    UPROPERTY()
    TArray<UDataTable*> LoadedTables;

    // Fallback template used when all pools are empty (should never happen in shipping).
    static const FString FallbackTemplate;
};
