#include "S47Main.h"
#include "S47DirectorCam.h"
#include "S47DevWaves.h"
#include "S47CarPaint.h"
#include "S47TripoCreature.h"
#include "S47EyeGlint.h"
#include "S47Sky.h"
#include "S47FreeformInWorldTest.h"
#include "S47FieldRadio.h"
#include "S47Stress.h"
#include "S47Omen.h"
#include "S47GearFlicker.h"
#include "S47Soundscape.h"
#include "S47Animals.h"
#include "S47StationInsects.h"
#include "S47StationDressing.h"
#include "S47StationLook.h"
#include "S47FreeformTest.h"
#include "S47FreeformJourneyTest.h"
#include "S47SaroResumeTest.h"
#include "S47Freeform.h"
#include "S47Thoughts.h"
#include "S47LocationCards.h"
#include "S47FreeformDirector.h"
#include "S47HighwayUfo.h"
#include "S47HighwayUfoTest.h"
#include "S47UfoVisualTest.h"
#include "S47Horror.h"
#include "S47Observing.h"
#include "S47Spine.h"
#include "S47DevEvents.h"
#include "S47Rhythm.h"
#include "S47HorrorTest.h"
#include "S47Evening.h"
#include "S47Substation.h"
#include "S47Office.h"
#include "S47RoomLights.h"
#include "S47SaroInterior.h"
#include "S47T02Test.h"
#include "S47GraphicsTest.h"
#include "S47SkyVisitors.h"
#include "S47LandPlaceFixes.h"
#include "S47DaleDocumentsTest.h"
#include "S47Anomaly.h"
#include "S47AnomalyWorld.h"
#include "S47AnomalyTest.h"
#include "S47Chapter5.h"
#include "S47Chapter6.h"
#include "SS47Widgets.h"
#include "S47TownLayer.h"
#include "S47MotelTv.h"
#include "S47Seen.h"
#include "S47Web.h"
#include "S47StoryDrive.h"
#include "Misc/Parse.h"
#include "S47TripoNpcs.h"
#include "S47Lived.h"
#include "S47FarmLights.h"
#include "S47SaroRooms.h"
#include "S47SaroLegacyCleanup.h"
#include "S47Rx3Detail.h"
#include "S47SaroPeriodProps.h"
#include "S47PeriodFixtures.h"
#include "S47SaroFixes.h"
#include "S47Lookdev.h"
#include "S47FinalePreview.h"
#include "S47FinaleMovie.h"

#include "S47Flow.h"
#include "S47StoryDirector.h"
#include "S47Radio.h"
#include "S47NightLine.h"
#include "S47TripRadio.h"
#include "S47Score.h"
#include "S47Sideband.h"
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

bool US47Main::OneAtATime() const { if (HighwayUfo && HighwayUfo->Busy()) return true; if (IsFreeform()) return (Freeform && Freeform->Director && Freeform->Director->Busy()) || FS47StoryDirector::Holds(this) || S47DirectorCam::On(this) || FS47DevWaves::On(this); /* STORY-ON-FREEFORM-01 (step 4): a Story night's script; false in any other night. DIRECTOR-MODE-01: the director's camera */ return (Spine && Spine->Protected()) || (Horror && Horror->Busy()) || (Evening && Evening->MealBusy()) || (Night && Night->OneAtATime()) || (AnomalyWorld && AnomalyWorld->Busy()); }

void US47Main::RequestAutosave(bool bMilestone)
{
	if (Flow) Flow->RequestAutosave(bMilestone);
}

void US47Main::Step(double Dt)
{
	T += Dt;
	if (Mode == EMode::Play) Playtime += Dt;
	if (Flow) Flow->Step(Dt);
	if (Audio && Audio->TitleTextureOn())
	{
		if (Mode != EMode::Title) Audio->TitleTexture(false);
		else if (Audio->Ctx) Audio->TitleTextureFrame(Audio->Ctx->CurrentTime());
	}
	const bool bFree = IsFreeform();
	if (Freeform && bFree) Freeform->Step(Dt);
	S47LandPlaceFixes::Update(this, Dt); // LAND-PLACES-FIXES-20261114: the shack's and the cairn's walk boxes and truck obstacles, where they stand
	S47Thoughts::Step(this, Dt); S47LocationCards::Step(this, Dt); // UI-20261112 (Tom): the thoughts that wait and 02:00; the location card of a place just come to
	if (bFree && StoryDirector && Flow && Flow->StoryNight() && StoryDirector->Active()) StoryDirector->Tick(Dt); // STORY-ON-FREEFORM-01: nothing outside a Story night
	if (!bFree && Evening) Evening->Step(Dt);
	if (Office) Office->Step(Dt); // Doors, drawers and room visibility also update in Freeform.
	if (!bFree && Observing) Observing->Step(Dt);
	if (!bFree && Horror) Horror->Step(Dt);
	if (!bFree && Spine) Spine->Step(Dt);
	if (World) World->Update(Dt, T); // Parked native vehicle choices also exist before any road leg is built.
	if (HighwayUfo) HighwayUfo->Step(Dt);
	if (HighwayUfoTest) HighwayUfoTest->Step(Dt);
	if (UfoVisualTest) UfoVisualTest->Step(Dt);
	if (DevEvents) { DevEvents->Step(Dt); DevEvents->ControlTest(Dt); }
	S47DirectorCam::Tick(this, Dt); // DIRECTOR-MODE-01: the director's free camera (its keys, its text, leaving by itself)
	if (Rhythm) Rhythm->Observe(Dt);
	if (Spine) Spine->ControlTest(Dt);
	if (HorrorTest) HorrorTest->Step(Dt);
	if (FreeformInWorldTest) FreeformInWorldTest->Step(Dt);
	if (FreeformTest) FreeformTest->Step(Dt);
	if (FreeformJourneyTest) FreeformJourneyTest->Step(Dt);
	if (SaroResumeTest) SaroResumeTest->Step(Dt);
	if (Substation)
	{
		if (bFree) Substation->StepVisuals(Dt); // Shared site feedback, without advancing authored T01/T02.
		else Substation->Step(Dt);
	}
 // Opt-in live contract tests must also observe legacy restores with Evening disabled.
 if (Evening) Evening->ControlServiceTest(Dt);
 if (Evening) Evening->ControlServiceDriveTest(Dt);
	if (!bFree && Mode == EMode::Play && Anomaly) Anomaly->Step(Dt);
	if (!bFree && Mode == EMode::Play && AnomalyWorld) AnomalyWorld->Step(Dt);
	const bool bOpening = Evening && Evening->Opening();
	// main.ts, after game.update: the walk gap to STATION 01 from chapter three, the old road's barricades
	// open from chapter five's road (World.setStoryPlaces)
	if (bFree && Mode == EMode::Play && World) World->SetStoryPlaces(true, true);
	if (!bFree && Mode == EMode::Play && World && Story)
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
		if (Score && Score->HasAudio()) Score->Stop();
		if (Radio && ((CarRadio && (CarRadio->bOn || CarRadio->Carrying())) || (DinerRadio && DinerRadio->bOn))) Radio->Reset(0);
	}
	if (!Player || !Audio) return;
	const FVector P = Player->Pos;
	// TV-SCHEDULE-20261109 (Tom): the late programme on the motel office's set in both modes, by the station's hours; the test card
	// the rest of the day (and at every hour without the local frame pack). -S47MotelTvFilm or -S47TvOnAir: the film at any hour.
	// TV-CHANNEL-20261110 (Tom: every set on the one channel): the station's day goes on here, once a frame. Freeform: on the air
	// from six until two, then the sign-off in front of whoever is watching, the card and snow (FS47TvProgramme::Advance); the
	// story keeps the late station's hours. Dale's set (S47OfficeTv) and the motel office's read it.
	const double TvClock = bFree && Freeform ? Freeform->Clock() : Story ? Story->Clock() : 0;
	FS47TvProgramme::ChannelStep(TvClock, Mode == EMode::Play ? Dt : 0.0, bFree && Freeform);
	if (TownLayer && TownLayer->MotelTv)
	{
		FS47MotelTv& Set = *TownLayer->MotelTv;
		Set.bChannel = bFree && Freeform; Set.Air = FS47TvProgramme::Air();
		Set.bCardOnly = Set.bChannel ? Set.Air != FS47TvProgramme::EAir::Programme && Set.Air != FS47TvProgramme::EAir::SignOff
			: !(FS47TvProgramme::ForcedOnAir() || FS47TvProgramme::OnAir(TvClock));
	}
	if (!bFree && Mode == EMode::Play && TownLayer) TownLayer->WorldStep(Dt);
	// MOTEL-REAL-20261108: in Freeform the town layer's world step does not run, so the motel office's set never got past the
	// first frame it was painted with. Its picture (the late programme, or the test card and its snow) and its sound go on here;
	// none of the story's television event (bOk false). -S47MotelTvFrozen: as before, for a picture before and after.
	if (bFree && Mode == EMode::Play && TownLayer && TownLayer->MotelTv && Web && !FParse::Param(FCommandLine::Get(), TEXT("S47MotelTvFrozen")))
	{
		FS47MotelTvIn Tv; Tv.bSaro = Web->Area() == TEXT("saro"); Tv.Cam = FS47Cam::Of(Player, 5000); Tv.Clock = Freeform ? Freeform->Clock() : 0;
		Tv.Jam = FMath::Max(Freeform ? double(Freeform->ScreenInterference()) : 0.0, FS47TvProgramme::JamAsked());
		TownLayer->MotelTv->Update(Dt, Tv);
	}
	if (!bFree && AnomalyWorld) AnomalyWorld->DreadStep(Dt);
	// main.ts: the closer look, the lens narrowing while right mouse is held (S47Phones.cpp)
	if (Phones) Phones->LookStep(Dt);
	// M2: what is out there near the player, then what reads it: the gear (before the flashlight's own update, which shows it),
	// the night's sounds, the animals. Each does nothing outside Freeform and gives back what it touched.
	if (!Omen) Omen = MakeShared<FS47Omen>(this);
	if (!Gear) Gear = MakeShared<FS47GearFlicker>(this);
	if (!Soundscape) Soundscape = MakeShared<FS47Soundscape>(this);
	if (!Animals) Animals = MakeShared<FS47Animals>(this);
	Omen->Step(Dt);
	Gear->Step(Dt);
	Soundscape->Step(Dt);
	Animals->Step(Dt);
	// GAS-STATION-LIFT-01: the moths under the gas station's tubes (after the night's sounds: they leave in its hush)
	if (!StationInsects) StationInsects = MakeShared<FS47StationInsects>(this);
	StationInsects->Step(Dt);
	if (!StationDressing) StationDressing = MakeShared<FS47StationDressing>(this);
	StationDressing->Step();
	if (!StationLook) StationLook = MakeShared<FS47StationLook>(this);
	StationLook->Step();
	// main.ts: the flashlight held from the eyes (core/Flashlight.ts)
	if (Night) Night->FlashlightStep(Dt);
	// the player's strain (after the flashlight: a lit lamp is one thing it reads)
	if (!Stress) Stress = MakeShared<FS47Stress>(this);
	Stress->Step(Dt);
	// main.ts radioStep: the car's and the diner's receivers, the radio, the trip's radio
	if (Mode == EMode::Play && !bOpening) RadioStep(Dt);
	// main.ts: the night outside (core/ambience.ts) heard from where the player stands
	if (Mode == EMode::Play) Audio->NightLife(float(ModalOpen() ? 0.0 : Dt), P, Story ? Story->Clock() : 0.0);
	// main.ts: the music by cue (core/score.ts)
	if (Mode == EMode::Play && Score && !bOpening)
	{
		// D45-20261110 (Tom, d45: the town's piece played on while the tall one came at the feed store): in Freeform the set's own
		// reading of what is near picks the cue (FS47FieldRadio::MusicMood: heard, eerie; seen or close, tension; coming, action).
		// Story keeps the music it had: only a body in view asks for anything.
		if (bFree && FieldRadio)
		{
			double MoodUrgency = 0;
			const FString MoodCue = FieldRadio->MusicMood(MoodUrgency);
			Score->RequestMood(MoodCue, MoodUrgency);
		}
		else
		{
			const double Danger = FieldRadio ? FieldRadio->ObservedThreat() : 0;
			Score->RequestMood(Danger >= 1 ? TEXT("action") : Danger > 0 ? TEXT("tension") : FString(), Danger);
		}
		// SIDEBAND-20261113, channels: in Freeform, with "Music reacts" on, an own mix plays as its stems and the tension lets them in and out
		Score->SetStems(bFree && Sideband && Sideband->Reacts == S47Sideband::EMode::On, Sideband ? Sideband->Tension() : 0.);
		Score->Update(bPausedByMenu ? 0 : Dt, Score->MusicWant(), Score->MusicHeld());
		// Critics, 10 Nov: the bought ground under a meeting is the music then (Tom's idea); the score stands back to a third.
		// D45-20261110: in Freeform a place's own piece (town, dawn, the motel, the drive) goes all but under it, and already as
		// something is heard coming, until the cue for it has crossed in.
		// SIDEBAND-20261113: with the colour bus in, the score stands back at the bus's head (the score alone: FLARES' bed and
		// stingers join after it), and the score's own bus keeps the listening duck, which takes all of it.
		const bool bPlacePiece = bFree && Score->PlacePieceSounding();
		const double Dread = FieldRadio ? FieldRadio->DreadNow() : 0., Heard = bFree && FieldRadio ? FieldRadio->Approach() : 0.;
		if (Sideband && Score->Inserted())
		{
			Score->SetDuck(Score->MusicDuck());
			Sideband->SetStandBack(Sideband->StandBackFor(bPlacePiece, Dread, Heard));
		}
		else Score->SetDuck(Score->MusicDuck() * FS47Score::DreadDuck(bPlacePiece, Dread, Heard));
	}
	if (Sideband) Sideband->Step(Mode == EMode::Play && !bPausedByMenu && !bOpening ? Dt : 0.);
	// main.ts: the face's print come up, seenStep, curveStep, then the night's things (S47Night.cpp)
	if (Night && !bOpening) Night->Step(Dt);
	if (Player) S47EyeGlint::Step(Player->EyeUE(), Life && Life->Sky.IsValid() ? Life->Sky->Daylight() : 0.);
	// main.ts: every frame, the dishes' lamps, the sky light, the reel's own timer (S47SaroNight.cpp)
	if (!bFree && SaroNight && !bOpening) SaroNight->Frame(Dt);
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
	if (RoomLights) RoomLights->Step(Dt);
	if (Web && Web->IsLoaded())
	{
		if (!TripoNpcs) TripoNpcs = MakeShared<FS47TripoNpcs>(this);
		TripoNpcs->Step();
		// SHERIFF-CAR (Tom 9 November: the cars in town drawn as grey clay): every car in the town's own materials (S47CarPaint.h).
		S47CarPaint::Step(Web.Get(), Dt);
		FS47SaroRooms::Update(this, Dt);
		S47Lived::Update(this, Dt);
		// FARM-LIGHTS (Tom, 9 November): the barn's swinging bulb, the yard's dusk-to-dawn lamp, the old house's lit room. Freeform only.
		S47FarmLights::Update(this, Dt);
		if (Web && Web->IsLoaded()) S47SaroLegacyCleanup::Build(Web.Get());
		FS47Rx3Detail::Update(this, Dt);
		FS47SaroPeriodProps::Update(this, Dt);
		S47PeriodFixtures::Update(this, Dt);
		// Tom's test of d36 at SARO (PT-RAMP, PT-PATHS, PT-STOP, PT-HALL, PT-DALE-LIGHT, PT-SWITCH; PT-RECORDS in its own file).
		S47SaroFixes::Update(this, Dt);
		S47RecordsDressing::Update(this, Dt);
		S47RecordsRoom::Update(this, Dt); // RECORDS-ROOM-LIFT-20261113: after the dressing it re-houses
		S47Lookdev::Update(this, Dt);
	}
	if (!bFree && SkyVisitors) SkyVisitors->Step(Dt);
	if (DaleDocumentsTest) DaleDocumentsTest->Step(Dt);
	if (AnomalyTest) AnomalyTest->Step(Dt);
	if (T02Test) T02Test->Step(Dt);
	if (GraphicsTest) GraphicsTest->Step(Dt);
	if(FieldRadio) FieldRadio->Step(Dt);
	FS47FinalePreview::Step(this,Dt);
	FS47FinaleMovie::Step(this,Dt);
}

void US47Main::InitRadio()
{
	if (Radio || !Audio || !Audio->Ctx) return;
	Radio = MakeShared<FS47Radio>(Audio);
	CarRadio = Radio->Receiver({0.38, 1.0}); DinerRadio = Radio->Receiver({0.3, 1.6});
	TripRadio = MakeShared<FS47TripRadio>(*Radio, TownSeen);
	Score = MakeShared<FS47Score>(this, Audio->Ctx.ToSharedRef(), Audio->Music);
	// SIDEBAND-20261113: between the score's decks and its bus; nothing else in the game passes it (-S47NoSideband: left out)
	if (!FParse::Param(FCommandLine::Get(), TEXT("S47NoSideband")))
	{
		Sideband = MakeShared<FS47Sideband>(this, Audio->Ctx.ToSharedRef());
		Score->Insert(Sideband->Bus->Head, Sideband->Bus->Out);
		if (Flow) Flow->ApplySideband();
	}
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
	// STORY-DRIVE-OWN-CAR-20261113: on a Story night's drive the wagon's radio is the local evening host's, on or off (Story nights only).
	if (FS47StoryDrive* Wagon = IsFreeform() ? FS47StoryDrive::Of(this) : nullptr; Wagon && Wagon->OwnsRadio())
	{
		if (Mode == EMode::Play && !ModalOpen() && World && World->bDriving) Wagon->ToggleRadio();
		return;
	}
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
	// STORY-DRIVE-OWN-CAR-20261113: while a Story night's wagon owns the dial (its evening host and bed, FS47StoryDrive) the car's receiver
	// is not in the cab: silent, no captions, its station kept for the truck after the drive. Never so outside a Story night.
	const FS47StoryDrive* Wagon = IsFreeform() ? FS47StoryDrive::Of(this) : nullptr;
	const bool bWagonRadio = Wagon && Wagon->OwnsRadio();
	CarRadio->Pos = Tr && !bWagonRadio ? TOptional<FVector>(RadioAt) : TOptional<FVector>();
	CarRadio->Power = (World->bDriving ? World->Trouble.RadioLevel() : 1) * (!World->bDriving && Space == TEXT("room") ? 0.3 : 1);
	CarRadio->Muffle = World->bDriving ? 0 : 1;
	if (Tr) Tr->SetRadio(S47Radio::DialX(bWagonRadio ? double(FS47StoryDrive::Dial) : S47Radio::Freq(CarRadio->Station)), bWagonRadio ? Wagon->RadioOn() : (CarRadio->bOn || CarRadio->Carrying()));
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
	// NIGHT-RADIO-20261109: in Freeform 1420 is Earl's Night Line from eight to four, made for the night (S47NightLine).
	// STORY-DRIVE-OWN-CAR-20261113: on a Story night 1420 is the old local evening host's programme before the Night Line's eight o'clock.
	Radio->bStoryEvening = IsFreeform() && Flow && Flow->StoryNight();
	if (IsFreeform() != bool(Radio->NightLineIn)) Radio->NightLineIn = IsFreeform() ? TFunction<bool(FS47NightLineIn&)>([this](FS47NightLineIn& In) { return S47NightLine::Fill(this, In); }) : TFunction<bool(FS47NightLineIn&)>();
	Radio->Update(Dt, Story ? Story->Clock() : 0);
	if (TripRadio) TripRadio->Update({World->TripNow(), Story ? Story->ChapterKey : FString(), Story ? Story->Clock() : 0, World->bDriving && CarRadio->Live() && !(Story && Story->IsCinematic()) ? CarRadio->Station : FString()});
	bRadioHeard = (CarRadio->Live() && bCarNear) || (DinerRadio->Live() && !World->bDriving && FVector::Distance(Player->Pos, CounterAt) < 12);
	if (!RadioTest.IsEmpty() && !bRadioTestLogged && CarRadio->Live() && Radio->Stations[RadioTest]->Awake())
	{
		bRadioTestLogged = true;
		UE_LOG(LogTemp, Display, TEXT("S47: radio %s live, AM %d, %s, listeners %d"), *RadioTest, S47Radio::Freq(RadioTest), *Radio->Stations[RadioTest]->Seg().Kind, Radio->Stations[RadioTest]->Listeners);
	}
}
