// main.ts: the time away and the trucks (abduction, truckTaken, truckOnB, lostNights, theThird, starHole,
// flashFrames), in the order main.ts builds and steps them
#include "S47Away.h"
#include "S47Keys.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/CommandLine.h"
#include "S47Album.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47Chapter1.h"
#include "S47Chapter6.h"
#include "S47Curve.h"
#include "S47FieldCamera.h"
#include "S47Flashlight.h"
#include "S47FlashFrames.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47HUD.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Legs.h"
#include "S47Lines.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Phones.h"
#include "S47Radio.h"
#include "S47RoadTerrain.h"
#include "S47Score.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47AwayOverlay.h"
#include "SS47Widgets.h"
#include "TextureResource.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Away, Log, All);

namespace
{
	using EMode = US47Main::EMode;
	int32 ChapterNo(const FString& Key) { return Key.StartsWith(TEXT("ch")) ? FCString::Atoi(*Key.Mid(2)) : 0; }
	bool Is(const TFunction<bool()>& F) { return F && F(); }
	double Smooth(double X, double Lo, double Hi) { if (X <= Lo) return 0; if (X >= Hi) return 1; const double T = (X - Lo) / (Hi - Lo); return T * T * (3 - 2 * T); }
	FVector ROAD_ORIGIN() { return S47Legs::ORIGINS(TEXT("road")); }
	FVector STATION_AT() { return S47Legs::ORIGINS(TEXT("station01")); }
	const FString NotRightNow = TEXT("Not right now. Let the moment finish first.");

	// world/Crossing.ts and world/Grounds.ts, SARO's ground (as S47Flow reads them for the place names)
	struct FRectT { double MinX, MaxX, MinZ, MaxZ; };
	const FRectT MOTEL_RECT{ -50 - 2.5, -27.7 - 2.5, 1.5 + 650, 70.8 + 650 };
	const double WEST_Z1 = 64, SHOULDER_X0 = -33.5, SHOULDER_X1 = -15, SHOULDER_Z1 = MOTEL_RECT.MaxZ + 4;
	bool Outside(const FVector& P) { return P.X < -6.3 && P.X > MOTEL_RECT.MinX && P.Z > -5 && P.Z < SHOULDER_Z1; }
	bool AtMotel(const FVector& P) { return P.X >= MOTEL_RECT.MinX && P.X <= MOTEL_RECT.MaxX + 1 && P.Z >= MOTEL_RECT.MinZ - 4 && P.Z <= MOTEL_RECT.MaxZ + 4; }
	bool OnHighway(const FVector& P) { return P.Z > WEST_Z1 - 20 && P.X > SHOULDER_X0 && P.X < SHOULDER_X1 && !AtMotel(P); }
	bool OnRoad(const FVector& P)
	{
		const double ROAD_W = 5.0, Cx = 5.8, Cz = 20.5, R = 4.0, ROAD_Z = Cz + R, FOOT = 16.6;
		if (P.Z > ROAD_Z - ROAD_W / 2 && P.Z < ROAD_Z + ROAD_W / 2 && P.X < Cx && P.X > -20) return true;
		if (P.X > 9.8 - ROAD_W / 2 && P.X < 9.8 + ROAD_W / 2 && P.Z > FOOT && P.Z < Cz) return true;
		const double D = FMath::Sqrt(FMath::Square(P.X - Cx) + FMath::Square(P.Z - Cz));
		return P.X >= Cx && P.Z >= Cz && D > R - ROAD_W / 2 && D < R + ROAD_W / 2;
	}
	/** world/arrayLayout.ts FENCE_Z. */
	const double FENCE_Z = -20;
	/** story/stationTaken.ts: the player is this far out of the hut door before it begins; the last clock it may begin at. */
	const double OUT_OF_DOOR = 2.4, STATION_LAST = 4 * 3600 + 35 * 60 - (17 * 60 + 60 + 47 + 3);
	/** main.ts FIXED: the story's next fixed point, by chapter. */
	TOptional<double> Fixed(const FString& Phase, double EventClock)
	{
		if (Phase == TEXT("ch1")) return EventClock + 40 * 60;
		if (Phase == TEXT("ch2")) return 3 * 3600 + 40 * 60.0;
		if (Phase == TEXT("ch3")) return 4 * 3600 + 35 * 60.0;
		if (Phase == TEXT("ch4")) return 5 * 3600.0;
		return {};
	}
}

// ---------- boot ----------

void US47Away::Init(US47Main* InM)
{
	M = InM;
	AS47Web* W = M->Web;
	TWeakObjectPtr<US47Away> Self = this;
	if (!W || !M->Story || !M->Night || !M->World) { UE_LOG(LogS47Away, Warning, TEXT("S47: the time away has no world")); return; }

	// ---------- the time away (story/abduction.ts): a frame of the scene from a camera of its own, drawn at once ----------
	FS47AbductDeps A;
	A.Audio = M->Audio; A.Player = M->Player; A.Web = W;
	A.Screen.Light = [Self](TOptional<double> K) { if (!Self.IsValid()) return; Self->EnsureOverlay(); if (Self->Overlay) Self->Overlay->SetLight(K); };
	A.Screen.Show = [Self](UTexture2D* T, double Alpha) { if (Self.IsValid()) Self->ShowFrame(T, Alpha); };
	A.Screen.Hide = [Self] { if (Self.IsValid()) Self->HideFrame(); };
	A.Screen.Held = [Self](bool bOn)
	{
		// while one of them is over you, nothing on the screen but the night (the words, the buttons come
		// back with the crickets): html.abduct-held .hud { visibility: hidden }
		if (!Self.IsValid() || !Self->M->Hud || !Self->M->Hud->HudWidget()) return;
		Self->bHeldHud = bOn;
		Self->M->Hud->HudWidget()->SetVisibility(bOn ? EVisibility::Hidden : EVisibility::HitTestInvisible);
	};
	A.Shoot = [Self](const FS47ShotCam& C, TArray<FColor>& Out) { return Self.IsValid() && Self->Shoot(C, 640, 400, Out, false); };
	// what would be behind them on waking: small, before the tape, with the open sky in one flat colour
	// (magenta) so it can be told from anything standing against it (the bottom row first, as WebGL reads)
	A.Probe = [Self](const FS47ShotCam& C, int32 Wd, int32 Ht, TArray<uint8>& Px)
	{
		TArray<FColor> Top;
		if (!Self.IsValid() || !Self->Shoot(C, Wd, Ht, Top, true)) return false;
		Px.SetNumUninitialized(Wd * Ht * 4);
		for (int32 Y = 0; Y < Ht; ++Y) for (int32 X = 0; X < Wd; ++X)
		{
			const FColor& S = Top[(Ht - 1 - Y) * Wd + X];
			uint8* D = &Px[(Y * Wd + X) * 4];
			D[0] = S.R; D[1] = S.G; D[2] = S.B; D[3] = 255;
		}
		return true;
	};
	A.Ground = [Self](double X, double Z) { return Self.IsValid() ? Self->Ground(X, Z) : 0.0; };
	A.Standable = [Self](double X, double Z) { return Self.IsValid() && Self->Standable(X, Z); };
	// the time away in the truck (story/truckTaken.ts): the truck, its cab and the head's look in it
	FS47AbductCab Cab;
	Cab.InCab = [Self] { return Self.IsValid() && Self->Driving(); };
	Cab.Group = [Self]() -> FS47Truck* { if (!Self.IsValid()) return nullptr; US47World* Wd = Self->M->World; FS47Truck* T = Wd->CabTruck(); return T ? T : Wd->Area == TEXT("saro") ? Wd->SaroTruck() : nullptr; };
	Cab.Look = [Self](double Yaw, double Pitch) { if (FS47DriveController* D = Self.IsValid() ? Self->M->World->CabDrive() : nullptr) D->SetLook(Yaw, Pitch); };
	Cab.LookNow = [Self]() -> TOptional<FVector2D> { FS47DriveController* D = Self.IsValid() ? Self->M->World->CabDrive() : nullptr; return D ? TOptional<FVector2D>(D->Look()) : TOptional<FVector2D>(); };
	Cab.LookBox = [Self](TOptional<FS47LookBox> B)
	{
		FS47DriveController* D = Self.IsValid() ? Self->M->World->CabDrive() : nullptr;
		if (!D) return;
		if (B) D->LookBox = FS47DriveController::FLookBox{ B->Yaw0, B->Yaw1, B->Pitch0, B->Pitch1 }; else D->LookBox.Reset();
	};
	Cab.Plan = [Self] { return Self.IsValid() ? Self->TruckTaken->Planned() : TOptional<FVector>(); };
	Cab.Wake = [Self](double Lost, bool bInCab) { return Self.IsValid() ? Self->TruckTaken->Wake(Lost, bInCab) : TOptional<FVector>(); };
	Cab.Woke = [Self](const FString& Fa, const FString& Fb, bool bInCab) { if (Self.IsValid()) Self->TruckTaken->Woke(Fa, Fb, bInCab); };
	Cab.BackText = [Self] { return Self.IsValid() ? Self->TruckTaken->BackText() : FString(); };
	Cab.Light = [Self](double K) { if (FS47Truck* T = Self.IsValid() ? Self->M->World->CabTruck() : nullptr) T->LightFromAbove(K); };
	A.Cab = Cab;
	// on the highway south of SARO: in the grass on the west verge, not on the road
	A.Settle = [Self](FVector& Q) { if (Self.IsValid() && Self->Area() == TEXT("saro") && OnHighway(Q)) Q.X = -30.6; };
	A.Flashlight = M->Night->Flashlight.Get();
	A.Hush = [Self](double Secs, double Level, TOptional<double> Tau, TOptional<double> Back)
	{
		const TSharedPtr<FS47Ambience> N = Self.IsValid() && Self->M->Audio ? Self->M->Audio->Night : nullptr;
		if (!N) return;
		if (Tau && Back) N->Hush(Secs, Level, *Tau, *Back); else if (Tau) N->Hush(Secs, Level, *Tau); else N->Hush(Secs, Level);
	};
	// (vhs.glitch: the tape's tracking fault, core/vhs.ts, is not in M_WebTone; it runs only while the
	// frames cover the screen, where it is not seen)
	A.Glitch = [](double) {};
	A.SkipClock = [Self](double Sec) { if (Self.IsValid()) Self->M->Story->SetClock(Self->M->Story->Clock() + Sec); };
	A.Note = [Self](const FString& T) { if (Self.IsValid()) Self->M->Story->Note(T); };
	A.Toast = [Self](const FString& T, float S) { if (Self.IsValid()) Self->M->UI->Toast(T, S); };
	A.Camera = [Self] { return Self.IsValid() && Self->M->FieldCamera && Self->M->FieldCamera->bHave; };
	A.File = [Self](const FS47Doc& D) { if (Self.IsValid()) Self->M->Story->AddDoc(D); };
	A.KeyStall = [Self] { if (Self.IsValid()) Self->M->World->bKeyStall = true; };
	A.Save = [Self] { if (Self.IsValid() && Self->M->Mode == EMode::Play) Self->RequestAutosave(); };
	A.Shine = [Self](const FVector& At) { return Self.IsValid() ? Self->BandShine(At) : 0.0; };
	A.Eye = [Self] { return Self.IsValid() ? Self->Camera() : FS47Cam(); };
	Abduction = MakeShared<FS47Abduction>(A);

	// ---------- taken in the truck (story/truckTaken.ts): southbound over B at SARO's turn-off, at the
	// night's peak, the warning comes in the cab; the truck is found 300 m on, nose to the north, the
	// radio on 1090, the odometer 31.4 miles more than at SARO ----------
	FS47TruckTakenDeps T;
	T.Trouble = &M->World->Trouble;
	T.Radio = [Self]() -> FS47Receiver* { return Self.IsValid() && Self->M->CarRadio ? Self->M->CarRadio.Get() : nullptr; };
	T.Audio = M->Audio; T.Seen = &M->TownSeen;
	T.Begin = [Self] { if (Self.IsValid()) Self->Abduction->BeginCab(Self->AbductIn(true)); };
	T.Hold = [Self](double K) { if (Self.IsValid()) Self->M->World->HoldTruck(K); };
	T.Hush = [Self](bool bOn) { if (Self.IsValid()) Self->M->World->HushTruck(bOn); };
	T.Roof = [Self]() -> TOptional<FVector>
	{
		if (!Self.IsValid()) return {};
		const FS47Truck* Tr = Self->M->World->CabTruck() ? Self->M->World->CabTruck() : Self->M->World->TruckHere();
		return Tr ? TOptional<FVector>(Tr->LocalToWorld(FVector(0, 1.85, 0.15))) : TOptional<FVector>();
	};
	T.Truck = [Self]() -> TOptional<FS47TruckTakenDeps::FTruckAt>
	{
		if (!Self.IsValid()) return {};
		US47World* Wd = Self->M->World;
		const FS47Truck* Tr = Wd->CabTruck() ? Wd->CabTruck() : Wd->Area == TEXT("saro") ? Wd->TruckHere() : nullptr;
		if (!Tr) return {};
		FS47TruckTakenDeps::FTruckAt At; At.X = Tr->Position.X; At.Z = Tr->Position.Z; At.H = Tr->Rotation.Y;
		return At;
	};
	T.MoveDriven = [Self](double X, double Z, double H) { return Self.IsValid() && Self->M->World->MoveDriven(X, Z, H); };
	T.MoveParked = [Self](double X, double Z, double H) { return Self.IsValid() && Self->M->World->MoveParked(X, Z, H, US47Night::SaroFloorAt(Self->M->Web, X, Z)); };
	T.Headlights = [Self](bool bOn) { if (FS47Truck* Tr = Self.IsValid() ? Self->M->World->CabTruck() : nullptr) Tr->SetHeadlights(bOn); };
	T.Frost = [Self](double K) { if (!Self.IsValid()) return; FS47Truck* Tr = Self->M->World->CabTruck() ? Self->M->World->CabTruck() : Self->M->World->SaroTruck(); if (Tr) Tr->SetFrost(K); };
	T.Odometer = [Self] { return Self.IsValid() ? Self->M->World->Odometer() : 0.0; };
	T.OdoAtSaro = [Self] { return Self.IsValid() ? Self->M->World->OdoAtSaro : TOptional<double>(); };
	T.AddMiles = [Self](double Mi) { if (Self.IsValid()) Self->M->World->AddMiles(Mi); };
	T.KeyStall = [Self] { if (Self.IsValid()) Self->M->World->bKeyStall = true; };
	T.Ground = [Self](double X, double Z) { return Self.IsValid() ? US47Night::SaroFloorAt(Self->M->Web, X, Z) : 0.0; };
	T.Toast = A.Toast; T.Note = A.Note; T.Save = A.Save;
	TruckTaken = MakeShared<FS47TruckTaken>(T);

	// ---------- four, and not yet seven: the truck on B (story/truckOnB.ts) ----------
	FS47TruckOnBDeps B;
	B.Trouble = &M->World->Trouble; B.Radio = T.Radio; B.Seen = &M->TownSeen; B.Note = A.Note; B.Toast = A.Toast;
	// a relay in the dash with each blink: a smaller K3
	B.Tick = [Self] { if (!Self.IsValid() || !Self->M->Audio) return; US47Audio* Au = Self->M->Audio; FS47Sound O; O.Gain = 0.22f; O.Rate = 1.3f; O.Lp = 2400; Au->Sfx(Au->Or(TEXT("rack_switch"), TEXT("switch")), O); };
	B.Save = A.Save;
	TruckB = MakeShared<FS47TruckOnB>(B);

	// ---------- the nights SARO printed a negative range are the nights Caliche lost time (story/lostNights.ts) ----------
	FS47LostNightsDeps L;
	L.Seen = &M->TownSeen;
	L.Links = [Self]() -> TArray<FString>& { static TArray<FString> None; return Self.IsValid() ? Self->Ln : None; };
	L.Open = [Self] { return Self.IsValid() && Self->At(TEXT("ch2")); };
	L.Binder = [Self] { return Self.IsValid() && Self->M->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("binder"); }); };
	L.Notes = [Self] { return Self.IsValid() ? Self->M->Story->Notes() : TArray<FString>(); };
	L.Note = A.Note;
	L.Clock = [Self] { return Self.IsValid() ? Self->Clock() : 0.0; };
	L.File = A.File; L.Save = A.Save;
	L.Refresh = [Self] { if (Self.IsValid() && Self->M->Board.Refresh) Self->M->Board.Refresh(); };
	L.Tape.Busy = [Self] { return Self.IsValid() && Is(Self->ReelTape.Busy); };
	L.Tape.Play = [Self](TFunction<void(const FS47LostTape::FCue&)> Cue, TFunction<void(bool)> Done)
	{
		if (Self.IsValid() && Self->ReelTape.Play) Self->ReelTape.Play(MoveTemp(Cue), MoveTemp(Done));
		else { UE_LOG(LogS47Away, Warning, TEXT("S47: Earl's deck (core/reelTape.ts) is not hooked to the lost nights")); if (Done) Done(false); }
	};
	L.Tape.Stop = [Self] { if (Self.IsValid() && Self->ReelTape.Stop) Self->ReelTape.Stop(); };
	// one thing at a time: not while the night has something of its own going on
	L.Quiet = [Self]
	{
		if (!Self.IsValid()) return false;
		const US47Night* N = Self->M->Night;
		const FS47NightHooks& H = N->Hooks;
		return !Self->Abduction->Busy() && !(H.TownEventsNow && H.TownEventsNow() != TEXT("none")) && !(N->NightEvents && N->NightEvents->Busy()) && !Is(H.RoofStepsBusy) && !Is(H.RoadsideSilent)
			&& N->FaceQuiet <= 0 && !(Self->M->Phones && Is(Self->M->Phones->CbBusy)) && !(Self->M->Audio && Self->M->Audio->PhonePlaying()) && !Is(H.MotelTvBusy) && !Is(H.LineLightsOn) && !Is(H.ArchiveBusy);
	};
	L.HushVoices = [Self] { if (!Self.IsValid()) return; if (Self->HushVoices) Self->HushVoices(); if (Self->M->Audio) Self->M->Audio->StopPhoneCalls(); };
	L.BoyCalled = [Self] { if (Self.IsValid() && Self->M->Radio) Self->M->Radio->bBoyCalled = true; };
	LostNights = MakeShared<FS47LostNights>(L);

	// ---------- no stars where the dishes point, from 02:15 (story/starHole.ts) ----------
	FS47StarHoleDeps S;
	S.Seen = &M->TownSeen; S.Note = A.Note; S.Toast = A.Toast;
	S.Clock = L.Clock;
	S.Chapter = [Self] { return Self.IsValid() ? Self->Phase() : FString(); };
	S.Saw = [Self] { if (!Self.IsValid()) return; FS47SawOpts O; O.bLookingSet = true; O.Looking = true; O.bNoWatch = true; Self->M->Night->Saw(TEXT("scare"), TEXT("no stars"), O); };
	S.Print = [Self](const FString& Id)
	{
		if (!Self.IsValid() || !Self->M->Night->Album) return FString();
		const FS47AlbumShot* Sh = Self->M->Night->Album->Shots().FindByPredicate([&Id](const FS47AlbumShot& X) { return X.Id == Id; });
		return Sh ? Sh->Url : FString();
	};
	S.Save = A.Save; S.Refresh = L.Refresh;
	StarHole = MakeShared<FS47StarHole>(S);

	// ---------- the third one on FRAME 03 (story/theThird.ts) ----------
	FilmThird = MakeShared<FS47FilmThird>(W);
	{
		// FRAME 03 pegged on the drying line in the photo lab, for a closer look (the zoom)
		FS47Interactable I;
		I.Id = TEXT("lab:print03"); I.Object = W->Obj(TEXT("yard.dryPrints[2]")); I.bLook = true; I.ExamineRange = 2.6;
		I.Label = [] { return FString(); };
		I.Use = [] {};
		I.Examine = [Self]() -> FString
		{
			const US47Chapter* C3 = Self.IsValid() ? Self->M->Story->Chapter(TEXT("ch3")) : nullptr;
			const S47J::FObj St = C3 ? C3->SaveState() : nullptr;
			const S47J::FObj F3 = S47J::Obj(St, TEXT("f3"));
			if (!F3.IsValid() || S47J::Num(St, TEXT("dev")) < 3) return FString();
			const S47J::FObj Tg = S47J::Obj(F3, TEXT("targets"));
			if (!S47J::Has(Tg, TEXT("band"))) return TEXT("FRAME 03 on the drying line: the empty footing, and the post with the triangle and bar on its pile of stones.");
			Self->ThirdNoticed();
			return FString::Printf(TEXT("FRAME 03 on the drying line. The footing, the post on its stones. And out in the dark past the fence%s, a short pale line at chest height. Something out there gave the flash back."), S47J::Has(Tg, TEXT("bandC")) ? TEXT(", along the C stakes") : TEXT(""));
		};
		M->Inter->Add(I);
	}

	// ---------- the night's hooks (S47Night: oneAtATime, the curve, the album), the phones, the music ----------
	FS47NightHooks& H = M->Night->Hooks;
	H.AbductionBusy = [Self] { return Self.IsValid() && Self->Abduction->Busy(); };
	H.AbductionExtra = [Self] { int32 N = 0; if (Self.IsValid()) for (const FS47AbductSpot& Sp : Self->Abduction->Data.Spots) N += Sp.Doc.IsSet() ? 1 : 0; return N; };
	H.TruckBBusy = [Self] { return Self.IsValid() && Self->TruckB->Busy(); };
	H.TruckBPeaking = [Self] { return Self.IsValid() && Self->TruckB->Peaking(); };
	H.TruckTakenBusy = [Self] { return Self.IsValid() && Self->TruckTaken->Busy(); };
	// the patch with no stars in it, on the print (and not while the CB is on the line)
	H.StarHolePhotographed = [Self](const FString& Id, bool bFree) { if (Self.IsValid()) Self->StarHole->Photographed(Id, bFree && !(Self->M->Phones && Is(Self->M->Phones->CbBusy))); };
	if (US47Phones* Ph = M->Phones)
	{
		Ph->AbductionCount = [Self] { return Self.IsValid() ? Self->Abduction->Data.Count : 0; };
		Ph->StarHoleExamine = [Self] { return Self.IsValid() ? Self->StarHole->Examine(Self->HoleIn(false)) : FString(); };
	}

	// ---------- the truck: Use in the cab, Q out of it, the prompt ----------
	US47World* Wd = M->World;
	{
		// main.ts useCurrent, in the truck: held at the wheel on waking, nothing; (chapter six's key); after
		// :40 in the truck's warning there is nothing left to turn it over with
		TFunction<bool()> Before = Wd->OnCabUse;
		Wd->OnCabUse = [Self, Before]
		{
			if (!Self.IsValid()) return Before && Before();
			if (Self->Holds()) return true;
			if (Is(Self->Ch6TurnKey)) return true;
			if (Before && Before()) return true;
			return Self->TruckTaken->UseKey(Self->WarnT());
		};
		// (not out in the middle of the truck's count on B; nor by Use in its warning on B: E is the key
		// there, and Q the way out, so a hand on E as the engine dies does not put the player in the road)
		Wd->CabStay = [Self] { return Self.IsValid() && (Self->TruckB->Racing() || Self->TruckTaken->Warning()); };
		Wd->CabRunOffered = [Self] { return Self.IsValid() && !Self->Holds() && ((Self->TruckTaken->Warning() && Self->M->World->CanGetOut(true)) || Is(Self->Ch6DoorOffered)); };
		Wd->CabHeld = [Self] { return Self.IsValid() && Self->Holds(); };
	}
	// Out of the truck in its warning on B, with the engine dead: Q, while Use turns the key
	M->OnAction(TEXT("alt"), [Self]
	{
		if (!Self.IsValid()) return false;
		US47Away* Aw = Self.Get();
		if (Aw->M->Mode != EMode::Play || Aw->Modal() || Aw->Cinematic() || !Aw->Driving() || Aw->Holds()) return false;
		// (on the line at 05:29: the handle moves, the door does not; story/Chapter6.ts)
		if (Is(Aw->Ch6TryDoor)) return true;
		if (Aw->TruckTaken->Warning() && Aw->M->World->CanGetOut(true)) Aw->M->World->GetOutHere(true);
		return true;
	});
	BindPlayer();
	SyncBooks();
	TestFlags();
}

void US47Away::BeginDestroy()
{
	if (Overlay.IsValid() && bOverlayAdded && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
	Overlay.Reset();
	Abduction.Reset(); FilmThird.Reset();
	Super::BeginDestroy();
}

void US47Away::BindPlayer()
{
	AS47Walker* P = M->Player;
	if (!P) return;
	TWeakObjectPtr<US47Away> Self = this;
	// the player held on waking: only the eyes, up, towards what is over you; at the wheel only the eyes,
	// stiffly, towards what is on the bonnet (main.ts step: the look while abduction.holds)
	TFunction<bool(double, const FVector2D&, const FVector2D&, FS47View&)> Before = P->CameraOverride;
	P->CameraOverride = [Self, Before](double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out)
	{
		US47Away* Aw = Self.Get();
		const bool bHeld = Aw && Aw->M->Mode == EMode::Play && Aw->Holds();
		if (bHeld && Aw->Driving()) return Before ? Before(Dt, FVector2D::ZeroVector, Aw->Modal() ? FVector2D::ZeroVector : Aw->Abduction->CabGaze(LookIn.X, LookIn.Y), Out) : false;
		if (bHeld)
		{
			if (!Aw->Modal()) Aw->Abduction->Gaze(LookIn.X, LookIn.Y);
			const AS47Walker* W = Aw->M->Player;
			Out.Eye = FVector(W->Pos.X, W->FloorY + W->Eye, W->Pos.Z); Out.Yaw = W->Yaw; Out.Pitch = W->Pitch; Out.Roll = 0;
			return true;
		}
		return Before ? Before(Dt, Move, LookIn, Out) : false;
	};
	// the silence takes the footsteps too (main.ts player.onStep)
	TFunction<void()> Step = P->OnStep;
	P->OnStep = [Self, Step]
	{
		if (!Self.IsValid() || Self->Abduction->bSilent) return;
		if (Step) { Step(); return; }
		if (Is(Self->M->Night->Hooks.RoadsideSilent) || !Self->M->Audio) return;
		Self->M->Audio->Step(Self->M->Night->SurfaceAt());
	};
}

void US47Away::EnsureOverlay()
{
	if (bOverlayAdded || !GEngine || !GEngine->GameViewport) return;
	Overlay = SNew(SS47AwayOverlay);
	// (over the HUD at 20, under a panel at 30)
	GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 25);
	bOverlayAdded = true;
}

void US47Away::ShowFrame(UTexture2D* Tx, double Alpha, bool bFadeBlack) { EnsureOverlay(); if (Overlay) Overlay->ShowFrame(Tx, Alpha, bFadeBlack); }
void US47Away::HideFrame() { if (Overlay) Overlay->HideFrame(); }

// ---------- the deps main.ts reads ----------

bool US47Away::Modal() const { return M->ModalOpen(); }
bool US47Away::Cinematic() const { return M->Story && M->Story->IsCinematic(); }
bool US47Away::Driving() const { return M->World && M->World->bDriving; }
FString US47Away::Phase() const { return M->Story ? M->Story->ChapterKey : FString(TEXT("prologue")); }
bool US47Away::At(const FString& Chapter) const { return ChapterNo(Phase()) >= ChapterNo(Chapter); }
double US47Away::Clock() const { return M->Story ? M->Story->Clock() : 0; }
FString US47Away::Area() const { return M->World ? M->World->Area : FString(TEXT("saro")); }
bool US47Away::AbductOn() const { return !M->Flow || M->Flow->bAbductOn; }
bool US47Away::CalmFlash() const { return M->Flow && M->Flow->bCalmFlash; }
FS47Cam US47Away::Camera() const { return FS47Cam::Of(M->Player, Area() == TEXT("roswell") ? 12000 : 5000); }
void US47Away::RequestAutosave() const { M->RequestAutosave(false); }
TOptional<double> US47Away::WarnT() const { const TOptional<FS47Abduction::FWarn> W = Abduction ? Abduction->Warn() : TOptional<FS47Abduction::FWarn>(); return W ? TOptional<double>(W->T) : TOptional<double>(); }

/** The truck in the area shown, the old road's too (World.truckHere has none there). */
FS47Truck* US47Away::TruckNow() const
{
	if (!M->World) return nullptr;
	FS47Truck* T = M->World->TruckHere();
	return T ? T : Area() == TEXT("roswell") ? M->World->OldTruck() : nullptr;
}

/** Where the player is: with the truck when driving (the body stays where they got in), else on foot. */
FVector US47Away::PlayerAt() const
{
	if (Driving()) if (const FS47Truck* T = TruckNow()) return T->Position;
	return M->Player ? M->Player->Pos : FVector::ZeroVector;
}

double US47Away::Ground(double X, double Z) const
{
	// player.floor: the area's ground (world/Grounds.ts floorAt at SARO, the road's surface, the station's)
	const FString A = Area();
	if (A == TEXT("road") || A == TEXT("diner")) { const FVector O = ROAD_ORIGIN(); return S47RoadTerrain::SurfaceY(X - O.X, Z - O.Z) + O.Y; }
	if (A == TEXT("station01")) return S47Legs::StationGroundAt(X, Z);
	if (A == TEXT("saro")) return US47Night::SaroFloorAt(M->Web, X, Z);
	return M->Player ? M->Player->FloorY : 0;
}

bool US47Away::Standable(double X, double Z) const
{
	// player.walkable(x, z) and nothing in the way within 0.4 m (the area's colliders)
	if (!M->Player || !M->Player->Walkable(X, Z)) return false;
	if (M->Web) for (const FS47Rect& C : M->Web->Colliders()) if (X > C.MinX - .4 && X < C.MaxX + .4 && Z > C.MinZ - .4 && Z < C.MaxZ + .4) return false;
	return true;
}

TOptional<FVector2D> US47Away::LocalOf(const FString& InArea, const FVector& P) const
{
	// a world point in an area's own terms (SARO's site, the road's, the station's, the old road's)
	if (InArea == TEXT("saro")) return FVector2D(P.X, P.Z);
	if (InArea == TEXT("road") || InArea == TEXT("diner")) return FVector2D(P.X - ROAD_ORIGIN().X, P.Z - ROAD_ORIGIN().Z);
	if (InArea == TEXT("station01")) return FVector2D(P.X - STATION_AT().X, P.Z - STATION_AT().Z);
	if (InArea == TEXT("room6")) return FVector2D(0, 0);
	return {};
}

TOptional<FVector2D> US47Away::MapOf(const FVector& P) const
{
	const FString A = Area();
	const TOptional<FVector2D> L = LocalOf(A, P);
	return L ? S47Lines::ToMap(A, L->X, L->Y) : TOptional<FVector2D>();
}

bool US47Away::NearArray(double X, double Z) const
{
	// story/dishesFollow.ts nearArray: out in the antenna field, or close enough to one of its dishes
	bool bOnField = false;
	if (Z < FENCE_Z && M->Web)
	{
		for (const FS47Rect& R : M->Web->ZonesMutable(TEXT("saro")))
		{
			if (!(R.Id == TEXT("fSpine") || R.Id.StartsWith(TEXT("fRow")) || R.Id.StartsWith(TEXT("fApron:")) || R.Id.StartsWith(TEXT("fSpur:")))) continue;
			if (X >= R.MinX - 3 && X <= R.MaxX + 3 && Z >= R.MinZ - 3 && Z <= R.MaxZ + 3) { bOnField = true; break; }
		}
	}
	if (bOnField) return true;
	double D = TNumericLimits<double>::Max();
	if (M->Web) for (int32 I = 0; I < M->Web->DishCount(); ++I) D = FMath::Min(D, FMath::Sqrt(FMath::Square(M->Web->DishX(I) - X) + FMath::Square(M->Web->DishZ(I) - Z)));
	return D < 45;
}

FS47AbductIn US47Away::AbductIn(bool bOk) const
{
	// where the lines run, for the time away (story/lines.ts; HENDELSER.md, section 3)
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	const FString A = Area();
	FS47AbductIn S;
	if (!Driving())
	{
		// on foot and out on the ground the time away can come on: SARO's (B's: the yard, the field, the
		// shoulder down to the motel), Caliche's street away from people (A's), STATION 01 (C's)
		bool bOut = false;
		if (A == TEXT("saro")) bOut = (US47Night::SaroFloorAt(M->Web, P.X, P.Z) < -0.3 || OnRoad(P) || Outside(P)) && !(M->Web && M->Web->Rect(TEXT("world.motel.zones[2]")).Contains(P));
		else if (A == TEXT("diner"))
		{
			// Town.inTown (its walk zones), not indoors, and away from people (main.ts townAlone)
			bool bInTown = false;
			if (M->Web) for (const FString& Pa : M->Web->PathsUnder(TEXT("world.town.zones["))) bInTown |= M->Web->Rect(Pa).Contains(P.X, P.Z);
			const double Lx = P.X - ROAD_ORIGIN().X, Lz = P.Z - ROAD_ORIGIN().Z;
			bool bAlone = Lz >= 2085;   // the diner, the gas station's window
			if (bAlone && Is(TownWomanIn) && FMath::Sqrt(FMath::Square(Lx + 14.4) + FMath::Square(Lz - 2132.7)) < 22) bAlone = false;
			if (bAlone && Is(TownSmokerIn) && FMath::Sqrt(FMath::Square(Lx - 24.1) + FMath::Square(Lz - 2159.4)) < 22) bAlone = false;
			bOut = bInTown && !(M->World && M->World->Indoors(P)) && bAlone;
		}
		else if (A == TEXT("station01")) bOut = !(M->World && M->World->Indoors(P));
		const TOptional<FVector2D> Lc = bOut ? LocalOf(A, P) : TOptional<FVector2D>();
		const TOptional<S47Lines::FOnLine> On = Lc ? S47Lines::LineAt(A, Lc->X, Lc->Y) : TOptional<S47Lines::FOnLine>();
		if (On)
		{
			S.Line = On->Line;
			// further along the line, where the ground there runs: down the highway's shoulder, down
			// Caliche's street, else the line itself
			if ((A == TEXT("saro") && OnHighway(P)) || A == TEXT("diner")) S.Along = FVector(0, 0, 1);
			else S.Along = FVector(On->Along.X, 0, On->Along.Y);
		}
	}
	S.bOk = bOk; S.bEnabled = AbductOn(); S.Clock = Clock();
	S.NextFixed = Fixed(Phase(), M->Story ? M->Story->EventClockSec() : 0).Get(-1e9);
	S.bLate = Phase() == TEXT("ch3") || Phase() == TEXT("ch4");
	S.bCalm = CalmFlash();
	// out among the dishes before they have turned: a little sooner there (story/dishesFollow.ts)
	S.Boost = S.Line == TEXT("B") && !(Abduction && Abduction->Data.Dishes.IsValid()) && NearArray(P.X, P.Z) ? 1.5 : 1;
	// the warning in the truck: the truck has let go (story/truckTaken.ts)
	S.bCabFree = TruckTaken && !TruckTaken->Free.IsEmpty();
	return S;
}

FString US47Away::StationDue() const
{
	// why everyone does not go at STATION 01 right now (story/stationTaken.ts; empty: it begins): after N.
	// Vega's call in chapter three, on foot outside the hut a step or two from its door, Missing time on,
	// not taken tonight yet, room for the lost time before 04:35, and nothing else of the night's going
	const US47Chapter* C3 = M->Story ? M->Story->Chapter(TEXT("ch3")) : nullptr;
	const S47J::FObj St = C3 && C3->bActive ? C3->SaveState() : nullptr;
	if (Phase() != TEXT("ch3") || !S47J::Bool(St, TEXT("called")) || S47J::Str(St, TEXT("stage")) != TEXT("return")) return TEXT("story");
	if (Area() != TEXT("station01") || Driving() || !M->Web) return TEXT("place");
	if (!AbductOn()) return TEXT("off");
	if (Abduction->Data.Count > 0 || !Abduction->Data.Station.IsEmpty() || Abduction->Stage != TEXT("calm") || (StationIdle && !StationIdle())) return TEXT("done");
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	if (M->World->Indoors(P)) return TEXT("inside");
	const FS47Obj Door = M->Web->Obj(TEXT("world.site.proxies.hutDoor"));
	if (Door) { const FVector D = Door.WorldPosition(); if (FMath::Sqrt(FMath::Square(P.X - D.X) + FMath::Square(P.Z - D.Z)) < OUT_OF_DOOR) return TEXT("door"); }
	if (Clock() > STATION_LAST) return TEXT("late");
	if (Cinematic() || M->OneAtATime() || (M->Audio && M->Audio->PhonePlaying())) return TEXT("busy");
	// (stationTaken.begin is its port's: without it, nothing shuts the hut)
	if (!StationBegin) return TEXT("story");
	return FString();
}

FS47TruckOnBIn US47Away::TruckBIn(bool bModal) const
{
	// the driven truck on the map and its speed, on a trip (not a free drive, not chapter six's road) and
	// where it began, chapters two to five, room 6 over, and nothing else of the night's going; on the
	// old road before the line, the miles; slow fades
	const FS47Truck* Tr = Driving() ? TruckNow() : nullptr;
	const US47Chapter* C4 = M->Story ? M->Story->Chapter(TEXT("ch4")) : nullptr;
	const S47J::FObj S4 = C4 ? C4->SaveState() : nullptr;
	const FString Stage4 = S47J::Str(S4, TEXT("stage"));
	FS47TruckOnBIn S;
	S.bOk = !bModal && !Cinematic() && !M->OneAtATime() && !(M->Audio && M->Audio->PhonePlaying()) && !(M->Phones && Is(M->Phones->CbBusy)) && !Is(Ch6CbDue);
	S.bTrip = M->World->TripSecs().IsSet();
	if (Tr && Area() != TEXT("roswell")) S.At = MapOf(Tr->Position);
	S.Speed = Driving() ? M->World->DrivingSpeed() : 0;
	S.bOpen = At(TEXT("ch2")) && Phase() != TEXT("ch6");
	S.bAfter6 = At(TEXT("ch5")) || (Phase() == TEXT("ch4") && C4 && C4->bActive && S47J::Bool(S4, TEXT("p12")) && (Stage4 == TEXT("leave") || Stage4 == TEXT("complete")));
	S.Clock = Clock();
	if (Area() == TEXT("roswell") && Ch6OldMi) S.OldMi = Ch6OldMi();
	const TOptional<FS47TripNow> Trip = M->World->TripNow();
	S.From = Trip ? Trip->From : FString();
	S.Metres = Trip ? Trip->Metres : 0;
	S.bCalm = CalmFlash();
	return S;
}

FS47TruckTakenIn US47Away::TruckTakenIn(bool bModal) const
{
	// the driven truck on the map in SARO's area, its speed and which way it faces, whether the time away
	// may come, a rung of the truck on B due, the warning, and where the player is from the truck
	US47World* Wd = M->World;
	const FS47Truck* Tr = Driving() ? TruckNow() : nullptr;
	const FS47Truck* Here = Area() == TEXT("saro") ? (Tr ? Tr : Wd->TruckHere()) : nullptr;
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	const double Foot = !Driving() && Here ? FMath::Sqrt(FMath::Square(P.X - Here->Position.X) + FMath::Square(P.Z - Here->Position.Z)) : 0;
	const FString Ph = Phase();
	FS47TruckTakenIn S;
	S.bOk = !bModal && !Cinematic() && (Ph == TEXT("ch1") || Ph == TEXT("ch2") || Ph == TEXT("ch3") || Ph == TEXT("ch4")) && M->Night->Dawn() < 0.3
		&& !M->OneAtATime() && !(M->Audio && M->Audio->PhonePlaying()) && !(M->Phones && Is(M->Phones->CbBusy)) && Wd->Trouble.Kind() == ES47Trouble::None;
	// (the curve's peak itself: not when it stands aside for a test of one thing)
	S.bMay = !M->Night->Curve->bFree && M->Night->Curve->May(TEXT("abduct"));
	S.bAllowed = Abduction->MayBegin(AbductIn(true)); S.bEnabled = AbductOn();
	S.bTrip = Driving() && Wd->Leg == TEXT("saro") && Wd->TripSecs().IsSet();
	if (Tr && Area() == TEXT("saro")) S.At = MapOf(Tr->Position);
	S.Speed = Driving() ? Wd->DrivingSpeed() : 0;
	S.South = Tr ? -FMath::Cos(Tr->Rotation.Y) : 0;
	S.bDue = TruckB->Due(TruckBIn(bModal));
	S.Stage = Abduction->Stage; S.WarnT = WarnT();
	S.bInCab = Driving(); S.Away = Foot; S.North = Here && !Driving() ? Here->Position.Z - P.Z : 0;
	S.Clock = Clock(); S.bOver = !Abduction->OverPhase().IsEmpty();
	return S;
}

TOptional<double> US47Away::HoleFrom() const
{
	// sky.holeFrom: no stars where the dishes point from the moment they have come round to 026 / 32
	if (!M->Story || !M->Story->At(ES47Phase::Turning)) return {};
	double Ev = 0;
	if (At(TEXT("ch1"))) if (const US47Chapter1* C1 = Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")))) Ev = C1->S.EventClock;
	if (!Ev) Ev = M->Story->EventClockSec();
	return Ev + FS47StarHole::STAR_HOLE.After;
}

FVector US47Away::HoleDir() const
{
	// sky.holeDir turned about the up axis by world.skyYaw
	const FVector D0 = FS47StarHole::SkyDir(FS47StarHole::STAR_HOLE.Az, FS47StarHole::STAR_HOLE.El);
	const double Yaw = M->World ? M->World->SkyYaw() : 0;
	return FVector(D0.X * FMath::Cos(Yaw) + D0.Z * FMath::Sin(Yaw), D0.Y, -D0.X * FMath::Sin(Yaw) + D0.Z * FMath::Cos(Yaw));
}

FS47StarHoleIn US47Away::HoleIn(bool bModal)
{
	// where the hole is from here and now (the sky turned with the area), and what the player is doing under it
	const FVector Dir = HoleDir();
	FS47StarHoleIn S;
	S.bOk = M->Mode == EMode::Play && !bModal && !Cinematic() && !Driving();
	S.bFree = !M->OneAtATime() && !(M->Audio && M->Audio->PhonePlaying()) && !(M->Phones && Is(M->Phones->CbBusy));
	S.bOutdoors = M->Space == TEXT("yard") && !Driving();
	S.bNight = M->Night->Dawn() < 0.3;
	S.Hole = FS47StarHole::At(Clock(), HoleFrom());
	S.Dir = Dir;
	S.Cam = Camera();
	S.bZoom = M->ZoomK > 0.6;
	// (nothing solid in the way up to 1500 m out, looked for at most every 0.4 s: a ray through the scene)
	TWeakObjectPtr<US47Away> Self = this;
	const FVector Eye = S.Cam.Pos;
	S.Clear = [Self, Eye, Dir]
	{
		if (!Self.IsValid()) return false;
		US47Away* Aw = Self.Get();
		const double T = Aw->M->T;
		if (T - Aw->HoleClearAt > 0.4 || T < Aw->HoleClearAt)
		{
			Aw->HoleClearAt = T;
			Aw->bHoleClear = Aw->M->Night->Album ? Aw->M->Night->Album->InSight(Eye, Eye + Dir * 1600, 1500) : true;
		}
		return Aw->bHoleClear;
	};
	return S;
}

double US47Away::BandShine(const FVector& At) const
{
	// how much light from near the eye falls on a point (the reflective band on a SARO jacket gives it
	// back the way it came): the flashlight's beam on foot, the headlights in the truck
	const FS47Truck* Tr = Driving() ? M->World->TruckHere() : nullptr;
	const FS47Cam C = Camera();
	const FVector From = Tr ? Tr->Position : C.Pos;
	const FVector Dir = Tr ? Tr->Quat().RotateVector(FVector(0, 0, -1)) : C.Forward;
	if (!Tr && !(M->Night->Flashlight && M->Night->Flashlight->bOn)) return 0;
	const FVector To = At - From;
	const double D = To.Size();
	const double Dd = FMath::Sqrt(Dir.SizeSquared() * To.SizeSquared());
	const double A = FMath::RadiansToDegrees(Dd == 0 ? UE_DOUBLE_HALF_PI : FMath::Acos(FMath::Clamp(FVector::DotProduct(Dir, To) / Dd, -1.0, 1.0)));
	const double Cone = Tr ? 16 : 15, Near = Tr ? 150 : 110, Far = Tr ? 280 : 220;
	return (1 - Smooth(A, Cone * 0.45, Cone)) * (1 - Smooth(D, Near, Far));
}

TArray<FS47MapMark> US47Away::MapMarks() const
{
	TArray<FS47MapMark> Out;
	if (!Abduction) return Out;
	for (const FS47AbductSpot& Sp : Abduction->Data.Spots)
	{
		const FString A = Sp.Scene == TEXT("road") ? FString(TEXT("diner")) : Sp.Scene;
		const TOptional<FVector2D> L = LocalOf(A, Sp.Take);
		const TOptional<S47Lines::FOnLine> On = L ? S47Lines::LineAt(A, L->X, L->Y) : TOptional<S47Lines::FOnLine>();
		FS47MapMark Mk;
		Mk.Kind = TEXT("away"); Mk.Clock = Sp.Clock;
		Mk.Line = On ? On->Line : Sp.Scene == TEXT("saro") ? FString(TEXT("B")) : Sp.Scene == TEXT("road") ? FString(TEXT("A")) : FString(TEXT("C"));
		if (Sp.Scene == TEXT("saro"))
		{
			FS47MapAt At;
			if (Sp.Take.Z > 64 || Sp.bCab) { At.Spot = TEXT("highway"); At.M = FMath::Max(0.0, Sp.Take.Z); At.bM = true; }
			else At.Spot = TEXT("yard");
			Mk.At = At;
		}
		else if (Sp.Scene == TEXT("station01")) { FS47MapAt At; At.Spot = TEXT("station01"); Mk.At = At; }
		Out.Add(Mk);
	}
	return Out;
}

bool US47Away::CbBlocked() const { return (Abduction && Abduction->Busy()) || (TruckTaken && (TruckTaken->Busy() || TruckTaken->Ahead())); }

// ---------- the camera of its own ----------

bool US47Away::Shoot(const FS47ShotCam& Cam, int32 Wd, int32 Ht, TArray<FColor>& Out, bool bProbe)
{
	// main.ts shoot(cam) and probe(cam, w, h): the scene from a camera of the time away's own, rendered at
	// once (a scene capture through the world's own post process, M_WebTone, without the tape). The probe
	// has the sky picture hidden and finds the open sky by depth: what nothing stands in front of.
	UWorld* World = M->Web ? M->Web->GetWorld() : nullptr;
	if (!World) return false;
	if (!Capture.IsValid())
	{
		FActorSpawnParameters Sp;
		Sp.ObjectFlags |= RF_Transient;
		Capture = World->SpawnActor<ASceneCapture2D>(Sp);
		if (!Capture.IsValid()) return false;
	}
	auto Target = [this](TObjectPtr<UTextureRenderTarget2D>& RT, int32 W, int32 H, EPixelFormat F)
	{
		if (!RT || RT->SizeX != W || RT->SizeY != H)
		{
			RT = NewObject<UTextureRenderTarget2D>(this);
			RT->ClearColor = FLinearColor::Black;
			RT->InitCustomFormat(W, H, F, true);
			RT->UpdateResourceImmediate(true);
		}
		return RT.Get();
	};
	USceneCaptureComponent2D* C = Capture->GetCaptureComponent2D();
	C->bCaptureEveryFrame = false;
	C->bCaptureOnMovement = false;
	C->bAlwaysPersistRenderingState = true;
	C->FOVAngle = S47Three::HorizontalFov(float(Cam.Fov), float(Cam.Aspect));
	C->bOverride_CustomNearClippingPlane = true;
	C->CustomNearClippingPlane = float(Cam.Near * 100.0);
	C->PostProcessBlendWeight = 1.f;
	C->ShowFlags.SetTemporalAA(false);
	const FVector F = Cam.Q.RotateVector(FVector(0, 0, -1)), U = Cam.Q.RotateVector(FVector(0, 1, 0));
	Capture->SetActorLocationAndRotation(S47Three::ToUE(Cam.Pos), FRotationMatrix::MakeFromXZ(S47Three::DirToUE(F), S47Three::DirToUE(U)).Rotator());
	UMaterialParameterCollection* Env = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Web/Base/MPC_Web.MPC_Web"));
	UMaterialParameterCollectionInstance* E = Env ? World->GetParameterCollectionInstance(Env) : nullptr;
	float Vhs = 0.f;
	if (E) { E->GetScalarParameterValue(TEXT("Vhs"), Vhs); E->SetScalarParameterValue(TEXT("Vhs"), 0.f); }
	UPrimitiveComponent* Sky = nullptr;
	if (bProbe) for (UActorComponent* Ac : M->Web->GetComponents()) if (Ac && Ac->GetFName() == TEXT("SkySphere")) Sky = Cast<UPrimitiveComponent>(Ac);
	if (Sky) C->HiddenComponents.AddUnique(Sky);
	C->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	C->TextureTarget = Target(bProbe ? ProbeTarget : ShotTarget, Wd, Ht, PF_B8G8R8A8);
	C->CaptureScene();
	Out.Reset();
	bool bOk = S47Image::ReadPixels(C->TextureTarget, Out) && Out.Num() == Wd * Ht;
	if (bOk && bProbe)
	{
		// the open sky: no depth in front of it (nothing within the sky picture's 1.5 km, the far plane)
		C->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
		C->TextureTarget = Target(DepthTarget, Wd, Ht, PF_A32B32G32R32F);
		C->CaptureScene();
		TArray<FLinearColor> Dp;
		FTextureRenderTargetResource* R = DepthTarget ? DepthTarget->GameThread_GetRenderTargetResource() : nullptr;
		if (R && R->ReadLinearColorPixels(Dp) && Dp.Num() == Wd * Ht)
		{
			for (int32 I = 0; I < Out.Num(); ++I) if (Dp[I].R > Cam.Far * 100.0 * 0.9) Out[I] = FColor(255, 0, 255, 255);
		}
	}
	if (Sky) C->HiddenComponents.Remove(Sky);
	if (E) E->SetScalarParameterValue(TEXT("Vhs"), Vhs);
	for (FColor& Px : Out) Px.A = 255;
	return bOk;
}

// ---------- the third one (story/theThird.ts; main.ts ch3.third, thirdNoticed) ----------

TOptional<FS47FilmShot> US47Away::ThirdShot(const FS47Cam& Cam)
{
	AS47Web* W = M->Web;
	if (Area() != TEXT("station01") || !W || M->OneAtATime()) return {};
	const FVector O = STATION_AT();
	// world/stationLayout.ts STATION_TRANSIT (3, -4) and STATION_STAKES_C (1, 0), in the site's terms
	FS47FilmIn In;
	In.Cam = Cam;
	In.Transit = FVector(3 + O.X, 0, -4 + O.Z);
	In.Transit.Y = S47Legs::StationGroundAt(In.Transit.X, In.Transit.Z);
	In.CDir = FVector(1, 0, 0);
	In.Ground = [](double X, double Z) { return S47Legs::StationGroundAt(X, Z); };
	TWeakObjectPtr<AS47Web> Wk = W;
	In.Clear = [Wk](const FVector& From, const FVector& To)
	{
		// the land between (sampled), then the hut, the shed and the footing
		for (double K = 0.08; K < 0.97; K += 0.04)
		{
			const double X = From.X + (To.X - From.X) * K, Y = From.Y + (To.Y - From.Y) * K, Z = From.Z + (To.Z - From.Z) * K;
			if (S47Legs::StationGroundAt(X, Z) > Y - 0.08) return false;
		}
		FVector D = To - From;
		const double Len = D.Size();
		if (!Wk.IsValid() || Len <= 0.7) return true;
		D /= Len;
		const FVector A = S47Three::ToUE(From + D * 0.2), B = S47Three::ToUE(From + D * (Len - 0.5));
		FCollisionQueryParams Q(SCENE_QUERY_STAT(S47ThirdClear), true);
		for (const FString& Path : Wk->PathsUnder(TEXT("world.site.occluders[")))
		{
			bool bHit = false;
			Wk->Obj(Path).Traverse([&](FS47Obj N)
			{
				const FS47WebNode* Wn = bHit || !N.W ? nullptr : N.W->Node(N.I);
				UPrimitiveComponent* Pc = Wn ? Cast<UPrimitiveComponent>(Wn->Comp.Get()) : nullptr;
				FHitResult Hr;
				if (Pc && Pc->LineTraceComponent(Hr, A, B, Q)) bHit = true;
			});
			if (bHit) return false;
		}
		return true;
	};
	return FilmThird->Shot(In);
}

void US47Away::ThirdNoticed()
{
	// the band on FRAME 03, looked at closely (the field prints, the print on the drying line): written
	// down once, in the player's words
	const US47Chapter* C3 = M->Story ? M->Story->Chapter(TEXT("ch3")) : nullptr;
	const S47J::FObj Tg = S47J::Obj(S47J::Obj(C3 ? C3->SaveState() : nullptr, TEXT("f3")), TEXT("targets"));
	if (!S47J::Has(Tg, TEXT("band")) || M->TownSeen.Contains(TEXT("third:band"))) return;
	M->TownSeen.Add(TEXT("third:band"));
	M->Story->Note(FString::Printf(TEXT("%s. FRAME 03, looked at closely: out in the dark past the fence%s, at chest height, a short pale line, and something darker than the dark round it. Something out there gave the flash back, the way the tape on a SARO jacket gives a light back. There was nobody out there when I took it."),
		*S47Text::ClockText(Clock(), false), S47J::Has(Tg, TEXT("bandC")) ? TEXT(", along the C stakes") : TEXT("")));
	if (M->Mode == EMode::Play) RequestAutosave();
}

// ---------- step() ----------

void US47Away::StepHole(double Dt, bool bModal)
{
	// no stars where the dishes point (story/starHole.ts): noticed once, looked at long enough
	if (M->Mode == EMode::Play && !StarHole->Noticed()) StarHole->Update(bModal ? 0 : Dt, HoleIn(bModal));
}

void US47Away::StepLate(double Dt, bool bModal)
{
	if (M->Mode != EMode::Play) return;
	// (the music held while these run: main.ts musicHeld)
	if (M->Score.IsValid())
	{
		TWeakObjectPtr<US47Away> Self = this;
		if (!M->Score->Held.AbductionBusy) M->Score->Held.AbductionBusy = [Self] { return Self.IsValid() && Self->Abduction->Busy(); };
		if (!M->Score->Held.TruckBRacing) M->Score->Held.TruckBRacing = [Self] { return Self.IsValid() && Self->TruckB->Racing(); };
	}
	// the truck on B (story/truckOnB.ts), and the compass on the dash of the truck being driven
	TruckB->Update(bModal ? 0 : Dt, TruckBIn(bModal));
	// (after it: a rung that is due has the pass over B; story/truckTaken.ts)
	TruckTaken->Update(bModal ? 0 : Dt, TruckTakenIn(bModal));
	if (FS47Truck* Ct = Driving() ? TruckNow() : nullptr)
	{
		Ct->CompassNorth = M->World->SkyYaw();
		if (TruckB->Pull) Ct->CompassPull = FS47Truck::FPull{ TruckB->Pull->Deg, TruckB->Pull->K }; else Ct->CompassPull.Reset();
	}
	// the array put back on the schedule (story/arrayBack.ts: S47SaroNight's), before the time away
	if (BeforeAbduction) BeforeAbduction(Dt, bModal);
	// the notebook or a paper open holds the warning where it is, as it holds the clock: it is not a way
	// out (abduction.ts lets the warning pass when ok goes false), and the dishes wait too
	FS47Abduction& Ab = *Abduction;
	if (!(bModal && Ab.Stage == TEXT("warning")))
	{
		const FString A = Area();
		Ab.SceneId = A == TEXT("saro") ? FString(TEXT("saro")) : A == TEXT("diner") || A == TEXT("road") ? FString(TEXT("road")) : A;
		// at STATION 01, on the way out of the hut after N. Vega's call: everyone goes (story/stationTaken.ts)
		if (!bModal && StationDue().IsEmpty()) { Ab.Force(AbductIn(true)); Ab.Data.Station = TEXT("taken"); StationBegin(Clock()); }
		const FS47NightHooks& H = M->Night->Hooks;
		const FString Ph = Phase();
		const bool bCalm = Ab.Stage == TEXT("calm");
		FS47AbductIn In = AbductIn(!bModal && !Cinematic() && (Ph == TEXT("ch1") || Ph == TEXT("ch2") || Ph == TEXT("ch3") || Ph == TEXT("ch4")) && M->Night->Dawn() < 0.3
			&& !Is(H.RoadsideSilent) && !(H.TownEventsNow && H.TownEventsNow() != TEXT("none")) && !Is(H.MotelTvBusy) && !Is(H.RoofStepsBusy) && !Is(H.ReelTapeBusy) && !Is(H.SaroDarkBusy) && !Is(H.OfficePhoneBusy) && !Is(H.ArchiveBusy)
			// (not to begin while the array is on the schedule, or the recording walks; one that has begun is not cut by them)
			&& !(Is(H.ArrayBackBusy) && bCalm) && !(Is(H.RecWalkBusy) && bCalm));
		In.May = M->Night->Curve->May(TEXT("abduct"));
		Ab.Update(Dt, In);
		// (stationTaken.update, the station passed, the recording's walk and the dishes that follow: their ports)
		if (!AbductOn() && Is(H.RecWalkBusy) && RecWalkReset) RecWalkReset();
		if (AfterAbduction) AfterAbduction(Dt, bModal);
	}
	// (the corner clock: abduction.warnClock heads the one chain S47SaroNight shows; the HUD gives it this)
	Frame(Dt);
	SyncBooks();
	// the log, for the tests and the screenshot runs
	if (Ab.Stage != LoggedStage) { LoggedStage = Ab.Stage; UE_LOG(LogS47Away, Display, TEXT("S47: time away %s (pressure %.2f)"), *LoggedStage, Ab.Pressure); }
	if (TruckTaken->Phase != LoggedTaken) { LoggedTaken = TruckTaken->Phase; UE_LOG(LogS47Away, Display, TEXT("S47: truck taken %s"), *LoggedTaken); }
	if (TruckB->Phase != LoggedB) { LoggedB = TruckB->Phase; UE_LOG(LogS47Away, Display, TEXT("S47: truck on B %s"), *LoggedB); }
}

void US47Away::Frame(double Dt)
{
	// the outlines over you turn round their upright to the eye (their onBeforeRender)
	Abduction->FaceOver(Camera().Pos);
	// FLASH's frames, one after another at their own lengths
	if (FlashI >= 0)
	{
		FlashT += Dt * 1000;
		while (FlashI < Flash.Num() && FlashT >= Flash[FlashI].Value) { FlashT -= Flash[FlashI].Value; FlashI++; }
		if (FlashI >= Flash.Num()) { FlashI = -1; HideFrame(); UE_LOG(LogS47Away, Display, TEXT("S47: FLASH frames shown, %d"), Flash.Num()); }
		else ShowFrame(Flash[FlashI].Key.Get(), 1);
	}
}

void US47Away::PlayFlash()
{
	Flash.Reset();
	for (const S47FlashFrames::FFrame& F : S47FlashFrames::FlashFrames()) Flash.Add({ TStrongObjectPtr<UTexture2D>(S47Image::FromPixels(F.Canvas.Colors(), F.Canvas.W, F.Canvas.H)), F.Ms });
	FlashI = 0; FlashT = 0;
	if (Flash.Num()) ShowFrame(Flash[0].Key.Get(), 1);
}

void US47Away::SyncBooks()
{
	// game.extraBooks: [...lostNights.books(), ..., ...starHole.books()]
	TArray<FS47RuleBook*> Want;
	Want.Append(LostNights->Books());
	Want.Append(StarHole->Books());
	for (FS47RuleBook* B : Mine) if (!Want.Contains(B)) M->Books.Remove(B);
	for (FS47RuleBook* B : Want) M->Books.AddUnique(B);
	Mine = Want;
}

// ---------- what the night did, and whether it was seen (main.ts seenStep) ----------

void US47Away::SeenStep()
{
	US47Night* N = M->Night;
	FS47Abduction& Ab = *Abduction;
	const FS47Cam Cam = Camera();
	// (it stands there for the rest of the night, shown only from more than 45 m: written down once, when
	// it is in the middle of the view and near enough to make out, as the cutout is)
	for (const FS47Abduction::FShown& R : Ab.RecordsShown())
	{
		// (which time away it stands for: the first, the second, the third)
		const FString What = FString::Printf(TEXT("record %d"), R.N);
		if (Was.Records.Contains(What)) continue;
		const FVector V(R.At.X, R.At.Y + 1.2, R.At.Z);
		if (!S47Facing(Cam, V, 160, 15)) continue;
		Was.Records.Add(What);
		FS47SawOpts O; O.At = V; O.bLookingSet = true; O.Looking = true; O.bNoWatch = true;
		N->Saw(TEXT("scare"), What, O);
	}
	const FString St = Ab.Stage;
	if (St != Was.Stage)
	{
		if (St == TEXT("warning") && Was.Stage == TEXT("calm")) { FS47SawOpts O; O.bLookingSet = true; WarnEntry = N->Saw(TEXT("abduction"), TEXT("warning"), O); }
		else if (Was.Stage == TEXT("warning") && WarnEntry) { WarnEntry->End = St == TEXT("light") ? TEXT("taken") : TEXT("away"); WarnEntry.Reset(); }
		Was.Stage = St;
	}
	// taken in the truck (story/truckTaken.ts): written when the warning comes in the cab, where the truck
	// was; how it ended: the key, out and away ('ran'), taken, or Missing time turned off
	if (TruckTaken->Phase != Was.Truck)
	{
		if (Was.Truck == TEXT("idle") && TruckTaken->Phase == TEXT("warning"))
		{
			const FS47Truck* Tr = TruckNow();
			FS47SawOpts O; if (Tr) O.At = Tr->Position; O.Line = FString(TEXT("B")); O.bLookingSet = true; O.bNoWatch = true;
			TruckEntry = N->Saw(TEXT("abduction"), TEXT("truck"), O);
		}
		if ((TruckTaken->Phase == TEXT("tail") || TruckTaken->Phase == TEXT("idle")) && TruckEntry) { TruckEntry->End = TruckTaken->Ended.IsEmpty() ? FString(TEXT("away")) : TruckTaken->Ended; TruckEntry.Reset(); }
		Was.Truck = TruckTaken->Phase;
	}
	const FString Op = Ab.OverPhase();
	if (Op == TEXT("there") && Was.Over != TEXT("there"))
	{
		const TArray<FVector> Heads = Ab.OverHeads();
		const int32 Nn = Ab.Data.Spots.Num() && Ab.Data.Spots.Last().Over ? *Ab.Data.Spots.Last().Over : 1;
		static const TCHAR* Which[] = { TEXT("first"), TEXT("second"), TEXT("third") };
		FS47SawOpts O; if (Heads.Num()) O.At = Heads[0]; O.bLookingSet = true; O.Looking = false; O.bNoWatch = true;
		OverEntry = N->Saw(TEXT("over"), Nn >= 1 && Nn <= 3 ? FString(Which[Nn - 1]) : FString::FromInt(Nn), O);
	}
	if (Op.IsEmpty() && !Was.Over.IsEmpty() && OverEntry) { OverEntry->Looking = Ab.bOverLooked; OverEntry.Reset(); }
	Was.Over = Op;
	// main.ts seenStep: the last mile, THE EVENT, and the third one once while it stands there.
	const US47Chapter6* C6 = M->Story ? Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6"))) : nullptr;
	const bool bLastMile = C6 && C6->bActive && C6->LastMile.OpenAt.IsSet();
	if (bLastMile && !Was.bLastMile) { FS47SawOpts O; O.Line = TEXT("C"); O.bLookingSet = true; N->Saw(TEXT("event"), TEXT("the last mile"), O); }
	Was.bLastMile = bLastMile;
	const bool bEvent = C6 && C6->Stage == TEXT("event");
	if (bEvent && !Was.bEvent) { FS47SawOpts O; O.Line = TEXT("C"); O.bLookingSet = true; N->Saw(TEXT("event"), TEXT("THE EVENT"), O); }
	Was.bEvent = bEvent;
	const TOptional<FVector> Third = Ch6ThirdAt ? Ch6ThirdAt() : TOptional<FVector>();
	if (Third && !Was.bThird)
	{
		FS47SawOpts O; O.At = Third; O.Line = TEXT("C");
		FS47SeenWatch W; W.Pos = Ch6ThirdAt; W.Secs = 34; W.Range = 140; W.Deg = 26; O.Watch = W;
		N->Saw(TEXT("scare"), TEXT("the third"), O); Was.bThird = true;
	}
	if (!bEvent) Was.bThird = false;
}

void US47Away::SeenSync()
{
	Was.Stage = Abduction->Stage; Was.Over = Abduction->OverPhase(); Was.Truck = TruckTaken->Phase;
	Was.Records.Reset();
	for (const FS47SeenRef& E : M->Night->SeenLog->Entries) if (E && E->Kind == TEXT("scare") && E->What.StartsWith(TEXT("record "))) Was.Records.Add(E->What);
	WarnEntry.Reset(); OverEntry.Reset(); TruckEntry.Reset();
	// (seenSync: event: game.ch6.stage === 'event', as it stands now)
	const US47Chapter6* C6 = M->Story ? Cast<US47Chapter6>(M->Story->Chapter(TEXT("ch6"))) : nullptr;
	Was.bLastMile = false; Was.bEvent = C6 && C6->Stage == TEXT("event"); Was.bThird = false;
}

// ---------- the saves ----------

void US47Away::Restore(const S47J::FObj& State)
{
	// (world.placeTruck: the truck where the save left it, its pose exactly: after the truck has been put
	// down 300 m on, story/truckTaken.ts, Continue finds it there)
	const FString SavedArea = S47J::Str(State, TEXT("area"));
	if (State.IsValid() && M->World) M->World->PlaceTruck(SavedArea == TEXT("station01") ? TEXT("station01") : SavedArea == TEXT("diner") ? TEXT("diner") : S47J::Str(State, TEXT("truck")) == TEXT("motel") ? TEXT("motel") : TEXT("saro"), S47J::Obj(State, TEXT("truckPose")));
	TruckTaken->Reset();
	if (M->World) M->World->OdoRestore(S47J::Obj(State, TEXT("odo")));   // (the miles on the odometer tonight; an older case has none)
	Abduction->Restore(S47J::Obj(State, TEXT("abduct")));
	TruckB->Reset(); StarHole->Reset();
	// (Night Shift's threads, NightShiftState.ln, as the case had them)
	Ln = S47J::Strs(S47J::Obj(S47J::Obj(State, TEXT("case")), TEXT("ns")), TEXT("ln"));
	LoggedStage.Reset(); LoggedTaken.Reset(); LoggedB.Reset();
}

void US47Away::Restored()
{
	Abduction->Refile();
	if (ReelTape.Stop) ReelTape.Stop();
	LostNights->Restore();
	SyncBooks();
}

void US47Away::Leave(bool bTitle)
{
	// (a warning left in the middle: its tone and pressure would go on droning under the title; and the
	// sound it cut off comes back, as startGame's audio.unlock resumes it in the web version)
	const bool bCut = Abduction->Stage == TEXT("taken") || Abduction->Stage == TEXT("black");
	Abduction->Halt();
	if (bCut && M->Audio) M->Audio->Suspend(false);
	TruckTaken->Reset();
	if (!bTitle)
	{
		TruckB->Reset();
		if (ReelTape.Stop) ReelTape.Stop();
		if (M->World) M->World->OdoRestore(nullptr);
		Ln.Reset();
	}
	FlashI = -1; HideFrame();
}

void US47Away::Save(const S47J::FObj& State) const
{
	S47J::Set(State, TEXT("abduct"), Abduction->Data.ToJson());
	if (M->World)
	{
		S47J::Set(State, TEXT("odo"), M->World->OdoSave());
		// the truck exactly where it was left; a save made on a drive finds it, and the player, where the
		// drive began (World.truckSave)
		const S47J::FObj Tk = M->World->TruckSave();
		if (Tk.IsValid()) { S47J::Set(State, TEXT("truck"), S47J::Str(Tk, TEXT("at"))); S47J::Set(State, TEXT("truckPose"), S47J::Obj(Tk, TEXT("pose"))); }
		else { if (!M->World->TruckAt.IsEmpty()) S47J::Set(State, TEXT("truck"), M->World->TruckAt); S47J::SetNull(State, TEXT("truckPose")); }
		if (M->World->bDriving && Tk.IsValid()) S47J::Set(State, TEXT("area"), S47J::Str(Tk, TEXT("area")));
	}
	// on your back after the time away, Continue stands you up looking ahead
	if (Holds()) if (const S47J::FObj Pose = S47J::Obj(State, TEXT("pose")); Pose.IsValid()) S47J::Set(Pose, TEXT("pitch"), -0.06);
	// (Night Shift's threads go with the case: S47Flow writes ns.ln from here)
}

FString US47Away::SaveBlock() const
{
	// (the 47 seconds of the warning are a race: a save and Continue is not a way out of them)
	if (Abduction && Abduction->Racing()) return NotRightNow;
	// (nor while the truck is in its count on B: story/truckOnB.ts)
	if (TruckB && TruckB->Racing()) return NotRightNow;
	return FString();
}

bool US47Away::HideOver(bool bHide) { return Abduction && Abduction->HideOver(bHide); }

// ---------- the developer menu's, for the tests ----------

void US47Away::TestFlags()
{
	// -S47Abduct[=<seconds>]: begin the warning here (the developer menu's "Begin the warning here"; a
	// shorter warning for a quick run); -S47Abduct=station: STATION 01, everyone goes; -S47TruckTaken[=next]:
	// taken in the truck now (driving), or the next pass over B southbound; -S47TruckB=four|seven: the
	// truck on B's rung now (driving); -S47StarHole: look up at the patch with no stars (after 02:15);
	// -S47LostNights: the binder read and the three in Caliche heard; -S47Flash: FLASH's frames;
	// -S47Third: the third one 9 m in front of the eye with its back to it, in light from behind (chapter
	// six's headlights in their wrong colours)
	const TCHAR* Cmd = FCommandLine::Get();
	FString Ab, Tt, Tb;
	const bool bAb = FParse::Value(Cmd, TEXT("S47Abduct="), Ab) || FParse::Param(Cmd, TEXT("S47Abduct"));
	const bool bTt = FParse::Value(Cmd, TEXT("S47TruckTaken="), Tt) || FParse::Param(Cmd, TEXT("S47TruckTaken"));
	const bool bTb = FParse::Value(Cmd, TEXT("S47TruckB="), Tb);
	const bool bHole = FParse::Param(Cmd, TEXT("S47StarHole")), bLost = FParse::Param(Cmd, TEXT("S47LostNights")), bFlash = FParse::Param(Cmd, TEXT("S47Flash")), bThird = FParse::Param(Cmd, TEXT("S47Third"));
	float Delay = 3.f;
	FParse::Value(Cmd, TEXT("S47AwayAt="), Delay);
	if (!(bAb || bTt || bTb || bHole || bLost || bFlash || bThird) || !M->Web) return;
	TWeakObjectPtr<US47Away> Self = this;
	FTimerHandle H;
	M->Web->GetWorldTimerManager().SetTimer(H, [Self, bAb, Ab, bTt, Tt, bTb, Tb, bHole, bLost, bFlash, bThird]
	{
		if (!Self.IsValid()) return;
		US47Away* Aw = Self.Get();
		FS47Abduction& A = *Aw->Abduction;
		if (bAb)
		{
			const FS47AbductIn S = Aw->AbductIn(true);
			if (A.Busy()) { UE_LOG(LogS47Away, Display, TEXT("S47: the time away: already under way")); }
			else if (Ab == TEXT("station"))
			{
				if (Aw->Area() != TEXT("station01") || (Aw->M->World && Aw->M->World->Indoors(Aw->M->Player->Pos))) { UE_LOG(LogS47Away, Display, TEXT("S47: the time away: outside the hut at STATION 01")); }
				else { A.Force(S); A.Data.Station = TEXT("taken"); if (Aw->StationBegin) Aw->StationBegin(Aw->Clock()); }
			}
			else if (S.Line.IsEmpty()) { UE_LOG(LogS47Away, Display, TEXT("S47: the time away: not on a line here")); }
			else
			{
				A.Begin(S);
				const double Len = FCString::Atod(*Ab);
				if (Len > 0) A.WarnLen = Len;   // (the tests' shorter warning)
			}
		}
		if (bTt)
		{
			if (Tt == TEXT("next")) { Aw->TruckTaken->bForce = true; UE_LOG(LogS47Away, Display, TEXT("S47: taken in the truck: the next time south over B at the turn-off")); }
			else
			{
				const FS47TruckTakenIn S = Aw->TruckTakenIn(false);
				if (Aw->TruckTaken->Busy() || A.Busy()) { UE_LOG(LogS47Away, Display, TEXT("S47: taken in the truck: already under way")); }
				else if (!S.bTrip) { UE_LOG(LogS47Away, Display, TEXT("S47: taken in the truck: drive, in the truck on SARO's ground")); }
				else
				{
					Aw->TruckTaken->Start(S.Clock);
					const double Len = FCString::Atod(*Tt);
					if (Len > 0) A.WarnLen = Len;   // (the tests' shorter warning)
				}
			}
		}
		if (bTb)
		{
			const FS47TruckOnBIn S = Aw->TruckBIn(false);
			if (!Aw->Driving()) { UE_LOG(LogS47Away, Display, TEXT("S47: the truck on B: drive over B at the turn-off to SARO")); }
			else Aw->TruckB->BeginNow(Tb == TEXT("seven") ? TEXT("stall") : TEXT("four"), S);
		}
		if (bHole)
		{
			// (from 02:15: the eyes up along the dishes)
			const FS47StarHoleIn S = Aw->HoleIn(false);
			if (S.Hole < 0.9) { UE_LOG(LogS47Away, Display, TEXT("S47: no stars at 026: only after 02:15 (%.2f)"), S.Hole); }
			else if (AS47Walker* P = Aw->M->Player) { P->Yaw = FMath::Atan2(-S.Dir.X, -S.Dir.Z); P->Pitch = FMath::Asin(S.Dir.Y); UE_LOG(LogS47Away, Display, TEXT("S47: no stars at 026: looking up at it")); }
		}
		if (bLost)
		{
			US47Main* Mn = Aw->M;
			// (game.ns.papers: the binder in the journal)
			if (!Mn->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("binder"); })) Mn->Story->AddDoc(S47Docs::Binder());
			for (const TCHAR* K : { TEXT("met:deputy"), TEXT("ask:deputy:map"), TEXT("ask:deputy:green"), TEXT("met:attendant"), TEXT("ask:attendant:old"), TEXT("met:radio_woman"), TEXT("ask:radio_woman:radio"),
				TEXT("ask:radio_woman:husband"), TEXT("met:host"), TEXT("ask:host:calls"), TEXT("ask:host:tape") }) Mn->TownSeen.Add(K);
			if (Mn->Board.Refresh) Mn->Board.Refresh();
			Aw->SyncBooks();
			UE_LOG(LogS47Away, Display, TEXT("S47: the lost nights: the binder and the green pins on the board, %d book(s)"), Aw->LostNights->Books().Num());
			// (the board open on Night Shift's cork, where they lie)
			if (Mn->Board.Open) Mn->Board.Open(FString(), TEXT("ns"));
		}
		if (bFlash) { Aw->PlayFlash(); UE_LOG(LogS47Away, Display, TEXT("S47: FLASH frames, %d"), Aw->Flash.Num()); }
		if (bThird)
		{
			const FS47Cam C = Aw->Camera();
			FVector F = C.Forward; F.Y = 0; F = F.GetSafeNormal();
			FVector At = C.Pos + F * 9; At.Y = Aw->Ground(At.X, At.Z);
			if (!Aw->TestThird) Aw->TestThird = MakeShared<FS47ThirdFigure>(Aw->M->Web, TEXT("third:test"));
			Aw->TestThird->Place(At, FMath::Atan2(-F.X, -F.Z));
			Aw->TestThird->Light(FLinearColor(0.55f, 0.5f, 0.42f), FLinearColor(0.01f, 0.012f, 0.016f), FLinearColor(1.0f, 0.22f, 0.18f));
			Aw->TestThird->SetFade(1); Aw->TestThird->SetVisible(true); Aw->TestThird->AddToScene();
			const FVector Hd = Aw->TestThird->Head();
			UE_LOG(LogS47Away, Display, TEXT("S47: the third one stands 9 m in front of the eye, its head at %.1f %.1f %.1f"), Hd.X, Hd.Y, Hd.Z);
		}
	}, Delay, false);
}

bool US47Away::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	bOk &= FS47Pix::SelfTest(Lines);
	bOk &= FS47Abduction::SelfTest(Lines);
	bOk &= FS47TruckTaken::SelfTest(Lines);
	bOk &= FS47TruckOnB::SelfTest(Lines);
	bOk &= FS47LostNights::SelfTest(Lines);
	bOk &= FS47StarHole::SelfTest(Lines);
	bOk &= FS47FilmThird::SelfTest(Lines);
	bOk &= S47FlashFrames::SelfTest(Lines);
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s away: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// main.ts's ground on SARO's highway and the fixed points
	Check(OnHighway(FVector(-25, 0, 100)) && !OnHighway(FVector(-30, 0, 680)) && OnRoad(FVector(0, 0, 24.5)), TEXT("the highway and the service road"));
	Check(Fixed(TEXT("ch2"), 0).Get(0) == 13200 && Fixed(TEXT("ch4"), 0).Get(0) == 18000 && !Fixed(TEXT("ch5"), 0) && Fixed(TEXT("ch1"), 8112).Get(0) == 8112 + 2400, TEXT("the story's next fixed points"));
	return bOk;
}
