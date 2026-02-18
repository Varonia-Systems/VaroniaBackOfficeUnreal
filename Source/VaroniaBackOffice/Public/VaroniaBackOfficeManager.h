#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LBE_Types.h"
#include "VaroniaMqttClient.h"
#include "VaroniaBackOfficeManager.generated.h"

// Custom log category — control in console: Log LogVaronia Verbose / Log LogVaronia Warning
DECLARE_LOG_CATEGORY_EXTERN(LogVaronia, Log, All);

UCLASS()
class VARONIABACKOFFICE_API UVaroniaBackOfficeManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // --- Config ---

    UFUNCTION(BlueprintCallable, Category = "Varonia|Config")
    bool LoadLBEConfig();

    UPROPERTY(BlueprintReadWrite, Category = "Varonia|Config")
    bool GameStarted;

    UPROPERTY(BlueprintReadWrite, Category = "Varonia|Config")
    ESoftState CurrentSoftState = ESoftState::GAME_LAUNCHED;

    UPROPERTY(BlueprintReadOnly, Category = "Varonia|Config")
    FLBEConfig CurrentConfig;

    UPROPERTY(BlueprintReadOnly, Category = "Varonia|Config")
    AActor* Varonia_BP = nullptr;


    UPROPERTY(BlueprintReadOnly, Category = "Varonia|MQTT")
    UVaroniaMqttClient* MqttHandler = nullptr;



    // --- Spatial ---

    UFUNCTION(BlueprintCallable, Category = "Varonia|Spatial")
    bool LoadSpatialConfig();

    UPROPERTY(BlueprintReadOnly, Category = "Varonia|Spatial")
    FSpatialConfig SpatialConfig;

    UPROPERTY(BlueprintReadOnly, Category = "Varonia|Spatial")
    bool bSpatialConfigLoaded = false;

    // --- Spatial Helpers ---

    UFUNCTION(BlueprintPure, Category = "Varonia|Spatial")
    bool GetMainBoundary(FSpatialBoundary& OutBoundary) const;

    UFUNCTION(BlueprintPure, Category = "Varonia|Spatial")
    TArray<FSpatialBoundary> GetSubBoundaries() const;

private:
    FString GetConfigPath();
    FString GetSpatialPath();

    void OnWorldCreated(UWorld* World, const UWorld::InitializationValues IValues);

    static FVector UnityToUnreal(float X, float Y, float Z);
    static FRotator UnityQuatToUnrealRotator(float X, float Y, float Z, float W);
  virtual void Deinitialize() override;
};