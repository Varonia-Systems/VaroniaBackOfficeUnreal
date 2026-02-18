// OrthoCapture.cpp
// Unreal Engine equivalent of the Unity Ortho View plugin
// Place in: Plugins/VaroniaBackOffice/Source/VaroniaBackOfficeEditor/Private/

#include "OrthoCapture.h"
#include "Containers/Ticker.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformFilemanager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Slate/SceneViewport.h"
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "HighResScreenshot.h"

#define LOCTEXT_NAMESPACE "OrthoCapture"

// Helper: safely get the editor world (returns nullptr if not available)
static UWorld* GetEditorWorld()
{
	if (!GEditor) return nullptr;
	return GEditor->GetEditorWorldContext().World();
}

// Helper: get the active level viewport client safely for UE 5.7
static FLevelEditorViewportClient* GetActiveLevelViewportClient()
{
	FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor");
	if (!LevelEditor) return nullptr;

	TSharedPtr<SLevelViewport> ActiveLevelViewport = LevelEditor->GetFirstActiveLevelViewport();
	if (!ActiveLevelViewport.IsValid()) return nullptr;

	return &ActiveLevelViewport->GetLevelViewportClient();
}

// =============================================================================
// SOrthoCapture — Slate widget (the panel itself)
// =============================================================================

void SOrthoCapture::Construct(const FArguments& InArgs)
{
	bIsSetupMode = false;
	bIsCleanedUp = false;
	RebuildUI();
}

SOrthoCapture::~SOrthoCapture()
{
	if (!bIsCleanedUp && TempCamActor.IsValid())
	{
		Cleanup();
	}
}

// --------------- UI construction ---------------

void SOrthoCapture::RebuildUI()
{
	ChildSlot
		[
			SNew(SBox)
				.Padding(10.f)
				[
					bIsSetupMode ? BuildSetupUI() : BuildStartUI()
				]
		];
}

TSharedRef<SWidget> SOrthoCapture::BuildStartUI()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("Info", "Click Setup to configure the orthographic view in the viewport."))
				.AutoWrapText(true)
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this]()
					{
						OnStartSetup();
						return FReply::Handled();
					})
				.Content()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("StartBtn", "Start Setup"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				]
		];
}

TSharedRef<SWidget> SOrthoCapture::BuildSetupUI()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("ViewSettings", "View Settings"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
				[
					SNew(STextBlock).Text(LOCTEXT("Zoom", "Zoom (Size)"))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SSpinBox<float>)
						.MinValue(1.f)
						.MaxValue(100.f)
						.Delta(1.f)
						.Value(OrthographicSize)
						.OnValueChanged(this, &SOrthoCapture::OnZoomChanged)
				]
		]

	+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
				[
					SNew(STextBlock).Text(LOCTEXT("Alt", "Altitude (Z)"))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SSpinBox<float>)
						.MinValue(1.f)
						.MaxValue(30000.f)
						.Value(CameraHeight)
						.OnValueChanged(this, &SOrthoCapture::OnAltitudeChanged)
				]
		]

	+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SSpacer)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SNew(SButton)
				.HAlign(HAlign_Center)
				.ButtonColorAndOpacity(FLinearColor::Green)
				.OnClicked_Lambda([this]()
					{
						CaptureAndExit();
						return FReply::Handled();
					})
				.Content()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("Capture", "CAPTURE & CLOSE (1080p)"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				]
		]

	+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SNew(SButton)
				.HAlign(HAlign_Center)
				.ButtonColorAndOpacity(FLinearColor::Red)
				.OnClicked_Lambda([this]()
					{
						Cleanup();
						TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
						if (ParentWindow.IsValid())
						{
							ParentWindow->RequestDestroyWindow();
						}
						return FReply::Handled();
					})
				.Content()
				[
					SNew(STextBlock).Text(LOCTEXT("Cancel", "Cancel"))
				]
		];
}

// --------------- Callbacks ---------------

void SOrthoCapture::OnZoomChanged(float NewValue)
{
	OrthographicSize = NewValue;
	UpdateCamera();
}

void SOrthoCapture::OnAltitudeChanged(float NewValue)
{
	CameraHeight = NewValue;
	UpdateCamera();
}

// --------------- Core logic ---------------

void SOrthoCapture::OnStartSetup()
{
	bIsSetupMode = true;
	bIsCleanedUp = false;
	UpdateCamera();
	RebuildUI();
}

void SOrthoCapture::UpdateCamera()
{
	UWorld* World = GetEditorWorld();
	if (!World) return;

	ACameraActor* CamActor = nullptr;

	if (TempCamActor.IsValid())
	{
		CamActor = TempCamActor.Get();
	}
	else
	{
		// Look for existing one first (leftover from previous session)
		for (TActorIterator<ACameraActor> It(World); It; ++It)
		{
			if (It->GetActorLabel() == TEXT("TempOrthoCam"))
			{
				CamActor = *It;
				break;
			}
		}

		if (!CamActor)
		{
			FActorSpawnParameters Params;
			// Use unique name to avoid conflicts with destroyed-but-not-yet-GC'd actors
			Params.Name = MakeUniqueObjectName(World, ACameraActor::StaticClass(), FName("TempOrthoCam"));
			CamActor = World->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			if (!CamActor)
			{
				UE_LOG(LogTemp, Error, TEXT("OrthoCapture: Failed to spawn camera actor!"));
				return;
			}
			CamActor->SetActorLabel(TEXT("TempOrthoCam"));
		}
		TempCamActor = CamActor;
	}

	UCameraComponent* CamComp = CamActor->GetCameraComponent();
	CamComp->SetProjectionMode(ECameraProjectionMode::Orthographic);
	CamComp->SetOrthoWidth(OrthographicSize * 355.f);

	CamActor->SetActorLocation(FVector(0.f, 0.f, CameraHeight));
	CamActor->SetActorRotation(FRotator(-90.f, 90.f, 0.f));

	FLevelEditorViewportClient* ViewportClient = GetActiveLevelViewportClient();
	if (ViewportClient)
	{
		ViewportClient->SetActorLock(CamActor);
		ViewportClient->SetViewMode(VMI_Lit);
		ViewportClient->SetWidgetMode(UE::Widget::WM_None);
		ViewportClient->EngineShowFlags.SetModeWidgets(false);
		ViewportClient->EngineShowFlags.SetSelection(false);
		ViewportClient->EngineShowFlags.SetBillboardSprites(false);
		
		ViewportClient->EngineShowFlags.SetGrid(false);
		GEditor->SelectNone(true, true, false);
		ViewportClient->Invalidate();
	}
}

void SOrthoCapture::CaptureAndExit()
{
	UWorld* World = GetEditorWorld();
	if (!World || !TempCamActor.IsValid()) return;

	FLevelEditorViewportClient* ViewportClient = GetActiveLevelViewportClient();
	if (!ViewportClient || !ViewportClient->Viewport)
	{
		Cleanup();
		return;
	}

	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	if (MapName.IsEmpty()) MapName = TEXT("UntitledMap");

	FString FileName = FString::Printf(TEXT("%s_%.1f"), *MapName, OrthographicSize);
	FString FullPath = FPaths::Combine(FPaths::ProjectDir(), FileName);

	FHighResScreenshotConfig& ScreenshotConfig = GetHighResScreenshotConfig();
	ScreenshotConfig.SetResolution(1920, 1080);
	ScreenshotConfig.SetFilename(FullPath);
	ScreenshotConfig.bMaskEnabled = false;


	if (TempCamActor.IsValid())
	{
		TempCamActor->SetIsTemporarilyHiddenInEditor(true);
	}


	// Prend le screenshot MAINTENANT pendant que la caméra ortho est active
	ViewportClient->Viewport->TakeHighResScreenShot();

	UE_LOG(LogTemp, Log, TEXT("OrthoCapture: Screenshot requested -> %s"), *FullPath);

	// Attendre plusieurs ticks pour que le screenshot soit terminé avant cleanup
	int32* TickCount = new int32(0);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[this, TickCount](float DeltaTime) -> bool
		{
			(*TickCount)++;
			if (*TickCount >= 10) // ~10 frames d'attente
			{
				delete TickCount;

				Cleanup();

				TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
				if (ParentWindow.IsValid())
				{
					ParentWindow->RequestDestroyWindow();
				}
				return false; // Stop le ticker
			}
			return true; // Continue
		}
	));
}

void SOrthoCapture::Cleanup()
{
	// Prevent double cleanup
	if (bIsCleanedUp) return;
	bIsCleanedUp = true;

	// Safety check
	if (!GEditor)
	{
		TempCamActor.Reset();
		return;
	}

	UWorld* World = GetEditorWorld();
	if (!World)
	{
		TempCamActor.Reset();
		return;
	}

	// Unlock viewport and restore gizmos
	FLevelEditorViewportClient* ViewportClient = GetActiveLevelViewportClient();
	if (ViewportClient)
	{
		ViewportClient->SetActorLock(nullptr);
		ViewportClient->EngineShowFlags.SetModeWidgets(true);
		ViewportClient->EngineShowFlags.SetSelection(true);
		ViewportClient->EngineShowFlags.SetBillboardSprites(true);
	
		ViewportClient->EngineShowFlags.SetGrid(true);
		ViewportClient->SetWidgetMode(UE::Widget::WM_Translate);
		ViewportClient->Invalidate();
	}

	// Destroy temp camera
	if (TempCamActor.IsValid())
	{
		TempCamActor->Destroy();
		TempCamActor.Reset();
	}
	else
	{
		// Fallback: find by label
		for (TActorIterator<ACameraActor> It(World); It; ++It)
		{
			if (It->GetActorLabel() == TEXT("TempOrthoCam"))
			{
				It->Destroy();
				break;
			}
		}
	}
}

// =============================================================================
// FOrthoCaptureModeModule — Menu registration
// =============================================================================

FName FOrthoCaptureModeModule::TabName = FName("OrthoCapture");

void FOrthoCaptureModeModule::Register()
{
	UE_LOG(LogTemp, Warning, TEXT("OrthoCapture: Register() called"));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TabName,
		FOnSpawnTab::CreateStatic(&FOrthoCaptureModeModule::SpawnTab))
		.SetDisplayName(LOCTEXT("TabTitle", "Ortho Capture"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus* ToolMenus = UToolMenus::Get();
	UToolMenu* MainMenu = ToolMenus->ExtendMenu("LevelEditor.MainMenu");

	MainMenu->AddSubMenu(
		"MainMenu",
		NAME_None,
		"Varonia",
		LOCTEXT("VaroniaMenu", "Varonia"),
		LOCTEXT("VaroniaMenuTip", "Varonia Tools")
	);

	UToolMenu* VaroniaSubMenu = ToolMenus->ExtendMenu("LevelEditor.MainMenu.Varonia");
	VaroniaSubMenu->AddMenuEntry(
		NAME_None,
		FToolMenuEntry::InitMenuEntry(
			"OrthoView",
			LOCTEXT("MenuOrtho", "Ortho View"),
			LOCTEXT("MenuOrthoTip", "Open the Orthographic Capture tool"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FOrthoCaptureModeModule::OpenWindow))
		)
	);

	ToolMenus->RefreshAllWidgets();
}

void FOrthoCaptureModeModule::Unregister()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

TSharedRef<SDockTab> FOrthoCaptureModeModule::SpawnTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SOrthoCapture)
		];
}

void FOrthoCaptureModeModule::OpenWindow()
{
	UE_LOG(LogTemp, Warning, TEXT("OrthoCapture: OpenWindow called"));

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("WindowTitle", "Ortho Capture"))
		.ClientSize(FVector2D(400, 250))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			SNew(SOrthoCapture)
		];

	FSlateApplication::Get().AddWindow(Window);

	UE_LOG(LogTemp, Warning, TEXT("OrthoCapture: Window opened"));
}

#undef LOCTEXT_NAMESPACE