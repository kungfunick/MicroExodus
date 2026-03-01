// MXMemorialSerializer.cpp — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Change Log:
//   v1.0 — Initial implementation

#include "MXMemorialSerializer.h"

// ---------------------------------------------------------------------------
// Memorial Serialization
// ---------------------------------------------------------------------------

void UMXMemorialSerializer::SerializeMemorial(FArchive& Ar, const TArray<FMXMemorialEntry>& Entries)
{
    // Archive before writing so old entries aren't storing full visual state on disk.
    TArray<FMXMemorialEntry> WorkingCopy = Entries;
    ArchiveOldEntries(WorkingCopy);

    int32 Count = WorkingCopy.Num();
    Ar << Count;
    for (const FMXMemorialEntry& Entry : WorkingCopy)
    {
        SerializeEntry(Ar, Entry);
    }

    UE_LOG(LogTemp, Log, TEXT("MXMemorialSerializer: Serialized %d memorial entries."), Count);
}

TArray<FMXMemorialEntry> UMXMemorialSerializer::DeserializeMemorial(FArchive& Ar)
{
    TArray<FMXMemorialEntry> Result;

    int32 Count = 0;
    Ar << Count;

    if (Count < 0 || Count > 100000)
    {
        UE_LOG(LogTemp, Error, TEXT("MXMemorialSerializer::DeserializeMemorial — implausible count: %d"), Count);
        return Result;
    }

    Result.Reserve(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); ++i)
    {
        Result.Add(DeserializeEntry(Ar));
    }

    UE_LOG(LogTemp, Log, TEXT("MXMemorialSerializer: Deserialized %d memorial entries."), Count);
    return Result;
}

// ---------------------------------------------------------------------------
// Entry Management
// ---------------------------------------------------------------------------

void UMXMemorialSerializer::AddToMemorial(
    TArray<FMXMemorialEntry>& Memorial,
    const FMXRobotProfile&    DeadRobot,
    const FMXEventData&       DeathEvent,
    const FString&            Eulogy,
    int32                     RunNumber,
    int32                     LevelNumber,
    bool                      bSacrifice,
    const TArray<FString>&    SavedRobots)
{
    FMXMemorialEntry Entry;
    Entry.ProfileSnapshot    = DeadRobot;
    Entry.DeathEvent         = DeathEvent;
    Entry.Eulogy             = Eulogy;
    Entry.RunNumber          = RunNumber;
    Entry.LevelNumber        = LevelNumber;
    Entry.DateOfDeath        = FDateTime::Now();
    Entry.bWasSacrifice      = bSacrifice;
    Entry.RobotsSaved        = SavedRobots;
    Entry.bVisualStateArchived = false;

    Memorial.Add(Entry);

    UE_LOG(LogTemp, Log, TEXT("MXMemorialSerializer: Added '%s' to memorial. Total entries: %d."),
        *DeadRobot.name, Memorial.Num());
}

void UMXMemorialSerializer::ArchiveOldEntries(TArray<FMXMemorialEntry>& Memorial, int32 KeepFullCount)
{
    const int32 ArchiveCount = Memorial.Num() - KeepFullCount;
    if (ArchiveCount <= 0) return;

    int32 Pruned = 0;
    for (int32 i = 0; i < ArchiveCount; ++i)
    {
        FMXMemorialEntry& Entry = Memorial[i];
        if (!Entry.bVisualStateArchived)
        {
            // Zero out only the visual state — everything else stays intact forever.
            Entry.ProfileSnapshot.visual_evolution_state = FMXVisualEvolutionState();
            Entry.bVisualStateArchived = true;
            ++Pruned;
        }
    }

    if (Pruned > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("MXMemorialSerializer::ArchiveOldEntries — archived visual state for %d entries."),
            Pruned);
    }
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

FMXMemorialEntry* UMXMemorialSerializer::GetMemorialEntry(
    TArray<FMXMemorialEntry>& Memorial, const FGuid& RobotId)
{
    for (FMXMemorialEntry& Entry : Memorial)
    {
        if (Entry.ProfileSnapshot.robot_id == RobotId)
        {
            return &Entry;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Private: Single Entry Serialization
// ---------------------------------------------------------------------------

void UMXMemorialSerializer::SerializeEntry(FArchive& Ar, const FMXMemorialEntry& Entry)
{
    // Block-size prefix for forward compatibility.
    const int64 SizePos = Ar.Tell();
    uint32 BlockSize = 0;
    Ar << BlockSize;
    const int64 DataStart = Ar.Tell();

    // Mutable copy — FArchive << requires non-const lvalue references.
    FMXMemorialEntry E = Entry;

    // ---- Profile snapshot ----
    // We always preserve: robot_id, name, date_of_birth, level, total_xp, role, mastery_title,
    // earned_titles, displayed_title, description, quirk, hat_history, visual_evolution_state.
    // (Visual state is zeroed by ArchiveOldEntries for old entries but the field is still written.)

    FGuid RobId = E.ProfileSnapshot.robot_id;   Ar << RobId;
    FString Name = E.ProfileSnapshot.name;       Ar << Name;
    FDateTime DOB = E.ProfileSnapshot.date_of_birth; Ar << DOB;
    int32 Level  = E.ProfileSnapshot.level;      Ar << Level;
    int32 TotXP  = E.ProfileSnapshot.total_xp;  Ar << TotXP;
    int32 RunsSurv = E.ProfileSnapshot.runs_survived; Ar << RunsSurv;
    int32 LvlComp  = E.ProfileSnapshot.levels_completed; Ar << LvlComp;

    uint8 Role   = (uint8)E.ProfileSnapshot.role;          Ar << Role;
    uint8 T2     = (uint8)E.ProfileSnapshot.tier2_spec;    Ar << T2;
    uint8 T3     = (uint8)E.ProfileSnapshot.tier3_spec;    Ar << T3;
    uint8 Mast   = (uint8)E.ProfileSnapshot.mastery_title; Ar << Mast;

    FString Desc  = E.ProfileSnapshot.description; Ar << Desc;
    FString Quirk = E.ProfileSnapshot.quirk;       Ar << Quirk;
    TArray<FString> Likes    = E.ProfileSnapshot.likes;    Ar << Likes;
    TArray<FString> Dislikes = E.ProfileSnapshot.dislikes; Ar << Dislikes;

    TArray<FString> Titles   = E.ProfileSnapshot.earned_titles; Ar << Titles;
    FString DispTitle = E.ProfileSnapshot.displayed_title;       Ar << DispTitle;

    TArray<int32> HatHistory = E.ProfileSnapshot.hat_history; Ar << HatHistory;
    int32 CurrentHat = E.ProfileSnapshot.current_hat_id;      Ar << CurrentHat;

    // Encounter / social stats (useful for memorial wall display).
    Ar << E.ProfileSnapshot.near_misses;
    Ar << E.ProfileSnapshot.fire_encounters;
    Ar << E.ProfileSnapshot.crush_encounters;
    Ar << E.ProfileSnapshot.fall_encounters;
    Ar << E.ProfileSnapshot.emp_encounters;
    Ar << E.ProfileSnapshot.ice_encounters;
    Ar << E.ProfileSnapshot.rescues_witnessed;
    Ar << E.ProfileSnapshot.deaths_witnessed;
    Ar << E.ProfileSnapshot.sacrifices_witnessed;

    // Visual evolution state (may be zeroed if archived).
    bool bVisualsArchived = E.bVisualStateArchived;
    Ar << bVisualsArchived;
    if (!bVisualsArchived)
    {
        uint8 WearLevel = (uint8)E.ProfileSnapshot.visual_evolution_state.wear_level;
        Ar << WearLevel;
        Ar << E.ProfileSnapshot.visual_evolution_state.burn_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.crack_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.weld_coverage;
        Ar << E.ProfileSnapshot.visual_evolution_state.patina_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.eye_brightness;
        Ar << E.ProfileSnapshot.visual_evolution_state.eye_somber;
        Ar << E.ProfileSnapshot.visual_evolution_state.fall_scar_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.ice_residue;
        Ar << E.ProfileSnapshot.visual_evolution_state.electrical_burn;
        Ar << E.ProfileSnapshot.visual_evolution_state.antenna_stage;
        Ar << E.ProfileSnapshot.visual_evolution_state.armor_stage;
        Ar << E.ProfileSnapshot.visual_evolution_state.aura_stage;
        Ar << E.ProfileSnapshot.visual_evolution_state.decal_seed;
    }

    // ---- Death event (key narrative fields only) ----
    uint8 EvType = (uint8)E.DeathEvent.event_type; Ar << EvType;
    FString MsgText = E.DeathEvent.message_text;   Ar << MsgText;
    uint8 HazElem = (uint8)E.DeathEvent.hazard_element; Ar << HazElem;
    int32 EvRun = E.DeathEvent.run_number;         Ar << EvRun;
    int32 EvLvl = E.DeathEvent.level_number;       Ar << EvLvl;
    FString HatWornName = E.DeathEvent.hat_worn_name; Ar << HatWornName;

    // ---- Memorial metadata ----
    FString Eulogy = E.Eulogy;       Ar << Eulogy;
    int32 RunNum   = E.RunNumber;    Ar << RunNum;
    int32 LvlNum   = E.LevelNumber; Ar << LvlNum;
    FDateTime DOD  = E.DateOfDeath; Ar << DOD;
    bool bSac      = E.bWasSacrifice; Ar << bSac;
    TArray<FString> Saved = E.RobotsSaved; Ar << Saved;

    // Fill block size.
    const int64 DataEnd = Ar.Tell();
    BlockSize = (uint32)(DataEnd - DataStart);
    Ar.Seek(SizePos);
    Ar << BlockSize;
    Ar.Seek(DataEnd);
}

FMXMemorialEntry UMXMemorialSerializer::DeserializeEntry(FArchive& Ar)
{
    FMXMemorialEntry Entry;
    FMXMemorialEntry& E = Entry;  // alias — serialization uses E throughout

    uint32 BlockSize = 0;
    Ar << BlockSize;
    const int64 DataStart = Ar.Tell();
    const int64 DataEnd   = DataStart + (int64)BlockSize;

    // Profile snapshot
    Ar << E.ProfileSnapshot.robot_id;
    Ar << E.ProfileSnapshot.name;
    Ar << E.ProfileSnapshot.date_of_birth;
    Ar << E.ProfileSnapshot.level;
    Ar << E.ProfileSnapshot.total_xp;
    Ar << E.ProfileSnapshot.runs_survived;
    Ar << E.ProfileSnapshot.levels_completed;

    uint8 Role = 0;  Ar << Role;  E.ProfileSnapshot.role          = (ERobotRole)Role;
    uint8 T2   = 0;  Ar << T2;    E.ProfileSnapshot.tier2_spec    = (ETier2Spec)T2;
    uint8 T3   = 0;  Ar << T3;    E.ProfileSnapshot.tier3_spec    = (ETier3Spec)T3;
    uint8 Mast = 0;  Ar << Mast;  E.ProfileSnapshot.mastery_title = (EMasteryTitle)Mast;

    Ar << E.ProfileSnapshot.description;
    Ar << E.ProfileSnapshot.quirk;
    Ar << E.ProfileSnapshot.likes;
    Ar << E.ProfileSnapshot.dislikes;
    Ar << E.ProfileSnapshot.earned_titles;
    Ar << E.ProfileSnapshot.displayed_title;
    Ar << E.ProfileSnapshot.hat_history;
    Ar << E.ProfileSnapshot.current_hat_id;

    Ar << E.ProfileSnapshot.near_misses;
    Ar << E.ProfileSnapshot.fire_encounters;
    Ar << E.ProfileSnapshot.crush_encounters;
    Ar << E.ProfileSnapshot.fall_encounters;
    Ar << E.ProfileSnapshot.emp_encounters;
    Ar << E.ProfileSnapshot.ice_encounters;
    Ar << E.ProfileSnapshot.rescues_witnessed;
    Ar << E.ProfileSnapshot.deaths_witnessed;
    Ar << E.ProfileSnapshot.sacrifices_witnessed;

    // Visual state
    Ar << E.bVisualStateArchived;
    if (!E.bVisualStateArchived)
    {
        uint8 WearLevel = 0;
        Ar << WearLevel;
        E.ProfileSnapshot.visual_evolution_state.wear_level = (EWearLevel)WearLevel;
        Ar << E.ProfileSnapshot.visual_evolution_state.burn_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.crack_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.weld_coverage;
        Ar << E.ProfileSnapshot.visual_evolution_state.patina_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.eye_brightness;
        Ar << E.ProfileSnapshot.visual_evolution_state.eye_somber;
        Ar << E.ProfileSnapshot.visual_evolution_state.fall_scar_intensity;
        Ar << E.ProfileSnapshot.visual_evolution_state.ice_residue;
        Ar << E.ProfileSnapshot.visual_evolution_state.electrical_burn;
        Ar << E.ProfileSnapshot.visual_evolution_state.antenna_stage;
        Ar << E.ProfileSnapshot.visual_evolution_state.armor_stage;
        Ar << E.ProfileSnapshot.visual_evolution_state.aura_stage;
        Ar << E.ProfileSnapshot.visual_evolution_state.decal_seed;
    }

    // Death event
    uint8 EvType = 0;
    Ar << EvType; E.DeathEvent.event_type = (EEventType)EvType;
    Ar << E.DeathEvent.message_text;
    uint8 HazElem = 0;
    Ar << HazElem; E.DeathEvent.hazard_element = (EHazardElement)HazElem;
    Ar << E.DeathEvent.run_number;
    Ar << E.DeathEvent.level_number;
    Ar << E.DeathEvent.hat_worn_name;

    // Memorial metadata
    Ar << E.Eulogy;
    Ar << E.RunNumber;
    Ar << E.LevelNumber;
    Ar << E.DateOfDeath;
    Ar << E.bWasSacrifice;
    Ar << E.RobotsSaved;

    // Forward-compatible: skip unknown trailing fields from newer versions.
    if (Ar.Tell() < DataEnd)
    {
        Ar.Seek(DataEnd);
    }

    return Entry;
}
