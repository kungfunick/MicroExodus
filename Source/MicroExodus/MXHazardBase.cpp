// MXHazardBase.cpp — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm

#include "MXHazardBase.h"
#include "MXConstants.h"
#include "MXSpecData.h"
#include "MXRobotProfile.h"
#include "MXTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Forward Declaration: EventBus Subsystem lookup
// (Full subsystem header omitted here; access pattern matches established contracts)
// ---------------------------------------------------------------------------
// UMXEventBusSubsystem is expected at:
//   GetWorld()->GetGameInstance()->GetSubsystem<UMXEventBusSubsystem>()
// and exposes: UMXEventBus* EventBus

// ---------------------------------------------------------------------------
// AMXHazardBase — Constructor
// ---------------------------------------------------------------------------

AMXHazardBase::AMXHazardBase()
{
    PrimaryActorTick.bCanEverTick = false; // timer-driven, not tick-driven

    // Root
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // Kill volume
    KillVolume = CreateDefaultSubobject<USphereComponent>(TEXT("KillVolume"));
    KillVolume->SetupAttachment(Root);
    KillVolume->SetSphereRadius(60.0f);
    KillVolume->SetCollisionProfileName(TEXT("OverlapAll"));
    KillVolume->SetGenerateOverlapEvents(true);

    // Warning volume — slightly larger
    WarningVolume = CreateDefaultSubobject<USphereComponent>(TEXT("WarningVolume"));
    WarningVolume->SetupAttachment(Root);
    WarningVolume->SetSphereRadius(60.0f + 200.0f); // KillRadius + WarningBuffer
    WarningVolume->SetCollisionProfileName(TEXT("OverlapAll"));
    WarningVolume->SetGenerateOverlapEvents(true);

    // DEMS component
    EventMessageComp = CreateDefaultSubobject<UMXEventMessageComponent>(TEXT("EventMessageComp"));
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXHazardBase::BeginPlay()
{
    Super::BeginPlay();

    // Sync component sizes to designer properties
    KillVolume->SetSphereRadius(KillRadius);
    WarningVolume->SetSphereRadius(KillRadius + WarningBuffer);

    // Bind collision delegates
    KillVolume->OnComponentBeginOverlap.AddDynamic(this, &AMXHazardBase::OnKillVolumeOverlap);
    KillVolume->OnComponentEndOverlap.AddDynamic(this, &AMXHazardBase::OnKillVolumeEndOverlap);

    WarningVolume->OnComponentBeginOverlap.AddDynamic(this, &AMXHazardBase::OnWarningVolumeOverlap);
    WarningVolume->OnComponentEndOverlap.AddDynamic(this, &AMXHazardBase::OnWarningVolumeEndOverlap);

    // Sync EventMessageComp fields
    EventMessageComp->EventFields = HazardFields;

    // Cache robot provider
    CachedRobotProvider = GetRobotProvider();

    // Pre-populate spec caches from all existing robots if provider is available
    if (CachedRobotProvider.GetObject())
    {
        TArray<FMXRobotProfile> AllProfiles = IMXRobotProvider::Execute_GetAllRobotProfiles(CachedRobotProvider.GetObject());
        for (const FMXRobotProfile& Profile : AllProfiles)
        {
            // Guardian/Sentinel — lethal_hits_per_run isn't in RobotProfile directly,
            // it lives in FMXSpecBonus (computed by Specialization module).
            // We approximate: Guardian role = 1 shield charge; Sentinel tier2 = 2; Immortal Guard tier3 = 3.
            // The Specialization module would normally inject FMXSpecBonus — for now, derive from profile.
            int32 Shields = 0;
            if (Profile.role == ERobotRole::Guardian)
            {
                if (Profile.tier3_spec == ETier3Spec::ImmortalGuard)       Shields = 3;
                else if (Profile.tier2_spec == ETier2Spec::Sentinel)       Shields = 2;
                else                                                         Shields = 1;
            }
            if (Shields > 0) ShieldCharges.Add(Profile.robot_id, Shields);

            // Ghost (Trailblazer path) — 1 phase charge per level
            if (Profile.tier3_spec == ETier3Spec::Ghost)
            {
                PhaseCharges.Add(Profile.robot_id, 1);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// IsHazardActive (default implementation)
// ---------------------------------------------------------------------------

bool AMXHazardBase::IsHazardActive_Implementation() const
{
    return bHazardActive;
}

// ---------------------------------------------------------------------------
// Activate / Deactivate / GetCycleTime (default implementations)
// ---------------------------------------------------------------------------

void AMXHazardBase::Activate_Implementation()
{
    bHazardActive = true;
}

void AMXHazardBase::Deactivate_Implementation()
{
    bHazardActive = false;
}

float AMXHazardBase::GetCycleTime_Implementation() const
{
    return 0.0f;
}

// ---------------------------------------------------------------------------
// CheckSpecProtection
// ---------------------------------------------------------------------------

bool AMXHazardBase::CheckSpecProtection(const FGuid& RobotId)
{
    // Ghost / phase-through — bypasses all hazards
    if (int32* PhaseCount = PhaseCharges.Find(RobotId))
    {
        if (*PhaseCount > 0)
        {
            --(*PhaseCount);
            return true; // absorbed — robot phases through
        }
    }

    // Bulwark / Anchor — immune to Slide / physics push (Conveyor, IceFloor)
    if (HazardFields.damage_type == EDamageType::Slide)
    {
        if (CachedRobotProvider.GetObject())
        {
            FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(
                CachedRobotProvider.GetObject(), RobotId);
            if (Profile.tier2_spec == ETier2Spec::Bulwark ||
                Profile.tier3_spec == ETier3Spec::Anchor)
            {
                return true; // immune to conveyor / slide push
            }
        }
    }

    // Guardian / Sentinel / ImmortalGuard — shield charges
    if (int32* Shields = ShieldCharges.Find(RobotId))
    {
        if (*Shields > 0)
        {
            --(*Shields);
            return true; // shield absorbed the hit
        }
    }

    return false; // no protection — robot dies
}

// ---------------------------------------------------------------------------
// Kill Volume Overlap
// ---------------------------------------------------------------------------

void AMXHazardBase::OnKillVolumeOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                          bool bFromSweep, const FHitResult& SweepResult)
{
    FGuid RobotId = ExtractRobotId(OtherActor);
    if (!RobotId.IsValid()) return;

    RobotsInKillZone.Add(RobotId);

    // Is hazard active?
    if (!IsHazardActive()) return;

    // Check spec protection
    if (CheckSpecProtection(RobotId))
    {
        // Absorbed — treat as near-miss (fire near-miss event, no death)
        EventMessageComp->TriggerNearMissEvent(RobotId);

        if (UMXEventBus* Bus = GetEventBus())
        {
            Bus->OnNearMiss.Broadcast(RobotId, HazardFields);
        }
        return;
    }

    // Robot dies — trigger DEMS death event first, then broadcast on EventBus
    EventMessageComp->TriggerDeathEvent(RobotId);

    // The EventBus broadcast is expected after DEMS (synchronous call already done above).
    // MXEventMessageComponent internally dispatches via MessageDispatcher, but the
    // raw OnRobotDied delegate must be fired from here so downstream systems get FMXEventData.
    // We create a minimal FMXEventData with what we have; a fuller version is built by DEMS.
    // For now, broadcast a shell event — the DEMS-dispatched event carries the full message.
    // Full integration: DEMS dispatcher should return FMXEventData from TriggerDeathEvent.
    // TODO: When DEMS returns FMXEventData, use that here instead of a default struct.
    FMXEventData DeathEvent;
    DeathEvent.robot_id = RobotId;
    DeathEvent.event_type = EEventType::Death;

    if (UMXEventBus* Bus = GetEventBus())
    {
        Bus->OnRobotDied.Broadcast(RobotId, DeathEvent);
    }

    // Remove from swarm — the SwarmController listens to OnRobotDied and handles removal.
}

void AMXHazardBase::OnKillVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    FGuid RobotId = ExtractRobotId(OtherActor);
    if (RobotId.IsValid())
    {
        RobotsInKillZone.Remove(RobotId);
    }
}

// ---------------------------------------------------------------------------
// Warning Volume Overlap — Near-Miss Detection
// ---------------------------------------------------------------------------

void AMXHazardBase::OnWarningVolumeOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                             bool bFromSweep, const FHitResult& SweepResult)
{
    FGuid RobotId = ExtractRobotId(OtherActor);
    if (RobotId.IsValid())
    {
        RobotsInWarningZone.Add(RobotId);
    }
}

void AMXHazardBase::OnWarningVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    FGuid RobotId = ExtractRobotId(OtherActor);
    if (!RobotId.IsValid()) return;

    bool bWasInWarning = RobotsInWarningZone.Remove(RobotId) > 0;

    // Near-miss: was in warning zone, but never entered kill zone (or exited alive)
    if (bWasInWarning && !RobotsInKillZone.Contains(RobotId))
    {
        EventMessageComp->TriggerNearMissEvent(RobotId);

        if (UMXEventBus* Bus = GetEventBus())
        {
            Bus->OnNearMiss.Broadcast(RobotId, HazardFields);
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

FGuid AMXHazardBase::ExtractRobotId(AActor* Actor) const
{
    if (!Actor) return FGuid();

    // Robot Blueprint actors are tagged "MXRobot" and expose RobotId via a named component
    // or an actor tag of the form "RobotId:XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
    // This is a forward-compatible approach: check tag prefix.
    for (const FName& Tag : Actor->Tags)
    {
        FString TagStr = Tag.ToString();
        if (TagStr.StartsWith(TEXT("RobotId:")))
        {
            FString GuidStr = TagStr.RightChop(8); // strip "RobotId:"
            FGuid OutGuid;
            if (FGuid::Parse(GuidStr, OutGuid))
            {
                return OutGuid;
            }
        }
    }
    return FGuid(); // invalid — not a robot
}

UMXEventBus* AMXHazardBase::GetEventBus() const
{
    // Access pattern: GameInstance subsystem
    // UMXEventBusSubsystem* Sub = GetWorld()->GetGameInstance()->GetSubsystem<UMXEventBusSubsystem>();
    // return Sub ? Sub->EventBus : nullptr;
    // Stub — full implementation wired when EventBus subsystem is registered
    return nullptr;
}

TScriptInterface<IMXRobotProvider> AMXHazardBase::GetRobotProvider() const
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

// ===========================================================================
// Subclass Implementations — BeginPlay timer setup
// ===========================================================================

// ---------------------------------------------------------------------------
// AMXHazard_FireVent
// ---------------------------------------------------------------------------

AMXHazard_FireVent::AMXHazard_FireVent()
{
    HazardFields.element       = EHazardElement::Fire;
    HazardFields.damage_type   = EDamageType::Burn;
    HazardFields.severity      = ESeverity::Major;
    HazardFields.camera_behavior = ECameraBehavior::Dramatic;
    HazardFields.visual_mark   = TEXT("burn_mark");
    bHazardActive = false; // starts inactive; cycle begins at BeginPlay
}

void AMXHazard_FireVent::BeginPlay()
{
    Super::BeginPlay();
    StartInactiveCycle();
}

void AMXHazard_FireVent::StartActiveCycle()
{
    Activate();
    GetWorldTimerManager().SetTimer(CycleTimer, this, &AMXHazard_FireVent::StartInactiveCycle,
                                    ActiveDuration, false);
}

void AMXHazard_FireVent::StartInactiveCycle()
{
    Deactivate();
    GetWorldTimerManager().SetTimer(CycleTimer, this, &AMXHazard_FireVent::StartActiveCycle,
                                    InactiveDuration, false);
}

// ---------------------------------------------------------------------------
// AMXHazard_Crusher
// ---------------------------------------------------------------------------

AMXHazard_Crusher::AMXHazard_Crusher()
{
    HazardFields.element       = EHazardElement::Mechanical;
    HazardFields.damage_type   = EDamageType::Crush;
    HazardFields.severity      = ESeverity::Major;
    HazardFields.camera_behavior = ECameraBehavior::Dramatic;
    HazardFields.visual_mark   = TEXT("impact_crack");
    bHazardActive = false;
}

void AMXHazard_Crusher::BeginPlay()
{
    Super::BeginPlay();
    StartTopPause();
}

void AMXHazard_Crusher::StartDescend()
{
    Activate(); // lethal as it descends
    GetWorldTimerManager().SetTimer(CycleTimer, this, &AMXHazard_Crusher::StartHold,
                                    DescendTime, false);
}

void AMXHazard_Crusher::StartHold()
{
    // Still active at bottom — robots caught here die
    GetWorldTimerManager().SetTimer(CycleTimer, this, &AMXHazard_Crusher::StartAscend,
                                    HoldTime, false);
}

void AMXHazard_Crusher::StartAscend()
{
    Deactivate(); // safe while rising
    GetWorldTimerManager().SetTimer(CycleTimer, this, &AMXHazard_Crusher::StartTopPause,
                                    AscendTime, false);
}

void AMXHazard_Crusher::StartTopPause()
{
    GetWorldTimerManager().SetTimer(CycleTimer, this, &AMXHazard_Crusher::StartDescend,
                                    TopPauseTime, false);
}

// ---------------------------------------------------------------------------
// AMXHazard_SpikeTrap
// ---------------------------------------------------------------------------

AMXHazard_SpikeTrap::AMXHazard_SpikeTrap()
{
    HazardFields.element       = EHazardElement::Mechanical;
    HazardFields.damage_type   = EDamageType::Impale;
    HazardFields.severity      = ESeverity::Major;
    HazardFields.camera_behavior = ECameraBehavior::Dramatic;
    HazardFields.visual_mark   = TEXT("impact_crack");
    bHazardActive = false;
}

// ---------------------------------------------------------------------------
// AMXHazard_Pit
// ---------------------------------------------------------------------------

AMXHazard_Pit::AMXHazard_Pit()
{
    HazardFields.element       = EHazardElement::Gravity;
    HazardFields.damage_type   = EDamageType::Fall;
    HazardFields.severity      = ESeverity::Dramatic;
    HazardFields.camera_behavior = ECameraBehavior::Cinematic;
    HazardFields.visual_mark   = TEXT("fall_scar");
    bHazardActive = true; // pits are always lethal
}

// ---------------------------------------------------------------------------
// AMXHazard_LavaPool
// ---------------------------------------------------------------------------

AMXHazard_LavaPool::AMXHazard_LavaPool()
{
    HazardFields.element       = EHazardElement::Fire;
    HazardFields.damage_type   = EDamageType::Melt;
    HazardFields.severity      = ESeverity::Dramatic;
    HazardFields.camera_behavior = ECameraBehavior::Cinematic;
    HazardFields.visual_mark   = TEXT("burn_mark");
    bHazardActive = true;
}

// ---------------------------------------------------------------------------
// AMXHazard_Conveyor
// ---------------------------------------------------------------------------

AMXHazard_Conveyor::AMXHazard_Conveyor()
{
    HazardFields.element       = EHazardElement::Mechanical;
    HazardFields.damage_type   = EDamageType::Slide;
    HazardFields.severity      = ESeverity::Minor;
    HazardFields.camera_behavior = ECameraBehavior::Subtle;
    HazardFields.visual_mark   = TEXT("scuff_mark");
    // Conveyors don't directly kill — they push robots into killers.
    // The death occurs when the robot reaches a pit or other hazard.
    bHazardActive = false; // doesn't kill by itself; push handled in Blueprint tick
}

// ---------------------------------------------------------------------------
// AMXHazard_BuzzSaw
// ---------------------------------------------------------------------------

AMXHazard_BuzzSaw::AMXHazard_BuzzSaw()
{
    HazardFields.element       = EHazardElement::Mechanical;
    HazardFields.damage_type   = EDamageType::Cut;
    HazardFields.severity      = ESeverity::Dramatic;
    HazardFields.camera_behavior = ECameraBehavior::Cinematic;
    HazardFields.visual_mark   = TEXT("impact_crack");
    bHazardActive = true; // always active while moving
}

// ---------------------------------------------------------------------------
// AMXHazard_IceFloor
// ---------------------------------------------------------------------------

AMXHazard_IceFloor::AMXHazard_IceFloor()
{
    HazardFields.element       = EHazardElement::Ice;
    HazardFields.damage_type   = EDamageType::Slide;
    HazardFields.severity      = ESeverity::Minor;
    HazardFields.camera_behavior = ECameraBehavior::Suppress;
    HazardFields.visual_mark   = TEXT("frost_residue");
    bHazardActive = false; // ice floor applies traction effect, not direct death
}

// ---------------------------------------------------------------------------
// AMXHazard_EMPField
// ---------------------------------------------------------------------------

AMXHazard_EMPField::AMXHazard_EMPField()
{
    HazardFields.element       = EHazardElement::Electrical;
    HazardFields.damage_type   = EDamageType::Disable;
    HazardFields.severity      = ESeverity::Minor;
    HazardFields.camera_behavior = ECameraBehavior::Subtle;
    HazardFields.visual_mark   = TEXT("emp_scorch");
    bHazardActive = false; // only active during pulse
}

void AMXHazard_EMPField::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(PulseTimer, this, &AMXHazard_EMPField::FirePulse,
                                    PulseInterval, true); // repeating
}

void AMXHazard_EMPField::FirePulse()
{
    Activate();
    // Schedule deactivation after brief pulse
    FTimerHandle DeactivateTimer;
    GetWorldTimerManager().SetTimer(DeactivateTimer, this, &AMXHazard_EMPField::Deactivate_Implementation,
                                    0.3f, false);
}

// ---------------------------------------------------------------------------
// AMXHazard_FallingDebris
// ---------------------------------------------------------------------------

AMXHazard_FallingDebris::AMXHazard_FallingDebris()
{
    HazardFields.element       = EHazardElement::Mechanical;
    HazardFields.damage_type   = EDamageType::Bury;
    HazardFields.severity      = ESeverity::Major;
    HazardFields.camera_behavior = ECameraBehavior::Dramatic;
    HazardFields.visual_mark   = TEXT("impact_crack");
    bHazardActive = false; // activated only at moment of impact
}

void AMXHazard_FallingDebris::BeginPlay()
{
    Super::BeginPlay();
    ScheduleNextDrop();
}

void AMXHazard_FallingDebris::ScheduleNextDrop()
{
    float NextInterval = FMath::RandRange(MinDropInterval, MaxDropInterval);
    GetWorldTimerManager().SetTimer(DropTimer, this, &AMXHazard_FallingDebris::SpawnDebrisImpact,
                                    NextInterval, false);
}

void AMXHazard_FallingDebris::SpawnDebrisImpact()
{
    // Blueprint extends this to spawn a visual debris mesh
    // C++ layer: briefly activate kill zone at impact, then schedule next drop
    Activate();
    FTimerHandle ImpactTimer;
    GetWorldTimerManager().SetTimer(ImpactTimer, [this]()
    {
        Deactivate();
        ScheduleNextDrop();
    }, 0.2f, false);
}
