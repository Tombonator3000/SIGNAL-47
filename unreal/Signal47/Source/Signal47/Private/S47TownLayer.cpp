// main.ts: the town, the road, the herd, the ninth, the office's television and the morning.
#include "S47TownLayer.h"
#include "S47Audio.h"
#include "S47Ambience.h"
#include "S47Caliche.h"
#include "S47Chapter.h"
#include "S47Curve.h"
#include "S47Cutouts.h"
#include "S47Diner.h"
#include "S47FieldCamera.h"
#include "S47Flashlight.h"
#include "S47Flow.h"
#include "S47Geo.h"
#include "S47Glimpses.h"
#include "S47Herd.h"
#include "S47Inter.h"
#include "S47Image.h"
#include "S47Lines.h"
#include "S47Main.h"
#include "S47MotelTv.h"
#include "S47MorningHolds.h"
#include "S47Night.h"
#include "S47NinthCow.h"
#include "S47NinthModel.h"
#include "S47Pasture.h"
#include "S47Phones.h"
#include "S47Radio.h"
#include "S47Roadside.h"
#include "S47RoadTerrain.h"
#include "S47Score.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Town.h"
#include "S47TownEvents.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Base64.h"
#include "TimerManager.h"
#if __has_include("S47Chapter3.h")
#include "S47Chapter3.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogS47Town, Log, All);
namespace
{
	int32 ChNo(const FString& K) { return K.StartsWith(TEXT("ch")) ? FCString::Atoi(*K.Mid(2)) : 0; }
	double DistXZ(const FVector& A, const FVector& B) { return FVector2D(A.X - B.X, A.Z - B.Z).Size(); }
	double Smooth(double A, double B, double V) { const double K = FMath::Clamp((V - A) / (B - A), 0., 1.); return K * K * (3 - 2 * K); }
}
FString US47TownLayer::Area() const { return M->World ? M->World->Area : TEXT("saro"); }
FString US47TownLayer::Phase() const { return M->Story ? M->Story->ChapterKey : TEXT("prologue"); }
bool US47TownLayer::At(const FString& Ch) const { return ChNo(Phase()) >= ChNo(Ch); }
bool US47TownLayer::Driving() const { return M->World && M->World->bDriving; }
bool US47TownLayer::Cinematic() const { return M->Story && M->Story->IsCinematic(); }
double US47TownLayer::Clock() const { return M->Story ? M->Story->Clock() : 0; }
FS47Cam US47TownLayer::Camera() const { return FS47Cam::Of(M->Player, Area() == TEXT("roswell") ? 12000 : 5000); }
FVector US47TownLayer::PlayerAt() const { if (Driving()) if (const FS47Truck* T = M->World->CabTruck()) return T->Position; return M->Player ? M->Player->Pos : FVector::ZeroVector; }

void US47TownLayer::Init(US47Main* InM)
{
	M = InM;
	Town = MakeShared<FS47Town>(M); Town->OnCarCollider = [this](const FS47Area& Box) { M->World->TownCarMoved(Box); }; Diner = MakeShared<FS47Diner>(M); Caliche = MakeShared<FS47Caliche>(M, Town.Get());
	Caliche->OnCharacterStart = Caliche->OnEarlStart = [this] { M->Audio->StopPhoneCalls(); };
	{ auto Before = M->Audio->OnPhoneCall; M->Audio->OnPhoneCall = [this, Before] { Caliche->StopSpeech(); if (Before) Before(); }; }
	TownEvents = MakeShared<FS47TownEvents>(M); TownEvents->Bind(Town.Get()); Roadside = MakeShared<FS47Roadside>(M);
	Cutouts = MakeShared<FS47Cutouts>(M, [this](bool bFirst)
	{
		if (bFirst) M->Story->Note(M->Story->ClockText(false) + TEXT(". A cardboard alien, standing where nobody would put one. Nobody was there."));
	}, [this](bool bLooked)
	{
		M->TownSeen.Add(TEXT("cut:post"));
		M->Story->Note(M->Story->ClockText(false) + TEXT(". ") + M->Flow->PlaceName(PlayerAt()) + TEXT(". The same shape. There was nothing there when I got to it."));
		if (!M->Night->bSeenQuiet) { FS47SawOpts O; O.At = Cutouts->Position() + FVector(0, .8, 0); O.bLookingSet = true; O.Looking = bLooked; O.bNoWatch = true; M->Night->Saw(TEXT("scare"), TEXT("cutout, post"), O); }
		M->RequestAutosave(false);
	});
	// World.herds has one copy on SARO's highway and one on the road's ground.
	Herds.Add(MakeShared<FS47Herd>(M, TEXT("world.herds.0"), TEXT("saro")));
	Herds.Add(MakeShared<FS47Herd>(M, TEXT("world.herds.1"), TEXT("road")));
	FS47PastureDeps Pd; Pd.Seen = &M->TownSeen;
	Pd.Herds = [this] { TArray<FS47Herd*> H; for (const auto& P : Herds) H.Add(P.Get()); return H; };
	Pd.Note = [this](const FString& T) { M->Story->Note(T); }; Pd.Toast = [this](const FString& T, double S) { M->UI->Toast(T, S); };
	Pd.Save = [this] { if (M->Mode == US47Main::EMode::Play) M->RequestAutosave(false); }; Pd.NinthFound = [this] { return M->TownSeen.Contains(TEXT("nine:found")); };
	Pasture = MakeShared<FS47Pasture>(Pd); NinthModel = MakeShared<FS47NinthModel>(M);
	FS47NinthDeps Nd; Nd.Seen = &M->TownSeen; Nd.Site = [this] { return NinthModel.Get(); }; Nd.TakeNinth = [this] { return Pasture->TakeNinth(); };
	Nd.Note = Pd.Note; Nd.Toast = Pd.Toast; Nd.Clock = [this] { return Clock(); }; Nd.Save = Pd.Save; Nd.Refresh = [this] { if (M->Board.Refresh) M->Board.Refresh(); };
	Nd.Hush = [this](double Secs) { if (M->Audio->Night) M->Audio->Night->Hush(Secs, .04); };
	Nd.Tick = [this](const FVector& P)
	{
		if (!NinthEar || FVector::Distance(NinthEarAt, P) > .5) { NinthEarAt = P; NinthEar = M->Audio->Spatial(P + FVector(0, .3, 0), 3); }
		if (NinthEar) { FS47Sound O; O.Gain = .55; O.Rate = 2.0 + FMath::FRand() * .5; O.Lp = 4800; O.DestNode = NinthEar; M->Audio->Sfx(TEXT("click"), O); }
	};
	Ninth = MakeShared<FS47NinthCow>(Nd);
	for (const FString& Part : { FString(TEXT("head")), FString(TEXT("body")), FString(TEXT("frost")) })
	{
		FS47Interactable I; I.Id = TEXT("s1ninth:") + Part; I.Object = Part == TEXT("head") ? NinthModel->HeadHit : Part == TEXT("body") ? NinthModel->BodyHit : NinthModel->FrostHit;
		I.Range = 2.6; I.ExamineRange = Part == TEXT("frost") ? 14 : 16; I.bLook = Part == TEXT("frost");
		I.Label = [this, Part] { return Part != TEXT("frost") && Area() == TEXT("station01") && M->Mode == US47Main::EMode::Play ? Ninth->Label() : FString(); };
		I.Use = [this, Part] { if (Part != TEXT("frost")) Ninth->Use(); }; I.Examine = [this, Part] { return Area() == TEXT("station01") ? Ninth->Examine(Part, Camera()) : FString(); }; M->Inter->Add(I);
	}
	NinthModel->SetNinth(Ninth->Down());
	for (const auto& H : Herds) for (int32 I = 0; I < H->LookAreas.Num(); ++I)
	{
		FS47Interactable It; It.Id = FString::Printf(TEXT("look:cow:%s:%d"), *H->Area, I); It.Object = H->LookAreas[I]; It.bLook = true; It.ExamineRange = 9; It.Label = [] { return FString(); }; It.Use = [] {};
		It.Examine = [this, I] { const FString Tag = FString::Printf(TEXT("A yellow tag in her left ear: %s %s."), S47Herd::TAG_RANCH, *S47Herd::Tags()[I]); return (Pasture->Stage == TEXT("still") || Pasture->Stage == TEXT("short") ? FString(TEXT("She stands quite still, her head up, facing the way the others face. ")) : FString(TEXT("A Hereford cow, red with a white face. "))) + Tag; }; M->Inter->Add(It);
	}
	FS47MotelTvDeps Td; Td.Truck = [this] { const auto* T = M->World ? M->World->SaroTruck() : nullptr; return T ? T->Group : FS47Obj(); };
	Td.File = [this](const FS47Doc& D) { M->Story->AddDoc(D); }; Td.Document = [this]() -> TOptional<FS47Doc> { for (const FS47Doc& D : M->Story->Docs()) if (D.Id == TEXT("motel-tv-frame")) return D; return {}; };
	Td.Open = [this](const FS47Doc& D) { M->UI->Document(D, [] {}); }; Td.Note = Pd.Note; Td.Toast = Pd.Toast; Td.Save = Pd.Save;
	MotelTv = MakeShared<FS47MotelTv>(M, Td); DinerCooler = MakeShared<FS47DinerCooler>(M->Audio); Morning = MakeShared<FS47MorningHolds>(M, DinerCooler.Get());
	Morning->RadioHeard = [this] { return M->DinerRadio && M->DinerRadio->Live() && !Driving() && M->Player && FVector::Distance(M->Player->Pos, M->CounterAt) < 12; };
	Morning->Radio = [this](double K) { if (M->DinerRadio) M->DinerRadio->Power = K; };
	Morning->Saw = [this] { if (M->Night->bSeenQuiet) return; FS47SawOpts O; O.bLookingSet = true; O.bNoWatch = true; M->Night->Saw(TEXT("event"), TEXT("the morning holds"), O); };
	FS47NightHooks& H = M->Night->Hooks;
	H.RoadsideSilent = [this] { return Roadside->bSilent; }; H.RoadsideSilence = [this] { SilencePlace = M->Flow->PlaceName(); Roadside->SilenceNow(RoadsideIn(false)); };
	H.TownEventsNow = [this] { return TownEvents->Now; }; H.TownEventsWaiting = [this] { return TownEvents->bWaiting; };
	H.CutoutPost = [this] { return Cutouts->Up == TEXT("post"); }; H.NinthQuiet = [this] { return Ninth->Quiet > 0; };
	H.MotelTvBusy = [this] { return MotelTv->Busy(); }; H.MotelTvWaiting = [this] { return MotelTv->bWaiting; }; H.MorningBusy = [this] { return Morning->Busy(); };
	if (M->Score) { auto& Held = M->Score->Held; Held.RoadsideSilent = H.RoadsideSilent; Held.MotelTvHeard = [this] { return MotelTv->Heard(); }; Held.NinthQuiet = [this] { return Ninth->Quiet; }; Held.MorningBusy = H.MorningBusy; }
	if (M->Phones) { auto Before = M->Phones->Foretell; M->Phones->Foretell = [this, Before] { if (Pasture->Stage == TEXT("graze")) Pasture->Now(); if (Before) Before(); }; }
	// Chapters three and five are integrated independently. Bind the available chapter-three hook.
#if __has_include("S47Chapter3.h")
	if (US47Chapter3* C = Cast<US47Chapter3>(M->Story->Chapter(TEXT("ch3")))) C->FreeShot = [this] { return FreeShot(); };
#endif
	// World.enter's Player.floor (Town.floorAt includes the diner); this also makes a direct
	// placement land on its area's ground, rather than raycasting from the previous area's height.
	{
		auto Before = M->World->OnEnter;
		M->World->OnEnter = [this, Before](const FString& A)
		{
			if (Before) Before(A);
			if (A == TEXT("diner")) M->Player->Floor = [this](double X, double Z) { return Town->FloorAt(X, Z); };
			else if (A == TEXT("saro")) M->Player->Floor = [this](double X, double Z) { return US47Night::SaroFloorAt(M->Web, X, Z); };
			else M->Player->Floor = {};
		};
		M->Player->Floor = [this](double X, double Z) { return US47Night::SaroFloorAt(M->Web, X, Z); };
	}

	if (M->Player)
	{
		auto Before = M->Player->OnStep; M->Player->OnStep = [this, Before] { if (Roadside->bSilent) return; if (Before) Before(); else M->Audio->Step(M->Night->SurfaceAt()); };
	}

	TestFlags();
}

FS47RoadsideIn US47TownLayer::RoadsideIn(bool bModal) const
{
	FS47RoadsideIn O; const auto& H = M->Night->Hooks; const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	O.bOk = !bModal && !Cinematic() && !Driving() && Area() == TEXT("saro") && P.Z > 64 - 20 && P.X > -33.5 && P.X < -15 && !M->World->AtMotel(P) && At(TEXT("ch1")) && Phase() != TEXT("ch6") && M->Night->Dawn() < .3 && !Is(H.ArrayBackBusy);
	O.Pos = P; O.Cam = Camera(); for (const FString& N : M->Story->Notes()) if (N.Contains(TEXT("No sound for 47 s"))) O.bSilenceDone = true;
	O.Hush = [this](double S, double K) { if (M->Audio->Night) M->Audio->Night->Hush(S, K); };
	O.Flicker = [this](double K) { M->Night->Flashlight->Flicker(K, TEXT("silence")); };
	O.SilenceOver = [this] { M->Story->Note(S47Text::ClockText(Clock(), false) + TEXT(". ") + (SilencePlace.IsEmpty() ? FString(TEXT("The highway shoulder between SARO and the motel")) : SilencePlace) + TEXT(". No sound for 47 s. Not my footsteps either.")); const_cast<US47TownLayer*>(this)->SilencePlace.Empty(); M->RequestAutosave(false); };
	O.May = [this](const FString& W) { return M->Night->Curve->May(W == TEXT("car") ? TEXT("event") : W == TEXT("silence") ? TEXT("long") : TEXT("glimpse")); }; return O;
}
void US47TownLayer::RoadsideStep(double Dt, bool bModal) { Roadside->Update(Dt, RoadsideIn(bModal)); }
void US47TownLayer::TownStep(double Dt, bool bModal)
{
	const auto& H = M->Night->Hooks; FS47TownEventsIn S;
	S.bOk = !bModal && !Cinematic() && At(TEXT("ch1")) && Phase() != TEXT("ch6") && (M->Night->Dawn() < .3 || Phase() == TEXT("ch5")) && !Is(H.AbductionBusy) && !M->Night->NightEvents->Busy();
	S.bInTown = Area() == TEXT("diner") && !M->World->Indoors(Camera().Pos) && Town->InTown(Camera().Pos.X, Camera().Pos.Z); S.bLater = At(TEXT("ch5"));
	S.Cam = Camera(); S.Clock = Clock(); S.T = M->T; S.bMay = M->Night->Curve->May(TEXT("event")); S.bMayLong = M->Night->Curve->May(TEXT("long")); S.Unrest = M->Night->Curve->Unrest(); TownEvents->Update(Dt, S);
	if (TownEvents->Now != LoggedTown) { LoggedTown = TownEvents->Now; UE_LOG(LogS47Town, Display, TEXT("S47: town event %s"), *LoggedTown); }
}
FS47CutoutsIn US47TownLayer::CutoutIn(bool bOk) const
{
	FS47CutoutsIn S; S.bOk = bOk; S.Cam = Camera(); S.Clock = Clock(); const FS47GlimpsesIn G = M->Night->GlimpseIn(bOk);
	if (!Driving() && M->Player)
	{
		S.Foot.Emplace(); S.Foot->Pos = M->Player->Pos; S.Foot->Walkable = [this](double X, double Z) { return M->Player->Walkable(X, Z); }; S.Foot->Floor = [this](double X, double Z) { return Area() == TEXT("saro") ? US47Night::SaroFloorAt(M->Web, X, Z) : Area() == TEXT("diner") ? Town->FloorAt(X, Z) : 0.; };
		S.Foot->Blocked = [this](double X, double Z) { for (const FS47Rect& C : M->Web->Colliders()) if (C.bEnabled && X > C.MinX - .45 && X < C.MaxX + .45 && Z > C.MinZ - .45 && Z < C.MaxZ + .45) return true; return false; };
	}
	const FS47Truck* Tr = Driving() ? M->World->TruckHere() : nullptr;
	if (Tr && (Area() == TEXT("saro") || Area() == TEXT("road") || Area() == TEXT("diner"))) { S.Drive.Emplace(); S.Drive->Pos = Tr->Position; S.Drive->Dir = S.Cam.Forward; S.Drive->Ground = G.Ground; }
	return S;
}
void US47TownLayer::CutoutsStep(double Dt, bool bModal)
{
	const auto& H = M->Night->Hooks; auto S = CutoutIn(!bModal && !Cinematic() && At(TEXT("ch1")) && !Is(H.AbductionBusy) && !Is(H.ArrayBackBusy));
	S.May = M->Night->Curve->May(TEXT("cutout")) && M->Night->Dawn() < .3; S.Keep = M->Night->Curve->CutoutKeep(); S.Pace = M->Night->Curve->Pace(TEXT("cutout"));
	S.Flood = Driving() ? M->Web->FloodSet(Area() == TEXT("saro") ? TEXT("site") : Area() == TEXT("road") || Area() == TEXT("diner") ? TEXT("road") : TEXT("")) : -1;
	S.Blink = [this] { if (Driving()) M->World->Trouble.Start(ES47Trouble::Blink); else PostFlick = .3; }; Cutouts->Update(Dt, S);
}
void US47TownLayer::EventsOnB(double Dt, bool bModal)
{
	const auto& H = M->Night->Hooks; const bool bOk = !bModal && !Cinematic() && At(TEXT("ch1")) && Phase() != TEXT("ch6") && M->Night->Dawn() < .3 && !Is(H.AbductionBusy) && !Is(H.OfficePhoneBusy) && !Is(H.ArrayBackBusy);
	FS47PastureIn P; P.bOk = bOk; P.Area = Area() == TEXT("saro") ? TEXT("saro") : Area() == TEXT("road") || Area() == TEXT("diner") ? TEXT("road") : TEXT(""); P.Cam = Camera(); P.Clock = Clock(); P.bDriving = Driving(); Pasture->Update(Dt, P);
	FS47MotelTvIn T; T.bOk = bOk; T.bSaro = Area() == TEXT("saro"); const FVector Pos = M->Player->Pos;
	const auto& AtM = S47Geo::MOTEL_AT(); const FVector Door(-41.1 + AtM.X, 0, 24.1 + AtM.Z);
	T.bHere = T.bSaro && !Driving() && (M->Web->Rect(TEXT("world.motel.zones[2]")).Contains(Pos) || DistXZ(Pos, Door) < 5); T.bTruckOnLot = M->World->TruckAt == TEXT("motel"); T.Cam = Camera(); T.Clock = Clock(); T.May = M->Night->Curve->May(TEXT("long")); T.Unrest = M->Night->Curve->Unrest(); MotelTv->Update(Dt, T);
	if ((LineBCheck += Dt) > 1 && !M->TownSeen.Contains(TEXT("lineB")))
	{
		LineBCheck = 0; int32 N = 0; for (const auto& Mk : S47TownMarks(M->TownSeen)) if (S47Lines::LineOfMark(Mk.Kind) == TEXT("B")) ++N;
		if (N >= 3) { M->TownSeen.Add(TEXT("lineB")); M->Story->Note(M->Story->ClockText(false) + TEXT(". Three on one line. Drawn in on the map: out of the dishes, across the highway at the turn-off to SARO, and on to the south-west, through STATION 01.")); M->RequestAutosave(false); }
	}
	if (Pasture->Stage != LoggedPasture) { LoggedPasture = Pasture->Stage; UE_LOG(LogS47Town, Display, TEXT("S47: pasture %s"), *LoggedPasture); }
	if (MotelTv->Mode != LoggedTv) { LoggedTv = MotelTv->Mode; UE_LOG(LogS47Town, Display, TEXT("S47: motel television %s"), *LoggedTv); }
}
void US47TownLayer::NinthStep(double Dt, bool bModal)
{
	FS47NinthIn S; S.bOk = !bModal && !Cinematic(); S.bFree = !M->Night->NightGoing() && !M->Audio->PhonePlaying() && !(M->Phones && Is(M->Phones->CbBusy)); S.Chapter = Phase(); S.Area = Area(); S.bDriving = Driving(); S.bInHut = Area() == TEXT("station01") && M->World->Indoors(M->Player->Pos); S.Cam = Camera(); S.bLight = M->Night->Flashlight->bOn; Ninth->Update(Dt, S);
	if (Ninth->Found()) M->Books.AddUnique(Ninth->Book.Get()); else M->Books.Remove(Ninth->Book.Get());
	if (Ninth->State() != LoggedNinth) { LoggedNinth = Ninth->State(); UE_LOG(LogS47Town, Display, TEXT("S47: ninth cow %s"), *LoggedNinth); }
}
FS47MorningIn US47TownLayer::MorningIn(bool bModal) const
{
	FS47MorningIn S; S.bArea = Area() == TEXT("diner") && Diner->Group.IsValid(); const US47Chapter* Ch = M->Story->Chapter(TEXT("ch5")); const auto State = Ch ? Ch->SaveState() : S47J::FObj();
	S.bHere = M->Mode == US47Main::EMode::Play && S.bArea && Phase() == TEXT("ch5") && Ch && Ch->bActive && S47J::Bool(State, TEXT("arrived")) && !Driving() && M->World->Indoors(M->Player->Pos);
	S.bFree = !M->OneAtATime() && !M->Audio->PhonePlaying() && !(M->Phones && Is(M->Phones->CbBusy)) && !Cinematic(); S.bPanel = bModal; S.Clock = Clock(); return S;
}
void US47TownLayer::MorningStep(double Dt, bool bModal)
{
	const bool bDn = Area() == TEXT("diner") && Diner->Group.IsValid(); DinerCooler->Update(bDn ? TOptional<FVector>(Diner->Group.LocalToWorld(FVector(-5.6, .9, -3.2))) : TOptional<FVector>(), M->Space == TEXT("room"));
	if (M->Audio->Night) M->Audio->Night->Window = bDn && M->Space == TEXT("room") ? 1 : 0;
	Morning->Update(Dt, MorningIn(bModal));
	// sameThread.update is next, in S47Phones::StepLate, followed by officePhone.update.
	if (Morning->Phase != LoggedMorning) { LoggedMorning = Morning->Phase; UE_LOG(LogS47Town, Display, TEXT("S47: morning %s"), *LoggedMorning); }
}
void US47TownLayer::WorldStep(double Dt)
{
	const FString A = Area(); const auto Cam = Camera();
	if (A == TEXT("diner") || A == TEXT("road") || A == TEXT("roswell"))
	{
		Diner->Update(Dt, M->T); const auto P = M->Player->Pos; bool bHold = false;
		if (A == TEXT("diner") && !Town->InTown(P.X, P.Z) && !M->World->Indoors(P)) for (const auto& Stop : S47RoadTerrain::ROAD_STOPS()) if (FString(Stop.Name) == TEXT("The highway north of Caliche") && P.X - 8000 >= Stop.MinX && P.X - 8000 <= Stop.MaxX && P.Z >= Stop.MinZ && P.Z <= Stop.MaxZ) bHold = true;
		Town->Update(M->T, Clock(), bHold);
	}
	const FString HArea = A == TEXT("saro") ? TEXT("saro") : A == TEXT("road") || A == TEXT("diner") ? TEXT("road") : TEXT("");
	TOptional<FS47HerdHeadlights> Lamps;
	if (Driving() && M->World->Leg == HArea) if (const FS47Truck* T = M->World->TruckHere())
	{
		(void)T;
		if (const auto* Drive = M->World->CabDrive()) if (Drive->Area && Drive->Area->Headlights && Drive->Area->Headlights->Slots.Num() >= 3)
		{
			const auto& H = *Drive->Area->Headlights; FVector P0, P2; float W0 = 0, W2 = 0; FLinearColor C0, C2;
			if (M->Web->FloodPool(H.Set, H.Slots[0], P0, W0, C0) && M->Web->FloodPool(H.Set, H.Slots[2], P2, W2, C2)) { FS47HerdHeadlights L; L.F0 = FVector4(P0, W0); L.F2 = FVector4(P2, W2); L.ColorR = C0.R; Lamps = L; }
		}
	}
	for (const auto& H : Herds) if (H->Area == HArea) H->Update(M->T, Lamps);
	const FVector O = Diner->Group.WorldPosition();
	if (A == TEXT("road") || A == TEXT("diner")) Diner->SetNear(1 - Smooth(120, 520, DistXZ(Cam.Pos, O)));
	if (A == TEXT("roswell")) { const bool bNear = DistXZ(Cam.Pos, O) < 2600; Diner->Group.SetVisible(bNear); Town->Group.SetVisible(bNear); Diner->SetNear(0); }
}
void US47TownLayer::ShareLights()
{
	const auto Data = M->Web->DriveData(); const auto Legs = S47J::Obj(Data, TEXT("legs")); const FString Source = Area() == TEXT("roswell") ? TEXT("roswell") : Area() == TEXT("road") || Area() == TEXT("diner") ? TEXT("road") : FString();
	const auto H = S47J::Obj(S47J::Obj(Legs, *Source), TEXT("head")); const int32 Set = S47J::Int(H, TEXT("set"), -1); TArray<int32> Slots; for (double V : S47J::Nums(H, TEXT("slots"))) Slots.Add(int32(V));
	Diner->Shine(Set, Slots, 2.5); Town->Shine(Set, Slots, 1.8);
	TArray<int32> TownSlots; for (double V : S47J::Nums(Data, TEXT("townSlots"))) TownSlots.Add(int32(V));
	const auto RoadHead = S47J::Obj(S47J::Obj(Legs, TEXT("road")), TEXT("head")); const int32 RoadSet = S47J::Int(RoadHead, TEXT("set"), -1);
	Town->LampsTo(RoadSet, TownSlots, Camera().Pos, .8);
	const auto Neon = S47J::Obj(Data, TEXT("neonSlots"));
	for (const FString& K : { FString(TEXT("road")), FString(TEXT("old")) })
	{
		TArray<int32> Ns; for (double V : S47J::Nums(Neon, *K)) Ns.Add(int32(V));
		const auto To = S47J::Obj(S47J::Obj(Legs, K == TEXT("road") ? TEXT("road") : TEXT("roswell")), TEXT("head"));
		Diner->Neon(S47J::Int(To, TEXT("set"), -1), Ns, 2, .3);
	}
}

bool US47TownLayer::FreeShot() const { if (Area() != TEXT("station01") || !NinthModel->bNinthDown || !M->FieldCamera) return false; const FVector P = NinthModel->NinthAt(TEXT("middle")); return FVector::Distance(P, Camera().Pos) < 14 && M->FieldCamera->InFrame(P, .2, .8, .12, .88); }

void US47TownLayer::CurveStep(double Dt)
{
	PostFlick = FMath::Max(0., PostFlick - Dt); M->Night->Flashlight->Flicker(PostFlick > 0 ? .95 : 0, TEXT("post"));
	if (Cutouts->Placed != CutPlaced) { if (Cutouts->Placed > CutPlaced && Cutouts->Up != TEXT("post")) M->Night->Curve->Started(TEXT("cutout")); CutPlaced = Cutouts->Placed; }
	PostStep(Dt);
}
void US47TownLayer::PostStep(double Dt)
{
	const auto* Tr = Driving() ? M->World->TruckHere() : nullptr;
	if (Tr) { const double D = DistXZ(Tr->Position, PostLast); PostSpeed = Dt > 0 && D < 60 ? FMath::Lerp(PostSpeed, D / Dt, .2) : 0; PostLast = Tr->Position; } else PostSpeed = 0;
	const auto C = M->Night->Curve;
	if (!Tr || !Cutouts->Up.IsEmpty() || PostSpeed < 7 || (Area() != TEXT("road") && Area() != TEXT("saro")) || PostBuild == C->Builds || !C->May(TEXT("post")) || M->TownSeen.Contains(TEXT("cut:post")) || M->Night->Dawn() >= .3) return;
	const FVector O = Area() == TEXT("road") ? FVector(8000, 0, 0) : FVector(-S47Geo::SARO_IN_ROAD.X, 0, -S47Geo::SARO_IN_ROAD.Z);
	const double Lx = Tr->Position.X - O.X, Lz = Tr->Position.Z - O.Z;
	FVector F = Camera().Forward; F.Y = 0; F.Normalize(); if (FMath::Abs(Lx) > 5.5 || FMath::Abs(F.Z) < .92) return;
	int32 Met = 0; for (const auto& E : M->Night->SeenLog->Entries) if ((E->Kind == TEXT("scare") && (E->What == TEXT("cutout") || E->What == TEXT("cutout, road"))) || (E->Kind == TEXT("event") && E->What == TEXT("cutout"))) ++Met;
	if (Met < 2) return;
	const double Dir = FMath::Sign(F.Z); TArray<FVector2D> Spots;
	// drive/roadProps.ts delineatorSpots, in the same side and z order.
	for (const int32 Side : { -1, 1 }) for (double Z = -1000 + (Side > 0 ? 40 : 0); Z <= 2900; Z += 80)
	{
		if (Side < 0 && Z > 470 && Z < 570) continue;
		if (Side < 0 && Z > S47RoadShape::MOTEL().Z - 42 && Z < S47RoadShape::MOTEL().Z + 42) continue;
		if (Z > S47RoadShape::DINER().Z - 30 && Z < S47RoadShape::TOWN().Z1) continue;
		const double A = (Z - Lz) * Dir; if (A > 150 && A < 210 && (Area() == TEXT("road") || (Z > -225 && Z < 600))) Spots.Add(FVector2D(Side * 7.2, Z));
	}
	if (Spots.IsEmpty()) return;
	Spots.StableSort([Dir](const FVector2D& P, const FVector2D& Q) { return (FMath::Sign(P.X) == -Dir ? 1 : 0) > (FMath::Sign(Q.X) == -Dir ? 1 : 0); });
	PostBuild = C->Builds; const auto& P = Spots[0]; Cutouts->PlacePost(FVector(P.X + O.X, S47RoadTerrain::SurfaceY(P.X, P.Y) + O.Y, P.Y + O.Z), Tr->Position);
}
void US47TownLayer::SeenSync() { WasCutSeen = Cutouts->Seen; WasRoad = Roadside->Last ? Roadside->Last->What : FString(); WasRoadAt = Roadside->Last ? Roadside->Last->At : -1; }
void US47TownLayer::SeenStep()
{
	if (M->Night->bSeenQuiet) return;
	if (Cutouts->Seen != WasCutSeen)
	{
		if (Cutouts->Seen > WasCutSeen) { FS47SawOpts O; O.At = Cutouts->Where(); O.bLookingSet = true; O.Looking = true; O.bNoWatch = true; M->Night->Saw(TEXT("scare"), Cutouts->Up == TEXT("shoulder") ? TEXT("cutout, road") : TEXT("cutout"), O); }
		WasCutSeen = Cutouts->Seen;
	}
	if (Roadside->Last && (Roadside->Last->What != WasRoad || Roadside->Last->At != WasRoadAt))
	{
		WasRoad = Roadside->Last->What; WasRoadAt = Roadside->Last->At; FS47SawOpts O;
		if (WasRoad == TEXT("car")) { O.At = Roadside->CarAt(); FS47SeenWatch W; W.Pos = [this] { return Roadside->CarAt(); }; W.Secs = 70; W.Range = 3000; O.Watch = W; M->Night->Saw(TEXT("scare"), TEXT("car lights"), O); }
		else if (WasRoad == TEXT("silence")) { O.bLookingSet = true; M->Night->Saw(TEXT("scare"), TEXT("silence"), O); }
		else if (WasRoad == TEXT("roof")) { O.At = Roadside->RoofAt(); FS47SeenWatch W; W.Pos = [this] { return Roadside->RoofAt(); }; W.Secs = 80; W.Deg = 25; W.Range = 300; O.Watch = W; M->Night->Saw(TEXT("glimpse"), TEXT("motel roof"), O); }
	}
}
void US47TownLayer::ResetTown() { Caliche->StopSpeech(); TownEvents->Reset(); }
void US47TownLayer::ResetCutouts() { Cutouts->Reset(); }
void US47TownLayer::ResetPastureTv() { Pasture->Reset(); MotelTv->Reset(); }
void US47TownLayer::ResetNinthMorning() { Ninth->Reset(); if (!Ninth->Found()) M->Books.Remove(Ninth->Book.Get()); Morning->Reset(); }
void US47TownLayer::Refile(const S47J::FObj& State)
{
	for (const FS47Doc& D : FS47Caliche::TownWitnessDocs(M->TownSeen)) M->Story->AddDoc(D);
	MotelTv->Refile(S47J::Str(S47J::Obj(State, TEXT("motelTv")), TEXT("url"))); Roadside->Reset();
}
void US47TownLayer::Save(const S47J::FObj& State) const
{
	FString Frame; for (const FS47Doc& D : M->Story->Docs()) if (D.Id == TEXT("motel-tv-frame")) Frame = D.Image;
	if (!Frame.IsEmpty()) { auto Tv = S47J::New(); S47J::Set(Tv, TEXT("id"), TEXT("motel-tv-frame")); S47J::Set(Tv, TEXT("url"), Frame); S47J::Set(State, TEXT("motelTv"), Tv); } else State->RemoveField(TEXT("motelTv"));
}
// main.ts: leavePlay resets townEvents; toTitle does not.
void US47TownLayer::Leave(bool bTitle) { Caliche->StopSpeech(); MotelTv->Silence(); Roadside->Reset(); if (!bTitle) TownEvents->Reset(); }

void US47TownLayer::TestFlags()
{
	const TCHAR* Cmd = FCommandLine::Get(); FString TownEv, Road, Herd, Tv, Cow, Cut, Person;
	const bool bTown = FParse::Value(Cmd, TEXT("S47Town="), TownEv), bRoad = FParse::Value(Cmd, TEXT("S47Roadside="), Road);
	const bool bHerd = FParse::Value(Cmd, TEXT("S47Pasture="), Herd) || FParse::Param(Cmd, TEXT("S47Pasture"));
	const bool bTv = FParse::Value(Cmd, TEXT("S47MotelTv="), Tv) || FParse::Param(Cmd, TEXT("S47MotelTv"));
	const bool bCow = FParse::Value(Cmd, TEXT("S47Ninth="), Cow) || FParse::Param(Cmd, TEXT("S47Ninth"));
	const bool bMorning = FParse::Param(Cmd, TEXT("S47Morning"));
	const bool bCut = FParse::Value(Cmd, TEXT("S47Cutout="), Cut) || FParse::Param(Cmd, TEXT("S47Cutout"));
	const bool bPerson = FParse::Value(Cmd, TEXT("S47Caliche="), Person);
	if (!(bTown || bRoad || bHerd || bTv || bCow || bMorning || bCut || bPerson) || !M->Web) return;
	TWeakObjectPtr<US47TownLayer> Self = this; FTimerHandle Timer;
	M->Web->GetWorldTimerManager().SetTimer(Timer, [Self, bTown, TownEv, bRoad, Road, bHerd, Herd, bTv, Tv, bCow, Cow, bMorning, bCut, Cut, bPerson, Person]
	{
		if (!Self.IsValid()) return; auto* L = Self.Get(); auto* M = L->M.Get(); M->Night->Curve->bFree = true;
		if (bTown)
		{
			const bool bSecond = TownEv == TEXT("phone2") || TownEv == TEXT("answer2");
			if (bSecond) M->Story->ChapterKey = TEXT("ch5"); // Developer context: the return call follows Room 6.
			M->World->Enter(TEXT("diner")); M->Story->SetClock(TownEv == TEXT("car") ? 3 * 3600 + 20 * 60 : bSecond ? 5 * 3600 + 10 * 60 : TownEv == TEXT("lamps") ? 3 * 3600 + 10 * 60 : 3 * 3600);
			if (TownEv == TEXT("stop"))
			{
				M->Player->Place(8000, 1690, 0); const bool bPaved = M->World->RoadStopAt(M->Player->Pos) && M->Night->SurfaceAt() == TEXT("concrete");
				M->Player->Place(8008, 1690, 0); const bool bDirt = M->World->RoadStopAt(M->Player->Pos) && M->Night->SurfaceAt() == TEXT("dirt");
				UE_LOG(LogS47Town, Display, TEXT("S47: town road stop %s"), bPaved && bDirt ? TEXT("OK") : TEXT("FAIL"));
			}
			else if (TownEv == TEXT("lamps")) { M->Player->Place(8000, 2150, 0); FS47TownEventsIn S; S.bOk = S.bInTown = true; S.bMayLong = false; S.Clock = L->Clock(); S.T = M->T; S.Cam.Pos = FVector(8000, 1.62, 2150); L->TownEvents->Update(40.1, S); }
			else if (TownEv == TEXT("cutout")) { L->Town->SetCutout(true); const FVector P = L->Town->Group.LocalToWorld(L->Town->CutoutAt); M->Player->Place(P.X, P.Z + 12, 0); }
			else if (TownEv == TEXT("car")) { L->Town->SetDeputy(false); L->Town->SetCarOut(true); const FVector P = L->Town->Group.LocalToWorld(L->Town->CarShoulder); M->Player->Place(P.X, P.Z + 12, 0); }
			else { if (bSecond) M->TownSeen.Add(TEXT("ev:phone1")); const FVector P = L->Town->Proxy(TEXT("payphone")).WorldPosition(); M->Player->Place(P.X - 1.5, P.Z, -PI / 2); L->TownEvents->Reset(); FS47TownEventsIn S; S.bOk = S.bInTown = true; S.bLater = bSecond; S.Cam = L->Camera(); S.Cam.Pos = FVector(P.X - 1.5, P.Y + 1.62, P.Z); S.Clock = M->Story->Clock(); S.T = M->T; L->TownEvents->Update(21, S); if (TownEv == TEXT("answer") || TownEv == TEXT("answer2")) L->TownEvents->Answer(); }
			UE_LOG(LogS47Town, Display, TEXT("S47: town test %s, %s"), *TownEv, *L->TownEvents->Now);
		}
		if (bRoad) { M->World->Enter(TEXT("saro")); M->Player->Place(-24, 220, 0); if (Road == TEXT("car")) L->Roadside->CarNow(M->Player->Pos); else if (Road == TEXT("roof")) { M->Player->Place(-24, 600, 2.3); L->Roadside->RoofNow(); } else L->Roadside->SilenceNow(L->RoadsideIn(false)); UE_LOG(LogS47Town, Display, TEXT("S47: roadside test %s"), *Road); }
		if (bHerd) { M->World->Enter(TEXT("saro")); L->Pasture->Now(); FS47PastureIn S; S.bOk = true; S.Cam = L->Camera(); S.Clock = L->Clock(); L->Pasture->Update(.05, S); M->Player->Place(-55, 856, -PI / 2); if (Herd == TEXT("missing")) L->Pasture->TakeNinth(); UE_LOG(LogS47Town, Display, TEXT("S47: pasture test %s"), *L->Pasture->Stage); }
		if (bTv) { M->World->PlaceTruck(TEXT("motel")); M->World->Enter(TEXT("saro")); const FS47Obj TvObj = M->Web->Obj(TEXT("world.motel.tv.group")); const FVector P = TvObj.WorldPosition(), Front = TvObj.WorldDirection(); M->Player->Place(P.X + Front.X * 1.8, P.Z + Front.Z * 1.8, FMath::Atan2(Front.X, Front.Z)); M->Player->Pitch = 0.04; M->Story->SetClock(3 * 3600 + 20 * 60); L->MotelTv->Now(); UE_LOG(LogS47Town, Display, TEXT("S47: motel television test %s"), *L->MotelTv->Mode);
			if (Tv == TEXT("frame"))
			{
				FTimerHandle FrameTimer; M->Web->GetWorldTimerManager().SetTimer(FrameTimer, [Self]
				{
					if (!Self.IsValid()) return; auto* L = Self.Get(); auto* M = L->M.Get();
					const auto State = S47J::Read(S47J::Write(M->Flow->GameState())); const FString Url = S47J::Str(S47J::Obj(State, TEXT("motelTv")), TEXT("url"));
					L->MotelTv->Reset(); M->Story->DocsMutable().RemoveAll([](const FS47Doc& D) { return D.Id == TEXT("motel-tv-frame"); }); L->MotelTv->Refile(Url);
					const FS47Doc* Doc = M->Story->Docs().FindByPredicate([](const FS47Doc& D) { return D.Id == TEXT("motel-tv-frame"); });
					TArray<FColor> Pixels; int32 Width = 0, Height = 0; double Sum = 0, Sum2 = 0;
					const bool bDecoded = S47Image::DecodePixels(Url, Pixels, Width, Height);
					for (const FColor& P : Pixels) { const double Y = .213 * P.R + .715 * P.G + .072 * P.B; Sum += Y; Sum2 += Y * Y; }
					const double Variance = Pixels.Num() ? Sum2 / Pixels.Num() - FMath::Square(Sum / Pixels.Num()) : 0;
					// This fixed lot must contain roofs, ground and the truck, not an empty or clipped capture.
					const bool bPicture = bDecoded && Width == 320 && Height == 240 && Variance > 25;
					const bool bOk = Doc && !Url.IsEmpty() && Doc->Image == Url && M->TownSeen.Contains(TEXT("ev:tv")) && bPicture;
					UE_LOG(LogS47Town, Display, TEXT("S47: motel television save roundtrip %s, image %d bytes, variance %.2f"), bOk ? TEXT("OK") : TEXT("FAIL"), Url.Len(), Variance);
					FString Dump; if (FParse::Value(FCommandLine::Get(), TEXT("S47TownDump="), Dump))
					{
						FFileHelper::SaveStringToFile(S47J::Write(State, true), *(Dump + TEXT(".json"))); TArray<uint8> Jpg;
						if (FBase64::Decode(Url.Mid(Url.Find(TEXT(",")) + 1), Jpg)) FFileHelper::SaveArrayToFile(Jpg, *(Dump + TEXT(".jpg")));
					}
					if (Doc) M->UI->Document(*Doc, [] {});
				}, 54.f, false);
			}
		}
		if (bCow) { M->World->Enter(TEXT("station01")); L->Ninth->Now(); const FVector P = L->NinthModel->NinthAt(TEXT("middle")); M->Player->Place(P.X, P.Z + 12, PI); FS47NinthIn S; S.bOk = S.bFree = true; S.Area = TEXT("station01"); S.Chapter = TEXT("ch3"); S.Cam.Pos = P + FVector(0, 1.62, 12); S.Cam.Forward = FVector(0, 0, 1); L->Ninth->Update(.05, S); M->Player->Yaw = 0; if (Cow == TEXT("found")) { M->Player->Place(P.X, P.Z + 5, 0); S.Cam.Pos = P + FVector(0, 1.62, 5); S.Cam.Forward = (P - S.Cam.Pos).GetSafeNormal(); S.bLight = true; L->Ninth->Update(2.1, S); } UE_LOG(LogS47Town, Display, TEXT("S47: ninth cow test %s"), *L->Ninth->State()); }
		if (bMorning) { M->World->Enter(TEXT("diner")); M->World->PlaceAtDiner(TEXT("inside")); M->Story->SetClock(S47Morning::FROM); FS47MorningIn S; S.bHere = S.bArea = S.bFree = true; S.Clock = L->Clock(); const bool b = L->Morning->Now(S); UE_LOG(LogS47Town, Display, TEXT("S47: morning test %s"), b ? TEXT("held") : TEXT("not here")); }
		if (bCut) { bool b; if (Cut == TEXT("post")) b = L->Cutouts->PlacePost(L->Camera().Pos + L->Camera().Forward * 18 - FVector(0, 1.62, 0), L->PlayerAt()); else b = L->Cutouts->Place(L->CutoutIn(true)); UE_LOG(LogS47Town, Display, TEXT("S47: cutout test %s %s"), *L->Cutouts->Up, b ? TEXT("placed") : TEXT("nowhere")); }
		if (bPerson) { M->World->Enter(TEXT("diner")); M->World->PlaceAtDiner(TEXT("inside")); M->Story->SetClock(3 * 3600); L->Caliche->Talk(Person); UE_LOG(LogS47Town, Display, TEXT("S47: Caliche conversation %s, people %d"), *Person, FS47Caliche::TownPeople(M->TownSeen).Num()); }
	}, 3.f, false);
}
bool US47TownLayer::SelfTest(TArray<FString>& Lines)
{
	bool bOk = FS47TownEvents::SelfTest(Lines); bOk &= FS47Town::SelfTest(Lines); bOk &= FS47Roadside::SelfTest(Lines); bOk &= FS47Pasture::SelfTest(Lines); bOk &= FS47Herd::SelfTest(Lines); bOk &= FS47NinthCow::SelfTest(Lines); bOk &= FS47NinthModel::SelfTest(Lines); bOk &= FS47MotelTv::SelfTest(Lines); bOk &= FS47MorningHolds::SelfTest(Lines); bOk &= FS47Cutouts::SelfTest(Lines); bOk &= FS47Caliche::SelfTest(Lines); bOk &= FS47Diner::SelfTest(Lines); return bOk;
}
