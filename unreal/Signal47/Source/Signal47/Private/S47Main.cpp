#include "S47Main.h"

#include "S47HUD.h"
#include "S47Story.h"
#include "S47UI.h"

namespace
{
	TWeakObjectPtr<US47Main> GMain; // the game in play (one world at a time)
}

US47Main* US47Main::Get(const UObject* WorldContext)
{
	(void)WorldContext;
	if (!GMain.IsValid())
	{
		// made on first use; the HUD fills it in when the game starts
		US47Main* M = NewObject<US47Main>(GetTransientPackage());
		M->AddToRoot();
		GMain = M;
	}
	return GMain.Get();
}

bool US47Main::ModalOpen() const { return UI && UI->Modal(); }

void US47Main::Step(double Dt)
{
	T += Dt;
	if (Mode == EMode::Play) Playtime += Dt;
}
