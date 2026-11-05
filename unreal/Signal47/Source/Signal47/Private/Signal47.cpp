#include "Modules/ModuleManager.h"
#include "S47Startup.h"
#include "S47CosmicAmbient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

class FSignal47Module final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Explicit offline audio tool, before any map, HUD or user save is loaded.
		FString Path;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47CosmicPreview="), Path))
		{
			const bool Ok = FS47CosmicAmbient::RenderPreview(Path);
			FPlatformMisc::RequestExitWithStatus(false, Ok ? 0 : 1);
			return;
		}
		S47Startup::Initialize();
	}
	virtual void ShutdownModule() override { S47Startup::Shutdown(); }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSignal47Module, Signal47, "Signal47");
