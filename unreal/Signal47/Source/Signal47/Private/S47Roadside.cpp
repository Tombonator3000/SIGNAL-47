// core/roadside.ts
#include "S47Roadside.h"

#include "S47Audio.h"
#include "S47Geo.h"
#include "S47Main.h"
#include "S47RoadShape.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	const double POLE_X = 15 - S47Geo::SARO_IN_ROAD.X;
	double PoleZ(int32 K) { const double Z0 = -1900 + 45 * K; return Z0 + (S47RoadShape::Hash(Z0, 1) - 0.5) * 4 - S47Geo::SARO_IN_ROAD.Z; }
	const double CAR_LANE = 1.8 - S47Geo::SARO_IN_ROAD.X;
	FVector RoofPoint() { const auto& A = S47Geo::MOTEL_AT(); return FVector(-46.5 + A.X, A.Y + 3.01, 31 + A.Z); }
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
}

FS47Roadside::FS47Roadside(US47Main* InM) : M(InM), Sil(InM ? InM->Web.Get() : nullptr, TEXT("roadside:figure"), 0x2c0e0b, TEXT("turned"))
{
	CarNext = Rnd(25, 60);
	Figure = Sil.Group;
	Figure.SetPosition(RoofPoint());
	Figure.SetRotY(UE_DOUBLE_PI / 2);
	Figure.SetVisible(false);
}

// UObject teardown may already have destroyed the web actor; only release the sound here.
FS47Roadside::~FS47Roadside() { if (Hum) { Hum->Stop(); Hum.Reset(); } }

void FS47Roadside::Update(double Dt, const FS47RoadsideIn& O)
{
	T += Dt;
	const FVector& P = O.Pos;
	if (!O.bOk)
	{
		OnFoot = 0; Walked = 0;
		if (Roof == TEXT("up")) HideRoof();
		if (Roof == TEXT("gone") && P.Z < S47Geo::MOTEL_RECT().MinZ - 200) Roof = TEXT("none");
	}
	else
	{
		Sil.Wake();
		OnFoot += Dt;
		if (P.Z > LastZ) Walked += P.Z - LastZ;
	}
	LastZ = P.Z;
	LineHum(Dt, O);
	CarStep(Dt, O);
	SilenceStep(Dt, O);
	RoofStep(Dt, O);
}

// ---------- the line ----------
void FS47Roadside::LineHum(double, const FS47RoadsideIn& O)
{
	const TSharedPtr<FS47AudioContext> Ctx = M && M->Audio ? M->Audio->Ctx : nullptr;
	if (!Ctx) return;
	FS47AudioContext::FBatch B(*Ctx);
	if (!Hum)
	{
		if (!O.bOk) return;
		const auto G = Ctx->CreateGain(); G->Gain.SetValue(0);
		const auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(420);
		const auto Am = Ctx->CreateGain(); Am->Gain.SetValue(0.75);
		const auto Lfo = Ctx->CreateOscillator(); Lfo->Frequency.SetValue(0.11);
		const auto Lg = Ctx->CreateGain(); Lg->Gain.SetValue(0.25); Lfo->Connect(Lg); Lg->Connect(Am->Gain);
		struct FOsc { double F; const TCHAR* Type; double V; };
		const FOsc Defs[] = { {60, TEXT("sine"), 0.5}, {120, TEXT("sine"), 0.35}, {180, TEXT("triangle"), 0.12}, {240.6, TEXT("sine"), 0.06} };
		TArray<TSharedPtr<FS47OscillatorNode>> Oscs;
		for (const FOsc& Def : Defs)
		{
			const auto S = Ctx->CreateOscillator(); S->SetType(Def.Type); S->Frequency.SetValue(Def.F);
			const auto Sg = Ctx->CreateGain(); Sg->Gain.SetValue(Def.V); S->Connect(Sg); Sg->Connect(Lp); S->Start(); Oscs.Add(S);
		}
		Lfo->Start();
		const auto Pan = StaticCastSharedPtr<FS47PannerNode>(M->Audio->Spatial(FVector(POLE_X, 8.6, 0), 2.0));
		Lp->Connect(Am); Am->Connect(G); G->Connect(Pan);
		Hum = FHum{ G, Pan, [Oscs, Lfo, G, Pan] { for (const auto& S : Oscs) S->Stop(); Lfo->Stop(); G->Disconnect(); if (Pan) Pan->Disconnect(); } };
	}
	const int32 K = int32(FMath::FloorToDouble((O.Pos.Z + S47Geo::SARO_IN_ROAD.Z + 1900) / 45 + 0.5));
	const double Z = PoleZ(K), Now = Ctx->CurrentTime();
	if (Hum->Pan)
	{
		Hum->Pan->PositionX.SetTargetAtTime(POLE_X, Now, 0.1);
		Hum->Pan->PositionY.SetTargetAtTime(8.6, Now, 0.1);
		Hum->Pan->PositionZ.SetTargetAtTime(Z, Now, 0.1);
	}
	const bool bOn = O.bOk && !bSilent && O.Pos.Z > 100;
	Hum->G->Gain.SetTargetAtTime(bOn ? 0.05 : 0, Now, bOn ? 1.2 : 0.3);
}

// ---------- the car far down the highway ----------
void FS47Roadside::CarStep(double Dt, const FS47RoadsideIn& O)
{
	CarAgain = FMath::Max(0.0, CarAgain - Dt);
	if (!Car && O.bOk && !bSilent && CarAgain <= 0 && OnFoot > CarNext && O.Pos.Z > 150 && O.Pos.Z < S47Geo::MOTEL_RECT().MinZ - 20 && (!O.May || O.May(TEXT("car")))) CarNow(O.Pos);
	if (!Car) return;
	FCar& C = *Car;
	C.T += Dt;
	if (C.State == TEXT("come"))
	{
		C.Z -= 11 * Dt;
		if (C.T > 48) { C.State = TEXT("stop"); C.T = 0; }
	}
	else if (C.State == TEXT("stop"))
	{
		if (C.T > 7) { C.State = TEXT("out"); C.T = 0; }
	}
	else
	{
		C.Pts.SetOpacity(FMath::Max(0.0, 1 - C.T / 0.5));
		if (C.T > 0.6) { C.Pts.SetVisible(false); Car.Reset(); return; }
	}
	if (C.State != TEXT("out")) C.Pts.SetOpacity(0.85 + 0.15 * FMath::Sin(T * 9.1) * FMath::Sin(T * 2.3));
	// The export keeps both point vertices at z=2600; moving their parent is the same translation.
	C.Pts.SetZ(C.Z - 2600);
}

void FS47Roadside::CarNow(const FVector& From)
{
	if (Car) return;
	const double Z = FMath::Max(From.Z, 0.0) + 2600;
	const FS47Obj Pts = S47SceneObject(M ? M->Web.Get() : nullptr, TEXT("roadside:car"));
	Car = FCar{ Pts, Z, TEXT("come"), 0 };
	Car->Pts.SetPosition(FVector(0, 0, Z - 2600)); Car->Pts.SetVisible(true); Car->Pts.SetOpacity(0.9);
	CarAgain = 15 * 60; CarNext = Rnd(25, 60);
	Last = FLast{ TEXT("car"), T };
}

TOptional<FS47Roadside::FCarState> FS47Roadside::CarState() const { return Car ? TOptional<FCarState>(FCarState{ Car->State, Car->Z }) : TOptional<FCarState>(); }
TOptional<FVector> FS47Roadside::CarAt() const { return Car && Car->State != TEXT("out") ? TOptional<FVector>(FVector(CAR_LANE, S47Geo::MOTEL_AT().Y + 0.8, Car->Z)) : TOptional<FVector>(); }
TOptional<FVector> FS47Roadside::RoofAt() const { return Roof == TEXT("up") ? TOptional<FVector>(RoofPoint() + FVector(0, 0.6, 0)) : TOptional<FVector>(); }

// ---------- the silence ----------
void FS47Roadside::SilenceStep(double Dt, const FS47RoadsideIn& O)
{
	if (bSilent)
	{
		SilenceT -= Dt;
		if (SilenceT <= 0) { bSilent = false; if (O.SilenceOver) O.SilenceOver(); }
		return;
	}
	if (O.bOk && !O.bSilenceDone && !Car && Walked > 100 && O.Pos.Z > 260 && O.Pos.Z < S47Geo::MOTEL_RECT().MinZ - 90 && (!O.May || O.May(TEXT("silence")))) SilenceNow(O);
}

void FS47Roadside::SilenceNow(const FS47RoadsideIn& O)
{
	if (bSilent) return;
	bSilent = true; SilenceT = 47;
	if (O.Hush) O.Hush(47, 0);
	if (O.Flicker) O.Flicker(0.4);
	Last = FLast{ TEXT("silence"), T };
}

// ---------- someone on the motel's roof ----------
void FS47Roadside::RoofStep(double Dt, const FS47RoadsideIn& O)
{
	const FVector& P = O.Pos;
	const double D = FVector::Distance(P, RoofPoint());
	if (Roof == TEXT("none") && O.bOk && !bSilent && P.Z > S47Geo::MOTEL_RECT().MinZ - 170 && P.Z < S47Geo::MOTEL_RECT().MinZ - 45 && OffCentre(O.Cam) > 32 && (!O.May || O.May(TEXT("roof"))))
	{
		if (FMath::FRand() < 0.6) RoofNow(); else Roof = TEXT("gone");
	}
	if (Roof != TEXT("up")) return;
	RoofT += Dt;
	if (OffCentre(O.Cam) < 25 || D < 40 || RoofT > 75 || !O.bOk) HideRoof();
}

void FS47Roadside::RoofNow() { Roof = TEXT("up"); RoofT = 0; Figure.SetVisible(true); Last = FLast{ TEXT("roof"), T }; }
void FS47Roadside::HideRoof() { Roof = TEXT("gone"); Figure.SetVisible(false); }
double FS47Roadside::OffCentre(const FS47Cam& Cam) const
{
	const FVector To = (RoofPoint() + FVector(0, 0.6, 0) - Cam.Pos).GetSafeNormal();
	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Cam.Forward, To), -1.0, 1.0)));
}

void FS47Roadside::Reset()
{
	bSilent = false; SilenceT = 0; OnFoot = 0; Walked = 0; CarAgain = 0; CarNext = Rnd(25, 60);
	if (Car) { Car->Pts.SetVisible(false); Car.Reset(); }
	HideRoof(); Roof = TEXT("none");
	if (Hum) { Hum->Stop(); Hum.Reset(); }
}

// Silhouette constructor/wake/useArt/pose/rim: use the exported original's body and shell.
FS47Silhouette::FS47Silhouette(AS47Web* Web, const FString& Name, uint32 InEdge, const FString& InWant) : Group(S47SceneObject(Web, Name)), Edge(InEdge), Want(InWant) { Rim(Edge); }
void FS47Silhouette::Wake() { /* The export contains its original geometry; painted cards require a fresh export. */ }
void FS47Silhouette::Pose(const FString& P) { Want = P; }
void FS47Silhouette::Rim(uint32 Hex) { Edge = Hex; const TArray<FS47Obj> Parts = Group.Children(); if (Parts.Num() > 1) Parts[1].SetColor(S47Three::Hex(Hex)); }

bool FS47Roadside::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s roadside: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47Roadside R(nullptr);
	FS47RoadsideIn O;
	int32 Ended = 0; double HushSecs = 0, HushLevel = -1, Flicker = 0;
	O.Hush = [&](double S, double K) { HushSecs = S; HushLevel = K; }; O.Flicker = [&](double K) { Flicker = K; }; O.SilenceOver = [&] { Ended++; };
	R.SilenceNow(O); R.Update(46.99, O);
	Check(R.bSilent && HushSecs == 47 && HushLevel == 0 && Flicker == 0.4 && Ended == 0, TEXT("47 seconds, no ambience, a 0.4 flicker"));
	R.Update(0.01, O); R.Update(1, O);
	Check(!R.bSilent && Ended == 1, TEXT("the silence ends once after exactly 47 seconds"));
	R.Reset(); R.CarNow(FVector(0, 0, 300));
	Check(R.CarState() && R.CarState()->State == TEXT("come") && R.CarAt()->Z == 2900, TEXT("headlights begin 2.6 km south"));
	R.Update(48.01, O); Check(R.CarState()->State == TEXT("stop") && FMath::IsNearlyEqual(R.CarAt()->Z, 2900 - 11 * 48.01), TEXT("11 m/s for 48 seconds, then stop"));
	R.Update(7.01, O); Check(R.CarState()->State == TEXT("out") && !R.CarAt(), TEXT("after seven seconds the lights go out"));
	R.Update(0.61, O); Check(!R.CarState(), TEXT("the lights are gone after their fade"));
	R.RoofNow(); O.bOk = true; O.Cam.Pos = RoofPoint() + FVector(0, 0, 100); O.Pos = O.Cam.Pos; O.Cam.Forward = (RoofPoint() + FVector(0, 0.6, 0) - O.Cam.Pos).GetSafeNormal();
	R.Update(0.1, O); Check(!R.RoofUp(), TEXT("looked at, the one on the roof is not there"));
	return bOk;
}
