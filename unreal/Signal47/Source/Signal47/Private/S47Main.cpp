#include "S47Main.h"
#include "S47Horror.h"
#include "S47Observing.h"
#include "S47Spine.h"
#include "S47DevEvents.h"
#include "S47HorrorTest.h"
#include "S47Evening.h"
#include "S47Substation.h"
#include "S47Office.h"
#include "S47SaroInterior.h"
#include "S47T02Test.h"
#include "S47GraphicsTest.h"
#include "S47SkyVisitors.h"
#include "S47DaleDocumentsTest.h"
#include "S47Anomaly.h"
#include "S47AnomalyWorld.h"
#include "S47AnomalyTest.h"
#include "S47Chapter5.h"
#include "S47Chapter6.h"
#include "SS47Widgets.h"
#include "S47TownLayer.h"
#include "S47TripoNpcs.h"

#include "S47Flow.h"
#include "S47Radio.h"
#include "S47TripRadio.h"
#include "S47Score.h"
#include "S47Three.h"
#include "Misc/CommandLine.h"
#include "S47HUD.h"
#include "S47Night.h"
#include "S47Phones.h"
#include "S47SaroNight.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47WorldLife.h"

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

bool US47Main::OneAtATime() const { return (Horror && Horror->Busy()) || (Evening && Evening->MealBusy()) || (Night && Night->OneAtATime()) || (AnomalyWorld && AnomalyWorld->Busy()); }

void US47Main::RequestAutosave(bool bMilestone)
{
	if (Flow) Flow->RequestAutosave(bMilestone);
}

void US47Main::Step(double Dt)
{
	T += Dt;
	if (Mode == EMode::Play) Playtime += Dt;
	if (Flow) Flow->Step(Dt);
	if (Evening) Evening->Step(Dt);
	if (Office) Office->Step(Dt);
	if (Observing) Observing->Step(Dt);
	if (Horror) Horror->Step(Dt);
	if (Spine) Spine->Step(Dt);
	if (DevEvents) { DevEvents->Step(Dt); DevEvents->ControlTest(Dt); }
	if (Spine) Spine->ControlTest(Dt);
	if (HorrorTest) HorrorTest->Step(Dt);
	if (Substation) Substation->Step(Dt);
 // Opt-in live contract tests must also observe legacy restores with Evening disabled.
 if (Evening) Evening->ControlServiceTest(Dt);
 if (Evening) Evening->ControlServiceDriveTest(Dt);
	if (Mode == EMode::Play && Anomaly) Anomaly->Step(Dt);
	if (Mode == EMode::Play && AnomalyWorld) AnomalyWorld->Step(Dt);
	const bool bOpening = Evening && Evening->Opening();
	// main.ts, after game.update: the walk gap to STATION 01 from chapter three, the old road's barricades
	// open from chapter five's road (World.setStoryPlaces)
	if (Mode == EMode::Play && World && Story)
	{
		const US47Chapter5* C5 = Cast<US47Chapter5>(Story->Chapter(TEXT("ch5")));
		const bool bRoad = C5 && C5->bActive && (C5->S.Stage == TEXT("road") || C5->S.Stage == TEXT("complete"));
		// (game.at: the chapter the story is in, by its key)
		auto At = [this](const TCHAR* Ch) { const FString K = Story->ChapterKey; return K.StartsWith(TEXT("ch")) && FCString::Atoi(*K.Mid(2)) >= FCString::Atoi(Ch + 2); };
		World->SetStoryPlaces(At(TEXT("ch3")), At(TEXT("ch6")) || bRoad);
	}
	if (Mode == EMode::End)
	{
		if (TownLayer) TownLayer->Leave(true);
		if (Score && !Score->Playing.IsEmpty()) Score->Stop();
		if (Radio && ((CarRadio && (CarRadio->bOn || CarRadio->Carrying())) || (DinerRadio && DinerRadio->bOn))) Radio->Reset(0);
	}
	if (!Player || !Audio) return;
	const FVector P = Player->Pos;
	if (Mode == EMode::Play && TownLayer) TownLayer->WorldStep(Dt);
	if (AnomalyWorld) AnomalyWorld->DreadStep(Dt);
	// main.ts: the closer look, the lens narrowing while right mouse is held (S47Phones.cpp)
	if (Phones) Phones->LookStep(Dt);
	// main.ts: the flashlight held from the eyes (core/Flashlight.ts)
	if (Night) Night->FlashlightStep(Dt);
	// main.ts radioStep: the car's and the diner's receivers, the radio, the trip's radio
	if (Mode == EMode::Play && !bOpening) RadioStep(Dt);
	// main.ts: the night outside (core/ambience.ts) heard from where the player stands
	if (Mode == EMode::Play) Audio->NightLife(float(ModalOpen() ? 0.0 : Dt), P, Story ? Story->Clock() : 0.0);
	// main.ts: the music by cue (core/score.ts)
	if (Mode == EMode::Play && Score && !bOpening) Score->Update(Dt, Score->MusicWant(), Score->MusicHeld());
	// main.ts: the face's print come up, seenStep, curveStep, then the night's things (S47Night.cpp)
	if (Night && !bOpening) Night->Step(Dt);
	// main.ts: every frame, the dishes' lamps, the sky light, the reel's own timer (S47SaroNight.cpp)
	if (SaroNight && !bOpening) SaroNight->Frame(Dt);
	// main.ts: html.at-line during THE EVENT (game.ch6.stage === 'event'): the prompt sits lower
	{
		const US47Chapter6* C6 = Story ? Cast<US47Chapter6>(Story->Chapter(TEXT("ch6"))) : nullptr;
		const bool bLine = Mode == EMode::Play && C6 && C6->Stage == TEXT("event");
		if (SS47Hud* H = Hud ? Hud->HudWidget() : nullptr; H && H->bAtLine != bLine) H->bAtLine = bLine;
	}
	// main.ts: the acoustic space, from where the player stands
	FString Sp;
	if (World && World->bDriving) Sp = TEXT("lab");
	else if (World && (World->Area == TEXT("station01") || World->Area == TEXT("room6") || World->Area == TEXT("diner"))) Sp = World->Indoors(P) ? TEXT("room") : TEXT("yard");
	else if (Web)
	{
		static const TCHAR* Indoors[] = { TEXT("room.bounds"), TEXT("annex.zone.southDoor"), TEXT("annex.zone.corridor"), TEXT("annex.zone.recordsDoor"), TEXT("annex.zone.records") };
		bool bIn = Web->Rect(TEXT("world.motel.zones[2]")).Contains(P);
		for (const TCHAR* B : Indoors) bIn |= Web->Rect(B).Contains(P);
		bIn |= Office && Office->Contains(P);
		bIn |= S47SaroInterior::ContainsPassingBay(Web, P);
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
		bIn |= Office && Office->Contains(P);
		bIn |= S47SaroInterior::ContainsPassingBay(Web, P);
		bIn |= Web->Rect(TEXT("room.bounds")).Contains(P) && Web->Obj(TEXT("room.southDoorHinge")).Rotation().Y > 0.01;
		bIn |= Web->DoorStates().FindRef(TEXT("exit")) && Web->Rect(TEXT("world.crossing.zone.stoop")).Contains(P);
		bIn |= Web->Rect(TEXT("annex.zone.exitDoor")).Contains(P);
		FS47Obj Interior = Web->Obj(TEXT("annex.interior"));
		Interior.SetVisible(Mode == EMode::Play && (!World || World->Area == TEXT("saro")) && bIn);
	}
	// main.ts: ui.examine, what a closer look says under the crosshair
	if (Phones) Phones->ExamineStep();
	// main.ts: the sky, the lamps, the shared lights, the fog's colour, the painted horizon, the areas'
	// glows and windows (and World.update's light in the air), the ground cover before the picture
	if (Life) Life->Step(Dt);
	if (Web && Web->IsLoaded())
	{
		if (!TripoNpcs) TripoNpcs = MakeShared<FS47TripoNpcs>(this);
		TripoNpcs->Step();
	}
	if (SkyVisitors) SkyVisitors->Step(Dt);
	if (DaleDocumentsTest) DaleDocumentsTest->Step(Dt);
	if (AnomalyTest) AnomalyTest->Step(Dt);
	if (T02Test) T02Test->Step(Dt);
	if (GraphicsTest) GraphicsTest->Step(Dt);
}

void US47Main::InitRadio()
{
	if (Radio || !Audio || !Audio->Ctx) return;
	Radio = MakeShared<FS47Radio>(Audio);
	CarRadio = Radio->Receiver({0.38, 1.0}); DinerRadio = Radio->Receiver({0.3, 1.6});
	TripRadio = MakeShared<FS47TripRadio>(*Radio, TownSeen);
	Score = MakeShared<FS47Score>(this, Audio->Ctx.ToSharedRef(), Audio->Music);
	if (World) World->OnRadio = [this] { TurnRadio(); };
	FParse::Value(FCommandLine::Get(), TEXT("S47Radio="), RadioTest);
	if (!S47Radio::DIAL.Contains(RadioTest)) RadioTest.Empty();
	Radio->OnLine = [this](const FString& Id, const FString& Text, const FString& By, TOptional<double> Duration)
	{
		const bool bCar = CarRadio->Live() && !CarRadio->Carrying() && CarRadio->Station == Id && bCarNear;
		const bool bDiner = DinerRadio->Live() && DinerRadio->Power > 0.5 && DinerRadio->Station == Id && World && !World->bDriving && Player && FVector::Distance(Player->Pos, CounterAt) < 12;
		if (Mode != EMode::Play || (Story && Story->IsCinematic()) || !(bCar || bDiner)) return;
		if (Radio->Stations[Id]->Strength(Radio->Clock, Radio->Time) < 0.35) return;
		const FString Caption = By.IsEmpty() ? FString::Printf(TEXT("RADIO: \"%s\""), *Text) : FString::Printf(TEXT("RADIO, %s: \"%s\""), *By, *Text);
		if (UI) UI->Toast(Caption, Duration.IsSet() ? FMath::Max(2.4, *Duration + 0.2) : FMath::Min(8., 2.2 + Text.Len() / 22.), [Ctx = Audio->Ctx] { return Ctx->CurrentTime(); });
		if (By == TEXT("a boy") && !TownSeen.Contains(TEXT("ev:boy")))
		{
			TownSeen.Add(TEXT("ev:boy")); TownSeen.Add(FString::Printf(TEXT("mark:boy@%d"), FMath::RoundToInt(Story ? Story->Clock() : Radio->Clock)));
			if (Story) Story->Note(Story->ClockText(false) + TEXT(". Night Line, 1420. A boy calling about a star with a tail, low in the south, and a light over the mesa. From his father's ranch. He said it was July."));
			RequestAutosave(false);
		}
	};
}
void US47Main::TurnRadio()
{
	if (Evening && Evening->Commute()) { Evening->Use(TEXT("evening:radio")); return; }
	if (Mode != EMode::Play || ModalOpen() || !World || !World->bDriving || (Story && Story->IsCinematic())) return;
	if (!World->TruckHere()) { if (UI) UI->Toast(TEXT("The radio stays on the Roswell station."), 2.5f); return; }
	if (!CarRadio) return;
	const FString St = CarRadio->Next(); FS47Sound O; O.Gain = 0.5; O.Rate = 0.8; Audio->Sfx(TEXT("click"), O);
	if (UI) UI->Toast(St.IsEmpty() ? TEXT("Radio off") : FString::Printf(TEXT("AM %d"), S47Radio::Freq(St)), 1.6f);
}
void US47Main::ResetRadio(double Clock)
{
	if (!Radio) return;
	Radio->bEveningTimeline = Story && Story->bEveningTimeline;
	Radio->bBoyCalled = TownSeen.Contains(TEXT("ev:boy")) || TownSeen.Contains(TEXT("ln:reel")); Radio->Reset(Clock); bRadioHeard = false;
	// TripRadio's trip/last refer to the previous night's stations, not to the new case.
	TripRadio = MakeShared<FS47TripRadio>(*Radio, TownSeen);
}
void US47Main::RadioStep(double Dt)
{
	if (!Radio || !World || !Player) return;
	FS47Truck* Tr = World->TruckHere();
	if (Tr) { RadioAt = Tr->Group.WorldPosition(); RadioAt.Y += 1.05; }
	CarRadio->Pos = Tr ? TOptional<FVector>(RadioAt) : TOptional<FVector>();
	CarRadio->Power = (World->bDriving ? World->Trouble.RadioLevel() : 1) * (!World->bDriving && Space == TEXT("room") ? 0.3 : 1);
	CarRadio->Muffle = World->bDriving ? 0 : 1;
	if (Tr) Tr->SetRadio(S47Radio::DialX(S47Radio::Freq(CarRadio->Station)), CarRadio->bOn || CarRadio->Carrying());
	bCarNear = Tr && (World->bDriving || FVector::Distance(Player->Pos, RadioAt) < 7);
	const FS47Obj Counter = Web ? Web->Obj(TEXT("world.diner.proxies.counter")) : FS47Obj(); bool bDinerOn = false;
	if (Counter && (World->Area == TEXT("diner") || World->Area == TEXT("road")))
	{
		CounterAt = Counter.WorldPosition();
		const bool bNear = FVector::Distance(World->bDriving ? S47Three::ToThree(Player->EyeUE()) : Player->Pos, CounterAt) < 60;
		const FString St = Radio->Stations[TEXT("caliche")]->OnAir() ? TEXT("caliche") : Radio->Stations[TEXT("roswell")]->OnAir() ? TEXT("roswell") : FString();
		if (bNear && !St.IsEmpty()) { bDinerOn = true; DinerRadio->Set(true, St); }
	}
	if (!bDinerOn && DinerRadio->bOn) DinerRadio->Set(false);
	DinerRadio->Pos = bDinerOn ? TOptional<FVector>(CounterAt) : TOptional<FVector>();
	// A test receiver uses the same graph, positioned by the camera, without starting a trip.
	if (!RadioTest.IsEmpty()) { CarRadio->Set(true, RadioTest); CarRadio->Pos = S47Three::ToThree(Player->EyeUE()); CarRadio->Power = 1; CarRadio->Muffle = 0; bCarNear = true; }
	Radio->Update(Dt, Story ? Story->Clock() : 0);
	if (TripRadio) TripRadio->Update({World->TripNow(), Story ? Story->ChapterKey : FString(), Story ? Story->Clock() : 0, World->bDriving && CarRadio->Live() && !(Story && Story->IsCinematic()) ? CarRadio->Station : FString()});
	bRadioHeard = (CarRadio->Live() && bCarNear) || (DinerRadio->Live() && !World->bDriving && FVector::Distance(Player->Pos, CounterAt) < 12);
	if (!RadioTest.IsEmpty() && !bRadioTestLogged && CarRadio->Live() && Radio->Stations[RadioTest]->Awake())
	{
		bRadioTestLogged = true;
		UE_LOG(LogTemp, Display, TEXT("S47: radio %s live, AM %d, %s, listeners %d"), *RadioTest, S47Radio::Freq(RadioTest), *Radio->Stations[RadioTest]->Seg().Kind, Radio->Stations[RadioTest]->Listeners);
	}
}
