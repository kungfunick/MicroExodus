// MXHatManager.cpp — Module Version 1.0
// Created: 2026-02-22
// Agent 3: Hats
// Change Log:
//   v1.0 — Initial implementation

#include "MXHatManager.h"
#include "MXHatStackEconomy.h"
#include "MXComboDetector.h"
#include "MXHatUnlockChecker.h"
#include "MXConstants.h"
#include "Engine/DataTable.h"
#include "JsonObjectConverter.h"
#include "Misc/DefaultValueHelper.h"

// ---------------------------------------------------------------------------
// Paint job milestone thresholds: unlock count → EPaintJob
// ---------------------------------------------------------------------------

namespace MXPaintMilestones
{
    static const TArray<TPair<int32, EPaintJob>> Thresholds =
    {
        {  10, EPaintJob::MatteBlack    },
        {  20, EPaintJob::Chrome        },
        {  30, EPaintJob::Camo          },
        {  40, EPaintJob::NeonGlow      },
        {  50, EPaintJob::Wooden        },
        {  60, EPaintJob::Marble        },
        {  70, EPaintJob::PixelArt      },
        {  80, EPaintJob::Holographic   },
        {  90, EPaintJob::Invisible     },
        { 100, EPaintJob::PureGold      },
    };
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXHatManager::UMXHatManager()
{
    // Sub-components created in Initialize()
}

// ---------------------------------------------------------------------------
// Initialize / Shutdown
// ---------------------------------------------------------------------------

void UMXHatManager::Initialize(UMXEventBus* InEventBus, UObject* InRobotProviderObject)
{
    if (bInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("UMXHatManager::Initialize called more than once — ignoring."));
        return;
    }

    EventBus            = InEventBus;
    RobotProviderObject = InRobotProviderObject;

    // Create sub-components
    StackEconomy  = NewObject<UMXHatStackEconomy>(this);
    ComboDetector = NewObject<UMXComboDetector>(this);
    UnlockChecker = NewObject<UMXHatUnlockChecker>(this);

    UnlockChecker->Initialize(RobotProviderObject);

    // Load hat definitions DataTable
    LoadHatDefinitions();

    // Bind event bus listeners
    if (EventBus)
    {
        EventBus->OnRobotDied.AddDynamic(this, &UMXHatManager::HandleRobotDied);
        EventBus->OnRunComplete.AddDynamic(this, &UMXHatManager::HandleRunComplete);
        EventBus->OnRunFailed.AddDynamic(this, &UMXHatManager::HandleRunFailed);
        EventBus->OnLevelComplete.AddDynamic(this, &UMXHatManager::HandleLevelComplete);
    }

    bInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("UMXHatManager initialized. Hat definitions loaded: %d"), HatDefinitions.Num());
}

void UMXHatManager::Shutdown()
{
    if (EventBus)
    {
        EventBus->OnRobotDied.RemoveDynamic(this, &UMXHatManager::HandleRobotDied);
        EventBus->OnRunComplete.RemoveDynamic(this, &UMXHatManager::HandleRunComplete);
        EventBus->OnRunFailed.RemoveDynamic(this, &UMXHatManager::HandleRunFailed);
        EventBus->OnLevelComplete.RemoveDynamic(this, &UMXHatManager::HandleLevelComplete);
    }
    bInitialized = false;
}

// ---------------------------------------------------------------------------
// IMXHatProvider
// ---------------------------------------------------------------------------

FMXHatDefinition UMXHatManager::GetHatData_Implementation(int32 HatId) const
{
    if (const FMXHatDefinition* Found = HatDefinitions.Find(HatId))
    {
        return *Found;
    }
    UE_LOG(LogTemp, Warning, TEXT("UMXHatManager::GetHatData — hat_id %d not found."), HatId);
    return FMXHatDefinition{};
}

FMXHatCollection UMXHatManager::GetHatCollection_Implementation() const
{
    return Collection;
}

bool UMXHatManager::ConsumeHat_Implementation(int32 HatId)
{
    return StackEconomy->DecrementStack(HatId, 1, Collection);
}

void UMXHatManager::ReturnHat_Implementation(int32 HatId)
{
    StackEconomy->IncrementStack(HatId, 1, Collection);
}

// ---------------------------------------------------------------------------
// Equip / Unequip
// ---------------------------------------------------------------------------

bool UMXHatManager::EquipHat(const FGuid& RobotId, int32 HatId)
{
    if (!bInitialized) return false;

    if (!IsHatUnlocked(HatId))
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipHat: Hat %d is not unlocked."), HatId);
        return false;
    }

    if (ActiveEquipments.Contains(RobotId))
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipHat: Robot %s already has a hat equipped."), *RobotId.ToString());
        return false;
    }

    if (!ConsumeHat(HatId))
    {
        UE_LOG(LogTemp, Log, TEXT("EquipHat: Hat %d stack is empty — cannot equip."), HatId);
        return false;
    }

    ActiveEquipments.Add(RobotId, HatId);

    if (EventBus)
    {
        EventBus->OnHatEquipped.Broadcast(RobotId, HatId);
    }

    return true;
}

void UMXHatManager::UnequipHat(const FGuid& RobotId)
{
    if (!bInitialized) return;

    if (int32* HatId = ActiveEquipments.Find(RobotId))
    {
        ReturnHat(*HatId);
        ActiveEquipments.Remove(RobotId);
    }
}

// ---------------------------------------------------------------------------
// Unlock System
// ---------------------------------------------------------------------------

void UMXHatManager::UnlockHat(int32 HatId)
{
    if (IsHatUnlocked(HatId)) return;

    Collection.unlocked_hat_ids.AddUnique(HatId);

    // Initialize a zero-count stack entry for this hat
    StackEconomy->EnsureStackEntry(HatId, Collection);

    if (EventBus)
    {
        EventBus->OnHatUnlocked.Broadcast(HatId);
    }

    UE_LOG(LogTemp, Log, TEXT("UMXHatManager: Hat %d unlocked. Total unlocked: %d"), HatId, GetUnlockedCount());
}

bool UMXHatManager::IsHatUnlocked(int32 HatId) const
{
    return Collection.unlocked_hat_ids.Contains(HatId);
}

int32 UMXHatManager::GetUnlockedCount() const
{
    return Collection.unlocked_hat_ids.Num();
}

// ---------------------------------------------------------------------------
// Run Economy
// ---------------------------------------------------------------------------

void UMXHatManager::DepositRunReward()
{
    for (int32 HatId : Collection.unlocked_hat_ids)
    {
        StackEconomy->IncrementStack(HatId, 1, Collection);
    }

    UE_LOG(LogTemp, Log, TEXT("UMXHatManager::DepositRunReward — incremented %d hat stacks."),
        Collection.unlocked_hat_ids.Num());
}

// ---------------------------------------------------------------------------
// Paint Job Milestones
// ---------------------------------------------------------------------------

TArray<EPaintJob> UMXHatManager::CheckPaintJobUnlocks()
{
    TArray<EPaintJob> NewlyEarned;
    const int32 UnlockedCount = GetUnlockedCount();

    for (const auto& [Threshold, PaintJob] : MXPaintMilestones::Thresholds)
    {
        if (UnlockedCount >= Threshold && !AwardedPaintJobs.Contains(PaintJob))
        {
            AwardedPaintJobs.Add(PaintJob);
            NewlyEarned.Add(PaintJob);
            UE_LOG(LogTemp, Log, TEXT("Paint job milestone reached: %d hats → %d"),
                Threshold, static_cast<int32>(PaintJob));
        }
    }

    return NewlyEarned;
}

// ---------------------------------------------------------------------------
// Collection Queries
// ---------------------------------------------------------------------------

TArray<int32> UMXHatManager::GetCurrentlyEquippedHatIds() const
{
    TArray<int32> Result;
    ActiveEquipments.GenerateValueArray(Result);
    return Result;
}

int32 UMXHatManager::GetStackCount(int32 HatId) const
{
    return StackEconomy->GetStackCount(HatId, Collection);
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

FString UMXHatManager::SerializeCollection() const
{
    FString JsonString;
    FJsonObjectConverter::UStructToJsonObjectString(Collection, JsonString, 0, 0);
    return JsonString;
}

bool UMXHatManager::DeserializeCollection(const FString& JsonString)
{
    FMXHatCollection Loaded;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Loaded, 0, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("UMXHatManager::DeserializeCollection — JSON parse failed."));
        return false;
    }

    Collection = Loaded;

    // Restore AwardedPaintJobs set from current unlocked count
    AwardedPaintJobs.Empty();
    const int32 Count = GetUnlockedCount();
    for (const auto& [Threshold, PaintJob] : MXPaintMilestones::Thresholds)
    {
        if (Count >= Threshold)
        {
            AwardedPaintJobs.Add(PaintJob);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UMXHatManager::DeserializeCollection — restored %d unlocked hats."),
        Collection.unlocked_hat_ids.Num());
    return true;
}

// ---------------------------------------------------------------------------
// Event Handlers
// ---------------------------------------------------------------------------

void UMXHatManager::HandleRobotDied(FGuid RobotId, FMXEventData DeathEvent)
{
    // Check if this robot was wearing a hat
    if (int32* HatId = ActiveEquipments.Find(RobotId))
    {
        const int32 LostHatId = *HatId;
        ActiveEquipments.Remove(RobotId);

        // The hat is NOT returned — it is destroyed with the robot
        // (ConsumeHat was already called at equip time; stack was already decremented)

        if (EventBus)
        {
            EventBus->OnHatLost.Broadcast(RobotId, LostHatId, DeathEvent);
        }

        UE_LOG(LogTemp, Log, TEXT("Hat %d lost with robot %s."), LostHatId, *RobotId.ToString());
    }
}

void UMXHatManager::HandleRunComplete(FMXRunData RunData)
{
    // Return hats for all surviving equipped robots
    TArray<FGuid> SurvivingRobots;
    ActiveEquipments.GenerateKeyArray(SurvivingRobots);
    for (const FGuid& RobotId : SurvivingRobots)
    {
        UnequipHat(RobotId);
    }
    ActiveEquipments.Empty();

    // Deposit stack reward
    DepositRunReward();

    // Check combos
    RunComboCheck();

    // Check standard, hidden, legendary unlocks
    RunEndUnlockChecks(RunData);

    // Check paint job milestones
    CheckPaintJobUnlocks();
}

void UMXHatManager::HandleRunFailed(FMXRunData RunData, int32 FailureLevel)
{
    // Hats on dead robots were already destroyed via HandleRobotDied
    // Clear any remaining equipped hats (they go down with the run — not returned)
    ActiveEquipments.Empty();

    // Partial reward if past level 5
    if (FailureLevel >= 5)
    {
        DepositRunReward();
    }

    // Still check unlocks — some hidden hats trigger on failure conditions
    RunEndUnlockChecks(RunData);
}

void UMXHatManager::HandleLevelComplete(int32 LevelNumber, const TArray<FGuid>& SurvivingRobots)
{
    if (!UnlockChecker) return;

    // Collect hat IDs from surviving robots
    TArray<FMXRobotProfile> AllProfiles;
    if (IMXRobotProvider* Provider = GetRobotProvider())
    {
        AllProfiles = Provider->GetAllRobotProfiles_Implementation();
    }

    TArray<int32> NewHatIds = UnlockChecker->CheckLevelSpecificUnlocks(LevelNumber, SurvivingRobots, AllProfiles, HatDefinitions);
    ProcessNewUnlocks(NewHatIds);
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

void UMXHatManager::LoadHatDefinitions()
{
    if (!HatDefinitionsTable.IsValid())
    {
        // Attempt synchronous load from known path
        UDataTable* DT = LoadObject<UDataTable>(nullptr,
            TEXT("/Game/Data/DataTables/DT_HatDefinitions.DT_HatDefinitions"));
        if (DT)
        {
            HatDefinitionsTable = DT;
        }
    }

    UDataTable* DT = HatDefinitionsTable.Get();
    if (!DT)
    {
        UE_LOG(LogTemp, Error, TEXT("UMXHatManager: DT_HatDefinitions not found — hat system will be empty."));
        return;
    }

    HatDefinitions.Empty();
    for (auto& Row : DT->GetRowMap())
    {
        FMXHatDefinition* Def = reinterpret_cast<FMXHatDefinition*>(Row.Value);
        if (Def)
        {
            HatDefinitions.Add(Def->hat_id, *Def);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("UMXHatManager: Loaded %d hat definitions."), HatDefinitions.Num());
}

void UMXHatManager::RunComboCheck()
{
    if (!ComboDetector) return;

    TArray<int32> AllEquipped = GetCurrentlyEquippedHatIds();
    // Also gather hats from AllRobotProfiles for this run
    if (IMXRobotProvider* Provider = GetRobotProvider())
    {
        for (const FMXRobotProfile& Profile : Provider->GetAllRobotProfiles_Implementation())
        {
            if (Profile.current_hat_id != -1)
            {
                AllEquipped.AddUnique(Profile.current_hat_id);
            }
        }
    }

    TArray<int32> NewCombos = ComboDetector->CheckCombos(AllEquipped, Collection, HatDefinitions);
    for (int32 ComboHatId : NewCombos)
    {
        Collection.discovered_combo_ids.AddUnique(ComboHatId);
        UnlockHat(ComboHatId);

        if (EventBus)
        {
            // Build the ingredient list for this combo
            TArray<int32> Ingredients;
            if (const FMXHatDefinition* Def = HatDefinitions.Find(ComboHatId))
            {
                Ingredients = Def->combo_requirements;
            }
            EventBus->OnComboDiscovered.Broadcast(Ingredients, ComboHatId);
        }
    }
}

void UMXHatManager::RunEndUnlockChecks(const FMXRunData& RunData)
{
    if (!UnlockChecker) return;

    TArray<FMXRobotProfile> AllProfiles;
    if (IMXRobotProvider* Provider = GetRobotProvider())
    {
        AllProfiles = Provider->GetAllRobotProfiles_Implementation();
    }

    TArray<int32> NewHats;

    TArray<int32> Standard  = UnlockChecker->CheckStandardUnlocks(RunData, AllProfiles);
    TArray<int32> Hidden    = UnlockChecker->CheckHiddenUnlocks(RunData, AllProfiles);
    TArray<int32> Legendary = UnlockChecker->CheckLegendaryUnlocks(RunData, Collection);

    NewHats.Append(Standard);
    NewHats.Append(Hidden);
    NewHats.Append(Legendary);

    ProcessNewUnlocks(NewHats);
}

void UMXHatManager::ProcessNewUnlocks(const TArray<int32>& NewHatIds)
{
    for (int32 HatId : NewHatIds)
    {
        UnlockHat(HatId);
    }
}

IMXRobotProvider* UMXHatManager::GetRobotProvider() const
{
    if (RobotProviderObject && RobotProviderObject->Implements<UMXRobotProvider>())
    {
        return Cast<IMXRobotProvider>(RobotProviderObject);
    }
    return nullptr;
}
