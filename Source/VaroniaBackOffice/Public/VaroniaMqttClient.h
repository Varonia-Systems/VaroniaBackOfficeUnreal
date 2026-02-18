#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interface/MqttClientInterface.h"
#include "Entities/MqttClientConfig.h"
#include "Entities/MqttConnectionData.h"
#include "VaroniaMqttClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVaroniaMqttConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVaroniaMqttDisconnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVaroniaMqttError, int32, Code, FString, Message);

UCLASS(BlueprintType)
class VARONIABACKOFFICE_API UVaroniaMqttClient : public UObject
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Varonia|MQTT")
    void Connect(const FString& Host, int32 Port, int32 InClientID);

    UFUNCTION(BlueprintCallable, Category = "Varonia|MQTT")
    void Disconnect();

    UFUNCTION(BlueprintPure, Category = "Varonia|MQTT")
    bool IsConnected() const { return bIsConnected; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Varonia|MQTT")
    FOnVaroniaMqttConnected OnConnected;

    UPROPERTY(BlueprintAssignable, Category = "Varonia|MQTT")
    FOnVaroniaMqttDisconnected OnDisconnected;

    UPROPERTY(BlueprintAssignable, Category = "Varonia|MQTT")
    FOnVaroniaMqttError OnError;

    UFUNCTION(BlueprintPure, Category = "Varonia|MQTT")
    TScriptInterface<IMqttClientInterface> GetMqttClient() const { return MqttClient; }

    UPROPERTY(BlueprintReadOnly, Category = "Varonia|MQTT")
    int32 ClientID = 0;


private:

    UPROPERTY()
    TScriptInterface<IMqttClientInterface> MqttClient;

    bool bIsConnected = false;

    UFUNCTION()
    void HandleConnected();

    UFUNCTION()
    void HandleDisconnected();

    UFUNCTION()
    void HandleError(int Code, FString Message);
};