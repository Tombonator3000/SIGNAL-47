#include "S47Main.h"

#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

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

void US47Main::RequestAutosave(bool bMilestone)
{
	if (Flow) Flow->RequestAutosave(bMilestone);
}

void US47Main::Step(double Dt)
{
	T += Dt;
	if (Mode == EMode::Play) Playtime += Dt;
	if (Flow) Flow->Step(Dt);
	if (!Player || !Audio) return;
	const FVector P = Player->Pos;
	// main.ts: the night outside (core/ambience.ts) heard from where the player stands
	if (Mode == EMode::Play) Audio->NightLife(float(ModalOpen() ? 0.0 : Dt), P, Story ? Story->Clock() : 0.0);
	// main.ts: the acoustic space, from where the player stands
	FString Sp;
	if (World && World->bDriving) Sp = TEXT("lab");
	else if (World && (World->Area == TEXT("station01") || World->Area == TEXT("room6") || World->Area == TEXT("diner"))) Sp = World->Indoors(P) ? TEXT("room") : TEXT("yard");
	else if (Web)
	{
		static const TCHAR* Indoors[] = { TEXT("room.bounds"), TEXT("annex.zone.southDoor"), TEXT("annex.zone.corridor"), TEXT("annex.zone.recordsDoor"), TEXT("annex.zone.records") };
		bool bIn = Web->Rect(TEXT("world.motel.zone.office")).Contains(P);
		for (const TCHAR* B : Indoors) bIn |= Web->Rect(B).Contains(P);
		Sp = bIn ? TEXT("room") : Web->Rect(TEXT("yard.zone.lab")).Contains(P) ? TEXT("lab") : TEXT("yard");
	}
	if (Sp != Space) { Space = Sp; Audio->SetSpace(Sp); }
}
