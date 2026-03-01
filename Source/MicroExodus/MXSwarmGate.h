// MXSwarmGate.h — Swarm Module v1.0
// Created: 2026-02-22
// Agent 8: Swarm
// Base classes for all cooperative puzzle gate elements.
// Gates require N robots to interact simultaneously (stand, push, chain, etc.)
// before a door/bridge/mechanism is activated.
//
// Engineer-role robots count for more toward gate thresholds (1.5x / 3x / 5x by tier).
// Sacrifice gates permanently kill the robots that activate them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "MXEventMessageComponent.h"
#include "MXInterfaces.h"
#include "MXEvents.h"
#include "MXTypes.h"
#include "MXSwarmGate.generated.h"

// ---------------------------------------------------------------------------
// AMXSwarmGate — Abstract Base
// ---------------------------------------------------------------------------

/**
 * AMXSwarmGate
 * Abstract base class for all swarm-cooperative puzzle elements.
 *
 * Tracks which robots are currently contributing to the gate and whether
 * the required threshold has been met. Fires satisfaction/unsatisfaction events
 * for Blueprint to use to open/close doors, extend bridges, etc.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class MICROEXODUS_API AMXSwarmGate : public AActor
{
    GENERATED_BODY()

public:

    AMXSwarmGate();

protected:

    virtual void BeginPlay() override;

public:

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /**
     * How many robots (in standard units) must be present/interacting to satisfy this gate.
     * Engineer-spec robots contribute more (see EngineerMultiplier).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate")
    int32 RequiredRobots = 10;

    /**
     * Multiplier applied to Engineer-role robots' contribution toward the gate threshold.
     * Tier 2 Engineer = 1.5, Tier 3 Engineer Mechanic = 3.0, Architect mastery = 5.0.
     * Set this per gate instance in the editor based on intended puzzle difficulty.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate")
    float EngineerMultiplier = 1.5f;

    /** Optional DEMS component for puzzle-solve narrative messages. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Gate")
    UMXEventMessageComponent* EventMessageComp;

    /**
     * Interaction zone — robots inside this box contribute to the gate count.
     * Override size per subclass in constructor.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Gate")
    UBoxComponent* InteractionZone;

    // -------------------------------------------------------------------------
    // Effective Count Calculation
    // -------------------------------------------------------------------------

    /**
     * Calculate the effective robot count for gate satisfaction, accounting for
     * Engineer-spec multipliers.
     * @param RobotsOnGate  Array of robot GUIDs currently interacting.
     * @return              Weighted effective count (float, may exceed integer).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Gate")
    float GetEffectiveCount(const TArray<FGuid>& RobotsOnGate) const;

    /**
     * Is the gate's effective robot count >= RequiredRobots?
     * @return True if the gate condition is satisfied.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Gate")
    bool IsGateSatisfied() const;

    /**
     * Return the current progress fraction (0.0–1.0) toward gate satisfaction.
     * Useful for progress bar UI.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Gate")
    float GetGateProgress() const;

    /**
     * Return the list of robot GUIDs currently contributing to this gate.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Gate")
    const TArray<FGuid>& GetContributingRobots() const { return ContributingRobots; }

    // -------------------------------------------------------------------------
    // Gate State Events — implement in Blueprint to open/close the mechanism
    // -------------------------------------------------------------------------

    /**
     * Called when the gate transitions from unsatisfied to satisfied.
     * Implement in Blueprint to play animations, open doors, extend bridges.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "MX|Gate")
    void OnGateSatisfied();
    virtual void OnGateSatisfied_Implementation();

    /**
     * Called when the gate transitions from satisfied back to unsatisfied.
     * Implement in Blueprint to reverse the mechanism (close door, retract bridge).
     */
    UFUNCTION(BlueprintNativeEvent, Category = "MX|Gate")
    void OnGateUnsatisfied();
    virtual void OnGateUnsatisfied_Implementation();

protected:

    // -------------------------------------------------------------------------
    // Interaction Zone Overlap
    // -------------------------------------------------------------------------

    /** Robot entered the interaction zone — add to contributing robots, re-evaluate. */
    UFUNCTION()
    void OnInteractionZoneOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                   bool bFromSweep, const FHitResult& SweepResult);

    /** Robot exited the interaction zone — remove from contributing robots, re-evaluate. */
    UFUNCTION()
    void OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    /** Re-evaluate gate satisfaction and call OnGateSatisfied / OnGateUnsatisfied if state changed. */
    void EvaluateGate();

    /** Extract robot GUID from an overlapping actor (same pattern as AMXHazardBase). */
    FGuid ExtractRobotId(AActor* Actor) const;

    /** Get Engineer multiplier for a specific robot based on its spec. */
    float GetEngineerMultiplierForRobot(const FGuid& RobotId) const;

    /** Retrieve RobotProvider from GameInstance. */
    TScriptInterface<IMXRobotProvider> GetRobotProvider() const;

    /** Retrieve EventBus from GameInstance subsystem. */
    UMXEventBus* GetEventBus() const;

    /** Current list of robots contributing to gate satisfaction. */
    TArray<FGuid> ContributingRobots;

    /** Whether the gate is currently in a satisfied state. */
    bool bSatisfied = false;

    /** Cached robot provider. */
    TScriptInterface<IMXRobotProvider> CachedRobotProvider;
};

// ---------------------------------------------------------------------------
// MXGate_WeightPlate — Standard weight puzzle (stand N robots)
// ---------------------------------------------------------------------------

/**
 * AMXGate_WeightPlate
 * N robots stand on the pressure plate to hold a door open.
 * Standard puzzle gate — no permanent consequences.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXGate_WeightPlate : public AMXSwarmGate
{
    GENERATED_BODY()
public:
    AMXGate_WeightPlate();

    /**
     * Whether the plate door locks open permanently once satisfied (one-shot).
     * If false, releasing robots closes the door again.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|WeightPlate")
    bool bLatchesOpen = false;

protected:
    bool bLatched = false;
    virtual void OnGateSatisfied_Implementation() override;
};

// ---------------------------------------------------------------------------
// MXGate_SacrificePlate — Robots that step on are permanently killed
// ---------------------------------------------------------------------------

/**
 * AMXGate_SacrificePlate
 * Robots that stand on this plate are permanently sacrificed.
 * The gate is satisfied when enough robots have sacrificed themselves.
 * Fires TriggerSacrificeEvent and OnRobotSacrificed for each robot.
 *
 * This is the most emotionally significant gate — camera behavior goes Epic.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXGate_SacrificePlate : public AMXSwarmGate
{
    GENERATED_BODY()
public:
    AMXGate_SacrificePlate();

    /**
     * If true, robots must actively "commit" (hold a button) to sacrifice themselves.
     * If false, stepping on the plate immediately sacrifices the robot.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|Sacrifice")
    bool bRequiresCommitInput = false;

    /**
     * How many robots the sacrifice saves (passed to TriggerSacrificeEvent for narrative).
     * Set this to the number of robots on the other side of the gate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|Sacrifice")
    int32 RobotsSavedBySacrifice = 0;

    /** Count of robots already sacrificed to this gate. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Gate|Sacrifice")
    int32 SacrificesCompleted = 0;

protected:
    /** Override overlap to immediately sacrifice robots instead of just counting them. */
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnSacrificeZoneOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                 bool bFromSweep, const FHitResult& SweepResult);

    /** Process a single robot sacrifice. */
    void ProcessSacrifice(const FGuid& RobotId);
};

// ---------------------------------------------------------------------------
// MXGate_SwarmDoor — Robots physically push the door open
// ---------------------------------------------------------------------------

/**
 * AMXGate_SwarmDoor
 * A heavy door that requires N robots pushing it simultaneously to open.
 * Door slowly opens based on the current robot count (partial push = partial open).
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXGate_SwarmDoor : public AMXSwarmGate
{
    GENERATED_BODY()
public:
    AMXGate_SwarmDoor();

    /**
     * How much the door moves per robot per second (cm/s per robot).
     * With 30 robots at 3.0, door opens at 90 cm/s.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|SwarmDoor")
    float OpenSpeedPerRobot = 3.0f;

    /** Total distance the door must travel to be fully open (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|SwarmDoor")
    float TotalOpenDistance = 200.0f;

    /** Current open progress (0.0 = closed, 1.0 = fully open). */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Gate|SwarmDoor")
    float OpenProgress = 0.0f;

    /** Whether the door closes when robots stop pushing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|SwarmDoor")
    bool bClosesWhenUnattended = true;

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;
};

// ---------------------------------------------------------------------------
// MXGate_ChainStation — Robots form a chain between two points
// ---------------------------------------------------------------------------

/**
 * AMXGate_ChainStation
 * Robots must spread out to form a physical chain/link between two nodes.
 * Used for power relay or bridge puzzles.
 * Satisfaction is determined by robots covering the distance between StartNode and EndNode.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXGate_ChainStation : public AMXSwarmGate
{
    GENERATED_BODY()
public:
    AMXGate_ChainStation();

    /** World position of the chain start anchor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|ChainStation")
    FVector StartNodePosition = FVector::ZeroVector;

    /** World position of the chain end anchor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|ChainStation")
    FVector EndNodePosition = FVector(500.0f, 0.0f, 0.0f);

    /**
     * Maximum gap allowed between adjacent robots in the chain (cm).
     * If any gap exceeds this, the chain is broken and gate is unsatisfied.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|ChainStation")
    float MaxLinkGap = 80.0f;

    /**
     * Check whether the current contributing robots form an unbroken chain
     * from StartNodePosition to EndNodePosition within MaxLinkGap.
     * Called during EvaluateGate instead of the standard effective count check.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Gate|ChainStation")
    bool IsChainComplete() const;

    virtual bool IsGateSatisfied() const;
};

// ---------------------------------------------------------------------------
// MXGate_WeightBridge — Robots hold a bridge section while others cross
// ---------------------------------------------------------------------------

/**
 * AMXGate_WeightBridge
 * A bridge segment that sags/collapses without enough counterweight robots.
 * The "holding" robots must remain on the counterweight plate while
 * the rest of the swarm crosses the bridge.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXGate_WeightBridge : public AMXSwarmGate
{
    GENERATED_BODY()
public:
    AMXGate_WeightBridge();

    /**
     * Interaction zone for the counterweight plate (separate from bridge crossing zone).
     * Robots in this zone hold the bridge; robots on BridgeZone are crossing.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Gate|WeightBridge")
    UBoxComponent* CounterweightZone;

    /**
     * Interaction zone covering the bridge itself.
     * Crossing robots here exert downward force on the bridge.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Gate|WeightBridge")
    UBoxComponent* BridgeZone;

    /**
     * The bridge sags proportionally to (crossing_robots - counterweight_effective).
     * This float [0.0–1.0] drives the bridge sag curve in Blueprint.
     */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Gate|WeightBridge")
    float BridgeSagFraction = 0.0f;

protected:
    virtual void BeginPlay() override;
    TArray<FGuid> RobotsOnBridge;
    TArray<FGuid> RobotsOnCounterweight;
    void UpdateBridgeSag();
};

// ---------------------------------------------------------------------------
// MXGate_OverrideStation — Multiple stations must be held simultaneously
// ---------------------------------------------------------------------------

/**
 * AMXGate_OverrideStation
 * An individual station node. Multiple OverrideStation actors must ALL be
 * simultaneously held (each by their own RequiredRobots count) to trigger
 * the shared mechanism.
 *
 * Usage: Place 2+ OverrideStation actors in the level and link them via
 * PartnerStations. The gate fires OnGateSatisfied only when ALL partners are satisfied.
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXGate_OverrideStation : public AMXSwarmGate
{
    GENERATED_BODY()
public:
    AMXGate_OverrideStation();

    /**
     * Array of other OverrideStation actors that must be simultaneously satisfied.
     * Set this in editor by filling the array with the partner station actors.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Gate|OverrideStation")
    TArray<AMXGate_OverrideStation*> PartnerStations;

    /**
     * Is this specific station currently satisfied (independent of partners)?
     * Used by partners to check status when evaluating the shared condition.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Gate|OverrideStation")
    bool IsThisStationSatisfied() const { return bSatisfied; }

protected:
    /** Override to also check all partner stations are satisfied. */
    virtual void OnGateSatisfied_Implementation() override;
    virtual void OnGateUnsatisfied_Implementation() override;

    /** Check whether all partner stations are currently satisfied. */
    bool AreAllPartnersSatisfied() const;
};
