// MXTemplateSelector.cpp — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS

#include "MXTemplateSelector.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// Static paths to Content/DataTables assets
// ---------------------------------------------------------------------------

const TCHAR* UMXTemplateSelector::DeathTablePath    = TEXT("/Game/DataTables/DT_DeathTemplates");
const TCHAR* UMXTemplateSelector::RescueTablePath   = TEXT("/Game/DataTables/DT_RescueTemplates");
const TCHAR* UMXTemplateSelector::SacrificeTablePath= TEXT("/Game/DataTables/DT_SacrificeTemplates");
const TCHAR* UMXTemplateSelector::NearMissTablePath = TEXT("/Game/DataTables/DT_NearMissTemplates");

const FString UMXTemplateSelector::FallbackTemplate = TEXT("{name} was {verb} {preposition} a {source}. Nobody seemed particularly surprised.");

// ---------------------------------------------------------------------------

UMXTemplateSelector::UMXTemplateSelector()
{
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

ESeverity UMXTemplateSelector::ParseSeverity(const FString& SeverityStr) const
{
    if (SeverityStr.Equals(TEXT("Major"),    ESearchCase::IgnoreCase)) return ESeverity::Major;
    if (SeverityStr.Equals(TEXT("Dramatic"), ESearchCase::IgnoreCase)) return ESeverity::Dramatic;
    return ESeverity::Minor;
}

FString UMXTemplateSelector::PickRandom(const TArray<FMXTemplateRow*>& Eligible) const
{
    if (Eligible.IsEmpty()) return FString();
    const int32 Idx = FMath::RandRange(0, Eligible.Num() - 1);
    return Eligible[Idx]->TemplateText;
}

TArray<FMXTemplateRow*> UMXTemplateSelector::FilterPool(
    const TArray<FMXTemplateRow*>& Pool,
    bool bHasHat,
    bool bHasSpec,
    ESeverity Severity,
    bool bReqHat,
    bool bReqSpec) const
{
    TArray<FMXTemplateRow*> Out;
    for (FMXTemplateRow* Row : Pool)
    {
        if (!Row) continue;
        // Hat and spec requirement must match exactly for this tier.
        if (Row->bRequiresHat  != bReqHat)  continue;
        if (Row->bRequiresSpec != bReqSpec)  continue;
        // Severity must meet the minimum threshold.
        if (ParseSeverity(Row->MinSeverity) > Severity) continue;
        Out.Add(Row);
    }
    return Out;
}

// ---------------------------------------------------------------------------
// LoadTemplates
// ---------------------------------------------------------------------------

void UMXTemplateSelector::LoadTemplates()
{
    struct FTableEntry
    {
        const TCHAR* Path;
        EEventType   EventType;
    };

    const TArray<FTableEntry> Entries =
    {
        { DeathTablePath,     EEventType::Death     },
        { RescueTablePath,    EEventType::Rescue    },
        { SacrificeTablePath, EEventType::Sacrifice },
        { NearMissTablePath,  EEventType::NearMiss  },
    };

    for (const FTableEntry& Entry : Entries)
    {
        UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, Entry.Path));
        if (!Table)
        {
            UE_LOG(LogTemp, Error, TEXT("[DEMS] Failed to load template DataTable: %s"), Entry.Path);
            continue;
        }

        LoadedTables.Add(Table);

        TArray<FMXTemplateRow*> Pool;
        TArray<FName> RowNames = Table->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            FMXTemplateRow* Row = Table->FindRow<FMXTemplateRow>(RowName, TEXT("MXTemplateSelector::LoadTemplates"));
            if (Row)
            {
                Pool.Add(Row);
            }
        }

        TemplatePools.Add(Entry.EventType, Pool);
        UE_LOG(LogTemp, Log, TEXT("[DEMS] Loaded %d templates for event type %d from %s"),
               Pool.Num(), (int32)Entry.EventType, Entry.Path);
    }
}

// ---------------------------------------------------------------------------
// SelectTemplate
// ---------------------------------------------------------------------------

FString UMXTemplateSelector::SelectTemplate(EEventType EventType, bool bHasHat, bool bHasSpec, ESeverity Severity) const
{
    const TArray<FMXTemplateRow*>* PoolPtr = TemplatePools.Find(EventType);
    if (!PoolPtr || PoolPtr->IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEMS] No templates found for event type %d — using fallback."), (int32)EventType);
        return FallbackTemplate;
    }

    const TArray<FMXTemplateRow*>& Pool = *PoolPtr;

    // Tier 1: Has both hat AND spec.
    if (bHasHat && bHasSpec)
    {
        TArray<FMXTemplateRow*> Tier1 = FilterPool(Pool, bHasHat, bHasSpec, Severity, true, true);
        if (!Tier1.IsEmpty()) return PickRandom(Tier1);
    }

    // Tier 2: Has hat only.
    if (bHasHat)
    {
        TArray<FMXTemplateRow*> Tier2 = FilterPool(Pool, bHasHat, bHasSpec, Severity, true, false);
        if (!Tier2.IsEmpty()) return PickRandom(Tier2);
    }

    // Tier 3: Has spec only.
    if (bHasSpec)
    {
        TArray<FMXTemplateRow*> Tier3 = FilterPool(Pool, bHasHat, bHasSpec, Severity, false, true);
        if (!Tier3.IsEmpty()) return PickRandom(Tier3);
    }

    // Tier 4: Base templates — no hat, no spec requirements.
    TArray<FMXTemplateRow*> Base = FilterPool(Pool, bHasHat, bHasSpec, Severity, false, false);
    if (!Base.IsEmpty()) return PickRandom(Base);

    // Nothing matched — fall back to absolute default.
    UE_LOG(LogTemp, Warning, TEXT("[DEMS] No eligible templates after all filter tiers for event %d — using fallback."), (int32)EventType);
    return FallbackTemplate;
}

// ---------------------------------------------------------------------------
// GetTemplateCount
// ---------------------------------------------------------------------------

int32 UMXTemplateSelector::GetTemplateCount(EEventType EventType) const
{
    const TArray<FMXTemplateRow*>* PoolPtr = TemplatePools.Find(EventType);
    return PoolPtr ? PoolPtr->Num() : 0;
}
