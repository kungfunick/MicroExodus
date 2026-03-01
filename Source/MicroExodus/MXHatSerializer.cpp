// MXHatSerializer.cpp — Persistence Module Version 1.0
// Created: 2026-02-24
// Agent 10: Persistence
// Change Log:
//   v1.0 — Initial implementation

#include "MXHatSerializer.h"

// ---------------------------------------------------------------------------
// Hat Collection Serialization
// ---------------------------------------------------------------------------

void UMXHatSerializer::SerializeHatCollection(FArchive& Ar, const FMXHatCollection& Collection)
{
    // 1. Unlocked hat IDs.
    TArray<int32> UnlockedIds = Collection.unlocked_hat_ids;
    Ar << UnlockedIds;

    // 2. Non-zero stacks only (sparse storage — avoid writing 100 zero-count entries).
    TArray<FMXHatStack> NonZeroStacks;
    for (const FMXHatStack& Stack : Collection.stacks)
    {
        if (Stack.count > 0)
        {
            NonZeroStacks.Add(Stack);
        }
    }

    int32 StackCount = NonZeroStacks.Num();
    Ar << StackCount;
    for (FMXHatStack& Stack : NonZeroStacks)
    {
        Ar << Stack.hat_id;
        Ar << Stack.count;
    }

    // 3. Discovered combo IDs.
    TArray<int32> ComboIds = Collection.discovered_combo_ids;
    Ar << ComboIds;

    UE_LOG(LogTemp, Log, TEXT("MXHatSerializer: Serialized %d unlocked hats, %d non-zero stacks, %d combos."),
        UnlockedIds.Num(), StackCount, ComboIds.Num());
}

FMXHatCollection UMXHatSerializer::DeserializeHatCollection(FArchive& Ar)
{
    FMXHatCollection Collection;

    // 1. Unlocked hat IDs.
    Ar << Collection.unlocked_hat_ids;

    // 2. Non-zero stacks.
    int32 StackCount = 0;
    Ar << StackCount;

    if (StackCount < 0 || StackCount > MXConstants::MAX_HATS)
    {
        UE_LOG(LogTemp, Error, TEXT("MXHatSerializer::DeserializeHatCollection — invalid stack count: %d"), StackCount);
        return FMXHatCollection();
    }

    for (int32 i = 0; i < StackCount; ++i)
    {
        FMXHatStack Stack;
        Ar << Stack.hat_id;
        Ar << Stack.count;
        Collection.stacks.Add(Stack);
    }

    // 3. Discovered combo IDs.
    Ar << Collection.discovered_combo_ids;

    UE_LOG(LogTemp, Log, TEXT("MXHatSerializer: Deserialized %d unlocked hats, %d stacks, %d combos."),
        Collection.unlocked_hat_ids.Num(), Collection.stacks.Num(), Collection.discovered_combo_ids.Num());

    return Collection;
}

// ---------------------------------------------------------------------------
// Paint Job Serialization
// ---------------------------------------------------------------------------

void UMXHatSerializer::SerializePaintJobs(FArchive& Ar, const TArray<EPaintJob>& UnlockedPaintJobs)
{
    int32 Count = UnlockedPaintJobs.Num();
    Ar << Count;
    for (EPaintJob PJ : UnlockedPaintJobs)
    {
        uint8 Val = (uint8)PJ;
        Ar << Val;
    }
}

TArray<EPaintJob> UMXHatSerializer::DeserializePaintJobs(FArchive& Ar)
{
    TArray<EPaintJob> Result;

    int32 Count = 0;
    Ar << Count;

    // Sanity: max 10 paint job milestones defined in the game.
    if (Count < 0 || Count > 32)
    {
        UE_LOG(LogTemp, Error, TEXT("MXHatSerializer::DeserializePaintJobs — implausible count: %d"), Count);
        return Result;
    }

    Result.Reserve(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        uint8 Val = 0;
        Ar << Val;
        Result.Add((EPaintJob)Val);
    }

    return Result;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool UMXHatSerializer::ValidateHatCollection(const FMXHatCollection& Collection) const
{
    bool bValid = true;

    for (const int32 HatId : Collection.unlocked_hat_ids)
    {
        if (HatId < 0 || HatId >= MXConstants::MAX_HATS)
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateHatCollection — unlocked_hat_ids contains out-of-range ID: %d"), HatId);
            bValid = false;
        }
    }

    for (const FMXHatStack& Stack : Collection.stacks)
    {
        if (Stack.hat_id < 0 || Stack.hat_id >= MXConstants::MAX_HATS)
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateHatCollection — stack has out-of-range hat_id: %d"), Stack.hat_id);
            bValid = false;
        }
        if (Stack.count < 0 || Stack.count > MXConstants::MAX_HAT_STACK)
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateHatCollection — stack hat %d has invalid count: %d"),
                Stack.hat_id, Stack.count);
            bValid = false;
        }
    }

    for (const int32 ComboId : Collection.discovered_combo_ids)
    {
        if (ComboId < 0 || ComboId >= MXConstants::MAX_HATS)
        {
            UE_LOG(LogTemp, Error, TEXT("ValidateHatCollection — discovered_combo_ids contains out-of-range ID: %d"), ComboId);
            bValid = false;
        }
    }

    return bValid;
}
