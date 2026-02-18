// OrthoCapture.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class ACameraActor;

/**
 * Editor tool: Orthographic view setup & capture (1080p JPG).
 */
class SOrthoCapture : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOrthoCapture) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SOrthoCapture();

private:
	// --- State ---
	bool bIsSetupMode = false;
	bool bIsCleanedUp = false;
	float OrthographicSize = 10.f;
	float CameraHeight = 50.f;

	TWeakObjectPtr<ACameraActor> TempCamActor;

	// --- UI builders ---
	TSharedRef<SWidget> BuildStartUI();
	TSharedRef<SWidget> BuildSetupUI();
	void RebuildUI();

	// --- Actions ---
	void OnStartSetup();
	void UpdateCamera();
	void CaptureAndExit();
	void Cleanup();

	// --- Slate callbacks ---
	void OnZoomChanged(float NewValue);
	void OnAltitudeChanged(float NewValue);
};

/**
 * Registers the "Varonia > Ortho View" menu entry.
 */
class FOrthoCaptureModeModule
{
public:
	static void Register();
	static void Unregister();
	static void OpenWindow();

	static FName TabName;

private:
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);
};