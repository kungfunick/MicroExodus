// MXHatStackEconomy.cpp — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Change Log:
//   v1.0 — Initial implementation

#include "MXHatStackEconomy.h"
#include "MXConstants.h"

// ---------------------------------------------------------------------------
// Core Stack Operations
// ---------------------------------------------------------------------------

int32 UMXHatStackEconomy::GetStackCount(int32 HatId, const FMXHatCollection& Collection) const
{
    const FMXHatStack* Stack = FindStackConst(HatId, Collection);
    return Stack ? Stack->count : 0;
}

void UMXHatStackEconomy::IncrementStack(int32 HatId, int32 Amount, FMXHatCollection& Collection)
{
    if (Amount <= 0) return;

    FMXHatStack* Stack = FindStack(HatId, Collection);
    if (!Stack)
    {
        // Create new entry
        FMXHatStack NewStack;
        NewStack.hat_id = HatId;
        NewStack.count  = 0;
        Collection.stacks.Add(NewStack);
        Stack = &Collection.stacks.Last();
    }

    const int32 NewCount = Stack->count + Amount;
    if (NewCount > MX::MAX_HAT_STACK)
    {
        UE_LOG(LogTemp, Verbose, TEXT("UMXHatStackEconomy: Hat %d stack capped at %d (attempted %d)."),
            HatId, MX::MAX_HAT_STACK, NewCount);
        Stack->count = MX::MAX_HAT_STACK;
    }
    else
    {
        Stack->count = NewCount;
    }
}

bool UMXHatStackEconomy::DecrementStack(int32 HatId, int32 Amount, FMXHatCollection& Collection)
{
    if (Amount <= 0) return true;

    FMXHatStack* Stack = FindStack(HatId, Collection);
    if (!Stack || Stack->count < Amount)
    {
        return false; // Insufficient copies — do NOT mutate
    }

    Stack->count -= Amount;
    return true;
}

void UMXHatStackEconomy::EnsureStackEntry(int32 HatId, FMXHatCollection& Collection)
{
    if (!FindStack(HatId, Collection))
    {
        FMXHatStack NewStack;
        NewStack.hat_id = HatId;
        NewStack.count  = 0;
        Collection.stacks.Add(NewStack);
    }
}

// ---------------------------------------------------------------------------
// Scarcity Queries
// ---------------------------------------------------------------------------

int32 UMXHatStackEconomy::GetRarestEquippedHat(
    const TArray<int32>& EquippedHatIds,
    const FMXHatCollection& Collection) const
{
    if (EquippedHatIds.IsEmpty()) return -1;

    int32 RarestHatId    = EquippedHatIds[0];
    int32 LowestCount    = GetStackCount(RarestHatId, Collection);

    for (int32 i = 1; i < EquippedHatIds.Num(); ++i)
    {
        const int32 Count = GetStackCount(EquippedHatIds[i], Collection);
        if (Count < LowestCount)
        {
            LowestCount = Count;
            RarestHatId = EquippedHatIds[i];
        }
    }

    return RarestHatId;
}

int32 UMXHatStackEconomy::GetTotalHatCopies(const FMXHatCollection& Collection) const
{
    int32 Total = 0;
    for (const FMXHatStack& Stack : Collection.stacks)
    {
        Total += Stack.count;
    }
    return Total;
}

TArray<FMXScarcityRecord> UMXHatStackEconomy::GetScarcityReport(
    const FMXHatCollection& Collection,
    const TMap<int32, FMXHatDefinition>& HatDefinitions) const
{
    TArray<FMXScarcityRecord> Report;

    for (const int32 HatId : Collection.unlocked_hat_ids)
    {
        FMXScarcityRecord Record;
        Record.hat_id = HatId;
        Record.count  = GetStackCount(HatId, Collection);

        if (const FMXHatDefinition* Def = HatDefinitions.Find(HatId))
        {
            Record.rarity = Def->rarity;
        }

        Report.Add(Record);
    }

    // Sort by count ascending (scarcest first)
    Report.Sort([](const FMXScarcityRecord& A, const FMXScarcityRecord& B)
    {
        return A.count < B.count;
    });

    return Report;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

FMXHatStack* UMXHatStackEconomy::FindStack(int32 HatId, FMXHatCollection& Collection)
{
    for (FMXHatStack& Stack : Collection.stacks)
    {
        if (Stack.hat_id == HatId)
        {
            return &Stack;
        }
    }
    return nullptr;
}

const FMXHatStack* UMXHatStackEconomy::FindStackConst(int32 HatId, const FMXHatCollection& Collection)
{
    for (const FMXHatStack& Stack : Collection.stacks)
    {
        if (Stack.hat_id == HatId)
        {
            return &Stack;
        }
    }
    return nullptr;
}
