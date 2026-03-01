// MXDeduplicationBuffer.cpp — DEMS Module v1.0
// Created: 2026-02-22
// Agent 1: DEMS

#include "MXDeduplicationBuffer.h"
#include "MXConstants.h"

UMXDeduplicationBuffer::UMXDeduplicationBuffer()
    : WriteIndex(0)
    , EntryCount(0)
{
    // Pre-allocate the buffer to avoid reallocation during gameplay.
    Buffer.SetNum(MXConstants::DEDUP_BUFFER_SIZE);
}

void UMXDeduplicationBuffer::Push(const FString& Verb, const FString& Source, const FString& Flavor)
{
    // Write at current head and advance the ring.
    Buffer[WriteIndex].Verb   = Verb;
    Buffer[WriteIndex].Source = Source;
    Buffer[WriteIndex].Flavor = Flavor;

    WriteIndex = (WriteIndex + 1) % MXConstants::DEDUP_BUFFER_SIZE;
    EntryCount = FMath::Min(EntryCount + 1, MXConstants::DEDUP_BUFFER_SIZE);
}

bool UMXDeduplicationBuffer::IsDuplicate(const FString& Verb, const FString& Source, const FString& Flavor) const
{
    for (int32 i = 0; i < EntryCount; ++i)
    {
        const FMXDedupEntry& Entry = Buffer[i];
        if (Entry.Verb == Verb && Entry.Source == Source && Entry.Flavor == Flavor)
        {
            return true;
        }
    }
    return false;
}

void UMXDeduplicationBuffer::Clear()
{
    WriteIndex = 0;
    EntryCount = 0;

    // Wipe strings to free memory and prevent ghost matches.
    for (FMXDedupEntry& Entry : Buffer)
    {
        Entry.Verb.Empty();
        Entry.Source.Empty();
        Entry.Flavor.Empty();
    }
}

int32 UMXDeduplicationBuffer::GetEntryCount() const
{
    return EntryCount;
}
