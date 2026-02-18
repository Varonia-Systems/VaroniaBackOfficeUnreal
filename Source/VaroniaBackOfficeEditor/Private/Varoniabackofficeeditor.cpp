// VaroniaBackOfficeEditor.cpp
#include "VaroniaBackOfficeEditor.h"
#include "OrthoCapture.h"

void FVaroniaBackOfficeEditorModule::StartupModule()
{
	FOrthoCaptureModeModule::Register();
}

void FVaroniaBackOfficeEditorModule::ShutdownModule()
{
	FOrthoCaptureModeModule::Unregister();
}

IMPLEMENT_MODULE(FVaroniaBackOfficeEditorModule, VaroniaBackOfficeEditor)
