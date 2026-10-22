#include "S47Main.h"

#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Night.h"
#include "S47Phones.h"
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

bool US47Main::OneAtATime() const { return Night && Night->OneAtATime(); }

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
	// main.ts: the closer look, the lens narrowing while right mouse is held (S47Phones.cpp)
	if (Phones) Phones->LookStep(Dt);
	// main.ts: the flashlight held from the eyes (core/Flashlight.ts)
	if (Night) Night->FlashlightStep(Dt);
	// main.ts: the night outside (core/ambience.ts) heard from where the player stands
	if (Mode == EMode::Play) Audio->NightLife(float(ModalOpen() ? 0.0 : Dt), P, Story ? Story->Clock() : 0.0);
	// main.ts: the face's print come up, seenStep, curveStep, then the night's things (S47Night.cpp)
	if (Night) Night->Step(Dt);
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
	// main.ts: the extension has no windows: draw its rooms only from inside, or through the open door
	// (the archive's drawer, box and switchboard and the records room's dust are in them; the exit's
	// leaf open is the door's state here, annex.exitOpen in the web version)
	if (Web)
	{
		bool bIn = false;
		for (const TCHAR* B : { TEXT("annex.zone.southDoor"), TEXT("annex.zone.corridor"), TEXT("annex.zone.recordsDoor"), TEXT("annex.zone.records") }) bIn |= Web->Rect(B).Contains(P);
		bIn |= Web->Rect(TEXT("room.bounds")).Contains(P) && Web->Obj(TEXT("room.southDoorHinge")).Rotation().Y > 0.01;
		bIn |= Web->DoorStates().FindRef(TEXT("exit")) && Web->Rect(TEXT("world.crossing.zone.stoop")).Contains(P);
		bIn |= Web->Rect(TEXT("annex.zone.exitDoor")).Contains(P);
		FS47Obj Interior = Web->Obj(TEXT("annex.interior"));
		Interior.SetVisible(Mode == EMode::Play && (!World || World->Area == TEXT("saro")) && bIn);
	}
	// main.ts: ui.examine, what a closer look says under the crosshair
	if (Phones) Phones->ExamineStep();
}
