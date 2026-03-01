// MXRosterSerializer.cpp — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Change Log:
//   v1.0 — Initial implementation

#include "MXRosterSerializer.h"
#include "MXConstants.h"

// ---------------------------------------------------------------------------
// Roster Serialization
// ---------------------------------------------------------------------------

void UMXRosterSerializer::SerializeRoster(FArchive& Ar, const TMap<FGuid, FMXRobotProfile>& Roster)
{
    // Write count so deserialization knows how many profiles follow.
    int32 Count = Roster.Num();
    Ar << Count;

    for (const auto& Pair : Roster)
    {
        SerializeProfile(Ar, Pair.Value);
    }

    UE_LOG(LogTemp, Log, TEXT("MXRosterSerializer: Serialized %d robot profiles."), Count);
}

TMap<FGuid, FMXRobotProfile> UMXRosterSerializer::DeserializeRoster(FArchive& Ar)
{
    TMap<FGuid, FMXRobotProfile> Result;

    int32 Count = 0;
    Ar << Count;

    if (Count < 0 || Count > MXConstants::MAX_ROBOTS * 2)  // Sanity cap: 2× max for safety margin
    {
        UE_LOG(LogTemp, Error, TEXT("MXRosterSerializer::DeserializeRoster — invalid count: %d"), Count);
        return Result;
    }

    Result.Reserve(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        FMXRobotProfile Profile = DeserializeProfile(Ar);
        if (Profile.robot_id.IsValid())
        {
            Result.Add(Profile.robot_id, Profile);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MXRosterSerializer: Profile %d had invalid GUID, skipping."), i);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXRosterSerializer: Deserialized %d robot profiles."), Result.Num());
    return Result;
}

// ---------------------------------------------------------------------------
// Life Log Serialization
// ---------------------------------------------------------------------------

void UMXRosterSerializer::SerializeLifeLogs(FArchive& Ar, const TMap<FGuid, TArray<FMXEventData>>& Logs)
{
    int32 MapCount = Logs.Num();
    Ar << MapCount;

    for (const auto& Pair : Logs)
    {
        FGuid RobotId = Pair.Key;
        Ar << RobotId;

        int32 EventCount = Pair.Value.Num();
        Ar << EventCount;

        for (const FMXEventData& Ev : Pair.Value)
        {
            SerializeEvent(Ar, Ev);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXRosterSerializer: Serialized life logs for %d robots."), MapCount);
}

TMap<FGuid, TArray<FMXEventData>> UMXRosterSerializer::DeserializeLifeLogs(FArchive& Ar)
{
    TMap<FGuid, TArray<FMXEventData>> Result;

    int32 MapCount = 0;
    Ar << MapCount;

    if (MapCount < 0 || MapCount > MXConstants::MAX_ROBOTS * 2)
    {
        UE_LOG(LogTemp, Error, TEXT("MXRosterSerializer::DeserializeLifeLogs — invalid map count: %d"), MapCount);
        return Result;
    }

    Result.Reserve(MapCount);
    for (int32 i = 0; i < MapCount; ++i)
    {
        FGuid RobotId;
        Ar << RobotId;

        int32 EventCount = 0;
        Ar << EventCount;

        // Sanity cap: life logs cannot legitimately hold millions of events.
        if (EventCount < 0 || EventCount > 100000)
        {
            UE_LOG(LogTemp, Warning, TEXT("MXRosterSerializer: Implausible event count %d for robot %s, skipping."),
                EventCount, *RobotId.ToString());
            continue;
        }

        TArray<FMXEventData> Events;
        Events.Reserve(EventCount);
        for (int32 j = 0; j < EventCount; ++j)
        {
            Events.Add(DeserializeEvent(Ar));
        }

        Result.Add(RobotId, MoveTemp(Events));
    }

    UE_LOG(LogTemp, Log, TEXT("MXRosterSerializer: Deserialized life logs for %d robots."), Result.Num());
    return Result;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool UMXRosterSerializer::ValidateRoster(const TMap<FGuid, FMXRobotProfile>& Roster) const
{
    TSet<FGuid> SeenIds;
    bool bValid = true;

    for (const auto& Pair : Roster)
    {
        const FMXRobotProfile& P = Pair.Value;

        // Duplicate ID check.
        if (SeenIds.Contains(P.robot_id))
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateRoster — duplicate robot_id: %s"), *P.robot_id.ToString());
            bValid = false;
        }
        SeenIds.Add(P.robot_id);

        // GUID validity.
        if (!P.robot_id.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateRoster — invalid GUID for robot '%s'."), *P.name);
            bValid = false;
        }

        // Level range.
        if (P.level < 1 || P.level > 100)
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateRoster — robot '%s' has out-of-range level: %d."), *P.name, P.level);
            bValid = false;
        }

        // XP non-negative.
        if (P.xp < 0 || P.total_xp < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateRoster — robot '%s' has negative XP."), *P.name);
            bValid = false;
        }

        // Run history sanity.
        if (P.runs_survived < 0 || P.levels_completed < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateRoster — robot '%s' has negative run history."), *P.name);
            bValid = false;
        }
    }

    return bValid;
}

// ---------------------------------------------------------------------------
// Life Log Pruning
// ---------------------------------------------------------------------------

void UMXRosterSerializer::PruneLifeLogs(TMap<FGuid, TArray<FMXEventData>>& Logs, int32 MaxEntriesPerRobot)
{
    for (auto& Pair : Logs)
    {
        TArray<FMXEventData>& Events = Pair.Value;

        if (Events.Num() <= MaxEntriesPerRobot)
        {
            continue;  // Within budget, nothing to do.
        }

        // Separate pinned events (always kept) from regular ones.
        TArray<FMXEventData> Pinned;
        TArray<FMXEventData> Regular;
        for (const FMXEventData& Ev : Events)
        {
            if (IsPinnedEventType(Ev.event_type))
            {
                Pinned.Add(Ev);
            }
            else
            {
                Regular.Add(Ev);
            }
        }

        // How many regular events can we keep?
        const int32 PinnedCount  = Pinned.Num();
        const int32 RegularBudget = FMath::Max(0, MaxEntriesPerRobot - PinnedCount);

        // Keep only the most recent regular events.
        if (Regular.Num() > RegularBudget)
        {
            const int32 DropCount = Regular.Num() - RegularBudget;
            Regular.RemoveAt(0, DropCount);
        }

        // Rebuild events array: pinned first (chronological within pinned), then recent regulars.
        // Sort by timestamp to merge cleanly.
        TArray<FMXEventData> Merged;
        Merged.Append(Pinned);
        Merged.Append(Regular);
        Merged.Sort([](const FMXEventData& A, const FMXEventData& B)
        {
            return A.timestamp < B.timestamp;
        });

        const int32 Before = Events.Num();
        Events = MoveTemp(Merged);

        UE_LOG(LogTemp, Log, TEXT("MXRosterSerializer::PruneLifeLogs — robot %s: %d → %d events."),
            *Pair.Key.ToString(), Before, Events.Num());
    }
}

// ---------------------------------------------------------------------------
// Per-Profile Serialization Helpers
// ---------------------------------------------------------------------------

void UMXRosterSerializer::SerializeProfile(FArchive& Ar, const FMXRobotProfile& Profile)
{
    // Serialize all fields in a stable order matching DeserializeProfile.
    // Future additions go at the END; use block-size prefixing in a future version if needed.

    // --- Identity ---
    FGuid Id = Profile.robot_id;
    Ar << Id;
    FString Name = Profile.name;
    Ar << Name;
    FDateTime DOB = Profile.date_of_birth;
    Ar << DOB;

    // --- Age ---
    FTimespan CalAge = Profile.calendar_age;
    Ar << CalAge;
    float ActiveAge = Profile.active_age_seconds;
    Ar << ActiveAge;

    // --- Progression ---
    int32 Level = Profile.level;
    Ar << Level;
    int32 XP = Profile.xp;
    Ar << XP;
    int32 TotalXP = Profile.total_xp;
    Ar << TotalXP;

    // --- Run / Level History ---
    int32 RunsSurvived = Profile.runs_survived;
    Ar << RunsSurvived;
    int32 LevelsCompleted = Profile.levels_completed;
    Ar << LevelsCompleted;
    int32 RunStreak = Profile.current_run_streak;
    Ar << RunStreak;

    // --- Encounter Stats ---
    int32 NearMisses      = Profile.near_misses;        Ar << NearMisses;
    int32 FireEnc         = Profile.fire_encounters;    Ar << FireEnc;
    int32 CrushEnc        = Profile.crush_encounters;   Ar << CrushEnc;
    int32 FallEnc         = Profile.fall_encounters;    Ar << FallEnc;
    int32 EmpEnc          = Profile.emp_encounters;     Ar << EmpEnc;
    int32 IceEnc          = Profile.ice_encounters;     Ar << IceEnc;

    // --- Social Stats ---
    int32 RescuesWit      = Profile.rescues_witnessed;      Ar << RescuesWit;
    int32 DeathsWit       = Profile.deaths_witnessed;       Ar << DeathsWit;
    int32 SacrificesWit   = Profile.sacrifices_witnessed;   Ar << SacrificesWit;
    int32 PuzzlesPartic   = Profile.puzzles_participated;   Ar << PuzzlesPartic;

    // --- Appearance ---
    int32 ChassisColor = Profile.chassis_color;   Ar << ChassisColor;
    int32 EyeColor     = Profile.eye_color;       Ar << EyeColor;
    int32 AntennaStyle = Profile.antenna_style;   Ar << AntennaStyle;
    uint8 PaintJob     = (uint8)Profile.paint_job; Ar << PaintJob;

    // --- Specialization ---
    uint8 Role        = (uint8)Profile.role;         Ar << Role;
    uint8 Tier2Spec   = (uint8)Profile.tier2_spec;   Ar << Tier2Spec;
    uint8 Tier3Spec   = (uint8)Profile.tier3_spec;   Ar << Tier3Spec;
    uint8 Mastery     = (uint8)Profile.mastery_title; Ar << Mastery;

    // --- Hat ---
    int32 CurrentHatId       = Profile.current_hat_id;       Ar << CurrentHatId;
    TArray<int32> HatHistory = Profile.hat_history;           Ar << HatHistory;
    int32 ConsecHatRuns      = Profile.consecutive_hat_runs;  Ar << ConsecHatRuns;
    int32 ConsecHatId        = Profile.consecutive_hat_id;    Ar << ConsecHatId;

    // --- Personality ---
    FString Desc   = Profile.description; Ar << Desc;
    FString Quirk  = Profile.quirk;       Ar << Quirk;
    TArray<FString> Likes    = Profile.likes;    Ar << Likes;
    TArray<FString> Dislikes = Profile.dislikes; Ar << Dislikes;

    // --- Titles ---
    TArray<FString> EarnedTitles  = Profile.earned_titles;  Ar << EarnedTitles;
    FString DisplayedTitle        = Profile.displayed_title; Ar << DisplayedTitle;

    // --- Visual Evolution State ---
    uint8  WearLevel   = (uint8)Profile.visual_evolution_state.wear_level;
    Ar << WearLevel;
    float BurnInt      = Profile.visual_evolution_state.burn_intensity;      Ar << BurnInt;
    float CrackInt     = Profile.visual_evolution_state.crack_intensity;     Ar << CrackInt;
    float WeldCov      = Profile.visual_evolution_state.weld_coverage;       Ar << WeldCov;
    float PatinaInt    = Profile.visual_evolution_state.patina_intensity;    Ar << PatinaInt;
    float EyeBright    = Profile.visual_evolution_state.eye_brightness;      Ar << EyeBright;
    float EyeSomber    = Profile.visual_evolution_state.eye_somber;          Ar << EyeSomber;
    float FallScar     = Profile.visual_evolution_state.fall_scar_intensity; Ar << FallScar;
    float IceResidue   = Profile.visual_evolution_state.ice_residue;         Ar << IceResidue;
    float ElecBurn     = Profile.visual_evolution_state.electrical_burn;     Ar << ElecBurn;
    int32 AntennaStage = Profile.visual_evolution_state.antenna_stage;       Ar << AntennaStage;
    int32 ArmorStage   = Profile.visual_evolution_state.armor_stage;         Ar << ArmorStage;
    int32 AuraStage    = Profile.visual_evolution_state.aura_stage;          Ar << AuraStage;
    int32 DecalSeed    = Profile.visual_evolution_state.decal_seed;          Ar << DecalSeed;
}

FMXRobotProfile UMXRosterSerializer::DeserializeProfile(FArchive& Ar)
{
    FMXRobotProfile P;

    // --- Identity ---
    Ar << P.robot_id;
    Ar << P.name;
    Ar << P.date_of_birth;

    // --- Age ---
    Ar << P.calendar_age;
    Ar << P.active_age_seconds;

    // --- Progression ---
    Ar << P.level;
    Ar << P.xp;
    Ar << P.total_xp;

    // --- Run / Level History ---
    Ar << P.runs_survived;
    Ar << P.levels_completed;
    Ar << P.current_run_streak;

    // --- Encounter Stats ---
    Ar << P.near_misses;
    Ar << P.fire_encounters;
    Ar << P.crush_encounters;
    Ar << P.fall_encounters;
    Ar << P.emp_encounters;
    Ar << P.ice_encounters;

    // --- Social Stats ---
    Ar << P.rescues_witnessed;
    Ar << P.deaths_witnessed;
    Ar << P.sacrifices_witnessed;
    Ar << P.puzzles_participated;

    // --- Appearance ---
    Ar << P.chassis_color;
    Ar << P.eye_color;
    Ar << P.antenna_style;
    uint8 PaintJob = 0; Ar << PaintJob; P.paint_job = (EPaintJob)PaintJob;

    // --- Specialization ---
    uint8 Role = 0;     Ar << Role;     P.role         = (ERobotRole)Role;
    uint8 T2   = 0;     Ar << T2;       P.tier2_spec   = (ETier2Spec)T2;
    uint8 T3   = 0;     Ar << T3;       P.tier3_spec   = (ETier3Spec)T3;
    uint8 Mast = 0;     Ar << Mast;     P.mastery_title = (EMasteryTitle)Mast;

    // --- Hat ---
    Ar << P.current_hat_id;
    Ar << P.hat_history;
    Ar << P.consecutive_hat_runs;
    Ar << P.consecutive_hat_id;

    // --- Personality ---
    Ar << P.description;
    Ar << P.quirk;
    Ar << P.likes;
    Ar << P.dislikes;

    // --- Titles ---
    Ar << P.earned_titles;
    Ar << P.displayed_title;

    // --- Visual Evolution State ---
    uint8 WearLevel = 0; Ar << WearLevel;
    P.visual_evolution_state.wear_level = (EWearLevel)WearLevel;
    Ar << P.visual_evolution_state.burn_intensity;
    Ar << P.visual_evolution_state.crack_intensity;
    Ar << P.visual_evolution_state.weld_coverage;
    Ar << P.visual_evolution_state.patina_intensity;
    Ar << P.visual_evolution_state.eye_brightness;
    Ar << P.visual_evolution_state.eye_somber;
    Ar << P.visual_evolution_state.fall_scar_intensity;
    Ar << P.visual_evolution_state.ice_residue;
    Ar << P.visual_evolution_state.electrical_burn;
    Ar << P.visual_evolution_state.antenna_stage;
    Ar << P.visual_evolution_state.armor_stage;
    Ar << P.visual_evolution_state.aura_stage;
    Ar << P.visual_evolution_state.decal_seed;

    return P;
}

void UMXRosterSerializer::SerializeEvent(FArchive& Ar, const FMXEventData& Ev)
{
    // Write a block-size prefix for forward-compatible skipping.
    // We'll fill it in after writing all fields.
    const int64 SizePos = Ar.Tell();
    uint32 BlockSize = 0;
    Ar << BlockSize;
    const int64 DataStart = Ar.Tell();

    FDateTime TS = Ev.timestamp;      Ar << TS;
    int32 RunNum  = Ev.run_number;    Ar << RunNum;
    int32 LvlNum  = Ev.level_number;  Ar << LvlNum;
    uint8 EvType  = (uint8)Ev.event_type; Ar << EvType;

    FGuid RobId = Ev.robot_id;        Ar << RobId;
    FString RobName = Ev.robot_name;  Ar << RobName;
    int32 RobLvl = Ev.robot_level;   Ar << RobLvl;
    FString RobSpec = Ev.robot_spec;  Ar << RobSpec;

    int32 HatWornId  = Ev.hat_worn_id;   Ar << HatWornId;
    FString HatWornName = Ev.hat_worn_name; Ar << HatWornName;

    uint8 HazElem  = (uint8)Ev.hazard_element; Ar << HazElem;
    uint8 DmgType  = (uint8)Ev.damage_type;    Ar << DmgType;

    FString Verb    = Ev.verb;       Ar << Verb;
    FString Source  = Ev.source;     Ar << Source;
    FString Flavor  = Ev.flavor;     Ar << Flavor;
    FString Prep    = Ev.preposition; Ar << Prep;
    uint8   Sev     = (uint8)Ev.severity;           Ar << Sev;
    uint8   CamBeh  = (uint8)Ev.camera_behavior;    Ar << CamBeh;
    FString VisMark = Ev.visual_mark;               Ar << VisMark;
    FString MsgText = Ev.message_text;              Ar << MsgText;

    FGuid RelRobId  = Ev.related_robot_id;           Ar << RelRobId;
    FString RelName = Ev.related_robot_name;         Ar << RelName;

    // Fill block size.
    const int64 DataEnd = Ar.Tell();
    BlockSize = (uint32)(DataEnd - DataStart);
    Ar.Seek(SizePos);
    Ar << BlockSize;
    Ar.Seek(DataEnd);
}

FMXEventData UMXRosterSerializer::DeserializeEvent(FArchive& Ar)
{
    FMXEventData Ev;

    uint32 BlockSize = 0;
    Ar << BlockSize;
    const int64 DataStart = Ar.Tell();
    const int64 DataEnd   = DataStart + BlockSize;

    Ar << Ev.timestamp;
    Ar << Ev.run_number;
    Ar << Ev.level_number;
    uint8 EvType = 0; Ar << EvType; Ev.event_type = (EEventType)EvType;

    Ar << Ev.robot_id;
    Ar << Ev.robot_name;
    Ar << Ev.robot_level;
    Ar << Ev.robot_spec;

    Ar << Ev.hat_worn_id;
    Ar << Ev.hat_worn_name;

    uint8 HazElem = 0;  Ar << HazElem; Ev.hazard_element = (EHazardElement)HazElem;
    uint8 DmgType = 0;  Ar << DmgType; Ev.damage_type    = (EDamageType)DmgType;

    Ar << Ev.verb;
    Ar << Ev.source;
    Ar << Ev.flavor;
    Ar << Ev.preposition;
    uint8 Sev = 0;   Ar << Sev;   Ev.severity         = (ESeverity)Sev;
    uint8 Cam = 0;   Ar << Cam;   Ev.camera_behavior  = (ECameraBehavior)Cam;
    Ar << Ev.visual_mark;
    Ar << Ev.message_text;
    Ar << Ev.related_robot_id;
    Ar << Ev.related_robot_name;

    // Forward-compatible: skip any extra bytes added in newer versions.
    if (Ar.Tell() < DataEnd)
    {
        Ar.Seek(DataEnd);
    }

    return Ev;
}

bool UMXRosterSerializer::IsPinnedEventType(EEventType Type) const
{
    return (Type == EEventType::Death      ||
            Type == EEventType::Sacrifice  ||
            Type == EEventType::LevelUp);
}
