// Copyright Epic Games, Inc. All Rights Reserved.

#include "VaroniaBackOffice.h"
//#include "OrthoCapture.h"

#define LOCTEXT_NAMESPACE "FVaroniaBackOfficeModule"

void FVaroniaBackOfficeModule::StartupModule()
{
	//FOrthoCaptureModeModule::Register();
}

void FVaroniaBackOfficeModule::ShutdownModule()
{
	//FOrthoCaptureModeModule::Unregister();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVaroniaBackOfficeModule, VaroniaBackOffice)