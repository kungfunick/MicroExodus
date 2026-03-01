// MXSaveManager.h — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Consumers: GameInstance (owns and calls), all IMXPersistable modules
// Change Log:
//   v1.0 — Initial implementation: rolling 3-slot backups, CRC32 validation, versioned header

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "MXInterfaces.h"
#include "MXEvents.h"
#include "MXSaveManager.generated.h"

// ---------------------------------------------------------------------------
// Save File Header — written at the start of every save blob
// ---------------------------------------------------------------------------

/**
 * FMXSaveHeader
 * Fixed-layout binary header. Written first; read first.
 * Version field enables forward-compatible deserialization.
 */
USTRUCT()
struct FMXSaveHeader
{
    GENERATED_BODY()

    /** Four-byte magic constant. Always "MXSV". Detects completely corrupt / wrong files. */
    char MagicNumber[4] = { 'M', 'X', 'S', 'V' };

    /** Incremented whenever the serialization format changes. Current: 1. */
    uint32 Version = 1;

    /** Wall-clock time at which this save was written. */
    FDateTime Timestamp = FDateTime::MinValue();

    /** Total runs completed at save time. Mirrored in the header for fast UI display. */
    uint32 RunCount = 0;

    /** Number of living robots on the roster at save time. */
    uint32 RosterCount = 0;

    /** Number of permanently unlocked hat types at save time. */
    uint32 HatCount = 0;

    /**
     * CRC32 of all bytes that follow this header in the save blob.
     * Computed after all module data is serialized; verified before deserialization.
     */
    uint32 Checksum = 0;
};

// ---------------------------------------------------------------------------
// Save Metadata — lightweight view returned to UI without full deserialization
// ---------------------------------------------------------------------------

/**
 * FMXSaveMetadata
 * Populated by reading only the save header — no module deserialization required.
 * Used to display save info on the main menu.
 */
USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXSaveMetadata
{
    GENERATED_BODY()

    /** Save format version number. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Save")
    int32 Version = 0;

    /** Timestamp of when the save was written. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Save")
    FDateTime Timestamp = FDateTime::MinValue();

    /** Total completed runs logged in the save. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Save")
    int32 RunCount = 0;

    /** Living robots on the roster at save time. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Save")
    int32 RosterCount = 0;

    /** Permanently unlocked hats at save time. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Save")
    int32 HatCount = 0;

    /** True if the header passed magic + checksum validation. */
    UPROPERTY(BlueprintReadOnly, Category = "MX|Save")
    bool bIsValid = false;
};

// ---------------------------------------------------------------------------
// UMXSaveManager
// ---------------------------------------------------------------------------

/**
 * UMXSaveManager
 * Master coordinator for all save/load operations.
 *
 * Architecture:
 *   - Maintains a list of IMXPersistable modules registered at startup.
 *   - On save: serializes all modules in a defined order into a single byte array,
 *     prepends a validated header (CRC32 over payload), writes to the latest slot,
 *     rotates backups.
 *   - On load: reads the blob, validates header, distributes payload to each module's
 *     Deserialize() in the same order. Forward-compatibility: if Version > current,
 *     unknown trailing data is skipped rather than causing a fault.
 *   - Keeps three slots: save_latest, save_backup1, save_backup2 (oldest).
 *
 * Module serialization order (must not change between versions without version bump):
 *   1. Identity (roster + life logs)
 *   2. Hats (collection + paint jobs)
 *   3. Roguelike (run count, tiers, modifiers)
 *   4. Reports (report archive + all-time records)
 *   5. Specialization (per-run charges)
 *
 * Usage:
 *   GameInstance::Init → RegisterPersistable() × N → LoadGame()
 *   End of run         → SaveGame()
 *   Manual save        → SaveGame()
 *   Hat unlock         → SaveGame()
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API UMXSaveManager : public UObject
{
    GENERATED_BODY()

public:

    UMXSaveManager();

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * Initialize the save manager and bind to the event bus for auto-save triggers.
     * Must be called once from GameInstance::Init before any other methods.
     *
     * @param InEventBus    The shared UMXEventBus instance.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    void Initialize(UMXEventBus* InEventBus);

    /**
     * Register an IMXPersistable implementor for inclusion in save/load cycles.
     * Must be called for every module before the first LoadGame(). Order of registration
     * determines serialization order; do not change after shipping.
     *
     * @param Module    A UObject implementing IMXPersistable.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    void RegisterPersistable(TScriptInterface<IMXPersistable> Module);

    // -------------------------------------------------------------------------
    // Core Save / Load
    // -------------------------------------------------------------------------

    /**
     * Serialize all registered modules, validate, and write to disk.
     * Rotates backups before writing the new latest slot.
     *
     * @return True on success; false if serialization or disk write failed.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    bool SaveGame();

    /**
     * Read the latest save from disk, validate header + checksum, and distribute
     * data to all registered modules via their Deserialize() methods.
     *
     * @return True on success; false if file missing, corrupt, or validation failed.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    bool LoadGame();

    // -------------------------------------------------------------------------
    // Save Existence / Deletion
    // -------------------------------------------------------------------------

    /** @return True if a latest save file exists on disk. */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    bool DoesGameSaveExist() const;

    /**
     * Permanently delete all save slots including backups.
     * Irreversible — caller must show a confirmation dialog before calling this.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    void DeleteGameSave();

    // -------------------------------------------------------------------------
    // Backup Management
    // -------------------------------------------------------------------------

    /**
     * Manually rotate the backup chain without triggering a new save.
     * Normally called automatically inside SaveGame() — exposed for emergency use.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    void CreateBackup();

    /**
     * Attempt to restore from a backup slot and apply it as the current game state.
     *
     * @param BackupIndex   0 = most recent backup, 1 = older backup.
     * @return              True if the backup was valid and successfully loaded.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    bool RestoreFromBackup(int32 BackupIndex);

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /**
     * Validate the magic number, version, and CRC32 checksum of a raw save blob.
     * Does NOT deserialize any module data.
     *
     * @param Data  Raw bytes as read from disk.
     * @return      True if all checks pass.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    bool ValidateSaveData(const TArray<uint8>& Data) const;

    // -------------------------------------------------------------------------
    // Metadata
    // -------------------------------------------------------------------------

    /**
     * Read only the save header from the latest slot — fast, no module deserialization.
     * Useful for main menu display (run count, timestamp, roster size).
     *
     * @return Populated FMXSaveMetadata; bIsValid will be false if header is corrupt.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    FMXSaveMetadata GetSaveMetadata() const;

    // -------------------------------------------------------------------------
    // Provider Injection (for metadata population)
    // -------------------------------------------------------------------------

    /** Inject a robot provider so the save header can read roster count. */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    void SetRobotProvider(TScriptInterface<IMXRobotProvider> InProvider);

    /** Inject a hat provider so the save header can read hat count. */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    void SetHatProvider(TScriptInterface<IMXHatProvider> InProvider);

    /** Inject a run provider so the save header can read run count. */
    UFUNCTION(BlueprintCallable, Category = "MX|Persistence")
    void SetRunProvider(TScriptInterface<IMXRunProvider> InProvider);

private:

    // -------------------------------------------------------------------------
    // Event Handlers (bound in Initialize)
    // -------------------------------------------------------------------------

    UFUNCTION()
    void HandleRunComplete(FMXRunData RunData);

    UFUNCTION()
    void HandleRunFailed(FMXRunData RunData, int32 FailureLevel);

    UFUNCTION()
    void HandleHatUnlocked(int32 HatId);

    UFUNCTION()
    void HandleComboDiscovered(TArray<int32> HatIds, int32 UnlockedHatId);

    // -------------------------------------------------------------------------
    // Internal Helpers
    // -------------------------------------------------------------------------

    /** Serialize all registered modules into OutBytes. Does NOT write the header. */
    bool SerializeModules(TArray<uint8>& OutPayload) const;

    /** Deserialize all registered modules from a payload array (header already stripped). */
    bool DeserializeModules(const TArray<uint8>& Payload);

    /** Build and return a fully populated FMXSaveHeader for the given payload. */
    FMXSaveHeader BuildHeader(const TArray<uint8>& Payload) const;

    /** Write raw bytes to a named save slot file. */
    bool WriteToDisk(const FString& SlotPath, const TArray<uint8>& Data) const;

    /** Read raw bytes from a named save slot file. Returns false if missing/unreadable. */
    bool ReadFromDisk(const FString& SlotPath, TArray<uint8>& OutData) const;

    /** Compute CRC32 over the given byte array. */
    uint32 ComputeCRC32(const TArray<uint8>& Data) const;

    /** Absolute filesystem path for a slot name ("save_latest", "save_backup1", etc.). */
    FString GetSlotPath(const FString& SlotName) const;

    /** Load and validate raw bytes from a slot; returns false on any failure. */
    bool LoadAndValidateSlot(const FString& SlotPath, TArray<uint8>& OutData) const;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /** All modules registered for serialization, in registration (= serialization) order. */
    UPROPERTY()
    TArray<TScriptInterface<IMXPersistable>> PersistableModules;

    /** Cached event bus reference. */
    UPROPERTY()
    TObjectPtr<UMXEventBus> EventBus;

    /** Optional robot provider for header metadata. */
    UPROPERTY()
    TScriptInterface<IMXRobotProvider> RobotProvider;

    /** Optional hat provider for header metadata. */
    UPROPERTY()
    TScriptInterface<IMXHatProvider> HatProvider;

    /** Optional run provider for header metadata. */
    UPROPERTY()
    TScriptInterface<IMXRunProvider> RunProvider;

    /** Directory where save files are written. Resolved in Initialize(). */
    FString SaveDirectory;

    /** Current save format version. Bump this any time the module serialization order changes. */
    static constexpr uint32 CURRENT_SAVE_VERSION = 1;

    /** Number of backup slots maintained alongside the latest save. */
    static constexpr int32 NUM_BACKUP_SLOTS = 2;

    bool bInitialized = false;
};
