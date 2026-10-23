#include "Modules/ModuleManager.h"
#include "S47Startup.h"

class FSignal47Module final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override { S47Startup::Initialize(); }
	virtual void ShutdownModule() override { S47Startup::Shutdown(); }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSignal47Module, Signal47, "Signal47");
