// main.ts: the night's layer (the seen log, the curve, the night's events, the calls, the glimpses,
// the flashlight, the album), in the order main.ts builds and steps them
#include "S47Night.h"

#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "S47Album.h"
#include "S47AlbumSubjects.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47Calls.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Curve.h"
#include "S47FieldCamera.h"
#include "S47FilmFace.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Glimpses.h"
#include "S47Legs.h"
#include "S47Lines.h"
#include "S47Main.h"
#include "S47OldRoad.h"
#include "S47Phones.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47SaroNight.h"
#include "S47Story.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Night, Log, All);

namespace
{
	using EMode = US47Main::EMode;
	FVector ROAD_ORIGIN() { return S47Legs::ORIGINS(TEXT("road")); }
	FVector STATION_AT() { return S47Legs::ORIGINS(TEXT("station01")); }
	int32 ChapterNo(const FString& Key) { return Key.StartsWith(TEXT("ch")) ? FCString::Atoi(*Key.Mid(2)) : 0; }
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); } // Math.round

	// core/hints.ts firstTime: a hint is said once on this machine (Saved/S47Hints.json, as chapter one keeps it)
	bool FirstTime(const FString& Id)
	{
		const FString Path = FPaths::ProjectSavedDir() / TEXT("S47Hints.json");
		TSet<FString> Said;
		FString Text;
		TArray<TSharedPtr<FJsonValue>> A;
		if (FFileHelper::LoadFileToString(Text, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), A))
		{
			for (const TSharedPtr<FJsonValue>& V : A) { FString S; if (V.IsValid() && V->TryGetString(S)) Said.Add(S); }
		}
		if (Said.Contains(Id)) return false;
		Said.Add(Id);
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const FString& S : Said) Out.Add(MakeShared<FJsonValueString>(S));
		FString W;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Wr = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&W);
		if (FJsonSerializer::Serialize(Out, Wr)) FFileHelper::SaveStringToFile(W, *Path);
		return true;
	}

	// world/Grounds.ts and its neighbours' numbers (site terms)
	const double GROUND = -0.6;                                                     // Crossing.ts
	struct FRampX { double X0, X1, Z0, Z1; };
	const FRampX RAMP = { -10.6, -7.6, 5.05, 6.35 };                                // the fire exit's ramp (Crossing.ts)
	const FRampX WEST = { -33.5, -6.75, -19.55, 64 };                               // Crossing.ts WEST
	struct FTruckRamp { double MinX, MaxX, Top, Foot; };
	const FTruckRamp TRUCK_RAMP = { 6.6, 13.0, 12.4, 16.6 };
	const FRampX STEPS = { 7.1, 8.0, -12.6, -11.4 };
	struct FR { double MinX, MaxX, MinZ, MaxZ; };
	const FR RAISED[] = {
		{ -6.7, 6.7, -5.9, 5.9 }, { -6.6, 2.6, 5.8, 11.1 }, { -7.6, -6.3, 4.75, 6.65 }, { 6.3, 11.0, -0.6, 3.4 }, { 8.0, 11.0, -19.6, -0.6 },
		{ 11.0, 12.6, -13.4, -8.6 }, { 11.0, 12.6, -1.9, -0.6 }, { 6.6, 13.0, 3.4, 12.4 }, { 12.6, 19.2, -6.0, 1.0 }, { 20.2, 39.0, -5.9, 5.9 },
	};
	// arrayLayout.ts ARRAY_GROUND: the flat ground north of the fence (FIELD_BOUNDS -236..216, -426..-20.15
	// from the field's walk zones, widened by 60 and 95 for ARRAY_FLAT, then 40 in, to FENCE_Z + 0.5)
	const FR ARRAY_GROUND = { -236 - 60 + 40, 216 + 60 - 40, -426 - 95 + 40, -20 + 0.5 };
	bool Within(const FR& B, double X, double Z) { return X >= B.MinX && X <= B.MaxX && Z >= B.MinZ && Z <= B.MaxZ; }
}

// ---------- boot ----------

void US47Night::Init(US47Main* InM)
{
	M = InM;
	// what the night did and whether it was seen (story/seen.ts): the events that leave a mark on the
	// map or a night event's key are written down as the key is added (not as a save puts it back)
	SeenLog = MakeShared<FS47SeenLog>();
	// one pressure for the whole night (story/curve.ts): it reads what was seen, and the night's things ask it
	Curve = MakeShared<FS47NightCurve>(*SeenLog);
	TWeakObjectPtr<US47Night> Self = this;
	M->TownSeen.OnFresh = [Self](const FString& K)
	{
		// (the log only watches: a fault in it never stops the night)
		if (Self.IsValid() && !Self->bSeenQuiet) Self->SawKey(K);
	};
	// the flashlight from the truck's passenger seat (core/Flashlight.ts, Tom 15 October)
	Flashlight = MakeShared<FS47Flashlight>(M->Web.Get());
	// something out in the dark now and then, when the player walks about with the flashlight (core/calls.ts)
	Calls = MakeShared<FS47NightCalls>(M->Audio.Get());
	// them, in the background, at the edge of the view (core/glimpses.ts)
	Glimpses = MakeShared<FS47Glimpses>(M->Web.Get());
	// the flashlight on the passenger seat comes along the first time the player gets in
	if (M->World)
	{
		TFunction<void()> Before = M->World->OnGetIn;
		M->World->OnGetIn = [Self, Before]
		{
			if (Before) Before();
			if (!Self.IsValid() || Self->Flashlight->bHave) return;
			Self->Flashlight->bHave = true;
			// which button switches it, the first time on this device
			Self->M->UI->Toast(FString(TEXT("You take the flashlight off the passenger seat.")) + (!FirstTime(TEXT("flashlight")) ? TEXT("") : TEXT(" F switches it on.")), 4.5f);
		};
	}
	// ---------- more that can happen in the night, anywhere (story/nightEvents.ts) ----------
	FS47NightDeps D;
	D.Audio = M->Audio;
	D.Note = [Self](const FString& Text)
	{
		if (!Self.IsValid()) return;
		Self->M->Story->Note(Text); Self->RequestAutosave();
		UE_LOG(LogS47Night, Display, TEXT("S47: night event written: %s"), *Text);
	};
	D.Seen = &M->TownSeen;
	D.Step = [Self] { return Self.IsValid() && Self->M->Audio ? Self->M->Audio->StepPick(Self->SurfaceAt()) : US47Audio::FStepPick(); };
	D.SetRadio = [Self](const FString& St) { if (Self.IsValid() && Self->Hooks.SetCarRadio) Self->Hooks.SetCarRadio(St); };
	D.Bulletin = [Self] { return Self.IsValid() && Self->Hooks.Bulletin ? Self->Hooks.Bulletin() : 0.0; };
	D.BulletinProgress = [Self] { return Self.IsValid() && Self->Hooks.BulletinProgress ? Self->Hooks.BulletinProgress() : TOptional<FS47ArchiveProgress>(); };
	D.BulletinHeard = [Self] { return !Self.IsValid() || !Self->Hooks.BulletinHeard || Self->Hooks.BulletinHeard(); };
	D.OpenDoor = [Self] { if (Self.IsValid() && Self->M->Web) Self->M->Web->SetDoorById(TEXT("east"), true, false); };
	D.Silence = [Self] { if (Self.IsValid() && Self->Hooks.RoadsideSilence) Self->Hooks.RoadsideSilence(); };
	D.Place = [Self] { return Self.IsValid() ? Self->PlaceName() : FString(); };
	D.Silent = [Self] { return Self.IsValid() && Self->Is(Self->Hooks.RoadsideSilent); };
	NightEvents = MakeShared<FS47NightEvents>(MoveTemp(D));

	// ---------- the free camera and the album (core/album.ts; todo.md, after They Are Here) ----------
	// Once the camera is had, the shutter takes a free frame anywhere a case frame is not to be had
	// (Chapter1.shutter); 36 in a night, kept with the case, in the notebook's album.
	FS47AlbumDeps A;
	A.Fcam = M->FieldCamera;
	A.Web = M->Web;
	A.Subjects = [Self]
	{
		if (!Self.IsValid()) return TArray<FS47Subject>();
		FS47SubjectDeps S;
		S.M = Self->M;
		S.Outdoors = [Self] { return Self.IsValid() && Self->M->Space == TEXT("yard"); };
		S.Night = [Self] { return Self.IsValid() && Self->Dawn() < 0.3; };
		S.Clock = [Self] { return Self.IsValid() ? Self->Clock() : 0.0; };
		return S47AlbumSubjects::AlbumSubjects(S);
	};
	A.Place = [Self] { return Self.IsValid() ? Self->PlaceName() : FString(); };
	A.Clock = [Self] { return Self.IsValid() ? Self->Clock() : 0.0; };
	A.Extra = [Self] { return Self.IsValid() && Self->Hooks.AbductionExtra ? Self->Hooks.AbductionExtra() : 0; };
	// (ext.dishArray.cull(photoCam): Unreal culls the instances per view by itself)
	A.Before = [] {};
	// someone on the film only: out in the night where the glimpses are (SARO, the highway, Caliche)
	A.GhostGround = [Self]() -> TFunction<double(double, double)>
	{
		if (!Self.IsValid()) return nullptr;
		const FS47GlimpsesIn G = Self->GlimpseIn(true);
		if (G.Scene.IsEmpty() || Self->M->Space != TEXT("yard") || Self->Driving() || !Self->At(TEXT("ch1")) || Self->Phase() == TEXT("ch6") || Self->Dawn() >= 0.3) return nullptr;
		// (Caliche's own ground by the diner's lot, world/Town.ts floorAt, is the town's: the road's ground here)
		return G.Ground;
	};
	A.Small = [Self] { return Self.IsValid() && Self->M->Flow && Self->M->Flow->Saves.bFallback; };
	A.Done = [Self](const FS47AlbumShot& Shot, bool bFirst, int32 Left)
	{
		if (!Self.IsValid()) return;
		US47Main* Mn = Self->M;
		if (Mn->Audio) Mn->Audio->Shutter();
		Mn->UI->ShutterFlash();
		// the patch with no stars in it, on the print (story/starHole.ts)
		const TPair<FString, FVector2D>* Hole = Shot.Targets.FindByPredicate([](const TPair<FString, FVector2D>& T) { return T.Key == TEXT("starhole"); });
		if (Hole && Self->Hooks.StarHolePhotographed) Self->Hooks.StarHolePhotographed(Shot.Id, !Self->OneAtATime() && !(Mn->Audio && Mn->Audio->PhonePlaying()));
		// on the film only (story/seen.ts): the face a foot from the lens (seen when the print is looked
		// at large, come up), someone at the edge of the frame
		const TOptional<FS47Album::FLast>& L = Self->Album->Last;
		if (L && L->bFace)
		{
			FS47SawOpts O; O.At = Self->Camera().Pos; O.bLookingSet = true; O.Looking = false; O.bNoWatch = true;
			Self->Saw(TEXT("face"), TEXT("on film"), O);
		}
		else if (L && L->bGhost && L->Odd.Num()) { FS47SawOpts O; O.bLookingSet = true; Self->Saw(TEXT("glimpse"), TEXT("on film"), O); }
		if (bFirst) Mn->UI->Toast(FString::Printf(TEXT("A free frame, out of the instant back. The print goes in the album in the notebook. %d left."), Left), 4.4f);
		else if (Left == 5 || Left == 0) Mn->UI->Toast(Left ? TEXT("5 free frames left on the roll.") : TEXT("That was the last free frame on the roll."), 3.f);
		UE_LOG(LogS47Night, Display, TEXT("S47: free frame %s: %s (%d left)"), *Shot.Id, *Shot.Text, Left);
		Self->RequestAutosave();
	};
	// once in the night: the face a foot from the lens, on the first free frame out of doors in
	// chapter four (core/filmFace.ts). Only on the print; in the scene there is nothing.
	A.Face = [Self] { return Self.IsValid() && Self->M->Mode == EMode::Play && Self->Phase() == TEXT("ch4") && Self->M->Space == TEXT("yard") && !Self->Driving() && !Self->Cinematic() && !Self->Is(Self->Hooks.AbductionBusy); };
	// the first close look at it, come up: the night goes quiet round the player while it is looked
	// at, the music with it, and the notebook gets a line in the player's hand
	A.FaceSeen = [Self](const FS47AlbumShot& Shot)
	{
		if (!Self.IsValid()) return;
		if (const FS47SeenRef E = Self->SeenLog->Last(TEXT("face"))) E->Looking = true;
		if (Self->M->Audio && Self->M->Audio->Night.IsValid()) Self->M->Audio->Night->Hush(FACE_QUIET, 0.02);
		Self->FaceQuiet = FACE_QUIET;
		FString Place = Shot.Place;
		if (Place.StartsWith(TEXT("The "), ESearchCase::CaseSensitive)) Place = TEXT("the ") + Place.Mid(4);
		Self->M->Story->Note(FString::Printf(TEXT("%s, %s. %s"), *S47Text::ClockText(Shot.Clock, false), *Place, FS47Album::FACE_LINE));
		Self->RequestAutosave();
	};
	Album = MakeShared<FS47Album>(MoveTemp(A));
	if (AS47Story* Host = M->Story)
	{
		if (US47Chapter1* Ch1 = Cast<US47Chapter1>(Host->Chapter(TEXT("ch1"))))
		{
			FS47FreeShots F;
			F.Left = [Self] { return Self.IsValid() ? Self->Album->Left() : 0; };
			F.Expose = [Self] { if (Self.IsValid()) Self->Album->Expose(); };
			Ch1->Free = F;
		}
		// chapter two: Ward's second call waits while one of the night's things is going
		if (US47Chapter2* Ch2 = Cast<US47Chapter2>(Host->Chapter(TEXT("ch2")))) Ch2->Quiet = [Self] { return !Self.IsValid() || (!Self->OneAtATime() && !(Self->M->Audio && Self->M->Audio->PhonePlaying())); };
	}
	// the kit's line for the camera (core/album.ts rollText)
	if (M->Flow) M->Flow->RollText = [Self] { return Self.IsValid() ? FS47Album::RollText(Self->Album->Left()) : FString(); };

	// main.ts input.onKey: F switches the flashlight
	M->OnKey(EKeys::F, [Self] { if (Self.IsValid()) Self->ToggleLight(); return true; });
	TestFlags();
}

void US47Night::ToggleLight()
{
	if (M->Mode != EMode::Play || Modal() || Cinematic() || Driving()) return;
	if (!Flashlight->Toggle()) M->UI->Toast(TEXT("No flashlight. There is one on the passenger seat of the truck."), 3);
}

// ---------- the deps main.ts reads ----------

FString US47Night::Phase() const { return M->Story ? M->Story->ChapterKey : FString(TEXT("prologue")); }
bool US47Night::At(const FString& Chapter) const { return ChapterNo(Phase()) >= ChapterNo(Chapter); }
double US47Night::Clock() const { return M->Story ? M->Story->Clock() : 0; }
bool US47Night::Modal() const { return M->ModalOpen(); }
bool US47Night::Cinematic() const { return M->Story && M->Story->IsCinematic(); }
bool US47Night::Driving() const { return M->World && M->World->bDriving; }
FString US47Night::Area() const { return M->World ? M->World->Area : M->Web ? M->Web->Area() : FString(TEXT("saro")); }
FS47Cam US47Night::Camera() const { return FS47Cam::Of(M->Player, Area() == TEXT("roswell") ? 12000 : 5000); }
void US47Night::RequestAutosave() const { M->RequestAutosave(false); }

FVector US47Night::PlayerAt() const
{
	// (in the truck the player is where the truck is, not where they got in)
	if (Driving()) if (const FS47Truck* T = M->World->CabTruck()) return T->Position;
	return M->Player ? M->Player->Pos : FVector::ZeroVector;
}

FString US47Night::PlaceName(const FVector* P) const
{
	if (!M->Flow) return FString();
	return P ? M->Flow->PlaceName(*P) : M->Flow->PlaceName();
}

double US47Night::Dawn() const
{
	// Sky.setClock: 0 until 04:40, sunrise at 05:30, times the dawn lift (main.ts: the old road 1, else 0.7)
	const double S = FMath::Fmod(FMath::Fmod(Clock(), 86400.0) + 86400.0, 86400.0);
	const double K = S > 12 * 3600 ? 0 : FMath::Clamp((S - (4 * 3600 + 40 * 60)) / (50 * 60), 0.0, 1.0);
	return FMath::Pow(K, 1.6) * (Area() == TEXT("roswell") ? 1 : 0.7);
}

double US47Night::SaroFloorAt(const AS47Web* Web, double X, double Z)
{
	// world/Grounds.ts floorAt: the height of the ground under the player anywhere in SARO's scene
	const S47Geo::FRect& Motel = S47Geo::MOTEL_RECT();
	if (Z > WEST.Z1)
	{
		// south of the site: the motel's lot and floors, or the road's land by the highway
		if (X >= Motel.MinX && X <= Motel.MaxX && Z >= Motel.MinZ && Z <= Motel.MaxZ) return S47Geo::MOTEL_AT().Y;
		return S47RoadShape::HighwayTop(X + S47Geo::SARO_IN_ROAD.X, Z + S47Geo::SARO_IN_ROAD.Z);
	}
	// west of the site, where its ground gives way to the road's land: eased over the last 24 m
	if (Z > WEST.Z1 - 24 && X < WEST.X1)
	{
		double K = (Z - (WEST.Z1 - 24)) / 24;
		K = K <= 0 ? 0 : K >= 1 ? 1 : K * K * (3 - 2 * K);
		return GROUND + (S47RoadShape::HighwayTop(X + S47Geo::SARO_IN_ROAD.X, Z + S47Geo::SARO_IN_ROAD.Z) - GROUND) * K;
	}
	// the slopes: the fire exit's ramp, the truck ramp (a little past its foot), the steps
	struct FSlope { FR R; bool bX; double Low, High; };
	const FSlope SLOPES[] = {
		{ { RAMP.X0, RAMP.X1, RAMP.Z0 - 0.1, RAMP.Z1 + 0.1 }, true, RAMP.X0, RAMP.X1 },
		{ { TRUCK_RAMP.MinX, TRUCK_RAMP.MaxX, TRUCK_RAMP.Top, TRUCK_RAMP.Foot + 0.1 }, false, TRUCK_RAMP.Foot, TRUCK_RAMP.Top },
		{ { STEPS.X0 - 0.1, STEPS.X1, STEPS.Z0, STEPS.Z1 }, true, STEPS.X0 - 0.1, STEPS.X1 },
	};
	for (const FSlope& S : SLOPES)
	{
		if (!Within(S.R, X, Z)) continue;
		const double V = S.bX ? X : Z;
		const double K = FMath::Clamp((V - S.Low) / (S.High - S.Low), 0.0, 1.0);
		return GROUND * (1 - K);
	}
	for (const FR& B : RAISED) if (Within(B, X, Z)) return 0;
	// the ground's zones (Grounds.ts this.ground, in the export)
	if (Web)
	{
		for (int32 I = 0; I < 16; ++I)
		{
			const FS47Area G = Web->Rect(FString::Printf(TEXT("world.grounds.ground[%d]"), I));
			if (!G.bValid) break;
			if (X >= G.MinX && X <= G.MaxX && Z >= G.MinZ && Z <= G.MaxZ) return GROUND;
		}
	}
	// the antenna field north of the array fence, its gate included: the array is graded flat at the ground's level
	if (Within(ARRAY_GROUND, X, Z)) return GROUND;
	return 0;
}

FString US47Night::SurfaceAt() const
{
	// main.ts surfaceAt: footsteps by what is underfoot (the town's own surfaces and the road's stops,
	// world/Town.ts and World.roadStopAt, are the town's: the road's dirt here)
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	const FString A = Area();
	const bool bIn = M->World && M->World->Indoors(P);
	if (A == TEXT("room6")) return TEXT("carpet");
	if (A == TEXT("diner")) return bIn ? TEXT("tile") : TEXT("dirt");
	if (A == TEXT("station01")) return bIn ? TEXT("wood") : TEXT("dirt");
	if (A != TEXT("saro")) return TEXT("dirt");
	if (M->Web && M->Web->Rect(TEXT("world.motel.zones[2]")).Contains(P)) return TEXT("carpet");
	if (M->Space != TEXT("yard")) return TEXT("tile");
	if (P.Z > 64 && FMath::Abs(P.X + 24) < 6) return TEXT("concrete");   // the highway south and its shoulders
	return M->Player && M->Player->FloorY < -0.3 ? TEXT("dirt") : TEXT("concrete");
}

// ---------- the night's curve (story/curve.ts; FORSLAG.md, 20 October, item 1) ----------

bool US47Night::OneAtATime() const
{
	// one of the night's things is going: nothing else starts (the one-at-a-time rule; each of them
	// also waits for the others itself, as before)
	return NightGoing() || Is(Hooks.NinthQuiet);
}

bool US47Night::NightGoing() const
{
	// all of oneAtATime but the quiet round the ninth cow (that quiet is hers)
	return Is(Hooks.AbductionBusy) || (NightEvents && NightEvents->Running()) || Is(Hooks.RoadsideSilent) || (Hooks.TownEventsNow && Hooks.TownEventsNow() != TEXT("none"))
		|| Is(Hooks.RoofStepsBusy) || Is(Hooks.MotelTvBusy) || Is(Hooks.LineLightsOn) || FaceQuiet > 0 || Is(Hooks.ReelTapeBusy) || Is(Hooks.SaroDarkBusy)
		|| Is(Hooks.OfficePhoneBusy) || Is(Hooks.ArchiveBusy)
		// (the array put back on the schedule, until it has gone back)
		|| Is(Hooks.ArrayBackBusy)
		// (the shape at the end of the headlights, until the lights have reached it)
		|| Is(Hooks.CutoutPost)
		// (the truck on B, its count and a few seconds after)
		|| Is(Hooks.TruckBBusy)
		// (taken in the truck, from the pass over B to a few seconds after the waking)
		|| Is(Hooks.TruckTakenBusy)
		// (the recording walking where the player was taken)
		|| Is(Hooks.RecWalkBusy)
		// (the morning holding its breath in the diner)
		|| Is(Hooks.MorningBusy);
}

void US47Night::CurveStep(double Dt, bool bModal)
{
	const FString Ph = Phase();
	const bool bInDiner = Area() == TEXT("diner") && M->World && M->Player && M->World->Indoors(M->Player->Pos);
	FS47CurveIn In;
	In.Chapter = Ph; In.Clock = Clock();
	// nothing during a cinematic, a panel, a call, Nora's room, or the diner and its map in chapter five
	In.bHeld = bModal || Cinematic() || (M->Audio && M->Audio->PhonePlaying()) || Area() == TEXT("room6") || (Ph == TEXT("ch5") && bInDiner);
	In.bExposed = Driving() || M->Space == TEXT("yard");
	In.bBusy = OneAtATime();
	In.bPeaking = Is(Hooks.AbductionBusy) || Is(Hooks.RoofStepsBusy) || Is(Hooks.RoadsideSilent) || FaceQuiet > 0 || Is(Hooks.ReelTapeBusy) || Is(Hooks.SaroDarkBusy)
		|| Is(Hooks.OfficePhoneRinging) || Is(Hooks.ArchivePeaking) || Is(Hooks.ArrayBackBusy) || Is(Hooks.TruckBPeaking) || Is(Hooks.RecWalkBusy);
	// a 47 that belongs to a place waits there for the player: the curve hurries for it
	In.bWaiting = Is(Hooks.MotelTvWaiting) || Is(Hooks.TownEventsWaiting);
	// (and the recording that could walk within sight)
	In.bWaitingScare = Is(Hooks.RecWalkWaiting);
	// chapter one's 47 is the story's own until it has come or its time is past (story/saroDark.ts),
	// and chapter two's is the archive's phone until it has come (story/archivePhone.ts)
	In.bKeep = (Ph == TEXT("ch1") && Is(Hooks.SaroDarkKeep)) || (Ph == TEXT("ch2") && Is(Hooks.ArchiveKeep));
	Curve->Update(Dt, In);
	if (M->Audio && M->Audio->Night.IsValid()) M->Audio->Night->Thin = Curve->Thin();
	if (Curve->bSign) CurveSign();
	SignFlick = FMath::Max(0.0, SignFlick - Dt); PostFlick = FMath::Max(0.0, PostFlick - Dt);
	Flashlight->Flicker(SignFlick > 0 ? 0.85 : 0, TEXT("curve"));
	Flashlight->Flicker(PostFlick > 0 ? 0.95 : 0, TEXT("post"));
	// (cutouts.placed: the cutout's start, and postStep, the shape at the end of the headlights: core/cutouts.ts, not ported yet)
}

void US47Night::CurveSign()
{
	// a sign where one can be had: driving, the radio slides under its static or the lights blink; on
	// foot outdoors with the flashlight on, it stutters
	FString What;
	if (Driving())
	{
		if (Hooks.CarRadioDrift && FMath::FRand() < 0.65 && Hooks.CarRadioDrift(2.5 + FMath::FRand() * 2.5)) What = TEXT("radio");
		else if (M->World->Trouble.Kind() == ES47Trouble::None && M->World->Trouble.Start(ES47Trouble::Blink)) What = TEXT("dash");
	}
	else if (M->Space == TEXT("yard") && Flashlight->bHave && Flashlight->bOn) { SignFlick = 0.7 + FMath::FRand() * 0.7; What = TEXT("flashlight"); }
	if (!What.IsEmpty())
	{
		CurveSigns.Add({ What, JsRound(Clock()) });
		if (CurveSigns.Num() > 40) CurveSigns.RemoveAt(0);
		UE_LOG(LogS47Night, Display, TEXT("S47: curve sign %s"), *What);
	}
}

// ---------- the night's events (story/nightEvents.ts) ----------

FS47NightIn US47Night::NightIn(bool bOk)
{
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	FS47Truck* Tr = M->World ? M->World->TruckHere() : nullptr;
	const bool bOut = Area() == TEXT("saro") ? M->Space == TEXT("yard") : !(M->World && M->World->Indoors(P));
	const bool bMoving = FVector::Dist(P, LastFoot) > 0.004; LastFoot = P;
	FS47NightIn S;
	S.bOk = bOk; S.Clock = Clock(); S.Cam = Camera(); S.bDriving = Driving();
	if (!S.bDriving) S.Foot = FS47NightIn::FFoot{ P, bOut, bMoving };
	// (the radio left on is the car radio's: without its port, no truck here to come back to)
	if (!S.bDriving && Tr && Hooks.CarRadioOn) S.Truck = FS47NightIn::FTruck{ Tr->Position, Hooks.CarRadioOn() };
	S.bRadio1420 = S.bDriving && Is(Hooks.Radio1420);
	if (Area() == TEXT("saro") && bOut && !S.bDriving && M->Web)
	{
		S.Saro = FS47NightIn::FSaro{ M->Web->Obj(TEXT("room.objs.phone")).WorldPosition(), M->Web->Obj(TEXT("room.objs.doorEast")).WorldPosition(), M->Web->DoorStates().FindRef(TEXT("east")) };
	}
	// (the silence is core/roadside.ts's: without it there is none to have)
	S.bSilenceFree = Hooks.RoadsideSilence && !Is(Hooks.RoadsideSilent) && !(M->Story && M->Story->Notes().ContainsByPredicate([](const FString& N) { return N.Contains(TEXT("No sound for 47 s")); }));
	return S;
}

FS47GlimpsesIn US47Night::GlimpseIn(bool bOk) const
{
	const FString A = Area();
	const bool bRoad = A == TEXT("road") || A == TEXT("diner");
	FS47Truck* Tr = Driving() ? M->World->TruckHere() : nullptr;
	FS47GlimpsesIn G;
	G.bOk = bOk;
	G.Scene = A == TEXT("saro") ? TEXT("saro") : bRoad ? TEXT("road") : TEXT("");
	G.Cam = Camera();
	G.Clock = Clock();
	const FVector Ro = ROAD_ORIGIN();
	const AS47Web* Web = M->Web;
	if (bRoad) G.Ground = [Ro](double X, double Z) { return S47RoadTerrain::SurfaceY(X - Ro.X, Z - Ro.Z) + Ro.Y; };
	else G.Ground = [Web](double X, double Z) { return SaroFloorAt(Web, X, Z); };
	if (Tr && bRoad) G.Truck = FS47GlimpsesIn::FTruck{ Tr->Position, G.Cam.Forward };
	G.bFlashlight = Flashlight->bOn;
	G.Zoom = M->ZoomK; // (the closer look, main.ts zoomK: S47Phones.cpp)
	return G;
}

// ---------- what the night did, and whether it was seen (story/seen.ts) ----------

TOptional<FVector2D> US47Night::LocalOf(const FString& InArea, const FVector& P) const
{
	if (InArea == TEXT("saro")) return FVector2D(P.X, P.Z);
	if (InArea == TEXT("road") || InArea == TEXT("diner")) return FVector2D(P.X - ROAD_ORIGIN().X, P.Z - ROAD_ORIGIN().Z);
	if (InArea == TEXT("station01")) return FVector2D(P.X - STATION_AT().X, P.Z - STATION_AT().Z);
	if (InArea == TEXT("roswell") && M->World && M->World->OldRoad()) { const FVector O = M->World->OldRoad()->ToWorld(0, 0, 0); return FVector2D(P.X - O.X, P.Z - O.Z); }
	if (InArea == TEXT("room6")) return FVector2D(0, 0);
	return {};
}

TOptional<FVector2D> US47Night::MapOf(const FVector& P) const
{
	const FString A = Area();
	const TOptional<FVector2D> L = LocalOf(A, P);
	return L ? S47Lines::ToMap(A, L->X, L->Y) : TOptional<FVector2D>();
}

FString US47Night::LineHere(const FVector& P) const
{
	const FString A = Area();
	const TOptional<FVector2D> L = LocalOf(A, P);
	const TOptional<S47Lines::FOnLine> On = L ? S47Lines::LineAt(A, L->X, L->Y) : TOptional<S47Lines::FOnLine>();
	return On ? On->Line : FString();
}

FS47SeenRef US47Night::Saw(const FString& Kind, const FString& What, const FS47SawOpts& O)
{
	// write it down: where the player is, the thing's place (a world point of the area shown, or a map
	// point), the line it is on (the thing's, else the player's), and a watch for the looking (by
	// default three seconds on the thing; null: nothing to look at)
	const TOptional<FVector> At = O.At;
	const TOptional<FVector2D> Mp = O.Map ? O.Map : At ? MapOf(*At) : TOptional<FVector2D>();
	const FVector Me = PlayerAt();
	FString Line;
	if (O.Line) Line = *O.Line;
	else if (At) Line = LineHere(*At);
	else if (O.Map) { const TOptional<S47Lines::FOnLine> On = S47Lines::LineAt(TEXT("road"), O.Map->X, O.Map->Y); Line = On ? On->Line : FString(); }
	else Line = LineHere(Me);
	const FString Place = Driving() ? PlaceName(&Me) + TEXT(", in the truck") : PlaceName();
	const FS47SeenRef E = MakeShared<FS47SeenEntry>();
	E->Kind = Kind; E->What = What; E->Clock = JsRound(Clock()); E->Place = Place; E->Area = Area(); E->Line = Line;
	if (Mp) E->At = FVector2D(JsRound(Mp->X), JsRound(Mp->Y));
	TOptional<FS47SeenWatch> W;
	if (!(O.bNoWatch || (!At && !(O.Watch && O.Watch->Pos))))
	{
		FS47SeenWatch Wt = O.Watch ? *O.Watch : FS47SeenWatch();
		if (!Wt.Pos) Wt.Pos = [At] { return At; };
		W = Wt;
	}
	const TOptional<FVector> Now = W ? (W->Pos() ? W->Pos() : At) : At;
	if (O.bLookingSet) E->Looking = O.Looking;
	else if (W) E->Looking = Now.IsSet() && S47Facing(Camera(), *Now, W->Range.Get(300), W->Deg.Get(30));
	else E->Looking.Reset();
	return SeenLog->Add(E, W);
}

void US47Night::SawKey(const FString& K)
{
	// an event's key has just been added (story/townEvents.ts, nightEvents.ts, pasture.ts, motelTv.ts,
	// caliche.ts, the boy on the radio): the map's mark or the night event's count
	FString What;
	if (K.StartsWith(TEXT("mark:")))
	{
		// /^mark:([a-z0-9]+)@/
		const int32 AtSign = K.Find(TEXT("@"));
		const FString Mk = AtSign > 5 ? K.Mid(5, AtSign - 5) : FString();
		bool bOk = !Mk.IsEmpty();
		for (const TCHAR Ch : Mk) bOk &= (Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9');
		if (bOk) What = Mk;
	}
	else if (K == TEXT("ev:cows")) What = TEXT("cows2");
	else if (K.StartsWith(TEXT("ne:")))
	{
		// /^ne:([a-z]+)\d+$/
		int32 I = 3;
		while (I < K.Len() && K[I] >= 'a' && K[I] <= 'z') ++I;
		int32 J = I;
		while (J < K.Len() && FChar::IsDigit(K[J])) ++J;
		if (I > 3 && J > I && J == K.Len()) { const FString N = K.Mid(3, I - 3); What = N == TEXT("phone") ? FString(TEXT("roomphone")) : N; }
	}
	// (the eight seen without the nine first get the herd's mark too: one sighting, one line)
	if (What == TEXT("cows") && M->TownSeen.Contains(TEXT("ev:cows"))) return;
	// (the lights are written down when they come, the silence when it starts: seenStep)
	if (What.IsEmpty() || What == TEXT("lights") || What == TEXT("silence")) return;
	// eventAt: where an event's thing is in the area shown, to see whether the player looks at it
	// (what is seen, and where what is heard comes from); unset for what is only heard down a line
	TOptional<FVector> At;
	auto ObjAt = [this](const TCHAR* Path) { return M->Web ? M->Web->Obj(Path).WorldPosition() : FVector::ZeroVector; };
	auto OnGroundAt = [this](const TCHAR* Kind, double Up) -> TOptional<FVector>
	{
		const S47Lines::FMapPt* Mp = S47Lines::PLACES().Find(Kind);
		const FString A = Area();
		const TOptional<FVector2D> L = Mp ? S47Lines::FromMap(A, Mp->X, Mp->Y) : TOptional<FVector2D>();
		if (!L) return {};
		const FVector O = A == TEXT("road") || A == TEXT("diner") ? ROAD_ORIGIN() : A == TEXT("station01") ? STATION_AT() : FVector::ZeroVector;
		FVector P(L->X + O.X, 1, L->Y + O.Z);
		// (world.groundUnder: the area's ground)
		P.Y = (A == TEXT("road") || A == TEXT("diner") ? S47RoadTerrain::SurfaceY(P.X - O.X, P.Z - O.Z) + O.Y : A == TEXT("station01") ? S47Legs::StationGroundAt(P.X, P.Z) : SaroFloorAt(M->Web, P.X, P.Z)) + Up;
		return P;
	};
	const FString A = Area();
	if (What == TEXT("cutout")) At = OnGroundAt(TEXT("cutout"), 0.9);
	else if (What == TEXT("car")) At = OnGroundAt(TEXT("car"), 0.9);
	else if (What == TEXT("lamps")) At = OnGroundAt(TEXT("lamps"), 5);
	else if (What == TEXT("cows") || What == TEXT("cows2")) At = OnGroundAt(TEXT("cows"), 1.2);
	else if (What == TEXT("tv")) { if (A == TEXT("saro")) At = ObjAt(TEXT("world.motel.tv.group")); }
	else if (What == TEXT("regular")) { const FS47Obj C = M->Web ? M->Web->Obj(TEXT("world.diner.objs.regularCup")) : FS47Obj(); if (C) At = C.WorldPosition(); }
	else if (What == TEXT("steps")) { const FVector S = NightEvents->StepsAt(); At = FVector(S.X, S.Y + 1, S.Z); }
	else if (What == TEXT("radio")) { if (const FS47Truck* Tr = M->World ? M->World->TruckHere() : nullptr) At = Tr->Position + FVector(0, 1.4, 0); }
	else if (What == TEXT("roomphone")) At = ObjAt(TEXT("room.objs.phone"));
	else if (What == TEXT("door")) At = ObjAt(TEXT("room.objs.doorEast"));
	// (ninth: world.site.ninthDown, story/ninthCow.ts, not ported yet)
	/** How far off each event's thing can be looked at (metres). */
	static const TMap<FString, double> EVENT_RANGE = { { TEXT("ninth"), 30 }, { TEXT("cutout"), 60 }, { TEXT("car"), 60 }, { TEXT("lamps"), 250 }, { TEXT("cows"), 160 }, { TEXT("cows2"), 160 },
		{ TEXT("tv"), 10 }, { TEXT("regular"), 15 }, { TEXT("steps"), 20 }, { TEXT("radio"), 60 }, { TEXT("roomphone"), 45 }, { TEXT("door"), 40 } };
	const S47Lines::FMapPt* Place = S47Lines::PLACES().Find(What);
	FS47SawOpts O;
	O.At = At;
	if (!At && Place) O.Map = *Place;
	if (Place) O.Line = S47Lines::LineOfMark(What);
	if (!At) { O.bLookingSet = true; O.bNoWatch = true; }
	else { FS47SeenWatch W; W.Secs = 3; const double* R = EVENT_RANGE.Find(What); W.Range = R ? *R : 120; O.Watch = W; }
	Saw(TEXT("event"), What, O);
}

void US47Night::SeenSync()
{
	// where things stand now, without writing anything down (a night started or put back)
	Was.Glimpse = Glimpses->Up;
	Was.Calls = Calls->Count;
	// (the roof, the office's telephone, the archive, the array tipped, reel 047: S47Phones.cpp)
	if (M->Phones) M->Phones->SeenSync();
	if (M->SaroNight) M->SaroNight->SeenSync();
}

void US47Night::SeenStep(double Dt)
{
	// the things that leave no key, written down as they change: the glimpses, the sounds out in the
	// dark. (The cutout, the records and the walk where the player was taken, the roadside's car and
	// silence, the time away and the one over you, the dishes following, the steps on the roof, the
	// lights on B, the regular's cup, Earl's reel, SARO dark and its figure, the office's telephone,
	// the archive's phone and the array tipped, reel 047, the array put back, the last mile, THE EVENT
	// and the third one are written here by main.ts as well, with their own modules: not ported yet.)
	if (bSeenQuiet) return;
	if (Glimpses->Up != Was.Glimpse)
	{
		const TOptional<FVector> At = Glimpses->Where();
		if (!Glimpses->Up.IsEmpty() && At)
		{
			TWeakPtr<FS47Glimpses> G = Glimpses;
			FS47SawOpts O; O.At = At;
			FS47SeenWatch W; W.Pos = [G] { const TSharedPtr<FS47Glimpses> P = G.Pin(); return P ? P->Where() : TOptional<FVector>(); }; W.Secs = 60; W.Deg = 25; W.Range = 320;
			O.Watch = W;
			Saw(TEXT("glimpse"), Glimpses->Up, O);
		}
		Was.Glimpse = Glimpses->Up;
	}
	if (Calls->Count != Was.Calls)
	{
		Was.Calls = Calls->Count;
		if (Calls->Last)
		{
			FS47SawOpts O; O.At = Calls->Last->At;
			FS47SeenWatch W; W.Secs = 8; W.Range = 160; O.Watch = W;
			Saw(TEXT("call"), Calls->Last->Name, O);
		}
	}
	// the steps on the roof, the office's telephone, the archive's phone, the array tipped, reel 047 (S47Phones.cpp)
	if (M->Phones) M->Phones->SeenStep();
	// (the recording's walk, the dishes that follow, the lights on B, Earl's reel, SARO dark and its
	// figure, reel 047, the array put back: S47SaroNight.cpp)
	if (M->SaroNight) M->SaroNight->SeenStep();
	SeenLog->Update(Dt, Camera());
}

// ---------- step() ----------

void US47Night::FlashlightStep(double Dt)
{
	// flashlight.update(dt, t, camera, mode !== 'play' || world.driving || game.cinematic, ultra.on)
	const bool bUltra = M->Flow && M->Flow->Quality == TEXT("ultra");
	Flashlight->Update(Dt, M->T, Camera(), M->Mode != EMode::Play || Driving() || Cinematic(), bUltra);
}

void US47Night::Step(double Dt)
{
	if (M->Mode != EMode::Play) return;
	const bool bModal = Modal();
	// the print with the face has come up (core/album.ts): said once, when nothing else holds the player
	if (FaceQuiet > 0) FaceQuiet -= Dt;
	if (!bModal && !Cinematic())
	{
		if (const FS47AlbumShot* Up = Album->FaceUp(Clock())) { M->UI->Toast(FString::Printf(TEXT("The print from %s has come up."), *S47Text::ClockText(Up->Clock, false)), 3.6f); RequestAutosave(); }
	}
	// what the night did, before it moves on (story/seen.ts): what came in the last step, and the
	// camera on what is still there
	SeenStep(Dt);
	// the night's one curve, before the night's things ask it (story/curve.ts)
	CurveStep(Dt, bModal);
	// (roadside.update: core/roadside.ts, not ported yet)
	const bool bSilent = Is(Hooks.RoadsideSilent);
	FS47CallsWhen W;
	W.bOk = !bModal && !Cinematic() && !Driving() && Flashlight->bHave && Flashlight->bOn && M->Space == TEXT("yard") && Dawn() < 0.3 && !bSilent && !Is(Hooks.ArrayBackBusy);
	W.Cam = Camera();
	TWeakPtr<FS47Flashlight> Fl = Flashlight;
	W.Flicker = [Fl](double K) { if (const TSharedPtr<FS47Flashlight> F = Fl.Pin()) F->Flicker(K); };
	TWeakObjectPtr<US47Audio> Au = M->Audio;
	W.Hush = [Au](double Secs) { if (Au.IsValid() && Au->Night.IsValid()) Au->Night->Hush(Secs); };
	W.May = Curve->May(TEXT("call")); W.Pace = Curve->Pace(TEXT("call"));
	Calls->Update(Dt, W);
	// (townEvents.update: story/townEvents.ts, not ported yet)
	const bool bTownNone = !Hooks.TownEventsNow || Hooks.TownEventsNow() == TEXT("none");
	// one thing at a time: not while the town has one of its own going
	const bool bNightOk = !bModal && !Cinematic() && At(TEXT("ch1")) && Phase() != TEXT("ch6") && Dawn() < 0.3 && !bSilent && !Is(Hooks.AbductionBusy) && bTownNone && !Is(Hooks.RoofStepsBusy)
		&& !Is(Hooks.ReelTapeBusy) && !Is(Hooks.OfficePhoneBusy) && !Is(Hooks.ArrayBackBusy) && !Is(Hooks.ArchiveBusy);
	FS47NightIn In = NightIn(bNightOk);
	In.May = Curve->May(TEXT("event")); In.MayLong = Curve->May(TEXT("long")); In.Pace = Curve->Pace(TEXT("event"));
	NightEvents->Update(Dt, In);
	// the field book, the steps on the roof, the archive's phone after them (S47Phones.cpp)
	if (M->Phones) M->Phones->StepNight(Dt, bNightOk);
	// 02:29: SARO goes dark (story/saroDark.ts), and Hondo's telex where the night never went dark
	if (M->SaroNight) M->SaroNight->StepDark(Dt, bModal);
	// (cutouts: not ported yet)
	// the flashlight flickers near the strange events: through the silence on the shoulder, while a
	// figure stands at the edge of the view, while the town's lamps go out or its payphone rings
	Flashlight->Flicker(bSilent ? 0.4 : 0, TEXT("silence"));
	Flashlight->Flicker(!Glimpses->Up.IsEmpty() ? 0.3 : 0, TEXT("glimpse"));
	Flashlight->Flicker(!bTownNone && Area() == TEXT("diner") ? 0.35 : 0, TEXT("town"));
	FS47GlimpsesIn G = GlimpseIn(!bModal && !Cinematic() && At(TEXT("ch1")) && Phase() != TEXT("ch6") && Dawn() < 0.3 && !bSilent && !Is(Hooks.AbductionBusy) && !Is(Hooks.SaroDarkBusy) && !Is(Hooks.ArrayBackBusy));
	G.May = Curve->May(TEXT("glimpse")); G.Pace = Curve->Pace(TEXT("glimpse"));
	Glimpses->Update(Dt, G);
	// the lights on B (story/lineLights.ts)
	if (M->SaroNight) M->SaroNight->StepLights(Dt, bModal);
	// (eventsOnB, ninth, starHole, the diner's morning: not ported yet)
	// the thread nobody tied, the office's telephone (S47Phones.cpp)
	if (M->Phones) M->Phones->StepLate(Dt, bModal);
	// (truckB, truckTaken, the time away: not ported yet)

	// the log, for the tests and the screenshot runs
	if (Calls->Count != CallsLogged && Calls->Last)
	{
		CallsLogged = Calls->Count;
		UE_LOG(LogS47Night, Display, TEXT("S47: call %s at %.0f m, %.0f degrees round"), *Calls->Last->Name, Calls->Last->Dist, FMath::RadiansToDegrees(Calls->Last->Off));
	}
	if (Glimpses->Up != GlimpseLogged)
	{
		GlimpseLogged = Glimpses->Up;
		UE_LOG(LogS47Night, Display, TEXT("S47: glimpse %s"), GlimpseLogged.IsEmpty() ? TEXT("gone") : *(TEXT("at ") + GlimpseLogged));
	}
	// the array put back on the schedule, the recording's walk and the dishes that follow (the time
	// away's block), the corner clock in seconds (S47SaroNight.cpp)
	if (M->SaroNight) M->SaroNight->StepLate(Dt, bModal);
}

// ---------- the saves (main.ts startGame, leavePlay, toTitle, writeSave) ----------

void US47Night::Restore(const S47J::FObj& State)
{
	Flashlight->bHave = S47J::Bool(State, TEXT("flashlight")); Flashlight->bOn = false;
	// what was seen in that night, and nothing written down while it is put back (story/seen.ts)
	bSeenQuiet = true;
	const TArray<TSharedPtr<FJsonValue>>* Seen = nullptr;
	SeenLog->Restore(State.IsValid() && State->TryGetArrayField(TEXT("seen"), Seen) ? Seen : nullptr);
	// where the night's curve was (a case without one goes on after a short quiet; a save from the
	// prologue, which keeps no case yet, gets the plain ten minutes when chapter one begins)
	Curve->Restore(S47J::Obj(State, TEXT("curve")), S47J::Has(State, TEXT("case")));
	M->TownSeen.Clear();
	for (const FString& Id : S47J::Strs(State, TEXT("town"))) M->TownSeen.Add(Id);
	Glimpses->Reset(); NightEvents->Reset();
	Album->Restore(S47J::Obj(State, TEXT("album")));   // the free frames of the night that was saved, or none
	FaceQuiet = 0;
	CurveSigns.Reset();
}

void US47Night::Restored()
{
	SeenSync();
	bSeenQuiet = false;
}

void US47Night::Leave(bool bTitle)
{
	if (!bTitle) { NightEvents->Reset(); bSeenQuiet = true; }
	Curve->Restore(nullptr, false);
}

void US47Night::Save(const S47J::FObj& State) const
{
	S47J::Set(State, TEXT("flashlight"), Flashlight->bHave);
	S47J::Set(State, TEXT("town"), M->TownSeen.Array());
	State->SetArrayField(TEXT("seen"), SeenLog->Data());
	S47J::Set(State, TEXT("curve"), Curve->Data());
	S47J::Set(State, TEXT("album"), Album->SaveData());
}

// ---------- the developer menu's, for the tests ----------

void US47Night::TestFlags()
{
	// -S47Night=<event id>: one of the night's events now, as the developer menu's "Night event: <id>"
	// (steps, knocks, radio, bulletin, phone, door; the silence is the roadside's); -S47Call: a sound out
	// in the dark (the flashlight given and switched on); -S47Glimpse[=<spot>]: the figure at a spot in
	// the view now; -S47Light: the flashlight on; -S47Curve=<phase>|free: the curve forced; -S47Album[=face]:
	// a free frame now (the field camera given), with the face if asked
	const TCHAR* Cmd = FCommandLine::Get();
	FString Ev, Spot, Ph, Al;
	const bool bEv = FParse::Value(Cmd, TEXT("S47Night="), Ev), bCall = FParse::Param(Cmd, TEXT("S47Call")), bLight = FParse::Param(Cmd, TEXT("S47Light"));
	const bool bGlimpse = FParse::Value(Cmd, TEXT("S47Glimpse="), Spot) || FParse::Param(Cmd, TEXT("S47Glimpse"));
	const bool bCurve = FParse::Value(Cmd, TEXT("S47Curve="), Ph);
	const bool bAlbum = FParse::Value(Cmd, TEXT("S47Album="), Al) || FParse::Param(Cmd, TEXT("S47Album"));
	if (!(bEv || bCall || bLight || bGlimpse || bCurve || bAlbum) || !M->Web) return;
	TWeakObjectPtr<US47Night> Self = this;
	FTimerHandle H;
	M->Web->GetWorldTimerManager().SetTimer(H, [Self, Ev, bEv, bCall, bLight, bGlimpse, Spot, bCurve, Ph, bAlbum, Al]
	{
		if (!Self.IsValid()) return;
		US47Night* N = Self.Get();
		if (bLight || bCall) { N->Flashlight->bHave = true; N->Flashlight->bOn = true; UE_LOG(LogS47Night, Display, TEXT("S47: flashlight on")); }
		if (bCurve)
		{
			if (Ph == TEXT("free")) N->Curve->bFree = true;
			else UE_LOG(LogS47Night, Display, TEXT("S47: curve %s: %s"), *Ph, N->Curve->Force(Ph, Ph == TEXT("build") ? 0.5 : 0) ? TEXT("forced") : TEXT("only from chapter one to five"));
		}
		if (bEv) UE_LOG(LogS47Night, Display, TEXT("S47: night event %s %s"), *Ev, N->NightEvents->Now(Ev, N->NightIn(true)) ? TEXT("started") : TEXT("not here"));
		if (bCall) { N->Calls->Now(); UE_LOG(LogS47Night, Display, TEXT("S47: a call asked for")); }
		if (bGlimpse)
		{
			static const TCHAR* SPOT_IDS[] = { TEXT("S-11"), TEXT("S-14"), TEXT("S-19"), TEXT("S-22"), TEXT("town-south"), TEXT("town-mast"), TEXT("town-west"), TEXT("diner-west"), TEXT("diner-north"), TEXT("shoulder") };
			bool bUp = false;
			const FS47GlimpsesIn G = N->GlimpseIn(true);
			if (!Spot.IsEmpty()) bUp = N->Glimpses->Now(G, Spot);
			else for (const TCHAR* Id : SPOT_IDS) if ((bUp = N->Glimpses->Now(G, Id))) break;
			UE_LOG(LogS47Night, Display, TEXT("S47: glimpse asked for: %s"), bUp ? *N->Glimpses->Up : TEXT("nowhere in the view"));
		}
		if (bAlbum && N->M->FieldCamera)
		{
			N->M->FieldCamera->bHave = true;
			// (face, ghost, open: joined with '+', the command line stops a value at a comma)
			if (Al.Contains(TEXT("face"))) N->Album->bForceFace = true;
			if (Al.Contains(TEXT("ghost"))) N->Album->bForceGhost = true;
			const FS47AlbumShot S = N->Album->Expose();
			if (N->Album->Last) UE_LOG(LogS47Night, Display, TEXT("S47: album names %s, odd %s, mean %.1f, %.0f ms%s%s"), *FString::Join(N->Album->Last->Names, TEXT(", ")), *FString::Join(N->Album->Last->Odd, TEXT(" ")),
				N->Album->Last->Mean, N->Album->Last->Ms, N->Album->Last->bFace ? TEXT(", the face") : TEXT(""), N->Album->Last->bGhost ? TEXT(", someone on the film") : TEXT(""));
			// -S47AlbumDump=<dir>: the print as the save holds it, for a look
			FString Dir;
			TArray<uint8> Jpeg;
			if (FParse::Value(FCommandLine::Get(), TEXT("S47AlbumDump="), Dir) && FBase64::Decode(S.Url.Mid(S.Url.Find(TEXT(",")) + 1), Jpeg))
			{
				FFileHelper::SaveArrayToFile(Jpeg, *(Dir / S.Id.Replace(TEXT("@"), TEXT("_")) + TEXT(".jpg")));
			}
			// the album's page in the notebook, the print still coming up
			if (Al.Contains(TEXT("open")) && N->M->Flow) { N->M->UI->NotebookAt(TEXT("photos")); N->M->Flow->OpenNotebook(); UE_LOG(LogS47Night, Display, TEXT("S47: album opened, %s"), *S.Id); }
		}
	}, 3.0f, false);
}

bool US47Night::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s night: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// Grounds.floorAt: the floors, the ground, the ramp half way, the array, the motel
	Check(SaroFloorAt(nullptr, 0, 0) == 0 && FMath::IsNearlyEqual(SaroFloorAt(nullptr, 9.8, 14.5), GROUND * 0.5, 1e-9) && SaroFloorAt(nullptr, -12, -208) == GROUND && FMath::IsNearlyEqual(SaroFloorAt(nullptr, -40, 680), S47Geo::MOTEL_AT().Y),
		TEXT("the ground under SARO's scene"));
	// the townSeen key set writes a fresh key down once
	FS47TownSeen T;
	TArray<FString> Fresh;
	T.OnFresh = [&Fresh](const FString& K) { Fresh.Add(K); };
	T.Add(TEXT("ne:steps1")); T.Add(TEXT("ne:steps1")); T.Add(TEXT("mark:cows@12000"));
	Check(Fresh.Num() == 2 && T.Contains(TEXT("mark:cows@12000")), TEXT("a key is written down once, when it is fresh"));
	// and each module of the night's layer
	bOk &= S47Seen::SelfTest(Lines);
	bOk &= S47Lines::SelfTest(Lines);
	bOk &= FS47NightCurve::SelfTest(Lines);
	bOk &= FS47NightEvents::SelfTest(Lines);
	bOk &= FS47NightCalls::SelfTest(Lines);
	bOk &= FS47Glimpses::SelfTest(Lines);
	bOk &= FS47Flashlight::SelfTest(Lines);
	bOk &= S47FilmFace::SelfTest(Lines);
	bOk &= FS47Album::SelfTest(Lines);
	return bOk;
}
