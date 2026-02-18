#include "VaroniaMqttLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FString UVaroniaMqttLibrary::FormatMqttMessage(int32 ClientID, FString MethodName, int32 SoftStateValue)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());

    RootObject->SetNumberField(TEXT("CallerDeviceID"), ClientID);
    RootObject->SetNumberField(TEXT("TargetDeviceID"), 0);
    RootObject->SetStringField(TEXT("sMethod"), MethodName);

    // On n'ajoute "Items" que si SoftStateValue a été fourni (!= -1)
    if (SoftStateValue != -1)
    {
        TSharedPtr<FJsonObject> ItemsObject = MakeShareable(new FJsonObject());
        ItemsObject->SetNumberField(TEXT("SoftState"), SoftStateValue);
        RootObject->SetObjectField(TEXT("Items"), ItemsObject);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    return OutputString;
}