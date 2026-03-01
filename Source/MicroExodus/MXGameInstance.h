// MXGameInstance.h — Phase 8: Reports Module
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MXGameInstance.generated.h"

// Identity
class UMXRobotManager;
class UMXNameGenerator;
class UMXPersonalityGenerator;
class UMXLifeLog;
class UMXAgingSystem;

// DEMS
class UMXMessageBuilder;
class UMXMessageDispatcher;
class UMXTemplateSelector;
class UMXDeduplicationBuffer;

// Roguelike
class UMXRunManager;

// Hats
class UMXHatManager;

// Specialization
class UMXSpecTree;

// Reports
class UMXRunReportEngine;

class UDataTable;

UCLASS()
class MICROEXODUS_API UMXGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    // ---- Identity ----
    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXRobotManager* GetRobotManager() const { return RobotManager; }

    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXNameGenerator* GetNameGenerator() const { return NameGenerator; }

    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXLifeLog* GetLifeLog() const { return LifeLog; }

    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXAgingSystem* GetAgingSystem() const { return AgingSystem; }

    // ---- DEMS ----
    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXMessageBuilder* GetMessageBuilder() const { return MessageBuilder; }

    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXMessageDispatcher* GetMessageDispatcher() const { return MessageDispatcher; }

    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXTemplateSelector* GetTemplateSelector() const { return TemplateSelector; }

    // ---- Roguelike ----
    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXRunManager* GetRunManager() const { return RunManager; }

    // ---- Hats ----
    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXHatManager* GetHatManager() const { return HatManager; }

    // ---- Specialization ----
    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXSpecTree* GetSpecTree() const { return SpecTree; }

    // ---- Reports ----
    UFUNCTION(BlueprintCallable, Category = "MX")
    UMXRunReportEngine* GetRunReportEngine() const { return RunReportEngine; }

    // ---- DataTable asset references ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> NamesTableAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> PersonalitiesTableAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> TierModifiersTableAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> XPCurveTableAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> HatDefinitionsTableAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> SpecTreeTableAsset;

    // Reports DataTables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> ReportOpeningTemplatesAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> ReportHighlightTemplatesAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> AwardTemplatesAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> EulogyTemplatesAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|DataTables")
    TSoftObjectPtr<UDataTable> ReportClosingTemplatesAsset;

private:

    // ---- Identity ----
    UPROPERTY() TObjectPtr<UMXRobotManager> RobotManager;
    UPROPERTY() TObjectPtr<UMXNameGenerator> NameGenerator;
    UPROPERTY() TObjectPtr<UMXPersonalityGenerator> PersonalityGenerator;
    UPROPERTY() TObjectPtr<UMXLifeLog> LifeLog;
    UPROPERTY() TObjectPtr<UMXAgingSystem> AgingSystem;

    // ---- DEMS ----
    UPROPERTY() TObjectPtr<UMXMessageBuilder> MessageBuilder;
    UPROPERTY() TObjectPtr<UMXMessageDispatcher> MessageDispatcher;
    UPROPERTY() TObjectPtr<UMXTemplateSelector> TemplateSelector;
    UPROPERTY() TObjectPtr<UMXDeduplicationBuffer> DedupBuffer;

    // ---- Roguelike ----
    UPROPERTY() TObjectPtr<UMXRunManager> RunManager;

    // ---- Hats ----
    UPROPERTY() TObjectPtr<UMXHatManager> HatManager;

    // ---- Specialization ----
    UPROPERTY() TObjectPtr<UMXSpecTree> SpecTree;

    // ---- Reports ----
    UPROPERTY() TObjectPtr<UMXRunReportEngine> RunReportEngine;
};
