#include "S47Spine.h"
// main.ts: SARO's own happenings (saroDark, saroPower, arrayBack, follow, lineLights, reel047,
// reelTape, recWalk), in the order main.ts builds and steps them
#include "S47SaroNight.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "S47Ambience.h"
#include "S47Annex.h"
#include "S47ArrayBack.h"
#include "S47Audio.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47ConsoleKit.h"
#include "S47Curve.h"
#include "S47DecoderDesk.h"
#include "S47DishArray.h"
#include "S47Docs.h"
#include "S47Figures.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Legs.h"
#include "S47LineLights.h"
#include "S47Lines.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Reel047.h"
#include "S47RoadTerrain.h"
#include "S47SaroDark.h"
#include "S47SaroPower.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Yard.h"
#include "TimerManager.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Saro, Log, All);

namespace
{
	const TCHAR* BUSY = TEXT("Not right now. Let the moment finish first.");
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }
	double Smoothstep(double X, double Min, double Max) { if (X <= Min) return 0; if (X >= Max) return 1; X = (X - Min) / (Max - Min); return X * X * (3 - 2 * X); }
	// world/Crossing.ts: the highway south of SARO, on the way to the motel (onHighway)
	const double WEST_Z1 = 64, SHOULDER_X0 = -33.5, SHOULDER_X1 = -15;
	bool AtMotel(const FVector& P) { const S47Geo::FRect& R = S47Geo::MOTEL_RECT(); return P.X >= R.MinX && P.X <= R.MaxX + 1 && P.Z >= R.MinZ - 4 && P.Z <= R.MaxZ + 4; }
	bool OnHighway(const FVector& P) { return P.Z > WEST_Z1 - 20 && P.X > SHOULDER_X0 && P.X < SHOULDER_X1 && !AtMotel(P); }
}

// ---------- boot ----------

void US47SaroNight::Init(US47Main* InM)
{
	M = InM;
	AS47Web* W = M->Web;
	TWeakObjectPtr<US47SaroNight> Self = this;
	// the dishes' own state and their red lamps (world/Dish.ts), the field's lamps (Exterior.fieldLights)
	Dishes = MakeShared<FS47DishArray>(W);
	if (W)
	{
		// FIELD_ZONES (world/arrayLayout.ts): the spine, the rows, each antenna's apron and spur, as exported
		TArray<FString> Ids = { TEXT("fSpine") };
		for (int32 R = 1; R <= 6; ++R) Ids.Add(FString::Printf(TEXT("fRow%d"), R));
		for (const FS47Dish& D : Dishes->Dishes) { Ids.Add(TEXT("fApron:") + D.Id); Ids.Add(TEXT("fSpur:") + D.Id); }
		for (const FString& Id : Ids) { const FS47Area R = W->Rect(TEXT("world.field.zone.") + Id); if (R.bValid) FieldZones.Add(R); }
	}

	// ---------- 02:29: SARO goes dark, and Hondo writes (story/saroDark.ts; chapter one, once) ----------
	FS47PowerParts Pp;
	Pp.Web = W; Pp.Dishes = Dishes.Get();
	Pp.Annex = [Self]() -> US47Annex* { if (!Self.IsValid() || !Self->M->Story) return nullptr; const US47Chapter2* C2 = Cast<US47Chapter2>(Self->M->Story->Chapter(TEXT("ch2"))); return C2 ? C2->Annex.Get() : nullptr; };
	Power = MakeShared<FS47SaroPower>(Pp);
	FS47DarkDeps Dd;
	Dd.Audio = M->Audio; Dd.Power = Power.Get(); Dd.Web = W;
	Dd.Ground = [W](double X, double Z) { return US47Night::SaroFloorAt(W, X, Z); };
	Dd.Rack = [Self] { return Self.IsValid() && Self->M->Story ? Self->M->Story->RackAt() : FVector::ZeroVector; };
	Dd.Teleprinter = [Self] { return Self.IsValid() && Self->M->Story ? Self->M->Story->TelexAt() : FVector::ZeroVector; };
	Dd.Relay = [Self] { return Self.IsValid() && Self->M->Story && Self->M->Story->RelayClack(); };
	Dd.Log = [Self](const FString& Text, double At) { if (Self.IsValid() && Self->M->Story) Self->M->Story->ConsoleLog(Text, At); };
	Dd.Telex = [Self](double Filed) { if (!Self.IsValid() || !Self->M->Story) return; Self->M->Story->HondoTelex(Filed); Self->Autosave(); };
	Dd.PowerOut = [Self](bool bOut) { if (Self.IsValid() && Self->M->Story) Self->M->Story->PowerOut(bOut); };
	Dd.Hush = [Self](double Secs, double Level, double Tau, double Back) { if (Self.IsValid() && Self->M->Audio && Self->M->Audio->Night.IsValid()) Self->M->Audio->Night->Hush(Secs, Level, Tau, Back); };
	Dd.Toast = [Self](const FString& Text, double Secs) { if (Self.IsValid()) Self->M->UI->Toast(Text, float(Secs)); };
	Dd.Note = [Self](const FString& Text) { if (!Self.IsValid()) return; Self->Note(Text); Self->Autosave(); };
	Dd.Calm = [Self] { return Self.IsValid() && Self->Calm(); };
	Dark = MakeShared<FS47SaroDark>(Dd);
	UE_LOG(LogS47Saro, Display, TEXT("S47: saro night: the figure %s, %d red lamps, %d field zones"), Dark->HasFigure() ? TEXT("in the export") : TEXT("MISSING"), Dishes->BeaconCount(), FieldZones.Num());

	// ---------- reel 047 on the deck (story/reel047.ts; the archive's phone plays it) ----------
	FS47Reel047Deps Rd;
	Rd.Audio = M->Audio;
	// (Unreal only, Tom's request: the processor is the bought unit on the console desk when the export has
	// it, S47ConsoleKit.h; the reel plays from there, and no reels turn: the deck on the rack is gone, and
	// the spare reel beside the unit is part of the desk's clutter, not a thing of its own)
	Rd.Deck = [Self]
	{
		if (!Self.IsValid() || !Self->M->Web) return FVector::ZeroVector;
		if (Self->M->Story && Self->M->Story->Decoder.IsValid()) return Self->M->Story->Decoder->SoundAt();
		return Self->M->Web->Obj(TEXT("room.objs.decoder")).WorldPosition() + FVector(-0.3, 1.1, 0);
	};
	if (W && FS47ConsoleKit::Find(W) < 0) for (const FString& H : W->PathsUnder(TEXT("room.reels["))) Rd.Reels.Add(W->Obj(H));
	if (W && FApp::CanEverRender())
	{
		// room.dspScreen: the material whose map is its canvas (room.dspScreen.tex)
		const int32 Tex = W->HandleTex(TEXT("room.dspScreen.tex"));
		for (const TSharedPtr<FJsonValue>& V : US47Yard::ManifestSection(TEXT("materials")))
		{
			const S47J::FObj O = V->AsObject();
			if (O.IsValid() && S47J::Has(O, TEXT("map")) && S47J::Int(O, TEXT("map"), -1) == Tex && Tex >= 0) { DspMat = S47J::Int(O, TEXT("id"), -1); break; }
		}
		DspTarget = NewObject<UTextureRenderTarget2D>(this);
		DspTarget->ClearColor = FLinearColor(0.001f, 0.005f, 0.002f);
		DspTarget->InitCustomFormat(256, 72, PF_B8G8R8A8, false);
		DspTarget->UpdateResourceImmediate(true);
		Dsp = MakeShared<FS47DspScreen>();
		Dsp->Init(DspTarget);
		Rd.Screen = Dsp.Get();
	}
	Rd.ShowScreen = [Self](bool bOn) { if (Self.IsValid()) Self->ShowDsp(bOn); };
	Rd.Hush = Dd.Hush;
	Reel047 = MakeShared<FS47Reel047>(Rd);
	UE_LOG(LogS47Saro, Display, TEXT("S47: saro night: reel 047's deck with %d reels, its display on material %d"), Rd.Reels.Num(), DspMat);

	// ---------- Earl's reel from 9/22/81 (core/reelTape.ts; the lost nights play it) ----------
	FS47ReelTapeDeps Td;
	Td.Audio = M->Audio;
	Td.BoyCallTimes = [Self] { return Self.IsValid() && Self->BoyCallTimes ? Self->BoyCallTimes() : TArray<FS47TapeLine>(); };
	Td.CallerVoice = [Self] { return Self.IsValid() && Self->CallerVoice && Self->CallerVoice(); };
	Td.BoyVoices = [Self](const TArray<FString>& Texts) { return Self.IsValid() && Self->BoyVoices ? Self->BoyVoices(Texts) : TArray<TSharedPtr<FS47AudioBuffer>>(); };
	ReelTape = MakeShared<FS47ReelTape>(Td);

	// ---------- the lights on B (story/lineLights.ts) ----------
	LineLights = MakeShared<FS47LineLights>(W, [Self](const FString& Text) { if (Self.IsValid()) Self->Note(Text); }, [Self](double Clock)
	{
		if (!Self.IsValid()) return;
		Self->M->TownSeen.Add(TEXT("ev:lights")); Self->M->TownSeen.Add(FString::Printf(TEXT("mark:lights@%d"), int32(JsRound(Clock)))); Self->Autosave();
	});
	LineLights->OnObserved = [Self](double Clock)
	{
		if (!Self.IsValid() || !Self->M->Night) return;
		FS47SawOpts O; O.At = Self->LineLights->At(); O.Line = S47Lines::LineOfMark(TEXT("lights"));
		if (const S47Lines::FMapPt* Mp = S47Lines::PLACES().Find(TEXT("lights"))) O.Map = *Mp;
		O.bLookingSet = true; O.Looking = true; O.bNoWatch = true;
		Self->M->Night->Saw(TEXT("event"), TEXT("lights"), O);
		Self->Autosave();
	};

	// ---------- the recording walks the player's last 47 seconds where they were taken (story/recording.ts) ----------
	FS47RecordingDeps Wd;
	// recs: () => abduction.recordings(), hold: (n, h) => abduction.holdRecord(n, h) (S47HUD.cpp gives them;
	// the developer's walk, -S47RecWalk, has its own figure and no time away's to hold)
	Wd.Recs = [Self] { return Self.IsValid() && Self->Abduct.Recordings ? Self->Abduct.Recordings() : (Self.IsValid() && Self->TestRec.IsValid() ? TArray<TSharedPtr<FS47Rec>>{ Self->TestRec } : TArray<TSharedPtr<FS47Rec>>()); };
	Wd.Hold = [Self](int32 N, bool bHeld) { if (Self.IsValid() && !Self->bTestWalk && Self->Abduct.HoldRecord) Self->Abduct.HoldRecord(N, bHeld); };
	Wd.Ground = [Self](double X, double Z) { return Self.IsValid() ? Self->FloorAt(X, Z) : 0.0; };
	Wd.Camera = [Self] { return Self.IsValid() ? Self->Camera() : FS47Cam(); };
	Wd.Note = [Self](const FString& Text) { if (Self.IsValid()) Self->Note(FString::Printf(TEXT("%s. %s"), *S47Text::ClockText(Self->Clock(), false), *Text)); };
	Wd.ClockText = [](double C) { return S47Text::ClockText(C, false); };
	Wd.Save = [Self] { if (Self.IsValid() && Self->M->Mode == US47Main::EMode::Play) Self->Autosave(); };
	Wd.Shine = [Self](const FVector& At) { return Self.IsValid() ? Self->BandShine(At) : 0.0; };
	RecWalk = MakeShared<FS47RecordingWalk>(Wd);

	// ---------- the dishes come down and follow the player through a warning (story/dishesFollow.ts) ----------
	FS47FollowDeps Fd;
	Fd.Dishes = Dishes.Get(); Fd.Audio = M->Audio;
	Fd.Get = [Self] { return Self.IsValid() ? Self->GetDishes() : TOptional<FS47DishesSave>(); };
	Fd.Set = [Self](const FS47DishesSave& V) { if (Self.IsValid()) Self->SetDishes(V); };
	Fd.Ground = [W](double X, double Z) { return US47Night::SaroFloorAt(W, X, Z); };
	Fd.Toast = Dd.Toast;
	Fd.Note = [Self](const FString& Text) { if (Self.IsValid()) Self->Note(Text); };
	Fd.Log = Dd.Log;
	Fd.Save = Wd.Save;
	Fd.Face = [Self](double Yaw) { if (Self.IsValid() && Self->M->Player) Self->M->Player->Yaw = Yaw; };
	Fd.NearArray = [Self](double X, double Z) { return Self.IsValid() && FS47DishesFollow::NearArray(Self->FieldZones, Self->Dishes.Get(), X, Z); };
	Follow = MakeShared<FS47DishesFollow>(Fd);

	// ---------- the array put back on the schedule from the operations terminal (story/arrayBack.ts) ----------
	FS47ArrayDeps Ad;
	Ad.Dishes = Dishes.Get(); Ad.Audio = M->Audio;
	// (kept with Night Shift's part of the case: game.ns.array)
	Ad.Get = [Self] { return Self.IsValid() && Self->M->Story ? FS47ArraySave::Copy(Self->M->Story->Night().Array) : TOptional<FS47ArraySave>(); };
	Ad.Set = [Self](const FS47ArraySave& V) { if (Self.IsValid() && Self->M->Story) Self->M->Story->NightMutable().Array = V.ToJson(); };
	Ad.Clock = [Self] { return Self.IsValid() ? Self->Clock() : 0.0; };
	Ad.Log = Dd.Log;
	Ad.Note = Fd.Note;
	Ad.Toast = Dd.Toast;
	Ad.Save = Wd.Save;
	Ad.Hush = Dd.Hush;
	Ad.S03 = [Self]
	{
		if (!Self.IsValid() || !Self->M->Story) return;
		if (US47Chapter1* C1 = Cast<US47Chapter1>(Self->M->Story->Chapter(TEXT("ch1")))) C1->RefileS03();
		if (Self->M->Board.Refresh) Self->M->Board.Refresh();
	};
	Ad.Field = [Self](TOptional<double> K, bool bRed) { if (Self.IsValid()) Self->Dishes->FieldLights(K, bRed); };
	ArrayBack = MakeShared<FS47ArrayBack>(Ad);

	// ---------- the hooks main.ts gives the story and the night ----------
	if (AS47Story* Host = M->Story)
	{
		// the console offers it once Ward has asked (or from chapter three, or once it has been done)
		auto Offered = [Self]
		{
			if (!Self.IsValid() || Self->M->Mode != US47Main::EMode::Play || !Self->AtCh(2) || Self->Phase() == TEXT("ch6")) return false;
			const US47Chapter2* C2 = Cast<US47Chapter2>(Self->M->Story->Chapter(TEXT("ch2")));
			return (C2 && C2->S.ArrayAsk.Get(false)) || Self->AtCh(3) || Self->ArrayBack->Count() > 0;
		};
		FS47OpsMore More;
		More.Run = [Self](const FString& C) { return Self.IsValid() ? Self->ArrayBack->Command(C, Self->ArrayGate()) : TOptional<FString>(); };
		More.Buttons = [Offered] { return Offered() ? TArray<FString>{ TEXT("ARRAY RESUME") } : TArray<FString>(); };
		More.Help = { TEXT("  ARRAY RESUME       ARRAY TO SCHEDULE") };
		Host->Ops().More = More;
		Host->ArrayStatus = [Self] { return Self.IsValid() ? Self->ArrayBack->Status() : TOptional<FS47ArrayStatus>(); };
		Host->ArrayText = [Self] { return Self.IsValid() ? Self->ArrayBack->StatusText() : FString(); };
		Host->DishSpinning = [Self](int32 I) { return Self.IsValid() && Self->Dishes->Dishes.IsValidIndex(I) && Self->Dishes->Dishes[I].Spin > 0.05; };
		Host->DishRogue = [Self](int32 I) { return Self.IsValid() && Self->Dishes->Dishes.IsValidIndex(I) && (Self->Dishes->Dishes[I].bRogue || Self->Dishes->Dishes[I].bBack); };
		if (US47Chapter1* C1 = Cast<US47Chapter1>(Host->Chapter(TEXT("ch1"))))
		{
			C1->Dark = [Self] { return Self.IsValid() && Self->Dark->Dark(); };
			C1->S03Since = [Self]() -> TOptional<FS47S03Since>
			{
				if (!Self.IsValid()) return {};
				const TOptional<FS47ArrayBack::FHistory> H = Self->ArrayBack->History();
				return H ? TOptional<FS47S03Since>(FS47S03Since{ H->Cmds, H->Text }) : TOptional<FS47S03Since>();
			};
			// main.ts ch1.onEnd: if SARO is going dark just then, the chapter waits for the light and the telex
			TFunction<void(const FString&)> Before = C1->OnEnd;
			TWeakObjectPtr<AS47Story> Hw = Host;
			TSharedRef<TFunction<void(const FString&)>> End = MakeShared<TFunction<void(const FString&)>>();
			*End = [Self, Before, Hw, End](const FString& Method)
			{
				TWeakPtr<TFunction<void(const FString&)>> Again = End;
				if (Self.IsValid() && Hw.IsValid() && Self->Dark->Busy() && Hw->ChapterKey == TEXT("ch1"))
				{
					Hw->After(0.25, [Again, Method] { if (const TSharedPtr<TFunction<void(const FString&)>> E = Again.Pin()) (*E)(Method); });
					return;
				}
				if (Before) Before(Method);
			};
			C1->OnEnd = [End](const FString& Method) { (*End)(Method); };
		}
		if (US47Chapter2* C2 = Cast<US47Chapter2>(Host->Chapter(TEXT("ch2")))) C2->ArrayTried = [Self] { return Self.IsValid() && (Self->ArrayBack->Count() > 0 || Self->ArrayBack->Busy()); };
	}
	if (US47Night* N = M->Night)
	{
		FS47NightHooks& H = N->Hooks;
		H.LineLightsOn = [Self] { return Self.IsValid() && Self->LineLights->State != TEXT("none"); };
		H.ReelTapeBusy = [Self] { return Self.IsValid() && Self->ReelTape->bBusy; };
		H.SaroDarkBusy = [Self] { return Self.IsValid() && Self->Dark->Busy(); };
		// chapter one's 47 is the story's own until it has come or its time is past (story/saroDark.ts)
		H.SaroDarkKeep = [Self] { return Self.IsValid() && Self->M->Story && !Self->M->Story->Night().Hondo && Self->Dark->Phase == TEXT("idle") && FS47SaroDark::Pending(Self->Clock(), Self->M->Story->bEveningTimeline); };
		H.ArrayBackBusy = [Self] { return Self.IsValid() && Self->ArrayBack->Busy(); };
		H.RecWalkBusy = [Self] { return Self.IsValid() && Self->RecWalk->Busy(); };
		H.RecWalkWaiting = [Self] { return Self.IsValid() && Self->RecWalk->bWaiting; };
	}
	TestFlags();
}

void US47SaroNight::BeginDestroy()
{
	Dsp.Reset();
	Super::BeginDestroy();
}

// ---------- the deps main.ts reads ----------

FString US47SaroNight::Phase() const { return M->Story ? M->Story->ChapterKey : FString(TEXT("prologue")); }
int32 US47SaroNight::ChapterNo() const { const FString K = Phase(); return K.StartsWith(TEXT("ch")) ? FCString::Atoi(*K.Mid(2)) : 0; }
double US47SaroNight::Clock() const { return M->Story ? M->Story->Clock() : 0; }
bool US47SaroNight::Cinematic() const { return M->Story && M->Story->IsCinematic(); }
bool US47SaroNight::Driving() const { return M->World && M->World->bDriving; }
FString US47SaroNight::Area() const { return M->World ? M->World->Area : M->Web ? M->Web->Area() : FString(TEXT("saro")); }
FS47Cam US47SaroNight::Camera() const { return FS47Cam::Of(M->Player, Area() == TEXT("roswell") ? 12000 : 5000); }
FVector US47SaroNight::Eye() const { return M->Player ? FVector(M->Player->Pos.X, M->Player->FloorY + M->Player->Eye, M->Player->Pos.Z) : FVector::ZeroVector; }
bool US47SaroNight::Inside(const TCHAR* Rect, const FVector& P) const { return M->Web && M->Web->Rect(Rect).Contains(P); }
double US47SaroNight::Dawn() const { return M->Night ? M->Night->Dawn() : 0; }
bool US47SaroNight::Calm() const { return M->Flow && M->Flow->bCalmFlash; }
bool US47SaroNight::AbductOn() const { return !M->Flow || M->Flow->bAbductOn; }
FString US47SaroNight::AbStage() const
{
	// (the developer's warning for the dishes, -S47Follow, comes before the time away's own)
	if (TestWarn) return TEXT("warning");
	return Abduct.Stage ? Abduct.Stage() : FString(TEXT("calm"));
}
FString US47SaroNight::SceneId() const
{
	// ab.sceneId: the area, the diner's and the road's together
	const FString A = Area();
	return A == TEXT("diner") || A == TEXT("road") ? FString(TEXT("road")) : A;
}
double US47SaroNight::FloorAt(double X, double Z) const
{
	// player.floor: the area's ground
	const FString A = Area();
	if (A == TEXT("road") || A == TEXT("diner")) { const FVector O = S47Legs::ORIGINS(TEXT("road")); return S47RoadTerrain::SurfaceY(X - O.X, Z - O.Z) + O.Y; }
	if (A == TEXT("station01")) return S47Legs::StationGroundAt(X, Z);
	return US47Night::SaroFloorAt(M->Web, X, Z);
}
void US47SaroNight::Note(const FString& Text) const { if (M->Story) M->Story->Note(Text); UE_LOG(LogS47Saro, Display, TEXT("S47: noted: %s"), *Text); }
void US47SaroNight::Autosave() const { M->RequestAutosave(false); }

double US47SaroNight::BandShine(const FVector& At) const
{
	// how much light from near the eye falls on a point (the reflective band on a SARO jacket gives it
	// back the way it came): the flashlight's beam on foot, the headlights in the truck
	const FS47Truck* Tr = Driving() && M->World ? M->World->TruckHere() : nullptr;
	const FS47Cam C = Camera();
	const FVector From = Tr ? Tr->Position : C.Pos;
	const FVector Dir = Tr ? S47Three::Euler(Tr->Rotation.X, Tr->Rotation.Y, Tr->Rotation.Z).RotateVector(FVector(0, 0, -1)) : C.Forward;
	if (!Tr && !(M->Night && M->Night->Flashlight->bOn)) return 0;
	const FVector To = At - From;
	const double Dd = To.Size(), A = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Dir.GetSafeNormal(), To.GetSafeNormal()), -1.0, 1.0)));
	const double Cone = Tr ? 16 : 15, Near = Tr ? 150 : 110, Far = Tr ? 280 : 220;
	return (1 - Smoothstep(A, Cone * 0.45, Cone)) * (1 - Smoothstep(Dd, Near, Far));
}

TOptional<FS47DishesSave> US47SaroNight::GetDishes() const { return Abduct.GetDishes ? Abduct.GetDishes() : DishesKept; }
void US47SaroNight::SetDishes(const FS47DishesSave& V) { if (Abduct.SetDishes) Abduct.SetDishes(V); else DishesKept = V; }

FString US47SaroNight::DarkWhere() const
{
	// where the player is for it: the photo lab, out in the yard and the grounds close by, the control room
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	if (Area() != TEXT("saro") || Driving()) return TEXT("away");
	if (Inside(TEXT("yard.zone.lab"), P) || Inside(TEXT("yard.zone.labDoor"), P)) return TEXT("lab");
	if (Inside(TEXT("room.bounds"), P)) return TEXT("room");
	// the annex: not out where the night is
	for (const TCHAR* B : { TEXT("room.bounds"), TEXT("annex.zone.southDoor"), TEXT("annex.zone.corridor"), TEXT("annex.zone.recordsDoor"), TEXT("annex.zone.records") }) if (Inside(B, P)) return TEXT("away");
	return FMath::Sqrt(FMath::Square(P.X - 6) + FMath::Square(P.Z + 2)) < 34 && !OnHighway(P) ? TEXT("yard") : TEXT("away");
}

FS47DarkIn US47SaroNight::DarkIn(bool bOk, bool bModal) const
{
	const US47Chapter1* C1 = M->Story ? Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1"))) : nullptr;
	FS47DarkIn S;
	S.bOk = bOk && Phase() == TEXT("ch1") && C1 && C1->Active && C1->S.Stage != TEXT("complete") && M->Story && !M->Story->Night().Hondo && !M->OneAtATime()
		&& !(M->Night && M->Night->Curve->Long.Contains(TEXT("ch1")));
	S.bHold = bModal || Cinematic() || (M->Audio && M->Audio->PhonePlaying());
	S.Where = DarkWhere(); S.Clock = Clock(); S.Cam = Camera(); S.Zoom = Zoom ? Zoom() : 0;
	S.bTorch = M->Night && M->Night->Flashlight->bHave && M->Night->Flashlight->bOn;
	AS47Web* W = M->Web;
	S.Open = [W](const FString& D) { return W && W->DoorStates().FindRef(D); };
	// (the truck where it is parked at SARO: yard.truckCol)
	if (M->World && M->World->TruckAt == TEXT("saro") && W) { const FS47Area R = W->Rect(TEXT("yard.truckCol")); if (R.bValid) S.Blocks.Add({ R.MinX, R.MaxX, R.MinZ, R.MaxZ }); }
	return S;
}

FString US47SaroNight::ArrayGate() const
{
	// why the console will not put the array back now (empty: it will): before 02:15 there is nothing
	// to resume; until Ward has asked it is held for her; not while the dishes move by themselves,
	// nor while one of the night's things is going (one at a time)
	if (!M->Story || !M->Story->At(ES47Phase::Event)) return TEXT("early");
	const US47Chapter2* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2")));
	if (!AtCh(2) || Phase() == TEXT("ch6") || (Phase() == TEXT("ch2") && !(C2 && C2->S.ArrayAsk.Get(false)))) return TEXT("release");
	if ((M->Night && M->Night->Hooks.AbductionBusy && M->Night->Hooks.AbductionBusy()) || Follow->Mode != TEXT("idle") || Is(TipMoving)) return TEXT("moving");
	// (tipped to the horizon by the archive's phone: they are its until they are back at 32, story/archiveDishes.ts)
	if (Is(TipHolds)) return TEXT("tipped");
	if (M->OneAtATime() || (M->Audio && M->Audio->PhonePlaying()) || Is(CbBusy)) return TEXT("busy");
	return FString();
}

// ---------- step() ----------

void US47SaroNight::StepDark(double Dt, bool bModal)
{
	if (M->Mode != US47Main::EMode::Play) return;
	// 02:29: SARO goes dark (story/saroDark.ts), on its own clock; one at a time
	Dark->bEveningTimeline = M->Story && M->Story->bEveningTimeline;
	Dark->Update(Dt, DarkIn(!Cinematic() && Dawn() < 0.3, bModal));
	// a night that never went dark still has Hondo Valley's telex, printed at 02:29 with nobody there
	if (M->Story && !M->Story->Night().Hondo && Dark->Phase == TEXT("idle") && AtCh(1))
	{
		// (not before its minute: a chapter one done early has chapter two before 02:29)
		const double C = FMath::Fmod(FMath::Fmod(Clock(), 86400.0) + 86400.0, 86400.0);
		const double Filed = FS47SaroDark::FILED - (M->Story->bEveningTimeline ? 2 * 3600 : 0);
		if (C >= Filed && C < 12 * 3600 && (Phase() != TEXT("ch1") || C >= Filed + 11 * 60))
		{
			M->Story->HondoTelex(Filed, Filed + 14);
			UE_LOG(LogS47Saro, Display, TEXT("S47: saro dark never came: Hondo Valley's telex printed at 02:29 with nobody there"));
		}
	}
}

void US47SaroNight::StepLights(double Dt, bool bModal)
{
	if (M->Mode != US47Main::EMode::Play) return;
	ControlLightsTest(Dt);
	FS47LineLightsIn S;
	S.bPaused = bModal || (M->UI && M->UI->Modal()) || Cinematic(); S.Camera = Camera();
	S.bOk = !bModal && !Cinematic() && AtCh(1) && Phase() != TEXT("ch6") && Dawn() < 0.3 && !(M->Night && M->Night->Hooks.AbductionBusy && M->Night->Hooks.AbductionBusy()) && !ArrayBack->Busy();
	if (Driving() && Area() == TEXT("saro")) if (const FS47Truck* Tr = M->World->TruckHere()) S.Truck = Tr->Position;
	// (the developer's drive past them: a truck going south down the highway from the turn-off)
	if (TestTruck) { if (!S.bPaused) TestTruck->Z += 12 * Dt; S.Truck = *TestTruck; if (TestTruck->Z > 430) TestTruck.Reset(); }
	S.Clock = Clock();
	AS47Web* W = M->Web;
	S.Ground = [W](double X, double Z) { return US47Night::SaroFloorAt(W, X, Z); };
	if (M->Night) S.May = M->Night->Curve->May(TEXT("event"));
	if (TestTruck) S.May.Reset();
	LineLights->Update(Dt, S);
}

void US47SaroNight::StepArray(double Dt, bool bModal)
{
	if (M->Mode != US47Main::EMode::Play) return;
	bArrayStepped = true;
 if(M->Spine && M->Spine->ArrayBusy())return; // SC08 owns its three actual encoders; the reel and lamps still frame.
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	// the array put back on the schedule (story/arrayBack.ts): its time stops while a panel is open,
	// as the night's does; before the time away, which may not begin while it runs
	{
		const bool bInSaro = Area() == TEXT("saro"), bOnFoot = bInSaro && !Driving();
		FS47ArrayIn S;
		S.bSaro = bInSaro; S.bRoom = bOnFoot && Inside(TEXT("room.bounds"), P); S.bOutside = bOnFoot && M->Space == TEXT("yard"); S.bCab = bInSaro && Driving();
		S.Cam = Camera(); S.Eye = Eye();
		S.bWarning = AbStage() != TEXT("calm");
		if (Follow->Mode != TEXT("idle")) { TSet<FString> Ids; for (const FS47DishesFollow::FState& St : Follow->State()) Ids.Add(St.Id); S.Taken = Ids; }
		S.bFollowIdle = Follow->Mode == TEXT("idle");
		S.bCalm = Calm();
		ArrayBack->Update(bModal ? 0 : Dt, S);
	}
}

void US47SaroNight::StepLate(double Dt, bool bModal)
{
	if (M->Mode != US47Main::EMode::Play) return;
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	// (the array: US47Away steps it before the time away, as main.ts does; here only without it)
	if (!bArrayStepped) StepArray(Dt, bModal);
	bArrayStepped = false;
	// the notebook or a paper open holds the warning where it is, as it holds the clock (and the dishes wait too)
	if (TestWarn && !bModal) { *TestWarn += Dt; if (*TestWarn >= S47Follow::WARN) TestWarn.Reset(); }
	if (!(bModal && AbStage() == TEXT("warning")))
	{
		// the recording, where the player was taken: once, later, within sight, in a build or a peak
		// (it belongs to the time away: not with Missing time turned off, and turned off it is not there)
		if (!AbductOn() && RecWalk->Busy()) RecWalk->Reset();
		if (!bModal)
		{
			FS47RecordingIn S;
			S.bOk = AbductOn() && !Cinematic() && (Phase() == TEXT("ch3") || Phase() == TEXT("ch4") || Phase() == TEXT("ch5")) && AbStage() == TEXT("calm");
			S.bMay = M->Night && M->Night->Curve->May(TEXT("scare")) && !M->OneAtATime();
			S.Scene = SceneId();
			S.Pos = Driving() && M->World->CabTruck() ? M->World->CabTruck()->Position : P;
			S.bOut = Driving() || M->Space == TEXT("yard");
			S.Clock = Clock();
			if (!RecWalk->Busy()) bTestWalk = false;
			// -S47RecWalk=near: abduction.recordings().filter((x) => x.path && x.scene === sc), the nearest
			if (bWalkNear && !RecWalk->Busy() && AbStage() == TEXT("calm") && Abduct.Recordings)
			{
				TSharedPtr<FS47Rec> Best;
				for (const TSharedPtr<FS47Rec>& R : Abduct.Recordings())
					if (R->Path.Num() && R->Scene == S.Scene && (!Best || FVector::DistSquared(R->Take, S.Pos) < FVector::DistSquared(Best->Take, S.Pos))) Best = R;
				if (Best)
				{
					bWalkNear = false; TestRec = Best;
					RecWalk->Begin(Best, Clock());
					UE_LOG(LogS47Saro, Display, TEXT("S47: recording walks time away %d at %.0f m"), Best->N, FVector::Dist(Best->Take, S.Pos));
				}
			}
			// (the developer's walk goes whatever the chapter: its own scene and time away)
			if (TestRec.IsValid() && RecWalk->Busy()) S.bOk = true;
			RecWalk->Update(Dt, S);
		}
		// the eyes from where the player stands now (the dishes have one owner at a time: while the
		// archive's phone has them tipped, the warning's dishes do not begin)
		if(!(M->Spine && M->Spine->ArrayBusy()))
  {
		FS47FollowIn F;
		F.Stage = AbStage();
		if (TestWarn) F.Warning = FS47FollowIn::FWarn{ *TestWarn, S47Follow::WARN };
		else if (Abduct.Warn) F.Warning = Abduct.Warn();
		F.bSaro = Area() == TEXT("saro") && (Follow->Mode != TEXT("idle") || !Is(TipHolds));
		F.bOnFoot = !Driving();
		F.Eye = Eye(); F.View = Camera().Forward;
		F.Clock = Clock();
		if (Abduct.BackAt) F.BackAt = Abduct.BackAt();
		F.bCalm = Calm();
		F.Woke = Abduct.Last ? Abduct.Last() : 0;
		F.bOver = Abduct.Over && Abduct.Over();
		Follow->Update(Dt, F);
  }
	}
	else Follow->Wait();
	// the corner clock in seconds while the warning counts its 47 (abduction.warnClock ?? saroDark.clockShow ?? archive.clockShow ?? ch6.clockShow)
	FString Secs = ClockBefore ? ClockBefore() : FString();
	if (Secs.IsEmpty() && Dark->ClockShow) Secs = Dark->ClockShow->Hm + TEXT(":") + Dark->ClockShow->Ss;
	if (Secs.IsEmpty() && ClockAfter) Secs = ClockAfter();
	// (only while one of these has it, or as it lets go: the others' ports set it the same way)
	if (!Secs.IsEmpty() || bClockShown) { M->UI->ClockSeconds(Secs); bClockShown = !Secs.IsEmpty(); }
	// (the developer's look at the figure once it stands: the eyes turned to its head)
	if (bDarkLook && M->Player) if (const TOptional<FVector> Fig = Dark->FigureAt())
	{
		bDarkLook = false;
		const FVector D = *Fig - Camera().Pos;
		M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = FMath::Atan2(D.Y, FMath::Sqrt(D.X * D.X + D.Z * D.Z));
		UE_LOG(LogS47Saro, Display, TEXT("S47: the eyes on the figure, %.1f m"), D.Size());
	}
	// reel 047 for the developer's run (the archive's phone steps it, story/archivePhone.ts)
	if (bReelTest) { Reel047->Update(bModal ? 0 : Dt); if (!Reel047->bPlaying) bReelTest = false; }
	LogStates();
}

void US47SaroNight::Frame(double Dt)
{
	// ext.update: the dishes' red lamps where their feeds are and in their blink (the dishes themselves
	// are drawn by AS47Web); the sky light as the eyes open (room.lights.hemi = hemiBase * saroDark.adapt)
	Dishes->Update(M->T, Camera().Pos);
	if (M->Web) M->Web->SetHemiScale(float(Dark->Adapt));
	// Earl's reel keeps its own time (window.setTimeout(step, 40))
	ReelTape->Update();
	// the lights on B keep four pixels across
	FVector2D Size(1920, 1080);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	LineLights->Draw(Camera(), Size.Y);
}

void US47SaroNight::LogStates()
{
	// the log, for the tests and the screenshot runs
	const FString Fig = Dark->Figure ? Dark->Figure->Id + (Dark->FigureAt() ? TEXT(" standing") : TEXT(" gone ") + Dark->Figure->Gone) : FString();
	if (Fig != LoggedFigure) { LoggedFigure = Fig; if (!Fig.IsEmpty()) UE_LOG(LogS47Saro, Display, TEXT("S47: saro dark figure %s%s"), *Fig, Dark->Figure->bLooked ? TEXT(", looked at") : TEXT("")); }
	if (Dark->Phase != LoggedDark) { LoggedDark = Dark->Phase; UE_LOG(LogS47Saro, Display, TEXT("S47: saro dark %s%s"), *LoggedDark, Dark->Figure ? *FString::Printf(TEXT(" (the figure %s%s)"), *Dark->Figure->Id, Dark->Figure->Gone.IsEmpty() ? TEXT("") : *(TEXT(", gone: ") + Dark->Figure->Gone)) : TEXT("")); }
	// (-S47ShotLit=<file>: a screenshot the first time the array's lamps flash, the figure stands, or the lights on B are up)
	const bool bLitNow = (ArrayBack->bLit && ArrayBack->Flashes == 2) || (Fig.EndsWith(TEXT("standing")) && Dark->T > 9) || (LineLights->State == TEXT("pace") && LineLights->Opacity() > 0.9);
	if (bLitNow && !bShotLit)
	{
		bShotLit = true;
		FString Shot;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47ShotLit="), Shot)) { FScreenshotRequest::RequestScreenshot(Shot, true, false); UE_LOG(LogS47Saro, Display, TEXT("S47: screenshot of it lit, %s"), *Shot); }
	}
	if (ArrayBack->Stage != LoggedArray) { LoggedArray = ArrayBack->Stage; UE_LOG(LogS47Saro, Display, TEXT("S47: array %s (%d flashes, %d tonight)"), *LoggedArray, ArrayBack->Flashes, ArrayBack->Count()); }
	if (Follow->Mode != LoggedFollow) { LoggedFollow = Follow->Mode; UE_LOG(LogS47Saro, Display, TEXT("S47: dishes follow %s (%d dishes)"), *LoggedFollow, Follow->State().Num()); }
	if (LineLights->State != LoggedLights) { LoggedLights = LineLights->State; UE_LOG(LogS47Saro, Display, TEXT("S47: line lights %s"), *LoggedLights); }
	if (RecWalk->Phase != LoggedWalk) { LoggedWalk = RecWalk->Phase; UE_LOG(LogS47Saro, Display, TEXT("S47: recording %s%s"), *LoggedWalk, RecWalk->Ended.IsEmpty() || LoggedWalk != TEXT("idle") ? TEXT("") : *(TEXT(", ended ") + RecWalk->Ended)); }
}

// ---------- what the night did, and whether it was seen (story/seen.ts) ----------

TOptional<FVector> US47SaroNight::FollowDish() const
{
	// the nearest of the dishes that have started to follow, as seen from here (its dish)
	if (Follow->Mode == TEXT("idle") || Follow->Mode == TEXT("home")) return {};
	const FS47Cam C = Camera();
	TOptional<FVector> Best; double Off = 1e18;
	for (const FS47DishesFollow::FState& R : Follow->State())
	{
		if (!R.bOn) continue;
		const int32 I = Dishes->IndexOf(R.Id);
		if (I < 0) continue;
		const FS47Dish& D = Dishes->Dishes[I];
		const FVector P(D.X, 5.5 * D.Scale, D.Z);
		const double A = FMath::Acos(FMath::Clamp(FVector::DotProduct(C.Forward.GetSafeNormal(), (P - C.Pos).GetSafeNormal()), -1.0, 1.0));
		if (A < Off) { Off = A; Best = P; }
	}
	return Best;
}

void US47SaroNight::SeenSync()
{
	Was.Dark = Dark->Phase; Was.bDarkFig = Dark->FigureAt().IsSet(); Was.bArray = ArrayBack->Busy(); Was.Follow = Follow->Mode; Was.Lights = LineLights->State;
	Was.bReel = ReelTape->bBusy; Was.bR47 = Reel047->bPlaying; Was.Walk = RecWalk->Phase;
	DarkEntry.Reset(); ArrayEntry.Reset(); ReelEntry.Reset(); R47Entry.Reset(); WalkEntry.Reset();
}

void US47SaroNight::SeenStep()
{
	US47Night* N = M->Night;
	if (!N) return;
	TWeakObjectPtr<US47SaroNight> Self = this;
	// the recording walking the player's last 47 seconds where they were taken (story/recording.ts):
	// written down when it starts (where the player is not looking), looked at once the eyes are on
	// it; how it ended: walked up to ('near'), gone through ('done'), or left ('left')
	if (RecWalk->Phase != Was.Walk)
	{
		if (Was.Walk == TEXT("idle") && RecWalk->Phase == TEXT("walk"))
		{
			FS47SawOpts O; O.At = RecWalk->Spot(); O.bLookingSet = true; O.Looking = false;
			FS47SeenWatch W; W.Pos = [Self] { return Self.IsValid() ? Self->RecWalk->At() : TOptional<FVector>(); }; W.Secs = 400; W.Deg = 18; W.Range = 320;
			W.Until = [Self] { return !Self.IsValid() || !Self->RecWalk->Busy(); };
			O.Watch = W;
			WalkEntry = N->Saw(TEXT("scare"), FString::Printf(TEXT("walk %d"), RecWalk->N), O);
		}
		if (RecWalk->Phase == TEXT("idle") && WalkEntry.IsValid()) { WalkEntry->End = RecWalk->Ended.IsEmpty() ? FString(TEXT("away")) : RecWalk->Ended; WalkEntry.Reset(); }
		Was.Walk = RecWalk->Phase;
	}
	if (Follow->Mode != Was.Follow)
	{
		if (Was.Follow == TEXT("idle") && Follow->Mode == TEXT("follow"))
		{
			// (the near ones, that come round first and are on the player within seconds)
			TArray<FString> Near;
			for (const FS47DishesFollow::FState& R : Follow->State()) if (R.Tier == 0) Near.Add(R.Id);
			const int32 First = Near.Num() ? Dishes->IndexOf(Near[0]) : -1;
			FS47SawOpts O;
			O.At = FollowDish();
			if (!O.At && First >= 0) O.At = FVector(Dishes->Dishes[First].X, 5.5 * Dishes->Dishes[First].Scale, Dishes->Dishes[First].Z);
			FS47SeenWatch W; W.Pos = [Self] { return Self.IsValid() ? Self->FollowDish() : TOptional<FVector>(); }; W.Secs = 180; W.Range = 320;
			W.Until = [Self] { return !Self.IsValid() || Self->Follow->Mode == TEXT("idle") || Self->Follow->Mode == TEXT("home"); };
			O.Watch = W;
			N->Saw(TEXT("dishes"), Near.Num() ? FString::Join(Near, TEXT(" ")) : FString(TEXT("the array")), O);
		}
		Was.Follow = Follow->Mode;
	}
	// LineLights owns its visibility/occlusion test. An offscreen occurrence is not a seen event;
	// the observation callback above writes exactly once, without a geometry-blind SeenLog watch.
	Was.Lights = LineLights->State;
	// Earl's reel from 9/22/81 (core/reelTape.ts): only heard, at the studio door; how it ended,
	// heard to the end or walked away from
	if (ReelTape->bBusy != Was.bReel)
	{
		if (ReelTape->bBusy) { FS47SawOpts O; O.bLookingSet = true; ReelEntry = N->Saw(TEXT("event"), TEXT("reel"), O); }
		else if (ReelEntry.IsValid()) { ReelEntry->End = M->TownSeen.Contains(TEXT("ln:reel")) ? TEXT("heard") : TEXT("away"); ReelEntry.Reset(); }
		Was.bReel = ReelTape->bBusy;
	}
	// 02:29, SARO dark (story/saroDark.ts): heard and felt, written when K3 goes; how it ended (the
	// figure in the dark looked at, or not); the figure itself as a scare when it stands
	if (Dark->Phase != Was.Dark)
	{
		if (Was.Dark == TEXT("idle") && Dark->Phase == TEXT("relay"))
		{
			FS47SawOpts O; O.At = M->Story ? M->Story->RackAt() : FVector::ZeroVector; O.Line = FString(); O.bLookingSet = true;
			DarkEntry = N->Saw(TEXT("event"), TEXT("blackout"), O);
		}
		if (Dark->Phase == TEXT("done") && DarkEntry.IsValid()) { DarkEntry->End = Dark->Figure && Dark->Figure->bLooked ? TEXT("figure") : Dark->Figure ? TEXT("figure unseen") : TEXT("dark"); DarkEntry.Reset(); }
		Was.Dark = Dark->Phase;
	}
	const TOptional<FVector> Fig = Dark->FigureAt();
	if (Fig.IsSet() != Was.bDarkFig)
	{
		if (Fig)
		{
			FS47SawOpts O; O.At = Fig; O.Line = FString();
			FS47SeenWatch W; W.Pos = [Self] { return Self.IsValid() ? Self->Dark->FigureAt() : TOptional<FVector>(); }; W.Secs = 47; W.Deg = 22; W.Range = 90;
			O.Watch = W;
			N->Saw(TEXT("scare"), TEXT("figure, dark"), O);
		}
		Was.bDarkFig = Fig.IsSet();
	}
	// reel 047 (story/reel047.ts): only heard
	if (Reel047->bPlaying != Was.bR47)
	{
		if (Reel047->bPlaying) { FS47SawOpts O; O.Line = FString(); O.bLookingSet = true; O.bNoWatch = true; R47Entry = N->Saw(TEXT("event"), TEXT("reel047"), O); }
		else if (R47Entry.IsValid()) { R47Entry->End = Reel047->bFinished ? TEXT("heard") : TEXT("away"); R47Entry.Reset(); }
		Was.bR47 = Reel047->bPlaying;
	}
	// the array put back on the schedule from the console (story/arrayBack.ts): written down as the
	// command goes in, felt (the curve lands on it); looked at if the array was in view while it ran
	if (ArrayBack->Busy() != Was.bArray)
	{
		if (ArrayBack->Busy())
		{
			FS47SawOpts O; O.At = FS47ArrayBack::Centre(); O.bLookingSet = true; O.Looking = false;
			FS47SeenWatch W;
			W.Pos = [Self]() -> TOptional<FVector>
			{
				if (!Self.IsValid()) return {};
				const FVector P = Self->M->Player ? Self->M->Player->Pos : FVector::ZeroVector;
				return Self->ArrayBack->Busy() && (Self->M->Space == TEXT("yard") || Self->Inside(TEXT("room.bounds"), P)) ? TOptional<FVector>(FS47ArrayBack::Centre()) : TOptional<FVector>();
			};
			W.Secs = 150; W.Range = 700; W.Deg = 55;
			W.Until = [Self] { return !Self.IsValid() || !Self->ArrayBack->Busy(); };
			O.Watch = W;
			ArrayEntry = N->Saw(TEXT("event"), TEXT("array"), O);
		}
		else if (ArrayEntry.IsValid()) { ArrayEntry->End = TEXT("back"); ArrayEntry.Reset(); }
		Was.bArray = ArrayBack->Busy();
	}
}

// ---------- the saves (main.ts startGame, leavePlay, toTitle, writeSave) ----------

void US47SaroNight::Restore(const S47J::FObj& State)
{
	RecWalk->Reset();
	// (AbductSave.dishes, kept here while the time away is not ported)
	DishesKept = FS47DishesSave::Copy(S47J::Obj(S47J::Obj(State, TEXT("abduct")), TEXT("dishes")));
	LineLights->Restore(S47J::Obj(State, TEXT("lineLights")), M->TownSeen.Contains(TEXT("ev:lights")));
	Dark->Halt();
	TestWarn.Reset(); TestTruck.Reset(); TestRec.Reset(); bTestWalk = false; bReelTest = false;
}

void US47SaroNight::Restored()
{
	// (after the case is back: the telex had, or not yet)
	Dark->Restore(M->Story && M->Story->Night().Hondo.IsSet());
	// after the log is back (game.start): a save made while the dishes were out
	Follow->Restore();
	// and one made while the array was on the schedule (story/arrayBack.ts)
	ArrayBack->Restore();
	ReelTape->Stop();
}

void US47SaroNight::Leave(bool bTitle)
{
	Dark->Halt();   // (and the power back, before the room is put back as the night began)
	Follow->Reset(); ArrayBack->Reset(); RecWalk->Reset(); bTestWalk = false;
	LineLights->Reset(LineLights->bDone);
	if (!bTitle) ReelTape->Stop();
	// (reel 047 stopped and the processor's display its own: archive.halt does it in the web version,
	// story/archivePhone.ts, whose port has the deck after this)
	Reel047->Reset();
	TestWarn.Reset(); TestTruck.Reset(); bReelTest = false;
}

void US47SaroNight::Save(const S47J::FObj& State) const
{
	S47J::Set(State, TEXT("lineLights"), LineLights->Save());
	// AbductSave.dishes: the time away's part of the state, which its own port writes once it is in
	if (Abduct.GetDishes || !DishesKept) return;
	S47J::FObj Ab = S47J::Obj(State, TEXT("abduct"));
	if (!Ab.IsValid()) { Ab = S47J::New(); State->SetObjectField(TEXT("abduct"), Ab); }
	Ab->SetObjectField(TEXT("dishes"), DishesKept->ToJson());
}

FString US47SaroNight::SaveBlock() const
{
	// (the array on the schedule and going back: 47 seconds and the rest of it, story/arrayBack.ts)
	if (ArrayBack->Busy()) return BUSY;
	// (nor while SARO goes dark: the relay, the 47 seconds, the telex)
	if (Dark->Busy()) return BUSY;
	return FString();
}

void US47SaroNight::ShowDsp(bool bOn)
{
	// the processor's display: the reel's picture, or the processor's own canvas again
	if (!M->Web || DspMat < 0 || !DspTarget) return;
	const FS47Mat Mt{ M->Web.Get(), DspMat };
	UMaterialInstanceDynamic* D = M->Web->SharedMid(DspMat);
	if (bOn)
	{
		if (!DspWas && D) DspWas = D->K2_GetTextureParameterValue(TEXT("Map"));
		Mt.SetMap(DspTarget, true);
	}
	else if (DspWas) Mt.SetMap(DspWas, true);
}

// ---------- the developer menu's, for the tests ----------

void US47SaroNight::TestFlags()
{
	// -S47Dark[=dark]: SARO goes dark from where the player is (chapter one; =dark: forty seconds of the
	// relay passed); -S47Array: ARRAY RESUME as the developer menu gives it; -S47Follow: a warning of
	// the time away played here (the abduction is its own port) with the player near the array;
	// -S47LineLights: a truck driven south past the turn-off; -S47Reel047, -S47ReelTape: the reels;
	// -S47RecWalk: a time away 90 m off with its 47 s kept, and the recording walks it; -S47RecWalk=near: the
	// developer menu's "The recording walks (the nearest time away)", once the time away has one here
	const TCHAR* Cmd = FCommandLine::Get();
	FString DarkArg;
	const bool bDark = FParse::Value(Cmd, TEXT("S47Dark="), DarkArg) || FParse::Param(Cmd, TEXT("S47Dark"));
	const bool bArray = FParse::Param(Cmd, TEXT("S47Array")), bFollow = FParse::Param(Cmd, TEXT("S47Follow")), bLights = FParse::Param(Cmd, TEXT("S47LineLights"));
	const bool bR47 = FParse::Param(Cmd, TEXT("S47Reel047")), bTape = FParse::Param(Cmd, TEXT("S47ReelTape"));
	FString WalkArg;
	FParse::Value(Cmd, TEXT("S47RecWalk="), WalkArg);
	bWalkNear = WalkArg == TEXT("near");
	const bool bWalk = !bWalkNear && FParse::Param(Cmd, TEXT("S47RecWalk"));
	if (!(bDark || bArray || bFollow || bLights || bR47 || bTape || bWalk || bWalkNear) || !M->Web) return;
	TWeakObjectPtr<US47SaroNight> Self = this;
	FTimerHandle H;
	M->Web->GetWorldTimerManager().SetTimer(H, [Self, bDark, DarkArg, bArray, bFollow, bLights, bR47, bTape, bWalk]
	{
		if (!Self.IsValid()) return;
		US47SaroNight* S = Self.Get();
		if (bDark)
		{
			const bool bNow = S->Phase() == TEXT("ch1") && S->Dark->Now(S->DarkIn(true, false));
			if (bNow && (DarkArg == TEXT("dark") || DarkArg == TEXT("look"))) S->Dark->T = 39.5;
			S->bDarkLook = DarkArg == TEXT("look");
			UE_LOG(LogS47Saro, Display, TEXT("S47: saro dark asked for from %s: %s"), *S->DarkWhere(), bNow ? TEXT("going") : TEXT("only at SARO in chapter one, once"));
		}
		if (bArray)
		{
			const TOptional<FString> R = S->AtCh(1) && !S->ArrayBack->Busy() ? S->ArrayBack->Command(TEXT("ARRAY RESUME"), FString()) : TOptional<FString>();
			UE_LOG(LogS47Saro, Display, TEXT("S47: array asked for: %s"), R ? *R->Replace(TEXT("\n"), TEXT(" / ")) : TEXT("only after 02:15"));
		}
		if (bFollow)
		{
			// (follow.reset(); delete abduction.data.dishes: they may come again)
			S->DishesKept.Reset(); if (S->Abduct.ClearDishes) S->Abduct.ClearDishes(); S->Follow->Reset();
			S->TestWarn = FS47DishesFollow::FirstAt(S47Follow::WARN);
			UE_LOG(LogS47Saro, Display, TEXT("S47: a warning for the dishes (near the array: %s)"), FS47DishesFollow::NearArray(S->FieldZones, S->Dishes.Get(), S->Eye().X, S->Eye().Z) ? TEXT("yes") : TEXT("no"));
		}
		if (bLights)
		{
			S->LineLights->Reset(false);
			S->TestTruck = FVector(-24, 0, 110);
			UE_LOG(LogS47Saro, Display, TEXT("S47: a truck south down the highway past the turn-off"));
		}
		if (bR47)
		{
			S->bReelTest = true;
			S->Reel047->Play([](const FS47ReelCue& C) { UE_LOG(LogS47Saro, Display, TEXT("S47: reel 047 %s at %.1f s"), *C.Kind, C.At); });
		}
		if (bTape)
		{
			S->ReelTape->Play([](const FS47TapeCue& C) { UE_LOG(LogS47Saro, Display, TEXT("S47: reel tape %s at %.1f s"), *C.Kind, C.At); },
				[](bool bComplete) { UE_LOG(LogS47Saro, Display, TEXT("S47: reel tape %s"), bComplete ? TEXT("heard to the end") : TEXT("stopped")); });
		}
		if (bWalk)
		{
			// a time away about 60 m off in the direction the player looks, its 47 s standing first, then a walk to the
			// side, a stop, and a stumbling run (the abduction keeps the real ones)
			const FS47Cam C = S->Camera();
			const FVector F = FVector(C.Forward.X, 0, C.Forward.Z).GetSafeNormal(), Side(-F.Z, 0, F.X);
			const FVector Take = C.Pos + F * 60;
			const TSharedPtr<FS47Rec> R = MakeShared<FS47Rec>();
			R->N = 1; R->Scene = S->SceneId(); R->Take = FVector(Take.X, 0, Take.Z); R->Clock = S->Clock() - 1800;
			FVector At = R->Take;
			for (int32 I = 0; I < 189; ++I)
			{
				const double T = I * S47Recording::PATH_DT, Speed = T < 6 ? 0 : T < 20 ? 1.9 : T < 26 ? 0 : 3.0;
				const FVector Dir = T < 18 ? Side : (Side + F * 0.6).GetSafeNormal();
				if (I) At += Dir * Speed * S47Recording::PATH_DT;
				R->Path.Append({ At.X, At.Z, FMath::Atan2(-Dir.X, -Dir.Z) });
			}
			R->Fig = MakeShared<FS47JacketWalker>(S->M->Web, TEXT("abduct:record"));
			R->Fig->G->SetVisible(false); R->Fig->G->AddToScene();
			if (S->RecWalk->Busy()) S->RecWalk->Reset();
			S->TestRec = R; S->bTestWalk = true;
			S->RecWalk->Begin(R, S->Clock());
			UE_LOG(LogS47Saro, Display, TEXT("S47: recording walks at %.0f m"), FVector::Dist(C.Pos, R->Take));
		}
	}, 3.0f, false);
}

bool US47SaroNight::SelfTest(TArray<FString>& Lines)
{
	bool bOk = FS47DishArray::SelfTest(Lines);
	bOk &= FS47SaroDark::SelfTest(Lines);
	bOk &= FS47ArrayBack::SelfTest(Lines);
	bOk &= FS47DishesFollow::SelfTest(Lines);
	bOk &= FS47LineLights::SelfTest(Lines);
	bOk &= FS47Reel047::SelfTest(Lines);
	bOk &= FS47RecordingWalk::SelfTest(Lines);
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s saro night: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// world/saroPower.ts: SARO's lamps by where they stand, the sodium heads by their colour
	Check(FS47SaroPower::SaroAt(9, -8) && !FS47SaroPower::SaroAt(-20.1, -32) && !FS47SaroPower::SaroAt(-31.5, 682) && FS47SaroPower::IsSodium(1, 0.4564, 0.1022) && !FS47SaroPower::IsSodium(0.08, 0.87, 0.69),
		TEXT("SARO's own lamps: not the highway's, not the motel's sign; the sodium heads by colour"));
	// core/reelTape.ts: its plan without the radio (no call tonight): the deck, the click, the end
	FS47ReelTape Tape(FS47ReelTapeDeps{});
	const TArray<FS47TapeCue> P = Tape.Plan();
	Check(P.Num() == 4 && P[0].Kind == TEXT("deck") && FMath::IsNearlyEqual(P[1].At, S47ReelTape::TAPE_BEFORE) && P[3].Kind == TEXT("end") && FMath::IsNearlyEqual(P[3].At, S47ReelTape::TAPE_BEFORE + 1.5 + 2.4),
		TEXT("Earl's reel: threaded, the line, the click, the deck stopped"));
	return bOk;
}
