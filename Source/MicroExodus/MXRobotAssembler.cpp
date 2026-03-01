// MXRobotAssembler.cpp — Robot Assembly Module v1.0
// Agent 13: Robot Assembly

#include "MXRobotAssembler.h"
#include "MXRobotProfile.h"
#include "MXConstants.h"

UMXRobotAssembler::UMXRobotAssembler()
{
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UMXRobotAssembler::InitialiseFromDataTable(UDataTable* PartsDataTable)
{
    AllParts.Empty();
    PartIdsBySlot.Empty();

    if (!PartsDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRobotAssembler: No DataTable provided. Call GeneratePlaceholderParts() for testing."));
        return;
    }

    const TArray<FName> RowNames = PartsDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        const FMXPartDefinition* Row = PartsDataTable->FindRow<FMXPartDefinition>(RowName, TEXT("MXRobotAssembler"));
        if (Row)
        {
            FMXPartDefinition Part = *Row;
            if (Part.PartId.IsEmpty())
            {
                Part.PartId = RowName.ToString();
            }
            AddPart(Part);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXRobotAssembler: Loaded %d parts from DataTable."), AllParts.Num());
    LogPartPoolSummary();
}

void UMXRobotAssembler::AddPart(const FMXPartDefinition& Part)
{
    if (Part.PartId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRobotAssembler: Cannot add part with empty PartId."));
        return;
    }

    AllParts.Add(Part.PartId, Part);
    PartIdsBySlot.FindOrAdd(Part.Slot).AddUnique(Part.PartId);
}

void UMXRobotAssembler::GeneratePlaceholderParts(int32 VariantsPerSlot)
{
    AllParts.Empty();
    PartIdsBySlot.Empty();

    // Socket names matching the standardised skeleton.
    static const FName SocketNames[] = {
        TEXT("socket_locomotion"),   // Locomotion
        TEXT("socket_body"),         // Body
        TEXT("socket_arm_l"),        // ArmLeft
        TEXT("socket_arm_r"),        // ArmRight
        TEXT("socket_head"),         // Head
    };

    static const TCHAR* SlotPrefixes[] = {
        TEXT("Loco"), TEXT("Body"), TEXT("ArmL"), TEXT("ArmR"), TEXT("Head")
    };

    // Locomotion variant names for display flavor.
    static const TCHAR* LocoNames[] = {
        TEXT("Tank Treads"), TEXT("All-Terrain Wheels"), TEXT("Bipedal Striders"),
        TEXT("Spider Legs"), TEXT("Hover Pads"), TEXT("Unicycle"),
        TEXT("Caterpillar Track"), TEXT("Balloon Wheels"), TEXT("Digitigrade Legs"),
        TEXT("Crab Walker"), TEXT("Magnetic Hover"), TEXT("Ball Drive"),
    };

    static const TCHAR* BodyNames[] = {
        TEXT("Barrel Chassis"), TEXT("Box Frame"), TEXT("Cylinder Core"),
        TEXT("Wedge Hull"), TEXT("Orb Shell"), TEXT("Skeletal Frame"),
        TEXT("Crate Body"), TEXT("Dome Torso"), TEXT("Pill Capsule"),
        TEXT("Hexagonal Cage"), TEXT("Tapered Fuselage"), TEXT("Flat Platform"),
    };

    static const TCHAR* ArmNames[] = {
        TEXT("Gripper Claw"), TEXT("Welding Torch"), TEXT("Drill Arm"),
        TEXT("Scanner Probe"), TEXT("Magnetic Grapple"), TEXT("Stub Arm"),
        TEXT("Pincer"), TEXT("Suction Pad"), TEXT("Blade Arm"),
        TEXT("Telescoping Rod"), TEXT("Hammer Fist"), TEXT("Saw Arm"),
    };

    static const TCHAR* HeadNames[] = {
        TEXT("Dome Sensor"), TEXT("Visor Plate"), TEXT("Camera Eye"),
        TEXT("Antenna Array"), TEXT("Flat Panel"), TEXT("Bucket Head"),
        TEXT("Periscope"), TEXT("Lantern Head"), TEXT("Radar Dish"),
        TEXT("Monocle"), TEXT("Twin Lens"), TEXT("Prism Head"),
    };

    const TCHAR** NamePools[] = { LocoNames, BodyNames, ArmNames, ArmNames, HeadNames };

    static const ELocomotionClass LocoClasses[] = {
        ELocomotionClass::Treads, ELocomotionClass::Wheels, ELocomotionClass::Bipedal,
        ELocomotionClass::Spider, ELocomotionClass::Hover, ELocomotionClass::Unicycle,
        ELocomotionClass::Treads, ELocomotionClass::Wheels, ELocomotionClass::Bipedal,
        ELocomotionClass::Spider, ELocomotionClass::Hover, ELocomotionClass::Unicycle,
    };

    const int32 MaxNames = 12;
    VariantsPerSlot = FMath::Clamp(VariantsPerSlot, 2, MaxNames);

    for (int32 SlotIdx = 0; SlotIdx < static_cast<int32>(EPartSlot::COUNT); ++SlotIdx)
    {
        const EPartSlot Slot = static_cast<EPartSlot>(SlotIdx);

        for (int32 v = 0; v < VariantsPerSlot; ++v)
        {
            FMXPartDefinition Part;
            Part.PartId = FString::Printf(TEXT("%s_%02d"), SlotPrefixes[SlotIdx], v);
            Part.Slot = Slot;
            Part.DisplayName = FString(NamePools[SlotIdx][v % MaxNames]);
            Part.SocketName = SocketNames[SlotIdx];
            Part.Mass = 3.0f + static_cast<float>(v) * 0.5f;
            Part.HitPoints = (Slot == EPartSlot::Body) ? 4 : 2;
            Part.Weight = 1.0f;
            Part.bMirrorable = (Slot == EPartSlot::ArmLeft || Slot == EPartSlot::ArmRight);

            if (Slot == EPartSlot::Locomotion)
            {
                Part.LocomotionClass = LocoClasses[v % MaxNames];
            }

            AddPart(Part);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("MXRobotAssembler: Generated %d placeholder parts (%d per slot)."),
        AllParts.Num(), VariantsPerSlot);
    LogPartPoolSummary();
}

// ---------------------------------------------------------------------------
// Recipe Generation
// ---------------------------------------------------------------------------

FMXAssemblyRecipe UMXRobotAssembler::GenerateRecipe(const FGuid& RobotId, int32 ChassisColor, int32 EyeColor) const
{
    FMXAssemblyRecipe Recipe;
    Recipe.RobotId = RobotId;
    Recipe.ChassisColor = ChassisColor;
    Recipe.EyeColor = EyeColor;
    Recipe.SlotPartIds.SetNum(static_cast<int32>(EPartSlot::COUNT));

    for (int32 SlotIdx = 0; SlotIdx < static_cast<int32>(EPartSlot::COUNT); ++SlotIdx)
    {
        const EPartSlot Slot = static_cast<EPartSlot>(SlotIdx);
        const int32 SlotSeed = DeriveSlotSeed(RobotId, Slot);
        FRandomStream Stream(SlotSeed);

        const FString PartId = PickPartForSlot(Slot, Stream);
        Recipe.SlotPartIds[SlotIdx] = PartId;

        // If this is the locomotion slot, extract the locomotion class.
        if (Slot == EPartSlot::Locomotion && !PartId.IsEmpty())
        {
            if (const FMXPartDefinition* Def = AllParts.Find(PartId))
            {
                Recipe.LocomotionType = Def->LocomotionClass;
            }
        }
    }

    // Arm mirroring: if the left arm part is mirrorable, use the same variant
    // for the right arm ~40% of the time for visual cohesion.
    const int32 MirrorSeed = DeriveSlotSeed(RobotId, EPartSlot::COUNT); // Extra seed.
    FRandomStream MirrorStream(MirrorSeed);
    if (MirrorStream.FRandRange(0.0f, 1.0f) < 0.4f)
    {
        const FString LeftPartId = Recipe.SlotPartIds[static_cast<int32>(EPartSlot::ArmLeft)];
        if (!LeftPartId.IsEmpty())
        {
            if (const FMXPartDefinition* LeftDef = AllParts.Find(LeftPartId))
            {
                if (LeftDef->bMirrorable)
                {
                    // Find matching right-arm variant with same index suffix.
                    const FString Suffix = LeftPartId.RightChop(LeftPartId.Find(TEXT("_"), ESearchCase::IgnoreCase, ESearchDir::FromEnd));
                    const FString RightId = FString::Printf(TEXT("ArmR%s"), *Suffix);
                    if (AllParts.Contains(RightId))
                    {
                        Recipe.SlotPartIds[static_cast<int32>(EPartSlot::ArmRight)] = RightId;
                    }
                }
            }
        }
    }

    return Recipe;
}

FMXAssemblyRecipe UMXRobotAssembler::GenerateRecipeFromProfile(const FMXRobotProfile& Profile) const
{
    return GenerateRecipe(Profile.robot_id, Profile.chassis_color, Profile.eye_color);
}

TArray<FMXAssemblyRecipe> UMXRobotAssembler::BatchGenerateRecipes(const TArray<FGuid>& RobotIds) const
{
    TArray<FMXAssemblyRecipe> Recipes;
    Recipes.Reserve(RobotIds.Num());

    for (const FGuid& Id : RobotIds)
    {
        Recipes.Add(GenerateRecipe(Id));
    }

    UE_LOG(LogTemp, Log, TEXT("MXRobotAssembler: Batch generated %d recipes."), Recipes.Num());
    return Recipes;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool UMXRobotAssembler::GetPartDefinition(const FString& PartId, FMXPartDefinition& OutPart) const
{
    if (const FMXPartDefinition* Found = AllParts.Find(PartId))
    {
        OutPart = *Found;
        return true;
    }
    return false;
}

TArray<FMXPartDefinition> UMXRobotAssembler::GetPartsForSlot(EPartSlot Slot) const
{
    TArray<FMXPartDefinition> Result;
    if (const TArray<FString>* Ids = PartIdsBySlot.Find(Slot))
    {
        for (const FString& Id : *Ids)
        {
            if (const FMXPartDefinition* Part = AllParts.Find(Id))
            {
                Result.Add(*Part);
            }
        }
    }
    return Result;
}

int32 UMXRobotAssembler::GetTotalPartCount() const
{
    return AllParts.Num();
}

int32 UMXRobotAssembler::GetPartCountForSlot(EPartSlot Slot) const
{
    if (const TArray<FString>* Ids = PartIdsBySlot.Find(Slot))
    {
        return Ids->Num();
    }
    return 0;
}

int64 UMXRobotAssembler::GetTotalCombinations() const
{
    int64 Total = 1;
    for (int32 SlotIdx = 0; SlotIdx < static_cast<int32>(EPartSlot::COUNT); ++SlotIdx)
    {
        const int32 Count = GetPartCountForSlot(static_cast<EPartSlot>(SlotIdx));
        if (Count > 0) Total *= static_cast<int64>(Count);
    }
    return Total;
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

void UMXRobotAssembler::LogPartPoolSummary() const
{
    static const TCHAR* SlotNames[] = {
        TEXT("Locomotion"), TEXT("Body"), TEXT("ArmLeft"), TEXT("ArmRight"), TEXT("Head")
    };

    UE_LOG(LogTemp, Log, TEXT(""));
    UE_LOG(LogTemp, Log, TEXT("╔═══════════════════════════════════════════╗"));
    UE_LOG(LogTemp, Log, TEXT("║       ROBOT PART POOL SUMMARY            ║"));
    UE_LOG(LogTemp, Log, TEXT("╠═══════════════════════════════════════════╣"));

    for (int32 i = 0; i < static_cast<int32>(EPartSlot::COUNT); ++i)
    {
        const EPartSlot Slot = static_cast<EPartSlot>(i);
        const int32 Count = GetPartCountForSlot(Slot);
        UE_LOG(LogTemp, Log, TEXT("║  %-12s : %3d variants              ║"), SlotNames[i], Count);

        if (const TArray<FString>* Ids = PartIdsBySlot.Find(Slot))
        {
            for (const FString& Id : *Ids)
            {
                if (const FMXPartDefinition* P = AllParts.Find(Id))
                {
                    UE_LOG(LogTemp, Verbose, TEXT("║    %-10s  %-24s  w=%.1f ║"),
                        *P->PartId, *P->DisplayName, P->Weight);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("╠═══════════════════════════════════════════╣"));
    UE_LOG(LogTemp, Log, TEXT("║  Total parts   : %4d                    ║"), AllParts.Num());
    UE_LOG(LogTemp, Log, TEXT("║  Combinations  : %s                     ║"),
        *FString::Printf(TEXT("%lld"), GetTotalCombinations()));
    UE_LOG(LogTemp, Log, TEXT("╚═══════════════════════════════════════════╝"));
    UE_LOG(LogTemp, Log, TEXT(""));
}

void UMXRobotAssembler::LogRecipe(const FMXAssemblyRecipe& Recipe) const
{
    static const TCHAR* SlotNames[] = {
        TEXT("Locomotion"), TEXT("Body"), TEXT("ArmLeft"), TEXT("ArmRight"), TEXT("Head")
    };

    UE_LOG(LogTemp, Log, TEXT("Assembly Recipe for robot %s:"), *Recipe.RobotId.ToString());
    for (int32 i = 0; i < Recipe.SlotPartIds.Num(); ++i)
    {
        const FString& PartId = Recipe.SlotPartIds[i];
        FString DisplayName = TEXT("(empty)");
        if (const FMXPartDefinition* P = AllParts.Find(PartId))
        {
            DisplayName = P->DisplayName;
        }
        UE_LOG(LogTemp, Log, TEXT("  %-12s → %s (%s)"),
            SlotNames[i], *PartId, *DisplayName);
    }
    UE_LOG(LogTemp, Log, TEXT("  Locomotion class: %d  Color: %d  Eye: %d"),
        static_cast<int32>(Recipe.LocomotionType), Recipe.ChassisColor, Recipe.EyeColor);
}

// ---------------------------------------------------------------------------
// Private: DeriveSlotSeed
// ---------------------------------------------------------------------------

int32 UMXRobotAssembler::DeriveSlotSeed(const FGuid& RobotId, EPartSlot Slot)
{
    // Use the GUID's four 32-bit components mixed with the slot index.
    // This ensures each slot gets an independent, deterministic seed.
    const uint32 SlotSalt = static_cast<uint32>(Slot) * 2654435761u; // Knuth multiplicative hash.
    const uint32 Mixed = RobotId.A ^ (RobotId.B * 31u) ^ (RobotId.C * 127u) ^ (RobotId.D * 8191u) ^ SlotSalt;

    // Ensure non-negative for FRandomStream.
    return static_cast<int32>(Mixed & 0x7FFFFFFF);
}

// ---------------------------------------------------------------------------
// Private: PickPartForSlot
// ---------------------------------------------------------------------------

FString UMXRobotAssembler::PickPartForSlot(EPartSlot Slot, FRandomStream& Stream) const
{
    const TArray<FString>* Ids = PartIdsBySlot.Find(Slot);
    if (!Ids || Ids->Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MXRobotAssembler: No parts registered for slot %d."),
            static_cast<int32>(Slot));
        return FString();
    }

    // Build weighted selection.
    float TotalWeight = 0.0f;
    TArray<float> CumulativeWeights;
    CumulativeWeights.Reserve(Ids->Num());

    for (const FString& Id : *Ids)
    {
        float Weight = 1.0f;
        if (const FMXPartDefinition* P = AllParts.Find(Id))
        {
            Weight = FMath::Max(0.01f, P->Weight);
        }
        TotalWeight += Weight;
        CumulativeWeights.Add(TotalWeight);
    }

    const float Roll = Stream.FRandRange(0.0f, TotalWeight);
    for (int32 i = 0; i < CumulativeWeights.Num(); ++i)
    {
        if (Roll <= CumulativeWeights[i])
        {
            return (*Ids)[i];
        }
    }

    // Fallback to last.
    return Ids->Last();
}
