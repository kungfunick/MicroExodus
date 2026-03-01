// MXGameInstance.cpp — Phase 8: Identity + DEMS + Hats + Roguelike + Specialization + Evolution + Reports
#include "MXGameInstance.h"
#include "MXEventBusSubsystem.h"
#include "MXRobotManager.h"
#include "MXNameGenerator.h"
#include "MXPersonalityGenerator.h"
#include "MXLifeLog.h"
#include "MXAgingSystem.h"
#include "MXMessageBuilder.h"
#include "MXMessageDispatcher.h"
#include "MXTemplateSelector.h"
#include "MXDeduplicationBuffer.h"
#include "MXRunManager.h"
#include "MXHatManager.h"
#include "MXSpecTree.h"
#include "MXRunReportEngine.h"
#include "MXInterfaces.h"
#include "Engine/DataTable.h"

void UMXGameInstance::Init()
{
    Super::Init();

    // ---- Verify EventBus ----
    UMXEventBusSubsystem* BusSub = GetSubsystem<UMXEventBusSubsystem>();
    UMXEventBus* EventBus = BusSub ? BusSub->EventBus : nullptr;
    if (EventBus)
    {
        UE_LOG(LogTemp, Log, TEXT("[MX] EventBus ready."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MX] EventBus NOT available!"));
    }

    // ================================================================
    // Phase 2: Identity
    // ================================================================

    NameGenerator        = NewObject<UMXNameGenerator>(this);
    PersonalityGenerator = NewObject<UMXPersonalityGenerator>(this);
    LifeLog              = NewObject<UMXLifeLog>(this);
    AgingSystem          = NewObject<UMXAgingSystem>(this);
    RobotManager         = NewObject<UMXRobotManager>(this);

    if (!NamesTableAsset.IsNull())
    {
        UDataTable* NamesTable = NamesTableAsset.LoadSynchronous();
        if (NamesTable) { NameGenerator->LoadFromDataTable(NamesTable); }
        else { UE_LOG(LogTemp, Warning, TEXT("[MX] Failed to load DT_Names.")); }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[MX] NamesTableAsset not set — NameGenerator will use fallback names."));
    }

    if (!PersonalitiesTableAsset.IsNull())
    {
        UDataTable* PersonalitiesTable = PersonalitiesTableAsset.LoadSynchronous();
        if (PersonalitiesTable) { PersonalityGenerator->LoadFromDataTable(PersonalitiesTable); }
        else { UE_LOG(LogTemp, Warning, TEXT("[MX] Failed to load DT_Personalities.")); }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[MX] PersonalitiesTableAsset not set — PersonalityGenerator will use defaults."));
    }

    RobotManager->Initialize(NameGenerator, PersonalityGenerator, LifeLog, AgingSystem);
    UE_LOG(LogTemp, Log, TEXT("[MX] Identity module created."));

    // ================================================================
    // Phase 3: DEMS
    // ================================================================

    DedupBuffer      = NewObject<UMXDeduplicationBuffer>(this);
    TemplateSelector = NewObject<UMXTemplateSelector>(this);
    MessageBuilder   = NewObject<UMXMessageBuilder>(this);
    MessageDispatcher = NewObject<UMXMessageDispatcher>(this);

    TemplateSelector->LoadTemplates();
    MessageBuilder->Initialize(TemplateSelector, DedupBuffer);

    if (RobotManager)
    {
        MessageDispatcher->RegisterListener(TScriptInterface<IMXEventListener>(RobotManager.Get()));
    }
    if (LifeLog)
    {
        MessageDispatcher->RegisterListener(TScriptInterface<IMXEventListener>(LifeLog.Get()));
    }

    UE_LOG(LogTemp, Log, TEXT("[MX] DEMS module created."));

    // ================================================================
    // Phase 5: Hats (before Roguelike — RunManager needs HatProvider)
    // ================================================================

    HatManager = NewObject<UMXHatManager>(this);

    if (!HatDefinitionsTableAsset.IsNull())
    {
        UDataTable* HatDefTable = HatDefinitionsTableAsset.LoadSynchronous();
        if (HatDefTable)
        {
            UE_LOG(LogTemp, Log, TEXT("[MX] HatDefinitionsTableAsset set."));
        }
    }

    HatManager->Initialize(EventBus, RobotManager.Get());
    UE_LOG(LogTemp, Log, TEXT("[MX] Hats module created."));

    // ================================================================
    // Phase 4: Roguelike (now with HatProvider)
    // ================================================================

    RunManager = NewObject<UMXRunManager>(this);

    TScriptInterface<IMXRobotProvider> RobotProviderSI;
    if (RobotManager)
    {
        RobotProviderSI.SetObject(RobotManager.Get());
        RobotProviderSI.SetInterface(
            static_cast<IMXRobotProvider*>(
                RobotManager->GetNativeInterfaceAddress(UMXRobotProvider::StaticClass())));
    }

    TScriptInterface<IMXHatProvider> HatProviderSI;
    if (HatManager)
    {
        HatProviderSI.SetObject(HatManager.Get());
        HatProviderSI.SetInterface(
            static_cast<IMXHatProvider*>(
                HatManager->GetNativeInterfaceAddress(UMXHatProvider::StaticClass())));
    }

    UDataTable* TierTable = nullptr;
    if (!TierModifiersTableAsset.IsNull())
    {
        TierTable = TierModifiersTableAsset.LoadSynchronous();
        if (!TierTable) { UE_LOG(LogTemp, Warning, TEXT("[MX] Failed to load DT_TierModifiers.")); }
    }

    UDataTable* XPTable = nullptr;
    if (!XPCurveTableAsset.IsNull())
    {
        XPTable = XPCurveTableAsset.LoadSynchronous();
        if (!XPTable) { UE_LOG(LogTemp, Warning, TEXT("[MX] Failed to load DT_XPCurve.")); }
    }

    RunManager->Initialise(EventBus, RobotProviderSI, HatProviderSI, TierTable, XPTable);
    UE_LOG(LogTemp, Log, TEXT("[MX] Roguelike module created."));

    // ================================================================
    // Phase 6: Specialization
    // ================================================================

    SpecTree = NewObject<UMXSpecTree>(this);

    UDataTable* SpecDataTable = nullptr;
    if (!SpecTreeTableAsset.IsNull())
    {
        SpecDataTable = SpecTreeTableAsset.LoadSynchronous();
        if (!SpecDataTable) { UE_LOG(LogTemp, Warning, TEXT("[MX] Failed to load DT_SpecTree.")); }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[MX] SpecTreeTableAsset not set — SpecTree will function without DataTable descriptions."));
    }

    SpecTree->Initialize(SpecDataTable, RobotProviderSI, EventBus);
    UE_LOG(LogTemp, Log, TEXT("[MX] Specialization module created."));

    // ================================================================
    // Phase 7: Evolution
    // ================================================================
    UE_LOG(LogTemp, Log, TEXT("[MX] Evolution module ready (static utilities + actor component)."));

    // ================================================================
    // Phase 8: Reports
    // ================================================================

    RunReportEngine = NewObject<UMXRunReportEngine>(this);

    // Build IMXRunProvider TScriptInterface from RunManager
    TScriptInterface<IMXRunProvider> RunProviderSI;
    if (RunManager)
    {
        RunProviderSI.SetObject(RunManager.Get());
        RunProviderSI.SetInterface(
            static_cast<IMXRunProvider*>(
                RunManager->GetNativeInterfaceAddress(UMXRunProvider::StaticClass())));
    }

    // Load Report DataTables (all optional)
    auto LoadOptionalDT = [](TSoftObjectPtr<UDataTable>& Ref, const TCHAR* Name) -> UDataTable*
    {
        if (Ref.IsNull()) return nullptr;
        UDataTable* DT = Ref.LoadSynchronous();
        if (!DT) { UE_LOG(LogTemp, Warning, TEXT("[MX] Failed to load %s."), Name); }
        return DT;
    };

    UDataTable* OpeningDT   = LoadOptionalDT(ReportOpeningTemplatesAsset,   TEXT("DT_ReportOpeningTemplates"));
    UDataTable* HighlightDT = LoadOptionalDT(ReportHighlightTemplatesAsset, TEXT("DT_ReportHighlightTemplates"));
    UDataTable* AwardDT     = LoadOptionalDT(AwardTemplatesAsset,           TEXT("DT_AwardTemplates"));
    UDataTable* EulogyDT    = LoadOptionalDT(EulogyTemplatesAsset,          TEXT("DT_EulogyTemplates"));
    UDataTable* ClosingDT   = LoadOptionalDT(ReportClosingTemplatesAsset,   TEXT("DT_ReportClosingTemplates"));

    RunReportEngine->Initialize(
        EventBus,
        RobotProviderSI,
        HatProviderSI,
        RunProviderSI,
        OpeningDT, HighlightDT, AwardDT, EulogyDT, ClosingDT
    );

    UE_LOG(LogTemp, Log, TEXT("[MX] Reports module created: RunReportEngine + StatCompiler + HighlightScorer + AwardSelector + ReportNarrator."));

    // ================================================================
    UE_LOG(LogTemp, Log, TEXT("[MX] ============================================"));
    UE_LOG(LogTemp, Log, TEXT("[MX]  Micro Exodus — Phase 8: Reports"));
    UE_LOG(LogTemp, Log, TEXT("[MX]  Contracts + EventBus + Identity + DEMS"));
    UE_LOG(LogTemp, Log, TEXT("[MX]  + Hats + Roguelike + Specialization"));
    UE_LOG(LogTemp, Log, TEXT("[MX]  + Evolution + Reports"));
    UE_LOG(LogTemp, Log, TEXT("[MX] ============================================"));
}

void UMXGameInstance::Shutdown()
{
    if (RunReportEngine)
    {
        RunReportEngine->Shutdown();
    }

    if (HatManager)
    {
        HatManager->Shutdown();
    }

    if (MessageDispatcher)
    {
        MessageDispatcher->ClearAllListeners();
    }

    UE_LOG(LogTemp, Log, TEXT("[MX] GameInstance::Shutdown."));
    Super::Shutdown();
}
