#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
// On inclut les headers nécessaires pour les structs et le JSON
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "VaroniaMqttLibrary.generated.h"

USTRUCT(BlueprintType)
struct FVaroniaMqttItems
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Varonia")
    int32 SoftState = 0;
};

USTRUCT(BlueprintType)
struct FVaroniaMqttPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Varonia")
    int32 CallerDeviceID = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Varonia")
    int32 TargetDeviceID = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Varonia")
    FString sMethod;

    UPROPERTY(BlueprintReadWrite, Category = "Varonia")
    FVaroniaMqttItems Items;
};

UCLASS()
class VARONIABACKOFFICE_API UVaroniaMqttLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Varonia|MQTT")
    static FString FormatMqttMessage(int32 ClientID, FString MethodName, int32 SoftStateValue = -1);
};