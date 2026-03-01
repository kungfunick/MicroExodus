// MXSaveManager.cpp — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Change Log:
//   v1.0 — Initial implementation

#include "MXSaveManager.h"
#include "Misc/FileHelper.h"
#include "Misc/CRC.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"

// ---------------------------------------------------------------------------
// Slot Names
// ---------------------------------------------------------------------------

static const FString SLOT_LATEST   = TEXT("save_latest.mxsv");
static const FString SLOT_BACKUP1  = TEXT("save_backup1.mxsv");
static const FString SLOT_BACKUP2  = TEXT("save_backup2.mxsv");

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXSaveManager::UMXSaveManager()
{
    // SaveDirectory resolved in Initialize()
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMXSaveManager::Initialize(UMXEventBus* InEventBus)
{
    if (bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMXSaveManager::Initialize called more than once — ignoring."));
        return;
    }

    EventBus = InEventBus;

    // Resolve save directory: <ProjectSavedDir>/MicroExodus/
    SaveDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MicroExodus"));
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*SaveDirectory))
    {
        PlatformFile.CreateDirectoryTree(*SaveDirectory);
    }

    // Bind auto-save triggers
    if (EventBus)
    {
        EventBus->OnRunComplete.AddDynamic(this, &UMXSaveManager::HandleRunComplete);
        EventBus->OnRunFailed.AddDynamic(this, &UMXSaveManager::HandleRunFailed);
        EventBus->OnHatUnlocked.AddDynamic(this, &UMXSaveManager::HandleHatUnlocked);
        EventBus->OnComboDiscovered.AddDynamic(this, &UMXSaveManager::HandleComboDiscovered);
    }

    bInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager initialized. Save directory: %s"), *SaveDirectory);
}

void UMXSaveManager::RegisterPersistable(TScriptInterface<IMXPersistable> Module)
{
    if (!Module.GetObject())
    {
        UE_LOG(LogTemp, Warning, TEXT("UMXSaveManager::RegisterPersistable — null module, skipping."));
        return;
    }
    PersistableModules.Add(Module);
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager: Registered %s for persistence."),
        *Module.GetObject()->GetName());
}

// ---------------------------------------------------------------------------
// Provider Injection
// ---------------------------------------------------------------------------

void UMXSaveManager::SetRobotProvider(TScriptInterface<IMXRobotProvider> InProvider)
{
    RobotProvider = InProvider;
}

void UMXSaveManager::SetHatProvider(TScriptInterface<IMXHatProvider> InProvider)
{
    HatProvider = InProvider;
}

void UMXSaveManager::SetRunProvider(TScriptInterface<IMXRunProvider> InProvider)
{
    RunProvider = InProvider;
}

// ---------------------------------------------------------------------------
// Core Save / Load
// ---------------------------------------------------------------------------

bool UMXSaveManager::SaveGame()
{
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager::SaveGame — starting serialization."));

    // 1. Rotate backups BEFORE overwriting latest so we never lose the last good save.
    CreateBackup();

    // 2. Serialize all module payloads.
    TArray<uint8> Payload;
    if (!SerializeModules(Payload))
    {
        UE_LOG(LogTemp, Error, TEXT("UMXSaveManager::SaveGame — module serialization failed."));
        return false;
    }

    // 3. Build header (includes CRC32 over payload).
    FMXSaveHeader Header = BuildHeader(Payload);

    // 4. Assemble final blob: header bytes + payload bytes.
    TArray<uint8> Blob;
    FMemoryWriter HeaderWriter(Blob, /*bIsPersistent=*/true);

    // Manually serialize header fields in a fixed layout.
    HeaderWriter.Serialize(Header.MagicNumber, 4);
    HeaderWriter << Header.Version;
    HeaderWriter << Header.Timestamp;
    HeaderWriter << Header.RunCount;
    HeaderWriter << Header.RosterCount;
    HeaderWriter << Header.HatCount;
    HeaderWriter << Header.Checksum;

    Blob.Append(Payload);

    // 5. Write to latest slot.
    const FString LatestPath = GetSlotPath(SLOT_LATEST);
    if (!WriteToDisk(LatestPath, Blob))
    {
        UE_LOG(LogTemp, Error, TEXT("UMXSaveManager::SaveGame — disk write failed: %s"), *LatestPath);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager::SaveGame — success. %d bytes written. Payload: %d bytes."),
        Blob.Num(), Payload.Num());
    return true;
}

bool UMXSaveManager::LoadGame()
{
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager::LoadGame — starting."));

    const FString LatestPath = GetSlotPath(SLOT_LATEST);
    TArray<uint8> Blob;

    if (!LoadAndValidateSlot(LatestPath, Blob))
    {
        UE_LOG(LogTemp, Warning, TEXT("UMXSaveManager::LoadGame — latest slot invalid. Attempting backup1."));

        // Attempt backup1
        const FString Backup1Path = GetSlotPath(SLOT_BACKUP1);
        if (!LoadAndValidateSlot(Backup1Path, Blob))
        {
            UE_LOG(LogTemp, Warning, TEXT("UMXSaveManager::LoadGame — backup1 invalid. Attempting backup2."));

            // Last resort: backup2
            const FString Backup2Path = GetSlotPath(SLOT_BACKUP2);
            if (!LoadAndValidateSlot(Backup2Path, Blob))
            {
                UE_LOG(LogTemp, Error, TEXT("UMXSaveManager::LoadGame — all slots invalid. Cannot load."));
                return false;
            }
            UE_LOG(LogTemp, Warning, TEXT("UMXSaveManager::LoadGame — restored from backup2."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UMXSaveManager::LoadGame — restored from backup1."));
        }
    }

    // Strip the header to isolate the payload.
    // Header layout: 4 (magic) + 4 (version) + 8 (datetime ticks) + 4 + 4 + 4 + 4 = 32 bytes.
    // Use a MemoryReader to parse the header properly.
    FMemoryReader Reader(Blob, /*bIsPersistent=*/true);

    char MagicRead[4];
    Reader.Serialize(MagicRead, 4);

    uint32 VersionRead = 0;
    Reader << VersionRead;

    FDateTime TimestampRead;
    Reader << TimestampRead;

    uint32 RunCountRead = 0, RosterCountRead = 0, HatCountRead = 0, ChecksumRead = 0;
    Reader << RunCountRead;
    Reader << RosterCountRead;
    Reader << HatCountRead;
    Reader << ChecksumRead;

    // Extract payload (everything after the header).
    const int64 HeaderSize = Reader.Tell();
    TArray<uint8> Payload;
    Payload.Append(Blob.GetData() + HeaderSize, Blob.Num() - (int32)HeaderSize);

    // Forward-compatibility: if a future version wrote more header fields, VersionRead > CURRENT
    // is already handled by ValidateSaveData not failing on higher versions. Payload isolation
    // above via Reader.Tell() accounts for any extra header bytes correctly.

    if (!DeserializeModules(Payload))
    {
        UE_LOG(LogTemp, Error, TEXT("UMXSaveManager::LoadGame — module deserialization failed."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager::LoadGame — success. Run#%d, %d robots, %d hats."),
        RunCountRead, RosterCountRead, HatCountRead);
    return true;
}

// ---------------------------------------------------------------------------
// Save Existence / Deletion
// ---------------------------------------------------------------------------

bool UMXSaveManager::DoesGameSaveExist() const
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.FileExists(*GetSlotPath(SLOT_LATEST));
}

void UMXSaveManager::DeleteGameSave()
{
    IFileManager& FM = IFileManager::Get();
    FM.Delete(*GetSlotPath(SLOT_LATEST));
    FM.Delete(*GetSlotPath(SLOT_BACKUP1));
    FM.Delete(*GetSlotPath(SLOT_BACKUP2));
    UE_LOG(LogTemp, Warning, TEXT("UMXSaveManager::DeleteGameSave — all save slots deleted."));
}

// ---------------------------------------------------------------------------
// Backup Management
// ---------------------------------------------------------------------------

void UMXSaveManager::CreateBackup()
{
    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    const FString LatestPath  = GetSlotPath(SLOT_LATEST);
    const FString Backup1Path = GetSlotPath(SLOT_BACKUP1);
    const FString Backup2Path = GetSlotPath(SLOT_BACKUP2);

    // Rotate: backup1 → backup2, latest → backup1.
    // Do this in reverse order to avoid overwriting before copying.
    if (PF.FileExists(*Backup1Path))
    {
        PF.CopyFile(*Backup2Path, *Backup1Path);
    }
    if (PF.FileExists(*LatestPath))
    {
        PF.CopyFile(*Backup1Path, *LatestPath);
    }
}

bool UMXSaveManager::RestoreFromBackup(int32 BackupIndex)
{
    const FString SlotPath = (BackupIndex == 0) ? GetSlotPath(SLOT_BACKUP1)
                                                 : GetSlotPath(SLOT_BACKUP2);

    TArray<uint8> Blob;
    if (!LoadAndValidateSlot(SlotPath, Blob))
    {
        UE_LOG(LogTemp, Error, TEXT("UMXSaveManager::RestoreFromBackup(%d) — slot invalid."), BackupIndex);
        return false;
    }

    // Parse header, extract payload.
    FMemoryReader Reader(Blob, /*bIsPersistent=*/true);
    char Magic[4]; Reader.Serialize(Magic, 4);
    uint32 Ver; Reader << Ver;
    FDateTime TS; Reader << TS;
    uint32 RC, RoC, HC, CS;
    Reader << RC; Reader << RoC; Reader << HC; Reader << CS;

    const int64 HeaderSize = Reader.Tell();
    TArray<uint8> Payload;
    Payload.Append(Blob.GetData() + HeaderSize, Blob.Num() - (int32)HeaderSize);

    if (!DeserializeModules(Payload))
    {
        UE_LOG(LogTemp, Error, TEXT("UMXSaveManager::RestoreFromBackup(%d) — deserialization failed."), BackupIndex);
        return false;
    }

    // Persist the restored state as the new latest.
    SaveGame();

    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager::RestoreFromBackup(%d) — success."), BackupIndex);
    return true;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool UMXSaveManager::ValidateSaveData(const TArray<uint8>& Data) const
{
    if (Data.Num() < 32)
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateSaveData — too small (%d bytes)."), Data.Num());
        return false;
    }

    // Check magic number.
    if (Data[0] != 'M' || Data[1] != 'X' || Data[2] != 'S' || Data[3] != 'V')
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateSaveData — magic number mismatch."));
        return false;
    }

    // Read version and checksum from the header.
    FMemoryReader Reader(const_cast<TArray<uint8>&>(Data), /*bIsPersistent=*/true);
    char Magic[4]; Reader.Serialize(Magic, 4);
    uint32 Version; Reader << Version;
    FDateTime TS; Reader << TS;
    uint32 RunCount, RosterCount, HatCount, StoredChecksum;
    Reader << RunCount; Reader << RosterCount; Reader << HatCount; Reader << StoredChecksum;

    // Compute CRC over everything after the header.
    const int64 HeaderSize = Reader.Tell();
    if (Data.Num() <= HeaderSize)
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateSaveData — no payload after header."));
        return false;
    }

    TArray<uint8> Payload;
    Payload.Append(Data.GetData() + HeaderSize, Data.Num() - (int32)HeaderSize);
    const uint32 ComputedChecksum = ComputeCRC32(Payload);

    if (ComputedChecksum != StoredChecksum)
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateSaveData — CRC mismatch. Stored=%u Computed=%u"),
            StoredChecksum, ComputedChecksum);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Metadata
// ---------------------------------------------------------------------------

FMXSaveMetadata UMXSaveManager::GetSaveMetadata() const
{
    FMXSaveMetadata Meta;
    Meta.bIsValid = false;

    TArray<uint8> Blob;
    if (!ReadFromDisk(GetSlotPath(SLOT_LATEST), Blob))
    {
        return Meta;
    }

    if (Blob.Num() < 32)
    {
        return Meta;
    }

    if (Blob[0] != 'M' || Blob[1] != 'X' || Blob[2] != 'S' || Blob[3] != 'V')
    {
        return Meta;
    }

    FMemoryReader Reader(Blob, /*bIsPersistent=*/true);
    char Magic[4]; Reader.Serialize(Magic, 4);
    uint32 Version; Reader << Version;
    FDateTime Timestamp; Reader << Timestamp;
    uint32 RunCount, RosterCount, HatCount, Checksum;
    Reader << RunCount; Reader << RosterCount; Reader << HatCount; Reader << Checksum;

    // Validate checksum.
    const int64 HeaderSize = Reader.Tell();
    if (Blob.Num() > HeaderSize)
    {
        TArray<uint8> Payload;
        Payload.Append(Blob.GetData() + HeaderSize, Blob.Num() - (int32)HeaderSize);
        Meta.bIsValid = (ComputeCRC32(Payload) == Checksum);
    }

    Meta.Version     = (int32)Version;
    Meta.Timestamp   = Timestamp;
    Meta.RunCount    = (int32)RunCount;
    Meta.RosterCount = (int32)RosterCount;
    Meta.HatCount    = (int32)HatCount;

    return Meta;
}

// ---------------------------------------------------------------------------
// Event Handlers
// ---------------------------------------------------------------------------

void UMXSaveManager::HandleRunComplete(FMXRunData RunData)
{
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager: Auto-save triggered by OnRunComplete."));
    SaveGame();
}

void UMXSaveManager::HandleRunFailed(FMXRunData RunData, int32 FailureLevel)
{
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager: Auto-save triggered by OnRunFailed."));
    SaveGame();
}

void UMXSaveManager::HandleHatUnlocked(int32 HatId)
{
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager: Auto-save triggered by OnHatUnlocked (hat %d)."), HatId);
    SaveGame();
}

void UMXSaveManager::HandleComboDiscovered(TArray<int32> HatIds, int32 UnlockedHatId)
{
    UE_LOG(LogTemp, Log, TEXT("UMXSaveManager: Auto-save triggered by OnComboDiscovered."));
    SaveGame();
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

bool UMXSaveManager::SerializeModules(TArray<uint8>& OutPayload) const
{
    OutPayload.Empty();
    FMemoryWriter Writer(OutPayload, /*bIsPersistent=*/true);

    for (const TScriptInterface<IMXPersistable>& Module : PersistableModules)
    {
        if (!Module.GetObject())
        {
            UE_LOG(LogTemp, Warning, TEXT("SerializeModules — null module entry, skipping."));
            continue;
        }

        // Record offset before this module so we can skip it on version mismatch.
        const int64 StartOffset = Writer.Tell();

        // Write a placeholder for the block size (4 bytes), filled in after.
        uint32 BlockSize = 0;
        Writer << BlockSize;

        const int64 DataStart = Writer.Tell();

        // Call the module's own serializer via direct C++ virtual dispatch.
        IMXPersistable* Persistable = Cast<IMXPersistable>(Module.GetObject());
        if (Persistable) Persistable->MXSerialize(Writer);

        const int64 DataEnd = Writer.Tell();
        BlockSize = (uint32)(DataEnd - DataStart);

        // Seek back and fill in the block size.
        Writer.Seek(StartOffset);
        Writer << BlockSize;
        Writer.Seek(DataEnd);
    }

    return (OutPayload.Num() > 0 || PersistableModules.Num() == 0);
}

bool UMXSaveManager::DeserializeModules(const TArray<uint8>& Payload)
{
    FMemoryReader Reader(const_cast<TArray<uint8>&>(Payload), /*bIsPersistent=*/true);

    for (const TScriptInterface<IMXPersistable>& Module : PersistableModules)
    {
        if (!Module.GetObject())
        {
            UE_LOG(LogTemp, Warning, TEXT("DeserializeModules — null module entry, skipping."));
            continue;
        }

        if (Reader.AtEnd())
        {
            // No data for this module — it may be a newly registered module from a newer version.
            // Leave it at default state rather than failing.
            UE_LOG(LogTemp, Warning, TEXT("DeserializeModules — no data for %s (newer save format?)."),
                *Module.GetObject()->GetName());
            continue;
        }

        // Read this block's byte count.
        uint32 BlockSize = 0;
        Reader << BlockSize;

        const int64 DataStart = Reader.Tell();
        const int64 DataEnd   = DataStart + BlockSize;

        // Guard: don't read past end of payload.
        if (DataEnd > Payload.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("DeserializeModules — block size %u exceeds payload for %s."),
                BlockSize, *Module.GetObject()->GetName());
            return false;
        }

        // Give the module a sub-reader scoped to exactly its block, then advance main reader.
        TArray<uint8> BlockData;
        BlockData.Append(Payload.GetData() + DataStart, BlockSize);
        FMemoryReader BlockReader(BlockData, /*bIsPersistent=*/true);

        IMXPersistable* Persistable = Cast<IMXPersistable>(Module.GetObject());
        if (Persistable) Persistable->MXDeserialize(BlockReader);

        // Skip to end of block in the main reader (forward-compatibility: if the module wrote
        // fewer bytes than we allocated space for, we skip the remainder).
        Reader.Seek(DataEnd);
    }

    return true;
}

FMXSaveHeader UMXSaveManager::BuildHeader(const TArray<uint8>& Payload) const
{
    FMXSaveHeader Header;
    Header.Version   = CURRENT_SAVE_VERSION;
    Header.Timestamp = FDateTime::Now();
    Header.Checksum  = ComputeCRC32(Payload);

    // Populate counts from providers if available.
    if (RunProvider.GetObject())
    {
        Header.RunCount = (uint32)IMXRunProvider::Execute_GetRunNumber(RunProvider.GetObject());
    }
    if (RobotProvider.GetObject())
    {
        Header.RosterCount = (uint32)IMXRobotProvider::Execute_GetRobotCount(RobotProvider.GetObject());
    }
    if (HatProvider.GetObject())
    {
        const FMXHatCollection Col = IMXHatProvider::Execute_GetHatCollection(HatProvider.GetObject());
        Header.HatCount = (uint32)Col.unlocked_hat_ids.Num();
    }

    return Header;
}

bool UMXSaveManager::WriteToDisk(const FString& SlotPath, const TArray<uint8>& Data) const
{
    return FFileHelper::SaveArrayToFile(Data, *SlotPath);
}

bool UMXSaveManager::ReadFromDisk(const FString& SlotPath, TArray<uint8>& OutData) const
{
    return FFileHelper::LoadFileToArray(OutData, *SlotPath);
}

uint32 UMXSaveManager::ComputeCRC32(const TArray<uint8>& Data) const
{
    return FCrc::MemCrc32(Data.GetData(), Data.Num(), 0u);
}

FString UMXSaveManager::GetSlotPath(const FString& SlotName) const
{
    return FPaths::Combine(SaveDirectory, SlotName);
}

bool UMXSaveManager::LoadAndValidateSlot(const FString& SlotPath, TArray<uint8>& OutData) const
{
    if (!ReadFromDisk(SlotPath, OutData))
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadAndValidateSlot — file not found: %s"), *SlotPath);
        return false;
    }

    if (!ValidateSaveData(OutData))
    {
        UE_LOG(LogTemp, Warning, TEXT("LoadAndValidateSlot — validation failed: %s"), *SlotPath);
        OutData.Empty();
        return false;
    }

    return true;
}
