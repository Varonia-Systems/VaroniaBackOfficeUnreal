// VaroniaBackOfficeEditor.h
#pragma once

#include "Modules/ModuleManager.h"

class FVaroniaBackOfficeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
