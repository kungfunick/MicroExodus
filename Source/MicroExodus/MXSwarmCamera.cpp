// MXSwarmCamera.cpp — Camera Module v1.0
// Created: 2026-02-22
// Agent 7: Camera

#include "MXSwarmCamera.h"
#include "MXCameraBehaviors.h"
#include "MXCameraPersonality.h"
#include "MXConstants.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

// ---------------------------------------------------------------------------
// Constants (GDD zoom table values)
// ---------------------------------------------------------------------------

// Camera heights (cm) for each swarm band — matches the GDD zoom table exactly.
// Entries are: { MinRobots, MaxRobots, TargetHeight }
// GDD heights are in metres — converted here to UE cm (×100).

static const TArray<FMXCameraZoomEntry> GDD_ZOOM_TABLE =
{
    { 1,   5,   300.0f  },   //  3m  — Close, intimate
    { 6,   15,  600.0f  },   //  6m  — Small group
    { 16,  30,  1000.0f },   // 10m  — Medium swarm
    { 31,  50,  1400.0f },   // 14m  — Large swarm
    { 51,  75,  1700.0f },   // 17m  — Mass movement
    { 76,  100, 2000.0f },   // 20m  — Full army
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXSwarmCamera::UMXSwarmCamera()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // Populate zoom table from GDD defaults (designers can override in Blueprint).
    ZoomTable = GDD_ZOOM_TABLE;
    CurrentHeight = ZoomTable[0].TargetHeight;
    TargetHeight  = ZoomTable[0].TargetHeight;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void UMXSwarmCamera::BeginPlay()
{
    Super::BeginPlay();

    // Attempt to find the SwarmController on the owning actor.
    if (AActor* Owner = GetOwner())
    {
        SwarmControllerRef = Owner->FindComponentByClass<UActorComponent>();
        // Specific cast to UMXSwarmController done at use-site via GetSwarmCenter().
    }
}

// ---------------------------------------------------------------------------
// TickComponent
// ---------------------------------------------------------------------------

void UMXSwarmCamera::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 1. Resolve position tracking (swarm follow / inspect / focus).
    TickPositionTracking(DeltaTime);

    // 2. Process active camera event (zoom overrides, shake, timing).
    TickActiveEvent(DeltaTime);
}

// ---------------------------------------------------------------------------
// SetSwarmTarget
// ---------------------------------------------------------------------------

void UMXSwarmCamera::SetSwarmTarget(const TArray<FGuid>& ActiveRobots)
{
    TrackedRobots = ActiveRobots;
    bIsSplit = false;
    SecondaryGroupRobots.Empty();
}

// ---------------------------------------------------------------------------
// SetSplitTarget
// ---------------------------------------------------------------------------

void UMXSwarmCamera::SetSplitTarget(const TArray<FGuid>& GroupA, const TArray<FGuid>& GroupB)
{
    // GroupA is dominant — camera follows their centroid.
    TrackedRobots       = GroupA;
    SecondaryGroupRobots = GroupB;
    bIsSplit            = true;
}

// ---------------------------------------------------------------------------
// EnterInspectMode / ExitInspectMode
// ---------------------------------------------------------------------------

void UMXSwarmCamera::EnterInspectMode(const FGuid& RobotId)
{
    bInInspectMode = true;
    InspectTargetId = RobotId;
    bFocusingOnRobot = false;
}

void UMXSwarmCamera::ExitInspectMode()
{
    bInInspectMode = false;
    InspectTargetId = FGuid();
}

// ---------------------------------------------------------------------------
// UpdateZoom
// ---------------------------------------------------------------------------

void UMXSwarmCamera::UpdateZoom(int32 SwarmCount, float DeltaTime)
{
    TargetHeight = GetIdealHeight(SwarmCount);

    // Smooth interpolation toward target height.
    CurrentHeight = FMath::FInterpTo(CurrentHeight, TargetHeight, DeltaTime, ZoomInterpSpeed);

    if (SpringArm)
    {
        SpringArm->TargetArmLength = CurrentHeight;
    }
}

// ---------------------------------------------------------------------------
// GetIdealHeight
// ---------------------------------------------------------------------------

float UMXSwarmCamera::GetIdealHeight(int32 SwarmCount) const
{
    if (ZoomTable.IsEmpty()) return 600.0f;

    // Find the two nearest bands and lerp between them.
    const FMXCameraZoomEntry* Lower = nullptr;
    const FMXCameraZoomEntry* Upper = nullptr;

    for (int32 i = 0; i < ZoomTable.Num(); ++i)
    {
        const FMXCameraZoomEntry& Entry = ZoomTable[i];

        if (SwarmCount >= Entry.MinRobots && SwarmCount <= Entry.MaxRobots)
        {
            // Exact match — return this height.
            return Entry.TargetHeight;
        }

        if (SwarmCount < Entry.MinRobots)
        {
            Upper = &Entry;
            break;
        }
        Lower = &Entry;
    }

    // Clamp to first or last entry.
    if (!Lower) return ZoomTable[0].TargetHeight;
    if (!Upper) return ZoomTable.Last().TargetHeight;

    // Interpolate between Lower and Upper bands based on count.
    float Alpha = static_cast<float>(SwarmCount - Lower->MaxRobots)
                / static_cast<float>(Upper->MinRobots - Lower->MaxRobots);
    return FMath::Lerp(Lower->TargetHeight, Upper->TargetHeight, FMath::Clamp(Alpha, 0.0f, 1.0f));
}

// ---------------------------------------------------------------------------
// FocusOnRobot
// ---------------------------------------------------------------------------

void UMXSwarmCamera::FocusOnRobot(const FGuid& RobotId, float Duration)
{
    PreFocusWorldPosition = CurrentTargetPosition;
    bFocusingOnRobot     = true;
    FocusTargetId        = RobotId;
    FocusTimeRemaining   = Duration;
    bReturningToSwarm    = false;
}

// ---------------------------------------------------------------------------
// ReturnToSwarm
// ---------------------------------------------------------------------------

void UMXSwarmCamera::ReturnToSwarm(float Duration)
{
    bFocusingOnRobot  = false;
    bInInspectMode    = false;
    bReturningToSwarm = true;
    ReturnTimeRemaining = Duration;
    ReturnDuration      = Duration;
}

// ---------------------------------------------------------------------------
// SetCameraMode
// ---------------------------------------------------------------------------

void UMXSwarmCamera::SetCameraMode(ECameraPersonalityMode Mode)
{
    if (CurrentMode == Mode) return;

    if (bLogEvents)
    {
        UE_LOG(LogTemp, Log, TEXT("[Camera] Mode changed: %s → %s"),
               *UMXCameraPersonality::GetModeDisplayName(CurrentMode),
               *UMXCameraPersonality::GetModeDisplayName(Mode));
    }

    CurrentMode = Mode;

    // In Locked mode, flush all queued events immediately.
    if (Mode == ECameraPersonalityMode::Locked)
    {
        FlushEventQueue();
    }
}

// ---------------------------------------------------------------------------
// QueueCameraEvent
// ---------------------------------------------------------------------------

void UMXSwarmCamera::QueueCameraEvent(const FMXCameraEvent& Event)
{
    if (CurrentMode == ECameraPersonalityMode::Locked) return;

    // Get the personality filter / personality helper.
    // (Personality object created lazily or injected — handled in GetPersonality().)
    // For simplicity, logic is inlined here; the heavy lifting is in MXCameraPersonality.
    // If no active event, start immediately.
    if (!ActiveEvent.IsSet())
    {
        ActiveEvent = Event;
        if (bLogEvents)
        {
            UE_LOG(LogTemp, Log, TEXT("[Camera] Event started immediately: behavior=%d duration=%.2f"),
                   (int32)Event.Behavior, Event.Duration);
        }
        return;
    }

    // Add to queue, maintaining priority sort (highest priority first).
    EventQueue.Add(Event);
    EventQueue.Sort([](const FMXCameraEvent& A, const FMXCameraEvent& B)
    {
        return A.Priority > B.Priority;
    });

    if (bLogEvents)
    {
        UE_LOG(LogTemp, Log, TEXT("[Camera] Event queued (queue size: %d): behavior=%d priority=%.1f"),
               EventQueue.Num(), (int32)Event.Behavior, Event.Priority);
    }
}

// ---------------------------------------------------------------------------
// TickCameraEvents
// ---------------------------------------------------------------------------

void UMXSwarmCamera::TickCameraEvents(float DeltaTime)
{
    if (!ActiveEvent.IsSet()) return;

    FMXCameraEvent& Current = ActiveEvent.GetValue();
    Current.TimeRemaining -= DeltaTime;

    if (Current.TimeRemaining <= 0.0f)
    {
        // Event complete — initiate return to swarm if this was a focus event.
        if (bFocusingOnRobot)
        {
            ReturnToSwarm(0.5f);
        }

        ActiveEvent.Reset();

        if (bLogEvents)
        {
            UE_LOG(LogTemp, Log, TEXT("[Camera] Event complete. Queue depth: %d"), EventQueue.Num());
        }

        // Pop next event.
        ActivateNextEvent();
    }
}

// ---------------------------------------------------------------------------
// FlushEventQueue
// ---------------------------------------------------------------------------

void UMXSwarmCamera::FlushEventQueue()
{
    EventQueue.Empty();
    ActiveEvent.Reset();
    bFocusingOnRobot = false;
    bReturningToSwarm = false;

    if (bLogEvents)
    {
        UE_LOG(LogTemp, Log, TEXT("[Camera] Event queue flushed."));
    }
}

// ===========================================================================
// Private Helpers
// ===========================================================================

FVector UMXSwarmCamera::ComputeCentroid(const TArray<FGuid>& RobotIds) const
{
    if (RobotIds.IsEmpty()) return FVector::ZeroVector;

    FVector Sum = FVector::ZeroVector;
    int32 Found = 0;

    for (const FGuid& Id : RobotIds)
    {
        if (const FVector* Pos = RobotPositions.Find(Id))
        {
            Sum += *Pos;
            ++Found;
        }
    }

    return Found > 0 ? Sum / static_cast<float>(Found) : FVector::ZeroVector;
}

void UMXSwarmCamera::TickPositionTracking(float DeltaTime)
{
    if (!SpringArm) return;

    FVector DesiredPosition;

    if (bInInspectMode)
    {
        // Lock to inspected robot.
        DesiredPosition = RobotPositions.Contains(InspectTargetId)
            ? RobotPositions[InspectTargetId]
            : CurrentTargetPosition;

        // Zoom in to inspect height.
        CurrentHeight = FMath::FInterpTo(CurrentHeight, InspectHeight, DeltaTime, ZoomInterpSpeed * 2.0f);
    }
    else if (bFocusingOnRobot && FocusTimeRemaining > 0.0f)
    {
        // Focus on specific robot during an event.
        FocusTimeRemaining -= DeltaTime;
        DesiredPosition = RobotPositions.Contains(FocusTargetId)
            ? RobotPositions[FocusTargetId]
            : CurrentTargetPosition;
    }
    else if (bReturningToSwarm && ReturnTimeRemaining > 0.0f)
    {
        // Blend from focus position back to swarm centroid.
        ReturnTimeRemaining -= DeltaTime;
        float Alpha = 1.0f - FMath::Clamp(ReturnTimeRemaining / ReturnDuration, 0.0f, 1.0f);
        FVector SwarmCentroid = ComputeCentroid(TrackedRobots);
        DesiredPosition = FMath::Lerp(PreFocusWorldPosition, SwarmCentroid, Alpha);

        if (ReturnTimeRemaining <= 0.0f)
        {
            bReturningToSwarm = false;
        }
    }
    else
    {
        // Default: follow dominant group centroid.
        DesiredPosition = ComputeCentroid(TrackedRobots);
    }

    // Smooth position tracking.
    CurrentTargetPosition = FMath::VInterpTo(CurrentTargetPosition, DesiredPosition,
                                              DeltaTime, PositionInterpSpeed);

    // Apply to spring arm actor's location.
    AActor* Owner = GetOwner();
    if (Owner)
    {
        FVector NewLocation = CurrentTargetPosition;
        NewLocation.Z = Owner->GetActorLocation().Z; // Camera rig Z managed by spring arm.
        Owner->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
}

void UMXSwarmCamera::TickActiveEvent(float DeltaTime)
{
    TickCameraEvents(DeltaTime);

    // Apply zoom override from active event if set.
    if (ActiveEvent.IsSet() && ActiveEvent.GetValue().ZoomOverride > 0.0f)
    {
        ApplyZoomOverride(ActiveEvent.GetValue().ZoomOverride, DeltaTime);
    }
}

void UMXSwarmCamera::ActivateNextEvent()
{
    if (EventQueue.IsEmpty()) return;

    // Queue is already sorted by priority — take the front.
    ActiveEvent = EventQueue[0];
    EventQueue.RemoveAt(0);

    if (bLogEvents)
    {
        UE_LOG(LogTemp, Log, TEXT("[Camera] Next event activated: behavior=%d duration=%.2f"),
               (int32)ActiveEvent.GetValue().Behavior,
               ActiveEvent.GetValue().Duration);
    }

    // If the new event targets a specific robot, begin focus.
    if (ActiveEvent.GetValue().TargetRobotId.IsValid())
    {
        FocusOnRobot(ActiveEvent.GetValue().TargetRobotId, ActiveEvent.GetValue().Duration);
    }
}

void UMXSwarmCamera::ApplyZoomOverride(float ZoomOverride, float DeltaTime)
{
    if (!SpringArm) return;
    CurrentHeight = FMath::FInterpTo(CurrentHeight, ZoomOverride, DeltaTime, ZoomInterpSpeed * 2.0f);
    SpringArm->TargetArmLength = CurrentHeight;
}
