// MXPartDestructionManager.cpp — Robot Assembly Module v1.0
// Agent 13: Robot Assembly

#include "MXPartDestructionManager.h"
#include "MXRobotAssembler.h"

UMXPartDestructionManager::UMXPartDestructionManager()
{
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXPartDestructionManager::SetAssembler(UMXRobotAssembler* InAssembler)
{
    Assembler = InAssembler;
}

void UMXPartDestructionManager::InitialiseForLevel(
    const TArray<FGuid>& RobotIds,
    const TArray<FMXAssemblyRecipe>& Recipes)
{
    Profiles.Empty();
    Profiles.Reserve(RobotIds.Num());

    for (int32 i = 0; i < RobotIds.Num(); ++i)
    {
        FMXDestructionProfile Profile;
        Profile.RobotId = RobotIds[i];
        Profile.AttachedPartCount = static_cast<int32>(EPartSlot::COUNT);
        Profile.bHasLostParts = false;
        Profile.Parts.SetNum(static_cast<int32>(EPartSlot::COUNT));

        const FMXAssemblyRecipe& Recipe = (i < Recipes.Num()) ? Recipes[i] : FMXAssemblyRecipe();

        for (int32 SlotIdx = 0; SlotIdx < static_cast<int32>(EPartSlot::COUNT); ++SlotIdx)
        {
            FMXPartState& State = Profile.Parts[SlotIdx];
            State.Slot = static_cast<EPartSlot>(SlotIdx);
            State.DamageState = EPartDamageState::Pristine;

            // Get part ID from recipe.
            if (SlotIdx < Recipe.SlotPartIds.Num())
            {
                State.PartId = Recipe.SlotPartIds[SlotIdx];
            }

            // Look up HP from part definition.
            int32 BaseHP = 2;
            if (Assembler && !State.PartId.IsEmpty())
            {
                FMXPartDefinition Def;
                if (Assembler->GetPartDefinition(State.PartId, Def))
                {
                    BaseHP = Def.HitPoints;
                }
            }

            // Body is tougher.
            if (SlotIdx == static_cast<int32>(EPartSlot::Body))
            {
                BaseHP = FMath::Max(BaseHP, 4);
            }

            State.MaxHP = BaseHP;
            State.CurrentHP = BaseHP;
            State.WearAmount = 0.0f;
        }

        Profiles.Add(RobotIds[i], Profile);
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXPartDestructionManager: Initialised destruction profiles for %d robots."),
        Profiles.Num());
}

void UMXPartDestructionManager::ResetForNewLevel()
{
    Profiles.Empty();
    UE_LOG(LogTemp, Log, TEXT("MXPartDestructionManager: Reset for new level."));
}

// ---------------------------------------------------------------------------
// Damage Application
// ---------------------------------------------------------------------------

FMXDestructionEvent UMXPartDestructionManager::ApplyDamage(
    const FGuid&    RobotId,
    EHazardElement  DamageElement,
    FVector         ImpactLocation,
    FVector         ImpactDirection,
    EPartSlot       TargetSlot,
    int32           DamageAmount)
{
    FMXDestructionProfile* Profile = Profiles.Find(RobotId);
    if (!Profile)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXPartDestructionManager: No profile for robot %s."),
            *RobotId.ToString());
        return FMXDestructionEvent();
    }

    // Auto-target if no slot specified.
    if (TargetSlot == EPartSlot::COUNT)
    {
        // Use robot GUID as seed for deterministic targeting within the same frame.
        FRandomStream TargetStream(static_cast<int32>(RobotId.A ^ RobotId.D));
        TargetSlot = AutoTargetSlot(*Profile, TargetStream);
    }

    const int32 SlotIdx = static_cast<int32>(TargetSlot);
    if (SlotIdx < 0 || SlotIdx >= Profile->Parts.Num())
    {
        return FMXDestructionEvent();
    }

    FMXPartState& Part = Profile->Parts[SlotIdx];

    // Already destroyed — no further damage.
    if (Part.DamageState == EPartDamageState::Destroyed)
    {
        return FMXDestructionEvent();
    }

    // Apply damage.
    Part.CurrentHP -= FMath::Max(1, DamageAmount);

    // Check for state transition.
    if (Part.CurrentHP <= 0)
    {
        return AdvanceDamageState(*Profile, TargetSlot, DamageElement, ImpactLocation, ImpactDirection);
    }

    // HP reduced but no state change — accumulate wear.
    Part.WearAmount = FMath::Clamp(
        1.0f - (static_cast<float>(Part.CurrentHP) / static_cast<float>(Part.MaxHP)),
        0.0f, 1.0f);

    UE_LOG(LogTemp, Verbose,
        TEXT("MXPartDestructionManager: Robot %s slot %d took %d damage. HP: %d/%d"),
        *RobotId.ToString(), SlotIdx, DamageAmount, Part.CurrentHP, Part.MaxHP);

    // Return a minor event even without state transition.
    FMXDestructionEvent Event;
    Event.RobotId = RobotId;
    Event.Slot = TargetSlot;
    Event.PartId = Part.PartId;
    Event.NewState = Part.DamageState;
    Event.ImpactLocation = ImpactLocation;
    Event.ImpactDirection = ImpactDirection;
    Event.DamageElement = DamageElement;
    return Event;
}

void UMXPartDestructionManager::DestroyAllParts(
    const FGuid& RobotId,
    EHazardElement DamageElement,
    FVector ImpactLocation)
{
    FMXDestructionProfile* Profile = Profiles.Find(RobotId);
    if (!Profile) return;

    const FVector OutwardDir = FVector(0.0f, 0.0f, 1.0f);

    for (int32 SlotIdx = 0; SlotIdx < Profile->Parts.Num(); ++SlotIdx)
    {
        FMXPartState& Part = Profile->Parts[SlotIdx];
        if (Part.DamageState == EPartDamageState::Destroyed) continue;

        // Force to destroyed.
        Part.CurrentHP = 0;
        Part.DamageState = EPartDamageState::Destroyed;
        Part.WearAmount = 1.0f;

        // Fire detachment event.
        FMXDestructionEvent Event;
        Event.RobotId = RobotId;
        Event.Slot = static_cast<EPartSlot>(SlotIdx);
        Event.PartId = Part.PartId;
        Event.NewState = EPartDamageState::Destroyed;
        Event.ImpactLocation = ImpactLocation;
        Event.ImpactDirection = OutwardDir;
        Event.DamageElement = DamageElement;

        OnPartDetached.Broadcast(Event);
    }

    Profile->AttachedPartCount = 0;
    Profile->bHasLostParts = true;

    UE_LOG(LogTemp, Log,
        TEXT("MXPartDestructionManager: Robot %s — total destruction. All parts destroyed."),
        *RobotId.ToString());
}

TArray<FMXDestructionEvent> UMXPartDestructionManager::ApplyElementalDestruction(
    const FGuid&    RobotId,
    EHazardElement  DamageElement,
    FVector         ImpactLocation,
    FVector         ImpactDirection)
{
    TArray<FMXDestructionEvent> Events;
    FMXDestructionProfile* Profile = Profiles.Find(RobotId);
    if (!Profile) return Events;

    // Element-specific destruction order.
    TArray<EPartSlot> Order;

    switch (DamageElement)
    {
    case EHazardElement::Fire:
        // Burns from outside in.
        Order = { EPartSlot::ArmLeft, EPartSlot::ArmRight, EPartSlot::Head,
                  EPartSlot::Locomotion, EPartSlot::Body };
        break;

    case EHazardElement::Mechanical:
        // Crush: collapses from top down.
        Order = { EPartSlot::Head, EPartSlot::ArmLeft, EPartSlot::ArmRight,
                  EPartSlot::Body, EPartSlot::Locomotion };
        break;

    case EHazardElement::Gravity:
        // Fall: locomotion breaks first.
        Order = { EPartSlot::Locomotion, EPartSlot::Body, EPartSlot::ArmLeft,
                  EPartSlot::ArmRight, EPartSlot::Head };
        break;

    case EHazardElement::Electrical:
        // EMP: head pops off, then cascade.
        Order = { EPartSlot::Head, EPartSlot::ArmRight, EPartSlot::ArmLeft,
                  EPartSlot::Locomotion, EPartSlot::Body };
        break;

    case EHazardElement::Ice:
        // Freeze-shatter: all parts, outer first.
        Order = { EPartSlot::ArmLeft, EPartSlot::ArmRight, EPartSlot::Head,
                  EPartSlot::Locomotion, EPartSlot::Body };
        break;

    default:
        // Generic: arms, head, locomotion, body.
        Order = { EPartSlot::ArmLeft, EPartSlot::ArmRight, EPartSlot::Head,
                  EPartSlot::Locomotion, EPartSlot::Body };
        break;
    }

    // Apply lethal damage in order.
    for (const EPartSlot Slot : Order)
    {
        const int32 SlotIdx = static_cast<int32>(Slot);
        if (SlotIdx < 0 || SlotIdx >= Profile->Parts.Num()) continue;

        FMXPartState& Part = Profile->Parts[SlotIdx];
        if (Part.DamageState == EPartDamageState::Destroyed) continue;

        // Force HP to 0 and advance.
        Part.CurrentHP = 0;
        FMXDestructionEvent Event = AdvanceDamageState(
            *Profile, Slot, DamageElement, ImpactLocation, ImpactDirection);

        // Keep advancing until destroyed.
        while (Part.DamageState != EPartDamageState::Destroyed &&
               Part.DamageState != EPartDamageState::Detached)
        {
            Part.CurrentHP = 0;
            Event = AdvanceDamageState(
                *Profile, Slot, DamageElement, ImpactLocation, ImpactDirection);
        }

        Events.Add(Event);
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXPartDestructionManager: Robot %s — elemental destruction (%d). %d events generated."),
        *RobotId.ToString(), static_cast<int32>(DamageElement), Events.Num());

    return Events;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool UMXPartDestructionManager::GetDestructionProfile(
    const FGuid& RobotId,
    FMXDestructionProfile& OutProfile) const
{
    if (const FMXDestructionProfile* Found = Profiles.Find(RobotId))
    {
        OutProfile = *Found;
        return true;
    }
    return false;
}

EPartDamageState UMXPartDestructionManager::GetPartDamageState(
    const FGuid& RobotId,
    EPartSlot Slot) const
{
    if (const FMXDestructionProfile* Profile = Profiles.Find(RobotId))
    {
        const int32 Idx = static_cast<int32>(Slot);
        if (Idx >= 0 && Idx < Profile->Parts.Num())
        {
            return Profile->Parts[Idx].DamageState;
        }
    }
    return EPartDamageState::Pristine;
}

bool UMXPartDestructionManager::HasLostParts(const FGuid& RobotId) const
{
    if (const FMXDestructionProfile* Profile = Profiles.Find(RobotId))
    {
        return Profile->bHasLostParts;
    }
    return false;
}

int32 UMXPartDestructionManager::GetDamagedRobotCount() const
{
    int32 Count = 0;
    for (const auto& Pair : Profiles)
    {
        if (Pair.Value.bHasLostParts) ++Count;
    }
    return Count;
}

// ---------------------------------------------------------------------------
// Private: AutoTargetSlot
// ---------------------------------------------------------------------------

EPartSlot UMXPartDestructionManager::AutoTargetSlot(
    const FMXDestructionProfile& Profile,
    FRandomStream& Stream) const
{
    // Exposure weights: outermost parts are hit first.
    // Body is hit last (most protected).
    static const float ExposureWeights[] = {
        2.0f,   // Locomotion — ground level, exposed
        0.5f,   // Body — central, protected
        3.0f,   // ArmLeft — most exposed
        3.0f,   // ArmRight — most exposed
        2.5f,   // Head — top, exposed
    };

    float TotalWeight = 0.0f;
    TArray<float> CumulativeWeights;

    for (int32 i = 0; i < static_cast<int32>(EPartSlot::COUNT); ++i)
    {
        const FMXPartState& Part = Profile.Parts[i];

        // Skip destroyed parts — they can't take more damage.
        if (Part.DamageState == EPartDamageState::Destroyed ||
            Part.DamageState == EPartDamageState::Detached)
        {
            CumulativeWeights.Add(TotalWeight);
            continue;
        }

        // Reduce weight for already-damaged parts (spread damage around).
        float Weight = ExposureWeights[i];
        if (Part.DamageState >= EPartDamageState::Cracked) Weight *= 0.5f;

        TotalWeight += Weight;
        CumulativeWeights.Add(TotalWeight);
    }

    if (TotalWeight <= 0.0f) return EPartSlot::Body; // Everything destroyed, hit body.

    const float Roll = Stream.FRandRange(0.0f, TotalWeight);
    for (int32 i = 0; i < CumulativeWeights.Num(); ++i)
    {
        if (Roll <= CumulativeWeights[i] && CumulativeWeights[i] > (i > 0 ? CumulativeWeights[i-1] : 0.0f))
        {
            return static_cast<EPartSlot>(i);
        }
    }

    return EPartSlot::Body;
}

// ---------------------------------------------------------------------------
// Private: AdvanceDamageState
// ---------------------------------------------------------------------------

FMXDestructionEvent UMXPartDestructionManager::AdvanceDamageState(
    FMXDestructionProfile& Profile,
    EPartSlot              Slot,
    EHazardElement         DamageElement,
    const FVector&         ImpactLocation,
    const FVector&         ImpactDirection)
{
    FMXPartState& Part = Profile.Parts[static_cast<int32>(Slot)];

    // Advance to next state.
    const EPartDamageState OldState = Part.DamageState;
    EPartDamageState NewState = OldState;

    switch (OldState)
    {
    case EPartDamageState::Pristine:   NewState = EPartDamageState::Scratched; break;
    case EPartDamageState::Scratched:  NewState = EPartDamageState::Cracked;   break;
    case EPartDamageState::Cracked:    NewState = EPartDamageState::Hanging;   break;
    case EPartDamageState::Hanging:    NewState = EPartDamageState::Detached;  break;
    case EPartDamageState::Detached:   NewState = EPartDamageState::Destroyed; break;
    case EPartDamageState::Destroyed:  break; // Already at terminal state.
    }

    // Body cannot detach — it goes straight from Hanging to Destroyed.
    if (Slot == EPartSlot::Body && NewState == EPartDamageState::Detached)
    {
        NewState = EPartDamageState::Destroyed;
    }

    Part.DamageState = NewState;
    Part.WearAmount = 1.0f; // Maxed out on state transition.

    // Reset HP for the new state (parts get tougher in later states).
    Part.CurrentHP = FMath::Max(1, Part.MaxHP / 2);
    if (NewState >= EPartDamageState::Hanging)
    {
        Part.CurrentHP = 1; // One more hit and it's gone.
    }
    if (NewState >= EPartDamageState::Detached)
    {
        Part.CurrentHP = 0;
    }

    // Track lost parts.
    if (NewState >= EPartDamageState::Detached)
    {
        Profile.AttachedPartCount = FMath::Max(0, Profile.AttachedPartCount - 1);
        Profile.bHasLostParts = true;
    }

    // Build event.
    FMXDestructionEvent Event;
    Event.RobotId = Profile.RobotId;
    Event.Slot = Slot;
    Event.PartId = Part.PartId;
    Event.NewState = NewState;
    Event.ImpactLocation = ImpactLocation;
    Event.ImpactDirection = ImpactDirection;
    Event.DamageElement = DamageElement;

    UE_LOG(LogTemp, Log,
        TEXT("MXPartDestructionManager: Robot %s part [%d] %s → %s (element=%d)"),
        *Profile.RobotId.ToString(),
        static_cast<int32>(Slot),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState),
        static_cast<int32>(DamageElement));

    // Fire appropriate delegate.
    if (NewState >= EPartDamageState::Detached)
    {
        OnPartDetached.Broadcast(Event);
    }
    else
    {
        OnPartDamaged.Broadcast(Event);
    }

    // Check if robot is now stripped.
    CheckStrippedState(Profile.RobotId, Profile);

    return Event;
}

// ---------------------------------------------------------------------------
// Private: CheckStrippedState
// ---------------------------------------------------------------------------

void UMXPartDestructionManager::CheckStrippedState(
    const FGuid& RobotId,
    const FMXDestructionProfile& Profile)
{
    // Stripped = only body remains (all other parts detached or destroyed).
    for (int32 i = 0; i < Profile.Parts.Num(); ++i)
    {
        if (static_cast<EPartSlot>(i) == EPartSlot::Body) continue;

        if (Profile.Parts[i].DamageState < EPartDamageState::Detached)
        {
            return; // At least one non-body part is still attached.
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXPartDestructionManager: Robot %s has been STRIPPED — only body remains."),
        *RobotId.ToString());

    OnRobotStripped.Broadcast(RobotId);
}
