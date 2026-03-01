// MXSwarmGate.cpp — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm

#include "MXSwarmGate.h"
#include "MXRobotProfile.h"
#include "MXConstants.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

// ===========================================================================
// AMXSwarmGate — Base
// ===========================================================================

AMXSwarmGate::AMXSwarmGate()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    InteractionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionZone"));
    InteractionZone->SetupAttachment(Root);
    InteractionZone->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
    InteractionZone->SetCollisionProfileName(TEXT("OverlapAll"));
    InteractionZone->SetGenerateOverlapEvents(true);

    EventMessageComp = CreateDefaultSubobject<UMXEventMessageComponent>(TEXT("EventMessageComp"));
}

void AMXSwarmGate::BeginPlay()
{
    Super::BeginPlay();

    InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &AMXSwarmGate::OnInteractionZoneOverlap);
    InteractionZone->OnComponentEndOverlap.AddDynamic(this, &AMXSwarmGate::OnInteractionZoneEndOverlap);

    CachedRobotProvider = GetRobotProvider();
}

// ---------------------------------------------------------------------------
// Effective Count
// ---------------------------------------------------------------------------

float AMXSwarmGate::GetEffectiveCount(const TArray<FGuid>& RobotsOnGate) const
{
    float Total = 0.0f;
    for (const FGuid& RobotId : RobotsOnGate)
    {
        Total += GetEngineerMultiplierForRobot(RobotId);
    }
    return Total;
}

bool AMXSwarmGate::IsGateSatisfied() const
{
    return GetEffectiveCount(ContributingRobots) >= static_cast<float>(RequiredRobots);
}

float AMXSwarmGate::GetGateProgress() const
{
    float Effective = GetEffectiveCount(ContributingRobots);
    return FMath::Clamp(Effective / static_cast<float>(FMath::Max(RequiredRobots, 1)), 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// Gate State Events (default implementations — Blueprint overrides)
// ---------------------------------------------------------------------------

void AMXSwarmGate::OnGateSatisfied_Implementation()
{
    // Blueprint implements: play open animation, unlock door, etc.
}

void AMXSwarmGate::OnGateUnsatisfied_Implementation()
{
    // Blueprint implements: play close animation, relock door, etc.
}

// ---------------------------------------------------------------------------
// Overlap Handlers
// ---------------------------------------------------------------------------

void AMXSwarmGate::OnInteractionZoneOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                              bool bFromSweep, const FHitResult& SweepResult)
{
    FGuid RobotId = ExtractRobotId(OtherActor);
    if (!RobotId.IsValid()) return;

    ContributingRobots.AddUnique(RobotId);
    EvaluateGate();
}

void AMXSwarmGate::OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    FGuid RobotId = ExtractRobotId(OtherActor);
    if (!RobotId.IsValid()) return;

    ContributingRobots.Remove(RobotId);
    EvaluateGate();
}

// ---------------------------------------------------------------------------
// Gate Evaluation
// ---------------------------------------------------------------------------

void AMXSwarmGate::EvaluateGate()
{
    bool bNowSatisfied = IsGateSatisfied();

    if (bNowSatisfied && !bSatisfied)
    {
        bSatisfied = true;
        OnGateSatisfied();
    }
    else if (!bNowSatisfied && bSatisfied)
    {
        bSatisfied = false;
        OnGateUnsatisfied();
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

FGuid AMXSwarmGate::ExtractRobotId(AActor* Actor) const
{
    if (!Actor) return FGuid();
    for (const FName& Tag : Actor->Tags)
    {
        FString TagStr = Tag.ToString();
        if (TagStr.StartsWith(TEXT("RobotId:")))
        {
            FGuid OutGuid;
            if (FGuid::Parse(TagStr.RightChop(8), OutGuid)) return OutGuid;
        }
    }
    return FGuid();
}

float AMXSwarmGate::GetEngineerMultiplierForRobot(const FGuid& RobotId) const
{
    if (!CachedRobotProvider.GetObject()) return 1.0f;

    FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(
        CachedRobotProvider.GetObject(), RobotId);

    if (Profile.role != ERobotRole::Engineer) return 1.0f;

    // Tier 3 Architect mastery path = highest multiplier
    if (Profile.tier3_spec != ETier3Spec::None)
    {
        return FMath::Max(EngineerMultiplier, 3.0f); // Tier 3 = 3x minimum, up to 5x if designer sets it
    }
    // Tier 2 Engineer = 1.5x baseline
    if (Profile.tier2_spec != ETier2Spec::None)
    {
        return FMath::Max(EngineerMultiplier, 1.5f);
    }
    // Base Engineer role
    return FMath::Max(EngineerMultiplier, 1.0f);
}

TScriptInterface<IMXRobotProvider> AMXSwarmGate::GetRobotProvider() const
{
    TScriptInterface<IMXRobotProvider> Provider;
    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (GI->GetClass()->ImplementsInterface(UMXRobotProvider::StaticClass()))
        {
            Provider = TScriptInterface<IMXRobotProvider>(GI);
        }
    }
    return Provider;
}

UMXEventBus* AMXSwarmGate::GetEventBus() const
{
    // TODO: return GetWorld()->GetGameInstance()->GetSubsystem<UMXEventBusSubsystem>()->EventBus;
    return nullptr;
}

// ===========================================================================
// AMXGate_WeightPlate
// ===========================================================================

AMXGate_WeightPlate::AMXGate_WeightPlate()
{
    InteractionZone->SetBoxExtent(FVector(200.0f, 200.0f, 50.0f));
    RequiredRobots = 10;
}

void AMXGate_WeightPlate::OnGateSatisfied_Implementation()
{
    Super::OnGateSatisfied_Implementation();
    if (bLatchesOpen)
    {
        bLatched = true;
    }
}

// ===========================================================================
// AMXGate_SacrificePlate
// ===========================================================================

AMXGate_SacrificePlate::AMXGate_SacrificePlate()
{
    InteractionZone->SetBoxExtent(FVector(150.0f, 150.0f, 60.0f));
    RequiredRobots = 3;
}

void AMXGate_SacrificePlate::BeginPlay()
{
    // Do NOT call Super::BeginPlay() binding — we use our own overlap handler
    // that processes sacrifices immediately rather than adding to ContributingRobots
    AActor::BeginPlay(); // Skip AMXSwarmGate binding intentionally

    InteractionZone->OnComponentBeginOverlap.AddDynamic(
        this, &AMXGate_SacrificePlate::OnSacrificeZoneOverlap);

    CachedRobotProvider = GetRobotProvider();
}

void AMXGate_SacrificePlate::OnSacrificeZoneOverlap(UPrimitiveComponent* OverlappedComp,
                                                      AActor* OtherActor,
                                                      UPrimitiveComponent* OtherComp,
                                                      int32 OtherBodyIndex, bool bFromSweep,
                                                      const FHitResult& SweepResult)
{
    FGuid RobotId = ExtractRobotId(OtherActor);
    if (!RobotId.IsValid()) return;
    if (!bRequiresCommitInput)
    {
        ProcessSacrifice(RobotId);
    }
    // If bRequiresCommitInput, the Blueprint handles the hold-to-confirm interaction
    // and calls ProcessSacrifice() when confirmed.
}

void AMXGate_SacrificePlate::ProcessSacrifice(const FGuid& RobotId)
{
    ++SacrificesCompleted;

    // Trigger DEMS sacrifice event
    EventMessageComp->TriggerSacrificeEvent(RobotId, RobotsSavedBySacrifice);

    // Broadcast on EventBus
    FMXEventData SacrificeEvent;
    SacrificeEvent.robot_id = RobotId;
    SacrificeEvent.event_type = EEventType::Sacrifice;
    if (UMXEventBus* Bus = GetEventBus())
    {
        Bus->OnRobotSacrificed.Broadcast(RobotId, SacrificeEvent);
    }

    // Check if gate is now satisfied
    if (SacrificesCompleted >= RequiredRobots && !bSatisfied)
    {
        bSatisfied = true;
        OnGateSatisfied();
    }
}

// ===========================================================================
// AMXGate_SwarmDoor
// ===========================================================================

AMXGate_SwarmDoor::AMXGate_SwarmDoor()
{
    PrimaryActorTick.bCanEverTick = true;
    InteractionZone->SetBoxExtent(FVector(100.0f, 300.0f, 150.0f));
    RequiredRobots = 15;
}

void AMXGate_SwarmDoor::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(true);
}

void AMXGate_SwarmDoor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    float EffectiveCount = GetEffectiveCount(ContributingRobots);
    float OpenVelocity = EffectiveCount * OpenSpeedPerRobot;

    // Close if unattended and closing is enabled
    if (EffectiveCount == 0.0f && bClosesWhenUnattended)
    {
        OpenVelocity = -OpenSpeedPerRobot * RequiredRobots * 0.5f; // closes at half max speed
    }

    float PreviousProgress = OpenProgress;
    OpenProgress = FMath::Clamp(OpenProgress + (OpenVelocity / TotalOpenDistance) * DeltaTime, 0.0f, 1.0f);

    // Gate transition detection
    if (OpenProgress >= 1.0f && !bSatisfied)
    {
        bSatisfied = true;
        OnGateSatisfied();
    }
    else if (OpenProgress < 1.0f && bSatisfied)
    {
        bSatisfied = false;
        OnGateUnsatisfied();
    }
    // Blueprint drives actual door mesh position from OpenProgress each tick
}

// ===========================================================================
// AMXGate_ChainStation
// ===========================================================================

AMXGate_ChainStation::AMXGate_ChainStation()
{
    // Large interaction zone covering the entire chain span
    InteractionZone->SetBoxExtent(FVector(600.0f, 100.0f, 100.0f));
    RequiredRobots = 5;
}

bool AMXGate_ChainStation::IsChainComplete() const
{
    if (ContributingRobots.Num() < RequiredRobots) return false;

    // Gather positions (requires SwarmController reference or Blueprint position tracking)
    // For now, approximate via robot count across the span.
    // Full implementation: query SwarmController for robot positions, sort by distance from
    // StartNodePosition, verify no gap exceeds MaxLinkGap between consecutive robots.
    // This is a placeholder that treats count >= Required as the chain condition.
    return GetEffectiveCount(ContributingRobots) >= static_cast<float>(RequiredRobots);
}

bool AMXGate_ChainStation::IsGateSatisfied() const
{
    return IsChainComplete();
}

// ===========================================================================
// AMXGate_WeightBridge
// ===========================================================================

AMXGate_WeightBridge::AMXGate_WeightBridge()
{
    RequiredRobots = 15;

    CounterweightZone = CreateDefaultSubobject<UBoxComponent>(TEXT("CounterweightZone"));
    CounterweightZone->SetupAttachment(GetRootComponent());
    CounterweightZone->SetBoxExtent(FVector(150.0f, 150.0f, 60.0f));
    CounterweightZone->SetCollisionProfileName(TEXT("OverlapAll"));
    CounterweightZone->SetGenerateOverlapEvents(true);

    BridgeZone = CreateDefaultSubobject<UBoxComponent>(TEXT("BridgeZone"));
    BridgeZone->SetupAttachment(GetRootComponent());
    BridgeZone->SetBoxExtent(FVector(400.0f, 100.0f, 80.0f));
    BridgeZone->SetCollisionProfileName(TEXT("OverlapAll"));
    BridgeZone->SetGenerateOverlapEvents(true);

    PrimaryActorTick.bCanEverTick = true;
}

void AMXGate_WeightBridge::BeginPlay()
{
    Super::BeginPlay();
    // Additional overlaps for counterweight and bridge zones handled in Blueprint
    // C++ layer tracks RobotsOnBridge and RobotsOnCounterweight via bound delegates
}

void AMXGate_WeightBridge::UpdateBridgeSag()
{
    float CrossingCount = static_cast<float>(RobotsOnBridge.Num());
    float CounterweightEffective = GetEffectiveCount(RobotsOnCounterweight);
    float NetLoad = CrossingCount - CounterweightEffective;
    BridgeSagFraction = FMath::Clamp(NetLoad / static_cast<float>(FMath::Max(RequiredRobots, 1)), 0.0f, 1.0f);
}

// ===========================================================================
// AMXGate_OverrideStation
// ===========================================================================

AMXGate_OverrideStation::AMXGate_OverrideStation()
{
    InteractionZone->SetBoxExtent(FVector(100.0f, 100.0f, 80.0f));
    RequiredRobots = 10;
}

void AMXGate_OverrideStation::OnGateSatisfied_Implementation()
{
    // Only fire the shared event if ALL partners are also satisfied
    if (AreAllPartnersSatisfied())
    {
        // Notify all partner stations that the combined override succeeded
        for (AMXGate_OverrideStation* Partner : PartnerStations)
        {
            if (Partner && Partner->IsThisStationSatisfied())
            {
                Partner->bSatisfied = true; // confirm — prevents double fire
            }
        }
        // Blueprint listens to this native event and activates the shared mechanism
        Super::OnGateSatisfied_Implementation();
    }
}

void AMXGate_OverrideStation::OnGateUnsatisfied_Implementation()
{
    // If any station is unsatisfied, the mechanism closes
    Super::OnGateUnsatisfied_Implementation();

    // Notify partners the combined override is broken
    for (AMXGate_OverrideStation* Partner : PartnerStations)
    {
        if (Partner && Partner->bSatisfied)
        {
            Partner->bSatisfied = false;
            Partner->OnGateUnsatisfied();
        }
    }
}

bool AMXGate_OverrideStation::AreAllPartnersSatisfied() const
{
    for (const AMXGate_OverrideStation* Partner : PartnerStations)
    {
        if (!Partner || !Partner->IsThisStationSatisfied()) return false;
    }
    return true;
}
