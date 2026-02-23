#include "VaroniaBackOfficeManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

// Define the log category
DEFINE_LOG_CATEGORY(LogVaronia);

// ============================================================================
// Coordinate conversion: Unity ? Unreal
// ============================================================================

FVector UVaroniaBackOfficeManager::UnityToUnreal(float X, float Y, float Z)
{
    return FVector(
        Z * 100.f,   // Unity Z (forward) ? Unreal X (forward)
        X * 100.f,   // Unity X (right)   ? Unreal Y (right)
        Y * 100.f    // Unity Y (up)      ? Unreal Z (up)
    );
}

FRotator UVaroniaBackOfficeManager::UnityQuatToUnrealRotator(float X, float Y, float Z, float W)
{
    FQuat UnrealQuat(Z, X, Y, W);
    return UnrealQuat.Rotator();
}

// ============================================================================
// Initialize
// ============================================================================

void UVaroniaBackOfficeManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadLBEConfig();
    LoadSpatialConfig();

    FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UVaroniaBackOfficeManager::OnWorldCreated);
}

// ============================================================================
// World Created ? Spawn MQTT Blueprint
// ============================================================================

void UVaroniaBackOfficeManager::OnWorldCreated(UWorld* World, const UWorld::InitializationValues IValues)
{


    if (!World || !World->IsGameWorld()) return;

    FString BPPath = TEXT("/VaroniaBackOffice/BP_Varonia.BP_Varonia_C");
    UClass* Varonia = StaticLoadClass(AActor::StaticClass(), nullptr, *BPPath);


    if (!MqttHandler)
    {
        MqttHandler = NewObject<UVaroniaMqttClient>(this);
        MqttHandler->Connect(CurrentConfig.MQTT_ServerIP, 1883, CurrentConfig.MQTT_IDClient);
    
    }


    if (Varonia)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Varonia_BP = World->SpawnActor<AActor>(Varonia, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

#if WITH_EDITOR
        if (Varonia_BP)
        {
            Varonia_BP->SetActorLabel(TEXT("[Global Varonia]"));
        }
#endif

        UE_LOG(LogVaronia, Log, TEXT("BP_Varonia spawned successfully"));
    }
    else
    {
        UE_LOG(LogVaronia, Error, TEXT("Failed to load BP_Varonia at: %s"), *BPPath);
    }
}


void UVaroniaBackOfficeManager::Deinitialize()
{
    if (MqttHandler)
    {
        MqttHandler->Disconnect();
    }
    Super::Deinitialize();
}




// ============================================================================
// Paths
// ============================================================================

FString UVaroniaBackOfficeManager::GetConfigPath()
{
    FString UserProfile = FPlatformMisc::GetEnvironmentVariable(TEXT("USERPROFILE"));
    FString FullPath = FPaths::Combine(UserProfile, TEXT("AppData"), TEXT("LocalLow"), TEXT("Varonia"), TEXT("GlobalConfig.json"));
    FPaths::NormalizeFilename(FullPath);

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    FString Directory = FPaths::GetPath(FullPath);
    if (!PlatformFile.DirectoryExists(*Directory)) { PlatformFile.CreateDirectoryTree(*Directory); }

    return FullPath;
}

FString UVaroniaBackOfficeManager::GetSpatialPath()
{
    FString UserProfile = FPlatformMisc::GetEnvironmentVariable(TEXT("USERPROFILE"));
    FString FullPath = FPaths::Combine(UserProfile, TEXT("AppData"), TEXT("LocalLow"), TEXT("Varonia"), TEXT("NewSpatial.json"));
    FPaths::NormalizeFilename(FullPath);
    return FullPath;
}

// ============================================================================
// LoadLBEConfig
// ============================================================================

bool UVaroniaBackOfficeManager::LoadLBEConfig()
{
    FString FilePath = GetConfigPath();
    FString JsonString;

    UE_LOG(LogVaronia, Verbose, TEXT("Config path: %s"), *FilePath);

    if (FFileHelper::LoadFileToString(JsonString, *FilePath)) {
        if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &CurrentConfig, 0, 0)) {

            const UEnum* ModeEnum = StaticEnum<EDeviceMode>();
            const UEnum* HandEnum = StaticEnum<EMainHand>();

            UE_LOG(LogVaronia, Log, TEXT("Config loaded successfully"));
            UE_LOG(LogVaronia, Log, TEXT("  PlayerName: %s"), *CurrentConfig.PlayerName);
            UE_LOG(LogVaronia, Log, TEXT("  ServerIP: %s"), *CurrentConfig.ServerIP);
            UE_LOG(LogVaronia, Log, TEXT("  MQTT_ServerIP: %s"), *CurrentConfig.MQTT_ServerIP);
            UE_LOG(LogVaronia, Log, TEXT("  MQTT_IDClient: %d"), CurrentConfig.MQTT_IDClient);
            UE_LOG(LogVaronia, Log, TEXT("  DeviceMode: %s"), *ModeEnum->GetNameStringByValue((int64)CurrentConfig.DeviceMode));
            UE_LOG(LogVaronia, Log, TEXT("  MainHand: %s"), *HandEnum->GetNameStringByValue((int64)CurrentConfig.MainHand));
            UE_LOG(LogVaronia, Log, TEXT("  Language: %s"), *CurrentConfig.Language);

            return true;
        }
        else
        {
            UE_LOG(LogVaronia, Error, TEXT("Failed to parse GlobalConfig.json"));
        }
    }
    else
    {
        UE_LOG(LogVaronia, Warning, TEXT("GlobalConfig.json not found, creating default"));
    }

    // Default config creation
    CurrentConfig = FLBEConfig();
    TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("ServerIP"), CurrentConfig.ServerIP);
    JsonObject->SetStringField(TEXT("MQTT_ServerIP"), CurrentConfig.MQTT_ServerIP);
    JsonObject->SetNumberField(TEXT("MQTT_IDClient"), (double)CurrentConfig.MQTT_IDClient);

    
    JsonObject->SetNumberField(TEXT("DeviceMode"), (int32)CurrentConfig.DeviceMode);
    JsonObject->SetStringField(TEXT("Language"), CurrentConfig.Language);

 
    JsonObject->SetNumberField(TEXT("MainHand"), (int32)CurrentConfig.MainHand);
    JsonObject->SetStringField(TEXT("PlayerName"), CurrentConfig.PlayerName);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    if (FJsonSerializer::Serialize(JsonObject, Writer)) { FFileHelper::SaveStringToFile(OutputString, *FilePath); }

    return false;
}

// ============================================================================
// LoadSpatialConfig
// ============================================================================

bool UVaroniaBackOfficeManager::LoadSpatialConfig()
{
    FString FilePath = GetSpatialPath();
    FString JsonString;

    UE_LOG(LogVaronia, Verbose, TEXT("Spatial path: %s"), *FilePath);

    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogVaronia, Warning, TEXT("NewSpatial.json not found at: %s"), *FilePath);
        bSpatialConfigLoaded = false;
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogVaronia, Error, TEXT("Failed to parse NewSpatial.json"));
        bSpatialConfigLoaded = false;
        return false;
    }

    // --- Root fields ---
    SpatialConfig.ID = RootObject->GetStringField(TEXT("ID"));
    SpatialConfig.Name = RootObject->GetStringField(TEXT("Name"));
    SpatialConfig.AreaValue = RootObject->GetStringField(TEXT("AreaValue"));
    SpatialConfig.MaxRect = RootObject->GetStringField(TEXT("MaxRect"));
    SpatialConfig.GroupName = RootObject->GetStringField(TEXT("GroupName"));
    SpatialConfig.MaxPlayer = (int32)RootObject->GetNumberField(TEXT("MaxPlayer"));
    SpatialConfig.Multiplier = (float)RootObject->GetNumberField(TEXT("Multiplier"));
    SpatialConfig.OrthoKey = RootObject->GetStringField(TEXT("OrthoKey"));

    // --- SyncPos ---
    const TSharedPtr<FJsonObject>* SyncPosObj;
    if (RootObject->TryGetObjectField(TEXT("SyncPos"), SyncPosObj))
    {
        float sx = (float)(*SyncPosObj)->GetNumberField(TEXT("x"));
        float sy = (float)(*SyncPosObj)->GetNumberField(TEXT("y"));
        float sz = (float)(*SyncPosObj)->GetNumberField(TEXT("z"));
        SpatialConfig.SyncPosition = UnityToUnreal(sx, sy, sz);
    }

    // --- SyncQuaternion ---
    const TSharedPtr<FJsonObject>* SyncQuatObj;
    if (RootObject->TryGetObjectField(TEXT("SyncQuaterion"), SyncQuatObj))
    {
        float qx = (float)(*SyncQuatObj)->GetNumberField(TEXT("x"));
        float qy = (float)(*SyncQuatObj)->GetNumberField(TEXT("y"));
        float qz = (float)(*SyncQuatObj)->GetNumberField(TEXT("z"));
        float qw = (float)(*SyncQuatObj)->GetNumberField(TEXT("w"));
        SpatialConfig.SyncRotation = UnityQuatToUnrealRotator(qx, qy, qz, qw);
    }

    // --- Boundaries ---
    const TArray<TSharedPtr<FJsonValue>>* BoundariesArray;
    if (RootObject->TryGetArrayField(TEXT("Boundaries"), BoundariesArray))
    {
        SpatialConfig.Boundaries.Empty();

        for (const TSharedPtr<FJsonValue>& BoundaryValue : *BoundariesArray)
        {
            const TSharedPtr<FJsonObject>& BObj = BoundaryValue->AsObject();
            if (!BObj.IsValid()) continue;

            FSpatialBoundary Boundary;
            Boundary.ID = BObj->GetStringField(TEXT("ID"));
            Boundary.DisplayDistance = (float)BObj->GetNumberField(TEXT("DisplayDistance"));
            Boundary.bReverse = BObj->GetBoolField(TEXT("Reverse"));
            Boundary.bBoundaryMoreVisible = BObj->GetBoolField(TEXT("BoundaryMoreVisible"));
            Boundary.bAlertLimit = BObj->GetBoolField(TEXT("AlertLimit"));
            Boundary.bMainBoundary = BObj->GetBoolField(TEXT("MainBoundary"));
            Boundary.bVisible = BObj->GetBoolField(TEXT("Visible"));

            // Color
            const TSharedPtr<FJsonObject>* ColorObj;
            if (BObj->TryGetObjectField(TEXT("BoundaryColor"), ColorObj))
            {
                Boundary.BoundaryColor = FLinearColor(
                    (float)(*ColorObj)->GetNumberField(TEXT("x")),
                    (float)(*ColorObj)->GetNumberField(TEXT("y")),
                    (float)(*ColorObj)->GetNumberField(TEXT("z")),
                    1.0f
                );
            }

            // Points
            const TArray<TSharedPtr<FJsonValue>>* PointsArray;
            if (BObj->TryGetArrayField(TEXT("Points"), PointsArray))
            {
                for (const TSharedPtr<FJsonValue>& PointValue : *PointsArray)
                {
                    const TSharedPtr<FJsonObject>& PObj = PointValue->AsObject();
                    if (!PObj.IsValid()) continue;

                    float px = (float)PObj->GetNumberField(TEXT("x"));
                    float py = (float)PObj->GetNumberField(TEXT("y"));
                    float pz = (float)PObj->GetNumberField(TEXT("z"));

                    Boundary.Points.Add(UnityToUnreal(px, py, pz));
                }
            }

            SpatialConfig.Boundaries.Add(Boundary);

            UE_LOG(LogVaronia, Verbose, TEXT("  Boundary [%s] — %d points | Main=%d | Visible=%d"),
                *Boundary.ID, Boundary.Points.Num(), Boundary.bMainBoundary, Boundary.bVisible);
        }
    }

    bSpatialConfigLoaded = true;

    UE_LOG(LogVaronia, Log, TEXT("Spatial loaded: %s (%s) — %d boundaries"),
        *SpatialConfig.Name, *SpatialConfig.AreaValue, SpatialConfig.Boundaries.Num());
    UE_LOG(LogVaronia, Verbose, TEXT("  SyncPos: %s"), *SpatialConfig.SyncPosition.ToString());
    UE_LOG(LogVaronia, Verbose, TEXT("  SyncRot: %s"), *SpatialConfig.SyncRotation.ToString());

    return true;
}

// ============================================================================
// Blueprint Helpers
// ============================================================================

bool UVaroniaBackOfficeManager::GetMainBoundary(FSpatialBoundary& OutBoundary) const
{
    for (const FSpatialBoundary& B : SpatialConfig.Boundaries)
    {
        if (B.bMainBoundary)
        {
            OutBoundary = B;
            return true;
        }
    }
    return false;
}

TArray<FSpatialBoundary> UVaroniaBackOfficeManager::GetSubBoundaries() const
{
    TArray<FSpatialBoundary> Result;
    for (const FSpatialBoundary& B : SpatialConfig.Boundaries)
    {
        if (!B.bMainBoundary)
        {
            Result.Add(B);
        }
    }
    return Result;
}