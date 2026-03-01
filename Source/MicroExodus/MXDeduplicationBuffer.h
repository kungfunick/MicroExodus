// MXDeduplicationBuffer.h — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS
// Prevents the narrator from saying the same thing twice in a row.
// A ring buffer of (verb, source, flavor) tuples checked before every dispatch.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MXConstants.h"
#include "MXDeduplicationBuffer.generated.h"

// ---------------------------------------------------------------------------
// FMXDedupEntry
// One slot in the deduplication ring buffer.
// ---------------------------------------------------------------------------

USTRUCT()
struct FMXDedupEntry
{
    GENERATED_BODY()

    /** The verb component of the resolved variety roll. */
    UPROPERTY()
    FString Verb;

    /** The source noun component of the resolved variety roll. */
    UPROPERTY()
    FString Source;

    /** The flavor suffix component (may be empty). */
    UPROPERTY()
    FString Flavor;
};

// ---------------------------------------------------------------------------
// UMXDeduplicationBuffer
// Fixed-size ring buffer for message deduplication.
// ---------------------------------------------------------------------------

UCLASS(BlueprintType)
class MICROEXODUS_API UMXDeduplicationBuffer : public UObject
{
    GENERATED_BODY()

public:

    UMXDeduplicationBuffer();

    /**
     * Push a new (Verb, Source, Flavor) combo into the ring buffer.
     * Overwrites the oldest entry when the buffer is full.
     * Call this after dispatching a message — not before checking.
     * @param Verb    The resolved past-tense verb.
     * @param Source  The resolved hazard source noun.
     * @param Flavor  The resolved flavor suffix (may be empty string).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dedup")
    void Push(const FString& Verb, const FString& Source, const FString& Flavor);

    /**
     * Check whether this (Verb, Source, Flavor) combo already exists in the buffer.
     * @param Verb    Verb to check.
     * @param Source  Source to check.
     * @param Flavor  Flavor to check.
     * @return True if a matching combo is found — caller should re-roll.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dedup")
    bool IsDuplicate(const FString& Verb, const FString& Source, const FString& Flavor) const;

    /**
     * Wipe all entries from the ring buffer.
     * Call this at the start of each new run to prevent stale deduplication.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dedup")
    void Clear();

    /**
     * Returns the number of entries currently held in the buffer.
     * Useful for debug logging and tests.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|DEMS|Dedup")
    int32 GetEntryCount() const;

private:

    /** The ring buffer storage. Fixed capacity of DEDUP_BUFFER_SIZE. */
    UPROPERTY()
    TArray<FMXDedupEntry> Buffer;

    /** Current write index. Advances and wraps on every Push(). */
    int32 WriteIndex;

    /** Tracks how many entries have been written (saturates at DEDUP_BUFFER_SIZE). */
    int32 EntryCount;
};
