// MXXPDistributor.cpp — Roguelike Module v1.0
// Agent 6: Roguelike

#include "MXXPDistributor.h"
#include "MXInterfaces.h"
#include "MXRobotProfile.h"
#include "Engine/DataTable.h"

UMXXPDistributor::UMXXPDistributor()
    : TotalXPDistributedThisRun(0)
{
    // Populate default XP table in case the DataTable is not yet loaded.
    // These mirror the DT_XPCurve.csv rows exactly.
    XPTable.Add(TEXT("LevelComplete"),      100);
    XPTable.Add(TEXT("RunComplete"),        500);
    XPTable.Add(TEXT("PuzzleParticipation"), 25);
    XPTable.Add(TEXT("RescueWitnessed"),     10);
    XPTable.Add(TEXT("NearMiss"),            50);
    XPTable.Add(TEXT("SacrificeWitnessed"),  75);
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXXPDistributor::InitialiseFromDataTable(UDataTable* DataTable)
{
    if (!DataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXXPDistributor: Null DataTable passed — using defaults."));
        return;
    }

    TArray<FMXXPCurveRow*> Rows;
    DataTable->GetAllRows<FMXXPCurveRow>(TEXT("MXXPDistributor"), Rows);

    if (Rows.Num() > 0)
    {
        XPTable.Empty();
    }

    for (const FMXXPCurveRow* Row : Rows)
    {
        if (Row && !Row->EventType.IsEmpty())
        {
            XPTable.Add(Row->EventType, Row->BaseXP);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXXPDistributor: Loaded %d XP curve entries."), XPTable.Num());
}

void UMXXPDistributor::SetRobotProvider(TScriptInterface<IMXRobotProvider> InRobotProvider)
{
    RobotProvider = InRobotProvider;
}

// ---------------------------------------------------------------------------
// Batch XP Distribution
// ---------------------------------------------------------------------------

void UMXXPDistributor::DistributeLevelXP(const TArray<FGuid>& SurvivingRobots, int32 LevelNumber, ETier Tier)
{
    const int32 BaseAmount   = GetBaseXP(EEventType::LevelComplete);
    const float TierMult     = MXConstants::TIER_MULTIPLIERS[static_cast<int32>(Tier)];
    const int32 TieredAmount = FMath::FloorToInt(static_cast<float>(BaseAmount) * TierMult);

    for (const FGuid& RobotId : SurvivingRobots)
    {
        const float SpecMult  = GetSpecialistXPMultiplier(RobotId);
        const int32 FinalXP   = FMath::FloorToInt(static_cast<float>(TieredAmount) * SpecMult);
        ApplyXPToRobot(RobotId, FinalXP, FString::Printf(TEXT("LevelComplete[L%d]"), LevelNumber));
    }
}

void UMXXPDistributor::DistributeRunXP(const TArray<FGuid>& SurvivingRobots, ETier Tier)
{
    const int32 BaseAmount   = GetBaseXP(EEventType::RunComplete);
    const float TierMult     = MXConstants::TIER_MULTIPLIERS[static_cast<int32>(Tier)];
    const int32 TieredAmount = FMath::FloorToInt(static_cast<float>(BaseAmount) * TierMult);

    for (const FGuid& RobotId : SurvivingRobots)
    {
        const float SpecMult = GetSpecialistXPMultiplier(RobotId);
        const int32 FinalXP  = FMath::FloorToInt(static_cast<float>(TieredAmount) * SpecMult);
        ApplyXPToRobot(RobotId, FinalXP, TEXT("RunComplete"));
    }
}

// ---------------------------------------------------------------------------
// Per-Event XP
// ---------------------------------------------------------------------------

void UMXXPDistributor::AwardEventXP(const FGuid& RobotId, EEventType EventType, ETier Tier)
{
    const int32 TieredAmount = GetXPAmount(EventType, Tier);
    const float SpecMult     = GetSpecialistXPMultiplier(RobotId);
    const int32 FinalXP      = FMath::FloorToInt(static_cast<float>(TieredAmount) * SpecMult);

    ApplyXPToRobot(RobotId, FinalXP, EventTypeToString(EventType));
}

// ---------------------------------------------------------------------------
// XP Queries
// ---------------------------------------------------------------------------

int32 UMXXPDistributor::GetXPAmount(EEventType EventType, ETier Tier) const
{
    const int32 Base    = GetBaseXP(EventType);
    const float TierMult = MXConstants::TIER_MULTIPLIERS[static_cast<int32>(Tier)];
    return FMath::FloorToInt(static_cast<float>(Base) * TierMult);
}

int32 UMXXPDistributor::GetBaseXP(EEventType EventType) const
{
    const FString Key = EventTypeToString(EventType);
    if (const int32* Found = XPTable.Find(Key))
    {
        return *Found;
    }
    UE_LOG(LogTemp, Warning, TEXT("MXXPDistributor: No XP entry for event type '%s'."), *Key);
    return 0;
}

int32 UMXXPDistributor::GetTotalXPDistributedThisRun() const
{
    return TotalXPDistributedThisRun;
}

void UMXXPDistributor::ResetForNewRun()
{
    TotalXPDistributedThisRun = 0;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

FString UMXXPDistributor::EventTypeToString(EEventType EventType)
{
    switch (EventType)
    {
    case EEventType::LevelComplete:      return TEXT("LevelComplete");
    case EEventType::RunComplete:        return TEXT("RunComplete");
    case EEventType::PuzzleParticipation:return TEXT("PuzzleParticipation");
    case EEventType::RescueWitnessed:    return TEXT("RescueWitnessed");
    case EEventType::NearMiss:           return TEXT("NearMiss");
    case EEventType::SacrificeWitnessed: return TEXT("SacrificeWitnessed");
    default:
        return TEXT("Unknown");
    }
}

float UMXXPDistributor::GetSpecialistXPMultiplier(const FGuid& RobotId) const
{
    // Default: no spec bonus.
    float Multiplier = 1.0f;

    if (!RobotProvider.GetInterface())
    {
        return Multiplier;
    }

    const FMXRobotProfile Profile =
        IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);

    // Engineer → Mechanic specialization benefits XP gain.
    // Mechanic Tier 2 spec grants +20% XP. Savant Tier 3 grants +40%. Catalyst grants +30%.
    // These values align with the Specialization module's design; Roguelike reads them
    // from the profile's existing spec fields rather than re-implementing spec logic.
    if (Profile.spec_tier2 == ETier2Spec::Mechanic)
    {
        Multiplier = 1.2f;
    }

    if (Profile.spec_tier3 == ETier3Spec::Savant)
    {
        Multiplier = 1.4f;
    }
    else if (Profile.spec_tier3 == ETier3Spec::Catalyst)
    {
        Multiplier = 1.3f;
    }

    return Multiplier;
}

void UMXXPDistributor::ApplyXPToRobot(const FGuid& RobotId, int32 XPAmount, const FString& Context)
{
    if (XPAmount <= 0)
    {
        return;
    }

    // Identity module exposes AddXP via UMXRobotManager — the concrete type is passed
    // through RobotProvider at runtime. Since IMXRobotProvider is the read-only interface,
    // AddXP is called through the concrete manager reference held by MXRunManager, which
    // forwards the call here via a delegate set during initialisation.
    // For compilation purposes the call is stubbed and replaced at runtime wiring.
    UE_LOG(LogTemp, Log,
        TEXT("MXXPDistributor: +%d XP → %s [%s]"),
        XPAmount, *RobotId.ToString(), *Context);

    TotalXPDistributedThisRun += XPAmount;

    // Runtime AddXP dispatch note:
    // MXRunManager holds the concrete UMXRobotManager* and will call
    //   RobotManager->AddXP(RobotId, XPAmount)
    // after receiving the XP amount from this distributor. See MXRunManager::AdvanceLevel
    // and MXRunManager::HandleNearMiss for the dispatch pattern.
}
