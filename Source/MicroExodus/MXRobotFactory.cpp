// MXRobotFactory.cpp — Robot Factory Module v1.0
// Agent 14: Robot Factory

#include "MXRobotFactory.h"
#include "MXRobotManager.h"
#include "MXRobotAssembler.h"
#include "MXNameGenerator.h"
#include "MXPersonalityGenerator.h"
#include "MXConstants.h"

UMXRobotFactory::UMXRobotFactory()
{
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXRobotFactory::Initialize(
    UMXRobotManager*         InRobotManager,
    UMXRobotAssembler*       InAssembler,
    UMXNameGenerator*        InNameGenerator,
    UMXPersonalityGenerator* InPersonalityGenerator)
{
    RobotManager         = InRobotManager;
    Assembler            = InAssembler;
    NameGenerator        = InNameGenerator;
    PersonalityGenerator = InPersonalityGenerator;

    EnsureUpgradesInitialised();

    UE_LOG(LogTemp, Log, TEXT("MXRobotFactory: Initialised. Salvage=%d, Upgrades=%d."),
        FactoryState.SalvageParts, FactoryState.Upgrades.Num());
}

void UMXRobotFactory::LoadFactoryState(const FMXFactoryState& State)
{
    FactoryState = State;
    EnsureUpgradesInitialised();
}

// ---------------------------------------------------------------------------
// Lobby Pool Generation
// ---------------------------------------------------------------------------

void UMXRobotFactory::BuildLobbyPool(
    const TArray<FGuid>&          SurvivorIds,
    const TArray<FGuid>&          RescuedIds,
    int32                         LevelNumber,
    ETier                         Tier,
    const TArray<FMXSalvageDrop>& PendingSalvage)
{
    CurrentPool = FMXLobbyPool();
    CurrentPool.LevelNumber = LevelNumber;
    CurrentPool.MaxSelectable = 5;
    CurrentPool.PendingSalvage = PendingSalvage;

    // 1. Add survivors (always available, auto-selected by default).
    for (const FGuid& Id : SurvivorIds)
    {
        FMXRecruitCandidate Candidate = CreateCandidateFromProfile(Id, ERecruitSource::Survivor);
        Candidate.bSelected = true; // Survivors default to selected.
        CurrentPool.Candidates.Add(Candidate);
    }

    // 2. Add rescued robots.
    for (const FGuid& Id : RescuedIds)
    {
        FMXRecruitCandidate Candidate = CreateCandidateFromProfile(Id, ERecruitSource::Rescued);
        CurrentPool.Candidates.Add(Candidate);
    }

    // 3. Generate fresh recruits.
    const int32 FreshCount = GetFreshRecruitCount(LevelNumber, Tier);
    TArray<FMXRecruitCandidate> FreshRecruits = GenerateFreshRecruits(FreshCount, LevelNumber);
    CurrentPool.Candidates.Append(FreshRecruits);
    CurrentPool.FreshRecruitCount = FreshCount;

    // 4. If fewer than 5 are selected, auto-fill from rescued then recruits.
    int32 Selected = 0;
    for (const FMXRecruitCandidate& C : CurrentPool.Candidates)
    {
        if (C.bSelected) ++Selected;
    }
    if (Selected < CurrentPool.MaxSelectable)
    {
        // Fill from rescued first.
        for (FMXRecruitCandidate& C : CurrentPool.Candidates)
        {
            if (Selected >= CurrentPool.MaxSelectable) break;
            if (!C.bSelected && C.Source == ERecruitSource::Rescued)
            {
                C.bSelected = true;
                ++Selected;
            }
        }
        // Then from fresh recruits.
        for (FMXRecruitCandidate& C : CurrentPool.Candidates)
        {
            if (Selected >= CurrentPool.MaxSelectable) break;
            if (!C.bSelected && C.Source == ERecruitSource::FreshRecruit)
            {
                C.bSelected = true;
                ++Selected;
            }
        }
    }

    // Process pending salvage.
    ProcessSalvageDrops(PendingSalvage);

    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("╔══════════════════════════════════════════════════╗"));
    UE_LOG(LogTemp, Log, TEXT("║           ROBOT FACTORY — LEVEL %2d LOBBY         ║"), LevelNumber);
    UE_LOG(LogTemp, Log, TEXT("╠══════════════════════════════════════════════════╣"));
    UE_LOG(LogTemp, Log, TEXT("║  Survivors:      %3d                             ║"), SurvivorIds.Num());
    UE_LOG(LogTemp, Log, TEXT("║  Rescued:        %3d                             ║"), RescuedIds.Num());
    UE_LOG(LogTemp, Log, TEXT("║  Fresh recruits: %3d                             ║"), FreshCount);
    UE_LOG(LogTemp, Log, TEXT("║  Total pool:     %3d                             ║"), CurrentPool.Candidates.Num());
    UE_LOG(LogTemp, Log, TEXT("║  Selected:       %3d / %d                          ║"), Selected, CurrentPool.MaxSelectable);
    UE_LOG(LogTemp, Log, TEXT("║  Salvage parts:  %4d                            ║"), FactoryState.SalvageParts);
    UE_LOG(LogTemp, Log, TEXT("╚══════════════════════════════════════════════════╝"));
    UE_LOG(LogTemp, Log, TEXT(""));

    OnLobbyPoolReady.Broadcast(CurrentPool);
}

int32 UMXRobotFactory::GetFreshRecruitCount(int32 LevelNumber, ETier Tier) const
{
    // Base fresh recruits by difficulty tier.
    // Easier = more choices, harder = fewer.
    int32 BaseCount = 0;
    switch (Tier)
    {
    case ETier::Standard:   BaseCount = 5; break;
    case ETier::Hardened:   BaseCount = 4; break;
    case ETier::Brutal:     BaseCount = 3; break;
    case ETier::Nightmare:  BaseCount = 2; break;
    case ETier::Extinction: BaseCount = 1; break;
    case ETier::Legendary:  BaseCount = 1; break;
    default:                BaseCount = 3; break;
    }

    // BatchSize upgrade adds +1 per level.
    BaseCount += GetUpgradeLevel(EFactoryUpgrade::BatchSize);

    // Level 1 always gets at least 5 (tutorial generosity).
    if (LevelNumber == 1) BaseCount = FMath::Max(BaseCount, 5);

    // Clamp to reasonable bounds.
    return FMath::Clamp(BaseCount, 1, 8);
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

bool UMXRobotFactory::ToggleCandidateSelection(const FGuid& RobotId)
{
    for (FMXRecruitCandidate& C : CurrentPool.Candidates)
    {
        if (C.RobotId == RobotId)
        {
            if (C.bSelected)
            {
                // Deselect.
                C.bSelected = false;
                return true;
            }
            else
            {
                // Check cap before selecting.
                if (GetSelectedCount() >= CurrentPool.MaxSelectable)
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("MXRobotFactory: Cannot select — already at cap (%d)."),
                        CurrentPool.MaxSelectable);
                    return false;
                }
                C.bSelected = true;
                return true;
            }
        }
    }
    return false;
}

void UMXRobotFactory::AutoSelectBest()
{
    // Deselect all first.
    for (FMXRecruitCandidate& C : CurrentPool.Candidates) C.bSelected = false;

    int32 Selected = 0;

    // Priority order: Survivors (by level desc) > Rescued (by level desc) > Recruits.
    // Sort by source priority then level.
    TArray<int32> Indices;
    for (int32 i = 0; i < CurrentPool.Candidates.Num(); ++i) Indices.Add(i);

    Indices.Sort([this](int32 A, int32 B)
    {
        const FMXRecruitCandidate& CA = CurrentPool.Candidates[A];
        const FMXRecruitCandidate& CB = CurrentPool.Candidates[B];

        // Source priority: Survivor=0, Rescued=1, CodeReplica=2, Fresh=3.
        const int32 PriorityA = (CA.Source == ERecruitSource::Survivor) ? 0 :
                                (CA.Source == ERecruitSource::Rescued) ? 1 :
                                (CA.Source == ERecruitSource::CodeReplica) ? 2 : 3;
        const int32 PriorityB = (CB.Source == ERecruitSource::Survivor) ? 0 :
                                (CB.Source == ERecruitSource::Rescued) ? 1 :
                                (CB.Source == ERecruitSource::CodeReplica) ? 2 : 3;

        if (PriorityA != PriorityB) return PriorityA < PriorityB;
        return CA.Level > CB.Level; // Higher level first within same source.
    });

    for (int32 Idx : Indices)
    {
        if (Selected >= CurrentPool.MaxSelectable) break;
        CurrentPool.Candidates[Idx].bSelected = true;
        ++Selected;
    }
}

TArray<FGuid> UMXRobotFactory::GetSelectedRobots() const
{
    TArray<FGuid> Result;
    for (const FMXRecruitCandidate& C : CurrentPool.Candidates)
    {
        if (C.bSelected) Result.Add(C.RobotId);
    }
    return Result;
}

int32 UMXRobotFactory::GetSelectedCount() const
{
    int32 Count = 0;
    for (const FMXRecruitCandidate& C : CurrentPool.Candidates)
    {
        if (C.bSelected) ++Count;
    }
    return Count;
}

// ---------------------------------------------------------------------------
// Robot Code System
// ---------------------------------------------------------------------------

FString UMXRobotFactory::GetRobotCode(const FGuid& RobotId) const
{
    FMXRobotCode Code = UMXRobotCodec::EncodeFromGuid(RobotId);
    return Code.CodeString;
}

bool UMXRobotFactory::RedeemCode(const FString& CodeString)
{
    // Check CodeTerminal upgrade.
    if (!IsCodeTerminalUnlocked())
    {
        OnCodeRejected.Broadcast(CodeString, TEXT("Code Terminal upgrade required."));
        return false;
    }

    // Validate format.
    if (!UMXRobotCodec::IsValidCodeFormat(CodeString))
    {
        OnCodeRejected.Broadcast(CodeString, TEXT("Invalid code format."));
        return false;
    }

    // Parse the code.
    FMXRobotCode Code = UMXRobotCodec::DecodeFromString(CodeString);
    if (!Code.bValid)
    {
        OnCodeRejected.Broadcast(CodeString, TEXT("Could not decode the code."));
        return false;
    }

    // Check for duplicate redemption.
    if (FactoryState.RedeemedCodes.Contains(Code.CodeString))
    {
        OnCodeRejected.Broadcast(CodeString, TEXT("This code has already been redeemed."));
        return false;
    }

    // Create the replica candidate.
    FMXRecruitCandidate Replica = CreateReplicaFromCode(Code);

    // Add to lobby pool.
    CurrentPool.Candidates.Add(Replica);

    // Record redemption.
    FactoryState.RedeemedCodes.Add(Code.CodeString);
    FactoryState.LifetimeCodesRedeemed++;

    UE_LOG(LogTemp, Log,
        TEXT("MXRobotFactory: Code redeemed: %s → Robot '%s'"),
        *Code.CodeString, *Replica.Name);

    OnCodeRedeemed.Broadcast(Code.CodeString, Replica);
    return true;
}

// ---------------------------------------------------------------------------
// Upgrades
// ---------------------------------------------------------------------------

bool UMXRobotFactory::CanPurchaseUpgrade(EFactoryUpgrade Upgrade) const
{
    const FMXFactoryUpgradeLevel* Entry = FindUpgrade(Upgrade);
    if (!Entry) return false;
    if (Entry->Level >= Entry->MaxLevel) return false;
    return FactoryState.SalvageParts >= Entry->NextUpgradeCost;
}

bool UMXRobotFactory::PurchaseUpgrade(EFactoryUpgrade Upgrade)
{
    if (!CanPurchaseUpgrade(Upgrade)) return false;

    FMXFactoryUpgradeLevel* Entry = FindUpgrade(Upgrade);
    if (!Entry) return false;

    FactoryState.SalvageParts -= Entry->NextUpgradeCost;
    Entry->Level++;
    Entry->NextUpgradeCost = ComputeUpgradeCost(Upgrade, Entry->Level);

    UE_LOG(LogTemp, Log,
        TEXT("MXRobotFactory: Upgrade '%d' purchased → Level %d. Remaining salvage: %d."),
        static_cast<int32>(Upgrade), Entry->Level, FactoryState.SalvageParts);

    OnUpgradePurchased.Broadcast(Upgrade, Entry->Level);
    return true;
}

int32 UMXRobotFactory::GetUpgradeLevel(EFactoryUpgrade Upgrade) const
{
    const FMXFactoryUpgradeLevel* Entry = FindUpgrade(Upgrade);
    return Entry ? Entry->Level : 0;
}

// ---------------------------------------------------------------------------
// Salvage
// ---------------------------------------------------------------------------

void UMXRobotFactory::AddSalvageParts(int32 Amount)
{
    FactoryState.SalvageParts += Amount;
    FactoryState.LifetimeSalvageParts += Amount;
}

void UMXRobotFactory::ProcessSalvageDrops(const TArray<FMXSalvageDrop>& Drops)
{
    int32 TotalScrap = 0;
    int32 NewUnlocks = 0;

    // Salvage efficiency multiplier from upgrade.
    const float Efficiency = 1.0f + 0.2f * GetUpgradeLevel(EFactoryUpgrade::SalvageEfficiency);

    for (const FMXSalvageDrop& Drop : Drops)
    {
        // Check if this part is a new unlock.
        if (!FactoryState.UnlockedCosmeticParts.Contains(Drop.PartId))
        {
            FactoryState.UnlockedCosmeticParts.Add(Drop.PartId);
            ++NewUnlocks;
        }

        // Add scrap value with efficiency bonus.
        const int32 ScrapWithBonus = FMath::CeilToInt32(Drop.ScrapValue * Efficiency);
        TotalScrap += ScrapWithBonus;
    }

    AddSalvageParts(TotalScrap);

    if (Drops.Num() > 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("MXRobotFactory: Processed %d salvage drops → %d parts, %d new unlocks."),
            Drops.Num(), TotalScrap, NewUnlocks);
    }
}

bool UMXRobotFactory::IsPartUnlocked(const FString& PartId) const
{
    return FactoryState.UnlockedCosmeticParts.Contains(PartId);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool UMXRobotFactory::IsCodeTerminalUnlocked() const
{
    return GetUpgradeLevel(EFactoryUpgrade::CodeTerminal) >= 1;
}

bool UMXRobotFactory::IsCosmeticWorkshopUnlocked() const
{
    return GetUpgradeLevel(EFactoryUpgrade::CosmeticWorkshop) >= 1;
}

// ---------------------------------------------------------------------------
// Private: Recruit Generation
// ---------------------------------------------------------------------------

TArray<FMXRecruitCandidate> UMXRobotFactory::GenerateFreshRecruits(
    int32 Count, int32 LevelNumber)
{
    TArray<FMXRecruitCandidate> Recruits;
    Recruits.Reserve(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        Recruits.Add(CreateFreshRecruit(i, LevelNumber));
    }

    FactoryState.LifetimeRobotsProduced += Count;
    return Recruits;
}

FMXRecruitCandidate UMXRobotFactory::CreateFreshRecruit(
    int32 RecruitIndex, int32 LevelNumber)
{
    FMXRecruitCandidate Candidate;

    // Generate a new GUID for this recruit.
    Candidate.RobotId = FGuid::NewGuid();
    Candidate.Source = ERecruitSource::FreshRecruit;
    Candidate.Level = 1;
    Candidate.Role = ERobotRole::None;
    Candidate.bFromCode = false;
    Candidate.bSelected = false;

    // Generate the robot code from the GUID.
    FMXRobotCode Code = UMXRobotCodec::EncodeFromGuid(Candidate.RobotId);
    Candidate.RobotCode = Code.CodeString;

    // Name generation (uses the NameGenerator if available, else placeholder).
    if (NameGenerator)
    {
        // Seed the name from the code for determinism.
        int32 NameSeed = 0, PersonalitySeed = 0, AppearanceSeed = 0;
        UMXRobotCodec::DeriveSeeds(Code, NameSeed, PersonalitySeed, AppearanceSeed);
        // NameGenerator::GenerateName() uses its own pool.
        // For code-determinism, we pass the seed. The existing generator
        // may not support seeded generation yet — use standard path for now.
        Candidate.Name = NameGenerator->GenerateName();
    }
    else
    {
        Candidate.Name = FString::Printf(TEXT("Unit-%04d"), RecruitIndex);
    }

    // Generate assembly recipe preview.
    if (Assembler)
    {
        Candidate.Recipe = Assembler->GenerateRecipe(Candidate.RobotId);
    }

    return Candidate;
}

FMXRecruitCandidate UMXRobotFactory::CreateCandidateFromProfile(
    const FGuid& RobotId, ERecruitSource Source)
{
    FMXRecruitCandidate Candidate;
    Candidate.RobotId = RobotId;
    Candidate.Source = Source;

    // Pull data from the robot manager.
    if (RobotManager)
    {
        FMXRobotProfile Profile = RobotManager->GetRobotProfile_Implementation(RobotId);
        Candidate.Name = Profile.name;
        Candidate.Level = Profile.level;
        Candidate.Role = Profile.role;
    }
    else
    {
        Candidate.Name = TEXT("Unknown");
        Candidate.Level = 1;
    }

    // Generate the code.
    FMXRobotCode Code = UMXRobotCodec::EncodeFromGuid(RobotId);
    Candidate.RobotCode = Code.CodeString;

    // Generate assembly recipe.
    if (Assembler)
    {
        Candidate.Recipe = Assembler->GenerateRecipe(RobotId);
    }

    return Candidate;
}

FMXRecruitCandidate UMXRobotFactory::CreateReplicaFromCode(const FMXRobotCode& Code)
{
    FMXRecruitCandidate Candidate;

    // Replica gets a NEW GUID (it's a different physical robot).
    Candidate.RobotId = FGuid::NewGuid();
    Candidate.Source = ERecruitSource::CodeReplica;
    Candidate.Level = 1;
    Candidate.Role = ERobotRole::None;
    Candidate.bFromCode = true;
    Candidate.bSelected = false;
    Candidate.RobotCode = Code.CodeString;

    // Derive seeds for deterministic generation.
    int32 NameSeed = 0, PersonalitySeed = 0, AppearanceSeed = 0;
    UMXRobotCodec::DeriveSeeds(Code, NameSeed, PersonalitySeed, AppearanceSeed);

    // Name: seeded generation for identical result.
    if (NameGenerator)
    {
        // Use the name seed to pick from the pool deterministically.
        FRandomStream NameStream(NameSeed);
        // GenerateNameSeeded would be ideal — for now use standard path.
        // The Identity module can be extended to support seeded names.
        Candidate.Name = NameGenerator->GenerateName();
        // TODO: Add NameGenerator->GenerateNameFromSeed(NameSeed) for true determinism.
    }
    else
    {
        Candidate.Name = FString::Printf(TEXT("Replica-%s"), *Code.CodeString.Left(4));
    }

    // Assembly: use the code's synthetic GUID so parts match the original.
    if (Assembler)
    {
        const FGuid AssemblyGuid = UMXRobotCodec::DeriveAssemblyGuid(Code);
        // Appearance seed drives color selection.
        FRandomStream AppearanceStream(AppearanceSeed);
        const int32 ChassisColor = AppearanceStream.RandRange(0, 11);
        const int32 EyeColor = AppearanceStream.RandRange(0, 7);

        Candidate.Recipe = Assembler->GenerateRecipe(AssemblyGuid, ChassisColor, EyeColor);
    }

    UE_LOG(LogTemp, Log,
        TEXT("MXRobotFactory: Created replica from code %s → '%s' (Assembly GUID: %s)"),
        *Code.CodeString, *Candidate.Name,
        *UMXRobotCodec::DeriveAssemblyGuid(Code).ToString());

    return Candidate;
}

// ---------------------------------------------------------------------------
// Private: Upgrade Helpers
// ---------------------------------------------------------------------------

void UMXRobotFactory::EnsureUpgradesInitialised()
{
    if (FactoryState.Upgrades.Num() > 0) return;

    // Create default entries for all upgrade types.
    static const EFactoryUpgrade AllUpgrades[] = {
        EFactoryUpgrade::RecruitQuality,
        EFactoryUpgrade::RecruitDiversity,
        EFactoryUpgrade::BatchSize,
        EFactoryUpgrade::SalvageEfficiency,
        EFactoryUpgrade::CodeTerminal,
        EFactoryUpgrade::CosmeticWorkshop,
        EFactoryUpgrade::RepairBay,
        EFactoryUpgrade::EliteForge,
    };

    // Max levels and base costs per upgrade type.
    static const int32 MaxLevels[] = { 5, 5, 3, 5, 1, 1, 3, 3 };
    static const int32 BaseCosts[] = { 10, 15, 20, 10, 50, 40, 25, 60 };

    for (int32 i = 0; i < 8; ++i)
    {
        FMXFactoryUpgradeLevel Entry;
        Entry.Upgrade = AllUpgrades[i];
        Entry.Level = 0;
        Entry.MaxLevel = MaxLevels[i];
        Entry.NextUpgradeCost = BaseCosts[i];
        FactoryState.Upgrades.Add(Entry);
    }
}

FMXFactoryUpgradeLevel* UMXRobotFactory::FindUpgrade(EFactoryUpgrade Upgrade)
{
    for (FMXFactoryUpgradeLevel& Entry : FactoryState.Upgrades)
    {
        if (Entry.Upgrade == Upgrade) return &Entry;
    }
    return nullptr;
}

const FMXFactoryUpgradeLevel* UMXRobotFactory::FindUpgrade(EFactoryUpgrade Upgrade) const
{
    for (const FMXFactoryUpgradeLevel& Entry : FactoryState.Upgrades)
    {
        if (Entry.Upgrade == Upgrade) return &Entry;
    }
    return nullptr;
}

int32 UMXRobotFactory::ComputeUpgradeCost(EFactoryUpgrade Upgrade, int32 CurrentLevel)
{
    // Exponential scaling: each level costs 1.5x the previous.
    static const int32 BaseCosts[] = { 10, 15, 20, 10, 50, 40, 25, 60 };
    const int32 Idx = FMath::Clamp(static_cast<int32>(Upgrade), 0, 7);
    const float Multiplier = FMath::Pow(1.5f, static_cast<float>(CurrentLevel));
    return FMath::CeilToInt32(BaseCosts[Idx] * Multiplier);
}
