#include "VaroniaMqttClient.h"
#include "MqttUtilitiesBPL.h"

DEFINE_LOG_CATEGORY_STATIC(LogVaroniaMqtt, Log, All);

void UVaroniaMqttClient::Connect(const FString& Host, int32 Port, int32 InClientID)
{

    ClientID = InClientID;

    if (MqttClient.GetObject())
    {
        UE_LOG(LogVaroniaMqtt, Warning, TEXT("MQTT client already exists. Disconnect first."));
        return;
    }

    // Config
    FMqttClientConfig Config;
    Config.HostUrl = Host;
    Config.Port = Port;
    Config.ClientId = FString::Printf(TEXT("Varonia_%d"), ClientID);

    // Create
    MqttClient = UMqttUtilitiesBPL::CreateMqttClient(Config);
    if (!MqttClient.GetObject())
    {
        UE_LOG(LogVaroniaMqtt, Error, TEXT("Failed to create MQTT client"));
        return;
    }

    // Error
    FOnMqttErrorDelegate OnErrorDelegate;
    OnErrorDelegate.BindDynamic(this, &UVaroniaMqttClient::HandleError);
    MqttClient->SetOnErrorHandler(OnErrorDelegate);

    // Connect
    FOnConnectDelegate OnConnectDelegate;
    OnConnectDelegate.BindDynamic(this, &UVaroniaMqttClient::HandleConnected);

    FMqttConnectionData ConnectionData;
    ConnectionData.Login = TEXT("");
    ConnectionData.Password = TEXT("");

    MqttClient->Connect(ConnectionData, OnConnectDelegate);

    UE_LOG(LogVaroniaMqtt, Log, TEXT("MQTT connecting to %s:%d (ID: %s)..."),
        *Host, Port, *Config.ClientId);
}

void UVaroniaMqttClient::Disconnect()
{
    if (!MqttClient.GetObject()) return;

    FOnDisconnectDelegate OnDisconnectDelegate;
    OnDisconnectDelegate.BindDynamic(this, &UVaroniaMqttClient::HandleDisconnected);
    MqttClient->Disconnect(OnDisconnectDelegate);
}

void UVaroniaMqttClient::HandleConnected()
{
    bIsConnected = true;
    UE_LOG(LogVaroniaMqtt, Log, TEXT("MQTT Connected!"));
    OnConnected.Broadcast();
}

void UVaroniaMqttClient::HandleDisconnected()
{
    bIsConnected = false;
    MqttClient = nullptr;
    UE_LOG(LogVaroniaMqtt, Log, TEXT("MQTT Disconnected"));
    OnDisconnected.Broadcast();
}

void UVaroniaMqttClient::HandleError(int Code, FString Message)
{
    UE_LOG(LogVaroniaMqtt, Error, TEXT("MQTT Error %d: %s"), Code, *Message);
    OnError.Broadcast(Code, Message);
}