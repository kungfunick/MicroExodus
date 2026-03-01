// MXSpecTree.cpp — Specialization Module Version 1.0
// Created: 2026-02-22
// Agent 5: Specialization
// Change Log:
//   v1.0 — Initial implementation

#include "MXSpecTree.h"
#include "MXRobotProfile.h"
#include "MXRunData.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXSpecTree::UMXSpecTree()
{
}

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void UMXSpecTree::Initialize(UDataTable* InSpecTreeTable,
                              TScriptInterface<IMXRobotProvider> InRobotProvider,
                              UMXEventBus* InEventBus)
{
    SpecTreeTable = InSpecTreeTable;
    RobotProvider = InRobotProvider;
    EventBus      = InEventBus;

    if (!SpecTreeTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpecTree: No spec tree DataTable provided."));
    }

    if (EventBus)
    {
        EventBus->OnLevelUp.AddDynamic(this, &UMXSpecTree::OnLevelUp);
        EventBus->OnRunComplete.AddDynamic(this, &UMXSpecTree::OnRunComplete);
        EventBus->OnRunFailed.AddDynamic(this, &UMXSpecTree::OnRunFailed);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpecTree: No EventBus provided — delegates not bound."));
    }
}

// ---------------------------------------------------------------------------
// GetAvailableChoices
// ---------------------------------------------------------------------------

TArray<FMXSpecChoice> UMXSpecTree::GetAvailableChoices(const FMXRobotProfile& Robot) const
{
    TArray<FMXSpecChoice> Choices;

    // Tier 1: Role selection at Lv5
    if (Robot.level >= ROLE_UNLOCK_LEVEL && Robot.role == ERobotRole::None)
    {
        const TArray<ERobotRole> Roles = { ERobotRole::Scout, ERobotRole::Guardian, ERobotRole::Engineer };
        for (ERobotRole R : Roles)
        {
            FMXSpecChoice Choice;
            Choice.ChoiceTier  = 1;
            Choice.RoleOption  = R;
            Choice.DisplayName = GetRoleDescription(R).Left(GetRoleDescription(R).Find(TEXT(":")));
            // Pull display name from enum
            const UEnum* RoleEnum = StaticEnum<ERobotRole>();
            if (RoleEnum)
            {
                Choice.DisplayName = RoleEnum->GetDisplayNameTextByValue((int64)R).ToString();
            }
            Choice.Description   = GetRoleDescription(R);
            // Visual preview from DataTable if available
            if (const FMXSpecDefinition* Row = FindSpecRow(R, ETier2Spec::None, ETier3Spec::None))
            {
                Choice.VisualPreview = Row->visual_indicator;
            }
            Choices.Add(Choice);
        }
        return Choices;
    }

    // Tier 2: selection at Lv10
    if (Robot.level >= TIER2_UNLOCK_LEVEL
        && Robot.role != ERobotRole::None
        && Robot.tier2_spec == ETier2Spec::None)
    {
        const TArray<ETier2Spec> Options = GetTier2ForRole(Robot.role);
        for (ETier2Spec S : Options)
        {
            FMXSpecChoice Choice;
            Choice.ChoiceTier   = 2;
            Choice.Tier2Option  = S;
            const UEnum* Enum   = StaticEnum<ETier2Spec>();
            if (Enum)
            {
                Choice.DisplayName = Enum->GetDisplayNameTextByValue((int64)S).ToString();
            }
            Choice.Description   = GetTier2Description(S);
            if (const FMXSpecDefinition* Row = FindSpecRow(Robot.role, S, ETier3Spec::None))
            {
                Choice.VisualPreview = Row->visual_indicator;
            }
            Choices.Add(Choice);
        }
        return Choices;
    }

    // Tier 3: selection at Lv20
    if (Robot.level >= TIER3_UNLOCK_LEVEL
        && Robot.tier2_spec != ETier2Spec::None
        && Robot.tier3_spec == ETier3Spec::None)
    {
        const TArray<ETier3Spec> Options = GetTier3ForTier2(Robot.tier2_spec);
        for (ETier3Spec S : Options)
        {
            FMXSpecChoice Choice;
            Choice.ChoiceTier   = 3;
            Choice.Tier3Option  = S;
            const UEnum* Enum   = StaticEnum<ETier3Spec>();
            if (Enum)
            {
                Choice.DisplayName = Enum->GetDisplayNameTextByValue((int64)S).ToString();
            }
            Choice.Description   = GetTier3Description(S);
            if (const FMXSpecDefinition* Row = FindSpecRow(Robot.role, Robot.tier2_spec, S))
            {
                Choice.VisualPreview = Row->visual_indicator;
            }
            Choices.Add(Choice);
        }
        return Choices;
    }

    // No pending choice
    return Choices;
}

// ---------------------------------------------------------------------------
// IsChoicePending
// ---------------------------------------------------------------------------

bool UMXSpecTree::IsChoicePending(const FMXRobotProfile& Robot) const
{
    if (Robot.level >= ROLE_UNLOCK_LEVEL  && Robot.role       == ERobotRole::None)   return true;
    if (Robot.level >= TIER2_UNLOCK_LEVEL && Robot.tier2_spec == ETier2Spec::None
        && Robot.role != ERobotRole::None)                                            return true;
    if (Robot.level >= TIER3_UNLOCK_LEVEL && Robot.tier3_spec == ETier3Spec::None
        && Robot.tier2_spec != ETier2Spec::None)                                      return true;
    return false;
}

// ---------------------------------------------------------------------------
// ApplyRoleChoice
// ---------------------------------------------------------------------------

void UMXSpecTree::ApplyRoleChoice(const FGuid& RobotId, ERobotRole Role)
{
    if (!RobotProvider.GetObject())
    {
        UE_LOG(LogTemp, Error, TEXT("MXSpecTree: No RobotProvider — cannot apply role choice."));
        return;
    }

    FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);

    if (!Profile.robot_id.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpecTree: ApplyRoleChoice — robot %s not found."), *RobotId.ToString());
        return;
    }

    if (!ValidateRoleChoice(Profile, Role))
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpecTree: Invalid role choice for robot '%s'."), *Profile.name);
        return;
    }

    // Note: The authoritative write goes through RobotManager (Agent 2).
    // We call ApplySpecChoice on it indirectly via the event bus broadcast.
    // MXRobotManager::ApplySpecChoice listens to OnSpecChosen and updates the profile.
    // Direct mutation here would require a write interface; instead we use the event to drive the update.
    // For modules that need immediate sync (e.g. UI), we read back via GetRobotProfile.

    Profile.role = Role;
    BroadcastSpecChosen(RobotId, Profile);

    UE_LOG(LogTemp, Log, TEXT("MXSpecTree: '%s' chose Role '%s'."),
        *Profile.name, *GetRoleDescription(Role));
}

// ---------------------------------------------------------------------------
// ApplyTier2Choice
// ---------------------------------------------------------------------------

void UMXSpecTree::ApplyTier2Choice(const FGuid& RobotId, ETier2Spec Spec)
{
    if (!RobotProvider.GetObject()) return;

    FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);
    if (!Profile.robot_id.IsValid()) return;

    if (!ValidateTier2Choice(Profile, Spec))
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpecTree: Invalid Tier2 choice for robot '%s'."), *Profile.name);
        return;
    }

    Profile.tier2_spec = Spec;
    BroadcastSpecChosen(RobotId, Profile);

    UE_LOG(LogTemp, Log, TEXT("MXSpecTree: '%s' chose Tier2 '%s'."),
        *Profile.name, *GetTier2Description(Spec));
}

// ---------------------------------------------------------------------------
// ApplyTier3Choice
// ---------------------------------------------------------------------------

void UMXSpecTree::ApplyTier3Choice(const FGuid& RobotId, ETier3Spec Spec)
{
    if (!RobotProvider.GetObject()) return;

    FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);
    if (!Profile.robot_id.IsValid()) return;

    if (!ValidateTier3Choice(Profile, Spec))
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpecTree: Invalid Tier3 choice for robot '%s'."), *Profile.name);
        return;
    }

    Profile.tier3_spec = Spec;
    BroadcastSpecChosen(RobotId, Profile);

    UE_LOG(LogTemp, Log, TEXT("MXSpecTree: '%s' chose Tier3 '%s'."),
        *Profile.name, *GetTier3Description(Spec));
}

// ---------------------------------------------------------------------------
// ResolveMasteryTitle (static)
// ---------------------------------------------------------------------------

EMasteryTitle UMXSpecTree::ResolveMasteryTitle(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3)
{
    // Scout → Pathfinder → Trailblazer : The Wayfinder
    if (Role == ERobotRole::Scout && Tier2 == ETier2Spec::Pathfinder && Tier3 == ETier3Spec::Trailblazer)
        return EMasteryTitle::TheWayfinder;

    // Scout → Pathfinder → Ghost : The Phantom
    if (Role == ERobotRole::Scout && Tier2 == ETier2Spec::Pathfinder && Tier3 == ETier3Spec::Ghost)
        return EMasteryTitle::ThePhantom;

    // Scout → Lookout → Oracle : The Seer
    if (Role == ERobotRole::Scout && Tier2 == ETier2Spec::Lookout && Tier3 == ETier3Spec::Oracle)
        return EMasteryTitle::TheSeer;

    // Scout → Lookout → Beacon : The Signal
    if (Role == ERobotRole::Scout && Tier2 == ETier2Spec::Lookout && Tier3 == ETier3Spec::Beacon)
        return EMasteryTitle::TheSignal;

    // Guardian → Sentinel → Vanguard : The Shield
    if (Role == ERobotRole::Guardian && Tier2 == ETier2Spec::Sentinel && Tier3 == ETier3Spec::Vanguard)
        return EMasteryTitle::TheShield;

    // Guardian → Sentinel → ImmortalGuard : The Undying
    if (Role == ERobotRole::Guardian && Tier2 == ETier2Spec::Sentinel && Tier3 == ETier3Spec::ImmortalGuard)
        return EMasteryTitle::TheUndying;

    // Guardian → Bulwark → Anchor : The Immovable
    if (Role == ERobotRole::Guardian && Tier2 == ETier2Spec::Bulwark && Tier3 == ETier3Spec::Anchor)
        return EMasteryTitle::TheImmovable;

    // Guardian → Bulwark → Wall : The Fortress
    if (Role == ERobotRole::Guardian && Tier2 == ETier2Spec::Bulwark && Tier3 == ETier3Spec::Wall)
        return EMasteryTitle::TheFortress;

    // Engineer → Mechanic → Savant : The Lifegiver
    if (Role == ERobotRole::Engineer && Tier2 == ETier2Spec::Mechanic && Tier3 == ETier3Spec::Savant)
        return EMasteryTitle::TheLifegiver;

    // Engineer → Mechanic → Catalyst : The Amplifier
    if (Role == ERobotRole::Engineer && Tier2 == ETier2Spec::Mechanic && Tier3 == ETier3Spec::Catalyst)
        return EMasteryTitle::TheAmplifier;

    // Engineer → Architect → Demolitionist : The Destroyer
    if (Role == ERobotRole::Engineer && Tier2 == ETier2Spec::Architect && Tier3 == ETier3Spec::Demolitionist)
        return EMasteryTitle::TheDestroyer;

    // Engineer → Architect → Foundation : The Cornerstone
    if (Role == ERobotRole::Engineer && Tier2 == ETier2Spec::Architect && Tier3 == ETier3Spec::Foundation)
        return EMasteryTitle::TheCornerstone;

    return EMasteryTitle::None;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool UMXSpecTree::ValidateRoleChoice(const FMXRobotProfile& Robot, ERobotRole Role) const
{
    if (Robot.level < ROLE_UNLOCK_LEVEL)      return false;
    if (Robot.role != ERobotRole::None)        return false;
    if (Role == ERobotRole::None)              return false;
    return true;
}

bool UMXSpecTree::ValidateTier2Choice(const FMXRobotProfile& Robot, ETier2Spec Spec) const
{
    if (Robot.level < TIER2_UNLOCK_LEVEL)         return false;
    if (Robot.role == ERobotRole::None)            return false;
    if (Robot.tier2_spec != ETier2Spec::None)      return false;
    if (Spec == ETier2Spec::None)                  return false;

    // Verify the spec belongs to the robot's role
    const TArray<ETier2Spec> Valid = GetTier2ForRole(Robot.role);
    return Valid.Contains(Spec);
}

bool UMXSpecTree::ValidateTier3Choice(const FMXRobotProfile& Robot, ETier3Spec Spec) const
{
    if (Robot.level < TIER3_UNLOCK_LEVEL)          return false;
    if (Robot.tier2_spec == ETier2Spec::None)       return false;
    if (Robot.tier3_spec != ETier3Spec::None)       return false;
    if (Spec == ETier3Spec::None)                   return false;

    // Verify the spec belongs to the robot's tier2 branch
    const TArray<ETier3Spec> Valid = GetTier3ForTier2(Robot.tier2_spec);
    return Valid.Contains(Spec);
}

// ---------------------------------------------------------------------------
// Description Queries
// ---------------------------------------------------------------------------

FString UMXSpecTree::GetRoleDescription(ERobotRole Role) const
{
    switch (Role)
    {
    case ERobotRole::Scout:
        return TEXT("Scout: +15% movement speed. Reveals nearby stranded robots on minimap. Lean build with green-tinted antenna.");
    case ERobotRole::Guardian:
        return TEXT("Guardian: Absorbs 1 lethal hit per run (refreshes between runs). -5% movement speed. Thicker chassis plating, wider stance.");
    case ERobotRole::Engineer:
        return TEXT("Engineer: +50% puzzle participation XP. Counts as 1.5 robots on weight plates. Wrench antenna tip, tool belt decal.");
    default:
        return TEXT("None");
    }
}

FString UMXSpecTree::GetTier2Description(ETier2Spec Spec) const
{
    switch (Spec)
    {
    case ETier2Spec::Pathfinder:
        return TEXT("Pathfinder: Reveals full room layout on entry. +20% total movement speed.");
    case ETier2Spec::Lookout:
        return TEXT("Lookout: Warns of hazards in 8m radius. Nearby robots auto-dodge 10% of hits.");
    case ETier2Spec::Sentinel:
        return TEXT("Sentinel: Absorbs 2 lethal hits per run. Adjacent robots take -20% damage.");
    case ETier2Spec::Bulwark:
        return TEXT("Bulwark: Counts as 2 robots on weight plates. Immune to conveyor push.");
    case ETier2Spec::Mechanic:
        return TEXT("Mechanic: Nearby robots earn +25% XP. Can revive 1 dead robot per run (at 1 HP).");
    case ETier2Spec::Architect:
        return TEXT("Architect: Counts as 3 robots on weight plates. Destructible objects take +50% damage nearby.");
    default:
        return TEXT("None");
    }
}

FString UMXSpecTree::GetTier3Description(ETier3Spec Spec) const
{
    switch (Spec)
    {
    case ETier3Spec::Trailblazer:
        return TEXT("Trailblazer: Leaves a 3-second speed boost trail for following robots.");
    case ETier3Spec::Ghost:
        return TEXT("Ghost: Can phase through 1 hazard per level.");
    case ETier3Spec::Oracle:
        return TEXT("Oracle: Predicts the next 2 hazard activation cycles.");
    case ETier3Spec::Beacon:
        return TEXT("Beacon: Rescued robots near this one gain +50% XP.");
    case ETier3Spec::Vanguard:
        return TEXT("Vanguard: Leads the swarm. Shields 3 robots directly behind.");
    case ETier3Spec::ImmortalGuard:
        return TEXT("Immortal Guard: Absorbs 3 lethal hits per run.");
    case ETier3Spec::Anchor:
        return TEXT("Anchor: Immune to all physics-based pushes.");
    case ETier3Spec::Wall:
        return TEXT("Wall: Blocks hazards in a 1m cone behind this robot.");
    case ETier3Spec::Savant:
        return TEXT("Savant: Can revive up to 3 dead robots per run.");
    case ETier3Spec::Catalyst:
        return TEXT("Catalyst: All XP gains for nearby robots are doubled.");
    case ETier3Spec::Demolitionist:
        return TEXT("Demolitionist: Destroys any destructible wall on contact.");
    case ETier3Spec::Foundation:
        return TEXT("Foundation: Counts as 5 robots on weight plates.");
    default:
        return TEXT("None");
    }
}

FString UMXSpecTree::GetMasteryDescription(EMasteryTitle Title) const
{
    switch (Title)
    {
    case EMasteryTitle::TheWayfinder:   return TEXT("The Wayfinder: Leaves a permanent glowing footprint trail.");
    case EMasteryTitle::ThePhantom:     return TEXT("The Phantom: Moves with a translucent shimmer effect.");
    case EMasteryTitle::TheSeer:        return TEXT("The Seer: A third-eye antenna glow marks this robot.");
    case EMasteryTitle::TheSignal:      return TEXT("The Signal: Emits a pulsing light ring from its chassis.");
    case EMasteryTitle::TheShield:      return TEXT("The Shield: Projects a forward-facing shield aura.");
    case EMasteryTitle::TheUndying:     return TEXT("The Undying: A flickering shield aura surrounds the chassis.");
    case EMasteryTitle::TheImmovable:   return TEXT("The Immovable: Stone-like chassis texture, immovable presence.");
    case EMasteryTitle::TheFortress:    return TEXT("The Fortress: Tiny structural beams orbit the chassis.");
    case EMasteryTitle::TheLifegiver:   return TEXT("The Lifegiver: Emits healing pulse particles.");
    case EMasteryTitle::TheAmplifier:   return TEXT("The Amplifier: Surrounded by a constant XP sparkle aura.");
    case EMasteryTitle::TheDestroyer:   return TEXT("The Destroyer: Crack aura radiates from hands.");
    case EMasteryTitle::TheCornerstone: return TEXT("The Cornerstone: Tiny structural beams orbit the chassis.");
    default:                            return TEXT("None");
    }
}

// ---------------------------------------------------------------------------
// Event Bus Handlers
// ---------------------------------------------------------------------------

void UMXSpecTree::OnLevelUp(FGuid RobotId, int32 NewLevel)
{
    if (!RobotProvider.GetObject()) return;

    FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);
    if (!Profile.robot_id.IsValid()) return;

    // Mastery is auto-assigned at Lv35 — no UI choice needed
    if (NewLevel >= MASTERY_UNLOCK_LEVEL
        && Profile.tier3_spec != ETier3Spec::None
        && Profile.mastery_title == EMasteryTitle::None)
    {
        TryAwardMastery(RobotId);
        return;
    }

    // For tiers 1–3: UI should query IsChoicePending / GetAvailableChoices
    // Log the pending choice; the UI subsystem will react via its own OnLevelUp binding
    if (IsChoicePending(Profile))
    {
        UE_LOG(LogTemp, Log, TEXT("MXSpecTree: Robot '%s' (Lv%d) has a pending spec choice."),
            *Profile.name, NewLevel);
    }
}

void UMXSpecTree::OnRunComplete(FMXRunData RunData)
{
    // No spec-tree state changes on run end. Per-run charges handled by MXRoleComponent.
}

void UMXSpecTree::OnRunFailed(FMXRunData RunData, int32 FailureLevel)
{
    // Same as above — per-run charges handled by MXRoleComponent.
}

// ---------------------------------------------------------------------------
// IMXPersistable
// ---------------------------------------------------------------------------

void UMXSpecTree::MXSerialize(FArchive& Ar)
{
    // UMXSpecTree holds no runtime state that isn't derived from the robot profiles.
    // Robot profiles are serialized by the Identity module.
}

void UMXSpecTree::MXDeserialize(FArchive& Ar)
{
    // Likewise — no unique state to restore here.
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

TArray<ETier2Spec> UMXSpecTree::GetTier2ForRole(ERobotRole Role) const
{
    switch (Role)
    {
    case ERobotRole::Scout:    return { ETier2Spec::Pathfinder, ETier2Spec::Lookout };
    case ERobotRole::Guardian: return { ETier2Spec::Sentinel,   ETier2Spec::Bulwark  };
    case ERobotRole::Engineer: return { ETier2Spec::Mechanic,   ETier2Spec::Architect };
    default:                   return {};
    }
}

TArray<ETier3Spec> UMXSpecTree::GetTier3ForTier2(ETier2Spec Tier2) const
{
    switch (Tier2)
    {
    case ETier2Spec::Pathfinder: return { ETier3Spec::Trailblazer,   ETier3Spec::Ghost        };
    case ETier2Spec::Lookout:    return { ETier3Spec::Oracle,         ETier3Spec::Beacon       };
    case ETier2Spec::Sentinel:   return { ETier3Spec::Vanguard,       ETier3Spec::ImmortalGuard };
    case ETier2Spec::Bulwark:    return { ETier3Spec::Anchor,          ETier3Spec::Wall         };
    case ETier2Spec::Mechanic:   return { ETier3Spec::Savant,          ETier3Spec::Catalyst     };
    case ETier2Spec::Architect:  return { ETier3Spec::Demolitionist,   ETier3Spec::Foundation   };
    default:                     return {};
    }
}

const FMXSpecDefinition* UMXSpecTree::FindSpecRow(ERobotRole Role, ETier2Spec Tier2, ETier3Spec Tier3) const
{
    if (!SpecTreeTable) return nullptr;

    TArray<FMXSpecDefinition*> Rows;
    SpecTreeTable->GetAllRows<FMXSpecDefinition>(TEXT("MXSpecTree::FindSpecRow"), Rows);

    for (FMXSpecDefinition* Row : Rows)
    {
        if (Row && Row->role == Role && Row->tier2 == Tier2 && Row->tier3 == Tier3)
        {
            return Row;
        }
    }
    return nullptr;
}

void UMXSpecTree::TryAwardMastery(const FGuid& RobotId)
{
    if (!RobotProvider.GetObject()) return;

    FMXRobotProfile Profile = IMXRobotProvider::Execute_GetRobotProfile(RobotProvider.GetObject(), RobotId);
    if (!Profile.robot_id.IsValid()) return;

    const EMasteryTitle Title = ResolveMasteryTitle(Profile.role, Profile.tier2_spec, Profile.tier3_spec);
    if (Title == EMasteryTitle::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXSpecTree: Robot '%s' reached Lv35 but path is incomplete — mastery not awarded."),
            *Profile.name);
        return;
    }

    Profile.mastery_title = Title;
    // Broadcast so Identity module (RobotManager) can update the profile
    BroadcastSpecChosen(RobotId, Profile);

    UE_LOG(LogTemp, Log, TEXT("MXSpecTree: Robot '%s' earned mastery title '%s'."),
        *Profile.name, *GetMasteryDescription(Title));
}

void UMXSpecTree::BroadcastSpecChosen(const FGuid& RobotId, const FMXRobotProfile& Profile)
{
    if (EventBus)
    {
        EventBus->OnSpecChosen.Broadcast(RobotId, Profile.role, Profile.tier2_spec, Profile.tier3_spec);
    }
}
