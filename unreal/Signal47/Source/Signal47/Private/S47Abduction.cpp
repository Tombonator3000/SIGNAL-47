// story/abduction.ts: the time away
#include "S47Abduction.h"

#include "Components/PointLightComponent.h"
#include "Engine/Texture2D.h"
#include "S47Figures.h"
#include "S47Flashlight.h"
#include "S47Image.h"
#include "S47Raster.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Abduction, Log, All);

namespace
{
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); } // Math.round
	double Smooth(double X, double Lo, double Hi) { if (X <= Lo) return 0; if (X >= Hi) return 1; const double T = (X - Lo) / (Hi - Lo); return T * T * (3 - 2 * T); } // MathUtils.smoothstep
	double AngleTo(const FVector& A, const FVector& B)
	{
		const double D = FMath::Sqrt(A.SizeSquared() * B.SizeSquared());
		return D == 0 ? UE_DOUBLE_HALF_PI : FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B) / D, -1.0, 1.0));
	}
	FString Clock(double S) { return S47Text::ClockText(S, false); }

	const int32 MAX = 3;
	const double GAP = 40 * 60, MIN_LOST = 17 * 60;
	// Over you on waking, the three times: whether the stars come first (then the blink, the head
	// lifted behind it); how far off each one stands beyond your feet (metres), how far aside in the
	// frame (of the half width) and how (the nearest with its head bent down to you); and up, where
	// the nearest one's head is in the frame (of the half height above the middle). Then how long it
	// stays, at most.
	const double STARS_T = 1.3, BLINK_T = 0.24, OVER_T = 1.5;
	using EPose = FS47CardSilhouette::EPose;
	struct FOverAt { double Dist, Side; EPose Pose; };
	struct FOverSpec { bool bStars; double Up; TArray<FOverAt> At; };
	const TArray<FOverSpec>& OVER()
	{
		static const TArray<FOverSpec> V = {
			{ true, 0.66, { { 2.8, 0.2, EPose::Bent } } },
			{ true, 0.6, { { 1.85, -0.05, EPose::Bent } } },
			{ false, 0.64, { { 2.2, -0.3, EPose::Bent }, { 3.7, 0.38, EPose::Turned } } },
		};
		return V;
	}
	/** The cold light of the sky along their edges out here. */
	const uint32 OVER_RIM = 0x232c3a;
	// Over you on waking in the truck: at the wheel, slumped, the dash dark; the blink; the head comes up
	// to look out through the glass, and one of them is on the bonnet with its head bent down to the
	// windshield, black against SARO's lamps 300 m off. Where each stands, in the truck's own metres (x
	// right, z forward negative; the bonnet's top is at HOOD). The first at the front of the bonnet, a
	// little to the left; the second straight in front of the driver and a little closer; the third time
	// two, the second one turned aside, with no dark first.
	const double HOOD = 1.12, SLUMP = -0.78;
	struct FCabAt { double X, Z; EPose Pose; };
	struct FCabSpec { bool bStars; double Up; TArray<FCabAt> At; };
	const TArray<FCabSpec>& OVER_CAB()
	{
		static const TArray<FCabSpec> V = {
			{ true, 0.3, { { -0.3, -2.45, EPose::Bent } } },
			{ true, 0.27, { { -0.42, -2.34, EPose::Bent } } },
			{ false, 0.28, { { -0.62, -2.4, EPose::Bent }, { 0.46, -2.5, EPose::Turned } } },
		};
		return V;
	}
	/** The driver's eye in the cab, the truck's own metres (drive/Truck.ts driverEye). */
	const FVector CAB_EYE(-0.42, 1.48, 0.2);
	/** Which way you lie is chosen from this many; the picture it is judged by is this many pixels across. */
	const int32 FACINGS = 8, PROBE_W = 112;
	/** How well the open night sky stands behind them, as against a lit thing (0.6 at most). */
	const double SKY_AS = 0.32;
}

// ---------- the save ----------

S47J::FObj S47DocToJson(const FS47Doc& D)
{
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("id"), D.Id);
	S47J::Set(O, TEXT("kind"), D.Kind);
	if (!D.Image.IsEmpty()) S47J::Set(O, TEXT("image"), D.Image);
	S47J::Set(O, TEXT("page"), D.Page);
	S47J::Set(O, TEXT("title"), D.Title);
	if (!D.Transcript.IsEmpty()) S47J::Set(O, TEXT("transcript"), D.Transcript);
	if (!D.Stamp.IsEmpty()) S47J::Set(O, TEXT("stamp"), D.Stamp);
	if (!D.Paper.IsEmpty()) S47J::Set(O, TEXT("paper"), D.Paper);
	return O;
}

FS47Doc S47DocFromJson(const S47J::FObj& O)
{
	FS47Doc D;
	D.Id = S47J::Str(O, TEXT("id")); D.Kind = S47J::Str(O, TEXT("kind")); D.Image = S47J::Str(O, TEXT("image"));
	D.Page = S47J::Str(O, TEXT("page")); D.Title = S47J::Str(O, TEXT("title")); D.Transcript = S47J::Str(O, TEXT("transcript"));
	D.Stamp = S47J::Str(O, TEXT("stamp")); D.Paper = S47J::Str(O, TEXT("paper"));
	return D;
}

S47J::FObj FS47AbductSave::ToJson() const
{
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("count"), Count);
	S47J::Set(O, TEXT("last"), Last);
	TArray<TSharedPtr<FJsonValue>> A;
	for (const FS47AbductSpot& Sp : Spots)
	{
		const S47J::FObj J = S47J::New();
		S47J::Set(J, TEXT("scene"), Sp.Scene);
		S47J::Set(J, TEXT("take"), TArray<double>{ Sp.Take.X, Sp.Take.Y, Sp.Take.Z });
		S47J::Set(J, TEXT("back"), TArray<double>{ Sp.Back.X, Sp.Back.Y, Sp.Back.Z });
		S47J::Set(J, TEXT("clock"), Sp.Clock);
		if (Sp.Pic) S47J::Set(J, TEXT("pic"), *Sp.Pic);
		if (Sp.Doc) S47J::Set(J, TEXT("doc"), S47DocToJson(*Sp.Doc));
		if (Sp.Over) S47J::Set(J, TEXT("over"), *Sp.Over);
		if (Sp.Path) S47J::Set(J, TEXT("path"), *Sp.Path);
		if (Sp.Walked) S47J::Set(J, TEXT("walked"), *Sp.Walked);
		if (Sp.bStation) S47J::Set(J, TEXT("station"), true);
		if (Sp.bCab) S47J::Set(J, TEXT("cab"), true);
		A.Add(MakeShared<FJsonValueObject>(J));
	}
	O->SetArrayField(TEXT("spots"), A);
	if (Dishes.IsValid()) S47J::Set(O, TEXT("dishes"), S47J::Clone(Dishes));
	if (!Station.IsEmpty()) S47J::Set(O, TEXT("station"), Station);
	return O;
}

FS47AbductSave FS47AbductSave::FromJson(const S47J::FObj& O)
{
	FS47AbductSave S;
	if (!O.IsValid()) return S;
	S.Count = S47J::Int(O, TEXT("count"));
	S.Last = S47J::Num(O, TEXT("last"), -1e9);
	auto V3 = [](const S47J::FObj& J, const TCHAR* K) { const TArray<double> A = S47J::Nums(J, K); return A.Num() >= 3 ? FVector(A[0], A[1], A[2]) : FVector::ZeroVector; };
	for (const TSharedPtr<FJsonValue>& V : S47J::Arr(O, TEXT("spots")))
	{
		const S47J::FObj J = V.IsValid() ? V->AsObject() : nullptr;
		if (!J.IsValid()) continue;
		FS47AbductSpot Sp;
		Sp.Scene = S47J::Str(J, TEXT("scene")); Sp.Take = V3(J, TEXT("take")); Sp.Back = V3(J, TEXT("back")); Sp.Clock = S47J::Num(J, TEXT("clock"));
		if (S47J::Has(J, TEXT("pic"))) Sp.Pic = S47J::Str(J, TEXT("pic"));
		if (const S47J::FObj Dc = S47J::Obj(J, TEXT("doc")); Dc.IsValid()) Sp.Doc = S47DocFromJson(Dc);
		if (S47J::Has(J, TEXT("over"))) Sp.Over = S47J::Int(J, TEXT("over"));
		if (S47J::Has(J, TEXT("path"))) Sp.Path = S47J::Nums(J, TEXT("path"));
		if (S47J::Has(J, TEXT("walked"))) Sp.Walked = S47J::Num(J, TEXT("walked"));
		Sp.bStation = S47J::Bool(J, TEXT("station"));
		Sp.bCab = S47J::Bool(J, TEXT("cab"));
		S.Spots.Add(MoveTemp(Sp));
	}
	if (const S47J::FObj Ds = S47J::Obj(O, TEXT("dishes")); Ds.IsValid()) S.Dishes = S47J::Clone(Ds);
	S.Station = S47J::Str(O, TEXT("station"));
	return S;
}

// ---------- the camera of its own ----------

void FS47ShotCam::LookAt(const FVector& Target)
{
	// Matrix4.lookAt for a camera: z from the target to the eye, x = up x z, y = z x x
	FVector Z = Pos - Target;
	if (Z.SizeSquared() < 1e-12) Z = FVector(0, 0, 1);
	Z.Normalize();
	FVector X = FVector::CrossProduct(FVector(0, 1, 0), Z);
	if (X.SizeSquared() < 1e-12) { Z.Z += 1e-4; Z.Normalize(); X = FVector::CrossProduct(FVector(0, 1, 0), Z); }
	X.Normalize();
	const FVector Y = FVector::CrossProduct(Z, X);
	double E[16] = { X.X, X.Y, X.Z, 0, Y.X, Y.Y, Y.Z, 0, Z.X, Z.Y, Z.Z, 0, 0, 0, 0, 1 };
	FVector P, S;
	S47Three::Decompose(E, P, Q, S);
}

FS47Cam FS47ShotCam::Cam() const
{
	FS47Cam C;
	C.Pos = Pos;
	C.Forward = Q.RotateVector(FVector(0, 0, -1));
	C.Right = Q.RotateVector(FVector(1, 0, 0));
	C.Up = Q.RotateVector(FVector(0, 1, 0));
	C.Fov = Fov; C.Aspect = Aspect; C.Near = Near; C.Far = Far;
	return C;
}

// ---------- the time away ----------

FS47Abduction::FS47Abduction(FS47AbductDeps InD) : D(MoveTemp(InD))
{
	Top = 1;
	Lying = MakeUnique<FS47Group>(D.Web, TEXT(""));
	{
		// jacketFigure(true): someone in a SARO jacket, the reflective band across it, lying on their back
		FS47Group& G = *Lying;
		UMaterialInstanceDynamic* Coat = G.Basic(S47Three::Hex(0x2a3036));
		UMaterialInstanceDynamic* Dark = G.Basic(S47Three::Hex(0x15171a));
		UMaterialInstanceDynamic* Band = G.Basic(S47Three::Hex(0xd8dccc));
		UMaterialInstanceDynamic* Skin = G.Basic(S47Three::Hex(0x6e5a4c));
		FS47Node* Body = G.Group(&G.Root());
		G.Box(Body, .2, .82, .2, Dark, FVector(-.11, .41, 0)); G.Box(Body, .2, .82, .2, Dark, FVector(.11, .41, 0));
		G.Box(Body, .5, .62, .28, Coat, FVector(0, 1.14, 0)); G.Box(Body, .52, .07, .3, Band, FVector(0, 1.2, 0));
		G.Box(Body, .12, .6, .14, Coat, FVector(-.32, 1.12, 0)); G.Box(Body, .12, .6, .14, Coat, FVector(.32, 1.12, 0));
		G.Sphere(Body, .13, Skin, FVector(0, 1.6, 0));
		Body->Rotation.X = -UE_DOUBLE_HALF_PI; Body->Position.Y = .14; Body->Apply();
		G.SetVisible(false); G.AddToScene();
	}
	{
		// selfFigure(): you, as the eye has you on your back with the head lifted, looking down along
		// yourself: the jacket over the chest, the jeans, the boots standing up at the end; built facing
		// -z from under the eye, dark, only ever an outline against the frost
		Self = MakeUnique<FS47Group>(D.Web, TEXT("abduct:self"));
		FS47Group& G = *Self;
		UMaterialInstanceDynamic* Coat = G.Basic(S47Three::Hex(0x0d0f12));
		UMaterialInstanceDynamic* Jeans = G.Basic(S47Three::Hex(0x0c0f14));
		UMaterialInstanceDynamic* Boot = G.Basic(S47Three::Hex(0x090807));
		// the chest and the belly, lying along the view, low
		G.Capsule(&G.Root(), 0.1, 0.4, Coat, FVector(0, 0.06, -0.34), FVector(UE_DOUBLE_HALF_PI, 0, 0), FVector(1.7, 1, 1));
		for (const double Sd : { -1.0, 1.0 })
		{
			// the legs a little apart, from the hips to the ankles, and the boots up, toes a little out
			G.Capsule(&G.Root(), 0.068, 0.82, Jeans, FVector(Sd * 0.115, 0.075, -1.06), FVector(UE_DOUBLE_HALF_PI, 0, 0));
			G.Capsule(&G.Root(), 0.052, 0.12, Boot, FVector(Sd * 0.125, 0.14, -1.55), FVector(-0.22, 0, Sd * 0.26), FVector(1, 1, 1.4));
		}
		G.SetVisible(false);
	}
	OverFigs.Add(MakeUnique<FS47CardSilhouette>(D.Web, TEXT("abduct:over"), OVER_RIM, EPose::Bent));
	OverFigs.Add(MakeUnique<FS47CardSilhouette>(D.Web, TEXT("abduct:over"), OVER_RIM, EPose::Turned));
	for (const TUniquePtr<FS47CardSilhouette>& F : OverFigs) F->G->RenderOrder(2);
}

FS47Abduction::~FS47Abduction()
{
	ToneOff();
}

FVector FS47Abduction::PlayerPos() const { return D.Player ? D.Player->Pos : FVector::ZeroVector; }
FS47Cam FS47Abduction::Camera() const { return D.Eye ? D.Eye() : FS47Cam::Of(D.Player); }

TOptional<FS47Abduction::FWarn> FS47Abduction::Warn() const
{
	if (Stage != TEXT("warning")) return {};
	FWarn W; W.T = St; W.Len = WarnLen;
	return W;
}

bool FS47Abduction::Holds() const { return Stage == TEXT("light") || Stage == TEXT("taken") || Stage == TEXT("black") || Stage == TEXT("back"); }

TOptional<FS47Abduction::FWarnClock> FS47Abduction::WarnClock() const
{
	const double S = Stage == TEXT("warning") ? FMath::Min(WarnLen, FMath::FloorToDouble(St)) : Stage == TEXT("light") || Stage == TEXT("taken") || Stage == TEXT("black") ? WarnLen : -1;
	if (S < 0) return {};
	FWarnClock C;
	C.Hm = Clock(WarnFrom);
	C.Ss = FString::Printf(TEXT("%02d"), int32(JsRound(S)));
	return C;
}

bool FS47Abduction::Racing() const { return Stage == TEXT("warning") || Stage == TEXT("light") || Stage == TEXT("taken") || Stage == TEXT("black"); }

void FS47Abduction::Update(double Dt, const FS47AbductIn& S)
{
	St += Dt;
	if (Stage == TEXT("calm")) Calm(Dt, S);
	else if (Stage == TEXT("warning")) Warning(Dt, S);
	else if (Stage == TEXT("light")) LightStep(S);
	else if (Stage == TEXT("taken")) Taken(S);
	else if (Stage == TEXT("black")) { if (St > 2) Wake(S); }
	else if (Stage == TEXT("back")) { if (Over) OverStep(Dt, S); else BackStep(Dt); }
}

// ---------- the pressure ----------

void FS47Abduction::Calm(double Dt, const FS47AbductIn& S)
{
	RecordsStep();
	const bool bRoom = S.NextFixed - S.Clock >= MIN_LOST + 60;
	const bool bCan = S.bOk && S.bEnabled && !S.Line.IsEmpty() && Data.Count < MAX && S.Clock - Data.Last >= GAP && bRoom;
	if (!bCan) { Pressure = FMath::Max(0.0, Pressure - Dt / 90); return; }
	// about four to eight minutes alone on a line, sooner towards morning, much sooner when the one
	// before chapter five is due and has not come
	const double Day = FMath::Fmod(FMath::Fmod(S.Clock, 86400.0) + 86400.0, 86400.0), K = FMath::Clamp((Day - 2.3 * 3600) / (2.5 * 3600), 0.0, 1.0);
	Pressure += Dt / 360 * (0.7 + 0.8 * K) * (S.bLate && Data.Count == 0 ? 2.5 : 1) * S.Boost;
	// (alone on a line in the build, it waits at the top for the peak)
	if (S.May.IsSet() && !*S.May) { Pressure = FMath::Min(Pressure, Top); return; }
	if (Pressure >= Top) Begin(S);
}

void FS47Abduction::Begin(const FS47AbductIn& S)
{
	Stage = TEXT("warning"); St = 0; WarnLen = WARN; bSilent = false;
	Beat = 0; WarnFrom = S.Clock;
	PathV.Reset(); PathT = 0; Sample();
	bHadLight = D.Flashlight && D.Flashlight->bOn;
	Take = PlayerPos();
	bCab = false; bCabIn = false;
	// (loadArtSome(ABDUCT_ART) and the outlines' wake: the pictures are shipped with the build)
	for (const TUniquePtr<FS47CardSilhouette>& F : OverFigs) F->Wake();
	if (D.Hush) D.Hush(WarnLen + 12, 0, {}, {});
	Bed.Stop();
	Bed = D.Audio ? D.Audio->Pressure() : FS47Pressure();
	UE_LOG(LogS47Abduction, Display, TEXT("S47: the time away: the warning, %s, at %s"), S.Line.IsEmpty() ? TEXT("no line") : *S.Line, *Clock(S.Clock));
}

void FS47Abduction::Force(const FS47AbductIn& S) { Begin(S); bForced = true; }

void FS47Abduction::BeginCab(const FS47AbductIn& S)
{
	Begin(S); bCab = true; PathV.Reset();
	if (FS47Truck* G = D.Cab && D.Cab->Group ? D.Cab->Group() : nullptr) Take = FVector(G->Position.X, 0, G->Position.Z);
}

bool FS47Abduction::MayBegin(const FS47AbductIn& S) const
{
	return Stage == TEXT("calm") && S.bEnabled && Data.Count < MAX && S.Clock - Data.Last >= GAP && S.NextFixed - S.Clock >= MIN_LOST + 60;
}

void FS47Abduction::Sample()
{
	// the player where they are now, on the way the recording will walk (a quarter second apart)
	const FVector P = PlayerPos();
	const double Yaw = D.Player ? D.Player->Yaw : 0;
	PathV.Add(JsRound(P.X * 100) / 100); PathV.Add(JsRound(P.Z * 100) / 100); PathV.Add(JsRound(Yaw * 100) / 100);
}

void FS47Abduction::Warning(double Dt, const FS47AbductIn& S)
{
	(void)Dt;
	const double T = St, W = WarnLen;
	US47Audio* A = D.Audio;
	// got away: off the line, inside, among people, before the light (not from the one that is forced:
	// only Missing time turned off, or the night left, ends that; nor from the truck's, which only the
	// truck letting go ends)
	if (bCab) { if (!S.bEnabled || S.bCabFree) { PassOver(); return; } }
	else if (!S.bOk || !S.bEnabled || (S.Line.IsEmpty() && !bForced)) { PassOver(); return; }
	if (!bCab) while (PathT + PATH_DT <= FMath::Min(T, W) + 1e-6) { PathT += PATH_DT; Sample(); }
	// :16, the beep of 02:13 (the same low tone, and a small one over it that a phone's speaker plays),
	// and the footsteps are gone
	if (T >= BEAT.Steps && Beat < 1)
	{
		Beat = 1; bSilent = true;
		if (A) { A->Beep(52, 2.8f, 0.09f, 0, TEXT("sine")); A->Beep(1046, 0.05f, 0.035f, 0, TEXT("sine")); }
	}
	// :31, the pressure: in at once, then rising to the light; the flashlight begins to stutter
	if (T >= BEAT.Tone && Beat < 2) { Beat = 2; if (!Tone) ToneOn(); }
	const double K = FMath::Clamp((T - BEAT.Tone) / (W - BEAT.Tone), 0.0, 1.0);
	if (Tone && Tone->Gain) Tone->Gain->Gain.SetValue(0.08 + 0.22 * K);
	if (Bed) Bed.Gain->Gain.SetValue(T < BEAT.Tone ? 0.015 : FMath::Min(0.3 * FMath::Min(1.0, (T - BEAT.Tone) / 0.8) + 0.2 * K, 0.5));
	if (T >= BEAT.Tone && T < BEAT.Dark && D.Flashlight) D.Flashlight->Flicker(0.2 + 0.8 * (T - BEAT.Tone) / (BEAT.Dark - BEAT.Tone));
	// :40, it dies, with the click K3 made when it tripped seven seconds before the event; seven
	// seconds of the dark (it does not come on again before the light)
	if (T >= BEAT.Dark && Beat < 3)
	{
		Beat = 3;
		if (A)
		{
			FS47Sound O; O.Gain = 0.75f; O.Rate = 0.62f; A->Sfx(A->Or(TEXT("rack_switch"), TEXT("switch")), O);
			FS47Sound Th; Th.Gain = 0.32f; Th.Rate = 1.4f; Th.When = 0.03f; A->Sfx(TEXT("thudSoft"), Th);
		}
		if (D.Flashlight) D.Flashlight->Flicker(0);
	}
	if (Beat >= 3 && D.Flashlight && D.Flashlight->bOn) D.Flashlight->bOn = false;
	if (T >= W)
	{
		Stage = TEXT("light"); St = 0; From = S.Clock;
		if (bCab)
		{
			// (in the truck: where the truck is; out of it, where the player is, and they wake by the
			// truck where it is put down)
			bCabIn = D.Cab && D.Cab->InCab && D.Cab->InCab();
			FS47Truck* G = D.Cab && D.Cab->Group ? D.Cab->Group() : nullptr;
			if (bCabIn && G) Take = FVector(G->Position.X, 0, G->Position.Z); else Take = PlayerPos();
			BackAt = D.Cab && D.Cab->Plan ? D.Cab->Plan() : TOptional<FVector>();
		}
		else { Take = PlayerPos(); BackAt = Further(Take, S.Along); }
		UE_LOG(LogS47Abduction, Display, TEXT("S47: the time away: the light"));
	}
}

void FS47Abduction::PassOver()
{
	Stage = TEXT("calm"); St = 0; bSilent = false; Pressure = 0.35; Top = Rnd(0.85, 1.15); Beat = 0; bForced = false; PathV.Reset(); bCab = false;
	ToneOff();
	if (D.Flashlight) D.Flashlight->Flicker(0);
	if (D.Hush) D.Hush(0.1, 1, {}, {});
	Bed.Stop(1); Bed = FS47Pressure();   // got away: it ebbs over a few seconds
	if (bHadLight && D.Flashlight && D.Flashlight->bHave) D.Flashlight->bOn = true;
	UE_LOG(LogS47Abduction, Display, TEXT("S47: the time away: the warning passed"));
}

// ---------- the light, the taking ----------

void FS47Abduction::LightStep(const FS47AbductIn& S)
{
	const double K = FMath::Min(1.0, St / 3);
	if (D.Screen.Light) D.Screen.Light(K);
	// (at the wheel it comes into the cab from above, through the glass, before it is everything)
	if (bCab && bCabIn && D.Cab && D.Cab->Light) D.Cab->Light(FMath::Min(1.0, St / 1.6));
	if (St >= 3)
	{
		if (bCab && bCabIn && D.Cab && D.Cab->Light) D.Cab->Light(0);
		Stage = TEXT("taken"); St = 0;
		ToneOff(); Bed.Stop(); Bed = FS47Pressure();
		if (D.Glitch) D.Glitch(1);
		MakeFrames();
		if (D.Audio) D.Audio->Suspend(true);
		const double Room = S.NextFixed - S.Clock - 60;
		Lost = JsRound(FMath::Min(Rnd(17, 47) * 60, Room));
		UE_LOG(LogS47Abduction, Display, TEXT("S47: the time away: taken, %d frames, %.0f s lost"), Frames.Num(), Lost);
	}
}

void FS47Abduction::Taken(const FS47AbductIn& S)
{
	const double Per = S.bCalm ? 1.4 : 0.95;
	const int32 I = FMath::FloorToInt(St / Per);
	const double K = St - I * Per;
	if (D.Screen.Light) D.Screen.Light({});
	if (D.Glitch) D.Glitch(FMath::Max(0.0, 1 - St * 2));
	if (I >= Frames.Num()) { Show(nullptr); Stage = TEXT("black"); St = 0; return; }
	if (S.bCalm) Show(Frames[I].Get(), 0.35 * FMath::Min3(1.0, K / 0.4, (Per - K) / 0.4));
	else Show(K < Per - 0.12 ? Frames[I].Get() : nullptr, 1);
}

void FS47Abduction::Show(UTexture2D* T, double Alpha)
{
	if (D.Screen.Show) D.Screen.Show(T, Alpha);
}

// ---------- back ----------

void FS47Abduction::Wake(const FS47AbductIn& S)
{
	AS47Walker* P = D.Player;
	if (D.SkipClock) D.SkipClock(Lost);
	const double T1 = From + Lost;
	// the truck's (story/truckTaken.ts): the truck put down further on, the player at its wheel, or (out
	// of it at the light) on their back on the verge beside it
	const bool bWasCab = bCab, bInCab = bWasCab && bCabIn;
	const TOptional<FVector> TruckAt = bWasCab && D.Cab && D.Cab->Wake ? D.Cab->Wake(Lost, bInCab) : TOptional<FVector>();
	if (bWasCab && !bInCab && TruckAt)
	{
		FS47Truck* G = D.Cab && D.Cab->Group ? D.Cab->Group() : nullptr;
		const double H = G ? G->Rotation.Y : 0;
		// (-1, 0, 0) turned about up by the truck's heading
		const FVector Side(-FMath::Cos(H), 0, FMath::Sin(H));
		BackAt = *TruckAt + Side * 3.4;
		if (D.Standable && !D.Standable(BackAt->X, BackAt->Z)) BackAt = *TruckAt - Side * 3.4;
	}
	FVector BackP;
	if (bInCab && TruckAt) BackP = *TruckAt;
	else
	{
		// further along the line, on ground that can be stood on
		BackP = BackAt ? *BackAt : Further(Take, S.Along);
		if (P)
		{
			P->Place(BackP.X, BackP.Z, P->Yaw + Rnd(-2, 2));
			P->Pitch = 1.3; P->Eye = 0.35;
		}
	}
	BackAt.Reset();
	BackP.Y = D.Ground ? D.Ground(BackP.X, BackP.Z) : 0; Take.Y = D.Ground ? D.Ground(Take.X, Take.Z) : 0;
	// (round the truck it is a wider ring, the truck in the middle of it, out over the asphalt)
	if (bInCab && TruckAt) Ring(BackP, 2.05); else Ring(BackP);
	if (bWasCab && !bInCab && TruckAt) Ring(FVector(TruckAt->X, D.Ground ? D.Ground(TruckAt->X, TruckAt->Z) : 0, TruckAt->Z), 2.05);
	if (D.Flashlight) D.Flashlight->Flicker(0);
	if (D.Audio) D.Audio->Suspend(false);
	if (D.Screen.Hide) D.Screen.Hide();
	bSilent = false;
	Data.Count++; Data.Last = T1;
	FS47AbductSpot Spot;
	Spot.Scene = SceneId; Spot.Take = Take; Spot.Back = BackP; Spot.Clock = From;
	if (PathV.Num() >= 6 && !bWasCab) Spot.Path = PathV;
	if (bForced) Spot.bStation = true;
	if (bWasCab) Spot.bCab = true;
	PathV.Reset(); bForced = false; bCab = false;
	Data.Spots.Add(Spot);
	const int32 SpotI = Data.Spots.Num() - 1;
	Record(Take, SceneId, bWasCab);
	if (!bInCab && D.KeyStall) D.KeyStall();
	// the traces
	const FString A = Clock(From), B = Clock(T1);
	if (D.Note)
	{
		D.Note(FString::Printf(TEXT("%s to %s. Nothing."), *A, *B));
		D.Note(FString::Printf(TEXT("%s. A line in the notebook in my own hand. I do not remember writing it: %s."), *B, FMath::FRand() < 0.5 ? TEXT("026") : *Clock(S.NextFixed)));
		D.Note(FString::Printf(TEXT("%s. A nosebleed. It had stopped by itself."), *B));
	}
	if (bWasCab && D.Cab && D.Cab->Woke) D.Cab->Woke(A, B, bInCab);
	if (D.Camera && D.Camera())
	{
		TOptional<FS47Pix> Pic = bInCab ? FromAbove(BackP, false, true) : FromAbove(BackP, true);
		if (!Pic) Pic = S47Abduct::Painted(bInCab ? TEXT("abductTruck") : TEXT("abductAbove"), 0.5, 21, 0.9);
		if (Pic)
		{
			FS47Doc Doc;
			Doc.Id = FString::Printf(TEXT("away%d"), Data.Count); Doc.Kind = TEXT("photo");
			Doc.Image = S47Image::ToDataUrl(Pic->Colors(), Pic->W, Pic->H, true, 80);
			Doc.Page = FString(); Doc.Title = FString::Printf(TEXT("PHOTO / Not exposed by me (%s)"), *A);
			Doc.Transcript = bInCab
				? FString::Printf(TEXT("One more exposure on the roll than there should be, with the counter at %s. Taken from above, from higher than anyone could stand: the truck standing in the road with every light out, white in a light from straight over it. Someone at the wheel. Me."), *B)
				: FString::Printf(TEXT("One more exposure on the roll than there should be, with the counter at %s. Taken from above, from higher than anyone could stand: someone lying on their back in the grass in a SARO jacket, the reflective band across the chest. Me."), *B);
			Data.Spots[SpotI].Doc = Doc;
			if (D.File) D.File(Doc);
		}
	}
	// over you: the first, the second, the third time (not with Missing time turned off on the way, nor
	// without the outlines: never the figure in code this near). After the pictures from above, so it is
	// in none of them. Until it is not there, no sound at all and no light; the case is kept from then.
	const int32 V = FMath::Min(OVER().Num(), Data.Count) - 1;
	bAwakeCab = bInCab;
	if (S.bEnabled && OverFigs.Num() && OverFigs[0]->Painted())
	{
		Data.Spots[SpotI].Over = V + 1;
		if (bInCab) OverStartCab(V); else OverStart(V);
	}
	else
	{
		if (D.Flashlight && D.Flashlight->bHave && !bInCab) D.Flashlight->bOn = true;
		if (bInCab && D.Cab && D.Cab->Look) D.Cab->Look(0, -0.6);
		if (D.Hush) D.Hush(6, 0.2, {}, {});
		if (D.Save) D.Save();
	}
	Stage = TEXT("back"); St = 0;
	UE_LOG(LogS47Abduction, Display, TEXT("S47: the time away: back at %s, %s to %s, %s"), *BackP.ToString(), *A, *B, Over ? TEXT("one of them over you") : TEXT("nobody there"));
}

// ---------- over you, in the truck ----------

void FS47Abduction::OverStartCab(int32 V)
{
	const FS47AbductCab& C = *D.Cab;
	const FCabSpec& Spec = OVER_CAB()[V];
	for (int32 i = 0; i < Spec.At.Num() && i < OverFigs.Num(); ++i) OverFigs[i]->Pose(Spec.At[i].Pose);
	if (C.Look) C.Look(0, SLUMP);
	FOver O; O.V = V; O.Phase = TEXT("stars"); O.Pitch = SLUMP; O.Yaw = 0; O.bCab = true;
	Over = O;
	bOverLooked = false;
	if (D.Hush) D.Hush(60, 0, 0.05, {});
	if (D.Screen.Held) D.Screen.Held(true);
	if (!Spec.bStars) OverRevealCab();
}

void FS47Abduction::OverRevealCab()
{
	FOver& O = *Over;
	const FS47AbductCab& C = *D.Cab;
	FS47Truck* G = C.Group ? C.Group() : nullptr;
	const FCabSpec& Spec = OVER_CAB()[O.V];
	if (!G) { OverGone(); return; }
	// the head raised to look out through the glass, a little towards the nearest of them, just so far
	// that its head is at `up` of the half height over the middle of the view, on any screen
	const FS47Cam Cam = Camera();
	const double Ty = FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2);
	const FCabAt& A0 = Spec.At[0];
	const double Hy = HOOD + OverFigs[0]->HeadY();
	const double Dx = A0.X - CAB_EYE.X, Dz = A0.Z - CAB_EYE.Z, Dist = FMath::Sqrt(Dx * Dx + Dz * Dz);
	const double Yaw = 0.55 * FMath::Atan2(-Dx, -Dz), Pitch = FMath::Atan2(Hy - CAB_EYE.Y, Dist) - FMath::Atan(Spec.Up * Ty);
	if (C.Look) C.Look(Yaw, Pitch);
	if (C.LookBox) { FS47LookBox B; B.Yaw0 = Yaw - 0.4; B.Yaw1 = Yaw + 0.4; B.Pitch0 = Pitch - 0.3; B.Pitch1 = Pitch + 0.5; C.LookBox(B); }
	O.Pitch = Pitch; O.Yaw = Yaw; O.Phase = TEXT("there"); O.T = 0;
	const FVector Eye = G->LocalToWorld(CAB_EYE);
	const FQuat Q = G->Quat() * S47Three::Euler(Pitch, Yaw, 0, TEXT("YXZ"));
	const FVector Look = Q.RotateVector(FVector(0, 0, -1));
	for (int32 i = 0; i < Spec.At.Num() && i < OverFigs.Num(); ++i)
	{
		FS47CardSilhouette& F = *OverFigs[i];
		const FVector Feet = G->LocalToWorld(FVector(Spec.At[i].X, HOOD, Spec.At[i].Z));
		F.G->Root().Position = Feet; F.G->Root().Apply();
		F.G->SetVisible(true); F.G->AddToScene();
		F.G->RenderOrder(2);
		const FVector At(Feet.X, Feet.Y + F.HeadY(), Feet.Z);
		const FVector Head = (At - Eye).GetSafeNormal();
		O.Heads.Add(At); O.Dirs.Add(Head); O.Off.Add(AngleTo(Look, Head));
		F.Face(Eye);
	}
}

// ---------- over you ----------

void FS47Abduction::OverStart(int32 V)
{
	AS47Walker* P = D.Player;
	const FOverSpec& Spec = OVER()[V];
	for (int32 i = 0; i < Spec.At.Num() && i < OverFigs.Num(); ++i) OverFigs[i]->Pose(Spec.At[i].Pose);
	// which way you lie: still black, so the looking round is not seen
	if (P) P->Yaw = Facing(V);
	FOver O; O.V = V; O.Phase = TEXT("stars"); O.Pitch = P ? P->Pitch : 0; O.Yaw = P ? P->Yaw : 0;
	Over = O;
	bOverLooked = false;
	if (D.Hush) D.Hush(60, 0, 0.05, {});
	if (D.Screen.Held) D.Screen.Held(true);
	if (!Spec.bStars) OverReveal();
}

void FS47Abduction::OverReveal()
{
	FOver& O = *Over;
	AS47Walker* P = D.Player;
	if (!P) { OverGone(); return; }
	const FLayout L = Layout(O.V, P->Yaw);
	P->Pitch = L.Pitch;
	O.Pitch = P->Pitch; O.Yaw = P->Yaw; O.Phase = TEXT("there"); O.T = 0;
	const FVector Eye(P->Pos.X, P->FloorY + P->Eye, P->Pos.Z);
	const FVector Look = S47Three::Euler(P->Pitch, P->Yaw, 0, TEXT("YXZ")).RotateVector(FVector(0, 0, -1));
	for (int32 i = 0; i < L.At.Num() && i < OverFigs.Num(); ++i)
	{
		FS47CardSilhouette& F = *OverFigs[i];
		F.G->Root().Position = L.At[i].Q; F.G->Root().Apply();
		F.G->SetVisible(true); F.G->AddToScene();
		F.G->RenderOrder(2);   // over the frost it stands in, not under it
		const FVector At(L.At[i].Q.X, L.At[i].Head, L.At[i].Q.Z);
		const FVector Head = (At - Eye).GetSafeNormal();
		O.Heads.Add(At); O.Dirs.Add(Head); O.Off.Add(AngleTo(Look, Head));
		F.Face(Eye);
	}
	// and you, along the bottom of the view: the jacket, the jeans, the boots up, dark
	Self->Root().Position = FVector(P->Pos.X, P->FloorY, P->Pos.Z); Self->Root().Rotation = FVector(0, P->Yaw, 0); Self->Root().Apply();
	Self->SetVisible(true); Self->AddToScene();
}

FS47Abduction::FLayout FS47Abduction::Layout(int32 V, double Yaw) const
{
	// where they stand this time if you lie facing yaw (on the ground there, the head's height), and how
	// far the view is turned down for the nearest one's head to be at its height in the frame
	const AS47Walker* P = D.Player;
	const FS47Cam Cam = Camera();
	const FOverSpec& Spec = OVER()[V];
	const double Ty = FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2), Tx = Ty * Cam.Aspect;
	const FVector Fwd(-FMath::Sin(Yaw), 0, -FMath::Cos(Yaw)), Right(FMath::Cos(Yaw), 0, -FMath::Sin(Yaw));
	const double EyeY = (P ? P->FloorY + P->Eye : 0);
	FLayout L;
	L.Right = Right;
	const FVector Pos = PlayerPos();
	for (int32 i = 0; i < Spec.At.Num(); ++i)
	{
		const FOverAt& A = Spec.At[i];
		const double Bk = FMath::Atan(A.Side * Tx);
		FVector Q = Pos + Fwd * (A.Dist * FMath::Cos(Bk)) + Right * (A.Dist * FMath::Sin(Bk));
		Q.Y = D.Ground ? D.Ground(Q.X, Q.Z) : 0;
		L.At.Add({ Q, Q.Y + (i < OverFigs.Num() ? OverFigs[i]->HeadY() : 1.1) });
	}
	L.Pitch = FMath::Atan2(L.At[0].Head - EyeY, Spec.At[0].Dist) - FMath::Atan(Spec.Up * Ty);
	return L;
}

double FS47Abduction::Facing(int32 V)
{
	// which way you lie: of a few ways round, one where they have ground to stand on with nothing between
	// them and you, and stand out black against something lighter (the sky, a lit dish, a lit wall); the
	// one you fell is kept when it is as good as any
	const double Yaw0 = D.Player ? D.Player->Yaw : 0;
	Facings.Reset();
	double Best = Yaw0, BestScore = TNumericLimits<double>::Max();
	for (int32 i = 0; i < FACINGS; ++i)
	{
		const double Yaw = Yaw0 + (i * 2 * UE_DOUBLE_PI) / FACINGS;
		const FLayout L = Layout(V, Yaw);
		bool bReach = true;
		for (const FLayout::FAt& A : L.At) bReach &= Reach(A.Q);
		const double Ag = Against(L, Yaw);
		Facings.Add({ Yaw, bReach, Ag });
		const double Score = (bReach ? 0 : 100) + Ag - (i == 0 ? 0.01 : 0);
		if (Score < BestScore) { BestScore = Score; Best = Yaw; }
	}
	return Best;
}

bool FS47Abduction::Reach(const FVector& Q) const
{
	// ground one of them could stand on, and nothing in the way between it and you
	const FVector P = PlayerPos();
	for (const double K : { 0.5, 0.75, 1.0 }) if (D.Standable && !D.Standable(P.X + (Q.X - P.X) * K, P.Z + (Q.Z - P.Z) * K)) return false;
	return true;
}

double FS47Abduction::Against(const FLayout& L, double Yaw) const
{
	// how badly they would stand out, seen from where you lie facing yaw (lower is better): in a small
	// picture of the scene without them, behind each head and shoulders, how much is open sky, and where
	// it is not, how dark and how uneven what stands there is; and how much glare there is round about
	if (!D.Probe || !D.Player) return 0;
	const FS47Cam Cam = Camera();
	FS47ShotCam Pc;
	Pc.Fov = Cam.Fov; Pc.Aspect = Cam.Aspect; Pc.Near = Cam.Near; Pc.Far = Cam.Far;
	Pc.Pos = FVector(D.Player->Pos.X, D.Player->FloorY + D.Player->Eye, D.Player->Pos.Z);
	Pc.Q = S47Three::Euler(L.Pitch, Yaw, 0, TEXT("YXZ"));
	const int32 W = Cam.Aspect >= 1 ? PROBE_W : int32(JsRound(PROBE_W * 0.6)), H = FMath::Max(24, int32(JsRound(W / Cam.Aspect)));
	TArray<uint8> Px;
	if (!D.Probe(Pc, W, H, Px) || Px.Num() < W * H * 4) return 0;
	const FS47Cam C = Pc.Cam();
	// the pixels of a box standing at q: half metres either side, from y0 to y1 above the ground
	auto Box = [&](const FVector& Q, double Half, double Y0, double Y1)
	{
		FVector V = Q - L.Right * Half; V.Y = Q.Y + Y1;
		const FVector Pa = C.Project(V);
		V = Q + L.Right * Half; V.Y = Q.Y + Y0;
		const FVector Pb = C.Project(V);
		auto Cx = [W](double N) { return FMath::Clamp(int32(JsRound((N + 1) / 2 * W)), 0, W); };
		auto Cy = [H](double N) { return FMath::Clamp(int32(JsRound((N + 1) / 2 * H)), 0, H); };
		return FIntRect(Cx(Pa.X), Cy(Pb.Y), Cx(Pb.X), Cy(Pa.Y)); // [x0, x1) [y0, y1)
	};
	auto Sky = [&](int32 I) { return Px[I] > 246 && Px[I + 1] < 10 && Px[I + 2] > 246; };
	auto Lum = [&](int32 I) { return FMath::Pow((0.2126 * Px[I] + 0.7152 * Px[I + 1] + 0.0722 * Px[I + 2]) / 255.0, 1 / 2.2); };
	double Score = 0;
	for (const FLayout::FAt& A : L.At)
	{
		const double Hy = A.Head - A.Q.Y;
		const FIntRect B = Box(A.Q, 0.34, Hy - 0.36, Hy + 0.3);
		int32 N = 0, Open = 0;
		double Sum = 0, Sq = 0;
		for (int32 Y = B.Min.Y; Y < B.Max.Y; ++Y) for (int32 X = B.Min.X; X < B.Max.X; ++X)
		{
			const int32 I = (Y * W + X) * 4; N++;
			if (Sky(I)) Open++; else { const double Lv = Lum(I); Sum += Lv; Sq += Lv * Lv; }
		}
		const FIntRect Bb = Box(A.Q, 1.5, 0.25, Hy + 1.4);
		int32 Nn = 0, Hot = 0;
		for (int32 Y = Bb.Min.Y; Y < Bb.Max.Y; ++Y) for (int32 X = Bb.Min.X; X < Bb.Max.X; ++X) { const int32 I = (Y * W + X) * 4; Nn++; if (!Sky(I) && Lum(I) > 0.7) Hot++; }
		const int32 K = N - Open;
		const double Mean = K ? Sum / K : 0, Sd = K ? FMath::Sqrt(FMath::Max(0.0, Sq / K - Mean * Mean)) : 0;
		const double SkyF = N ? double(Open) / N : 0, Glare = Nn ? double(Hot) / Nn : 0;
		Score += -(SkyF * SKY_AS + (1 - SkyF) * (FMath::Min(Mean, 0.6) - 1.5 * Sd)) + 5 * Glare;
	}
	return Score;
}

void FS47Abduction::OverStep(double Dt, const FS47AbductIn& S)
{
	// still on your back: the stars, the blink, then it is there for a second and a half. It goes when
	// that is up, or when the eyes go to it; and not with an effect: it was not there.
	FOver& O = *Over;
	if (!S.bEnabled) { OverGone(); return; }   // Missing time turned off in the pause menu: not there
	O.T += Dt;
	if (O.Phase == TEXT("stars"))
	{
		if (O.T >= STARS_T) { O.Phase = TEXT("blink"); O.T = 0; Show(nullptr); }
		return;
	}
	if (O.Phase == TEXT("blink"))
	{
		if (O.T >= BLINK_T)
		{
			if (O.bCab) OverRevealCab(); else OverReveal();
			if (D.Screen.Hide) D.Screen.Hide();
			if (Over && Over->Phase == TEXT("there")) UE_LOG(LogS47Abduction, Display, TEXT("S47: the time away: one of them over you (%d)"), Over->V + 1);
		}
		return;
	}
	// (in the cab the eyes are the camera's, turned by the drive's look and the truck under it)
	const FVector Look = O.bCab ? Camera().Forward : S47Three::Euler(D.Player ? D.Player->Pitch : 0, D.Player ? D.Player->Yaw : 0, 0, TEXT("YXZ")).RotateVector(FVector(0, 0, -1));
	bool bLooked = false;
	for (int32 i = 0; i < O.Dirs.Num(); ++i) if (AngleTo(Look, O.Dirs[i]) < O.Off[i] - 0.05) bLooked = true;
	if (bLooked) bOverLooked = true;
	if (O.T >= OVER_T || bLooked) OverGone();
}

void FS47Abduction::OverGone()
{
	const bool bWasCab = Over && Over->bCab;
	OverDrop();
	// the crickets, all at once, and the rest of the night after them; the light, on (in the cab: the
	// instruments, story/truckTaken.ts)
	if (D.Hush) D.Hush(5, 0.6, 0.06, 1.8);
	if (D.Flashlight && D.Flashlight->bHave && !bWasCab) D.Flashlight->bOn = true;
	if (D.Save) D.Save();
	St = 0;
	UE_LOG(LogS47Abduction, Display, TEXT("S47: the time away: not there%s"), bOverLooked ? TEXT(" (the eyes went to it)") : TEXT(""));
}

void FS47Abduction::OverDrop()
{
	for (const TUniquePtr<FS47CardSilhouette>& F : OverFigs) { F->G->SetVisible(false); F->G->RemoveFromScene(); }
	if (Self) { Self->SetVisible(false); Self->RemoveFromScene(); }
	if (D.Screen.Held) D.Screen.Held(false);
	if (Over && Over->Phase == TEXT("blink") && D.Screen.Hide) D.Screen.Hide();
	if (Over && Over->bCab && D.Cab && D.Cab->LookBox) D.Cab->LookBox({});
	Over.Reset();
}

FVector2D FS47Abduction::CabGaze(double Dx, double Dy) const
{
	return Over && Over->bCab && Over->Phase == TEXT("there") ? FVector2D(Dx * 0.45, Dy * 0.45) : FVector2D::ZeroVector;
}

void FS47Abduction::Gaze(double Dx, double Dy)
{
	if (!Over || Over->Phase != TEXT("there") || !D.Player) return;
	AS47Walker* P = D.Player;
	P->Yaw = FMath::Clamp(P->Yaw - Dx * 0.45, Over->Yaw - 0.4, Over->Yaw + 0.4);
	P->Pitch = FMath::Clamp(P->Pitch - Dy * 0.45, Over->Pitch - 0.3, Over->Pitch + 0.5);
}

FString FS47Abduction::OverPhase() const { return Over ? Over->Phase : FString(); }
TArray<FVector> FS47Abduction::OverHeads() const { return Over && Over->Phase == TEXT("there") ? Over->Heads : TArray<FVector>(); }

bool FS47Abduction::HideOver(bool bHide)
{
	if (!Over || Over->Phase != TEXT("there")) return false;
	for (const TUniquePtr<FS47CardSilhouette>& F : OverFigs) if (F->G->InScene()) F->G->SetVisible(!bHide);
	if (Self) Self->SetVisible(!bHide);
	return true;
}

void FS47Abduction::FaceOver(const FVector& Eye)
{
	if (!Over || Over->Phase != TEXT("there")) return;
	for (const TUniquePtr<FS47CardSilhouette>& F : OverFigs) if (F->G->InScene()) F->Face(Eye);
}

void FS47Abduction::BackStep(double Dt)
{
	if (bAwakeCab) { BackCab(Dt); return; }
	AS47Walker* P = D.Player;
	const double K = FMath::Min(1.0, St / 6);
	if (P)
	{
		P->Eye = 0.35 + (1.62 - 0.35) * Smooth(K, 0.3, 1);
		P->Pitch += (-0.06 - P->Pitch) * FMath::Min(1.0, Dt * (K > 0.4 ? 1.4 : 0.2));
	}
	if (St > 2.5 && St - Dt <= 2.5 && D.Toast) D.Toast(TEXT("On your back in the grass. Stars. The flashlight is lying beside you, on. The grass round you is flat, and white with frost."), 6);
	if (K >= 1) { if (P) P->Eye = 1.62; Stage = TEXT("calm"); St = 0; Pressure = 0; Top = Rnd(0.85, 1.15); }
}

void FS47Abduction::BackCab(double Dt)
{
	// at the wheel: the head comes down to the dash, where the odometer is, and the night goes on
	const FS47AbductCab* C = D.Cab.GetPtrOrNull();
	const TOptional<FVector2D> Now = C && C->LookNow ? C->LookNow() : TOptional<FVector2D>();
	// (down to the instruments, where the odometer is under the speedometer)
	if (Now && C->Look) C->Look(Now->X + (0 - Now->X) * FMath::Min(1.0, Dt * 1.2), Now->Y + (-0.6 - Now->Y) * FMath::Min(1.0, Dt * 1.2));
	if (St > 1.6 && St - Dt <= 1.6 && C && D.Toast && C->BackText) D.Toast(C->BackText(), 6.5f);
	if (St >= 4) { bAwakeCab = false; Stage = TEXT("calm"); St = 0; Pressure = 0; Top = Rnd(0.85, 1.15); }
}

FVector FS47Abduction::Further(const FVector& FromP, const FVector& Along) const
{
	FVector Dir = Along; Dir.Y = 0;
	Dir = Dir.GetSafeNormal();
	for (const double Dist : { Rnd(220, 400), Rnd(100, 220), 100.0, 60.0, 30.0 })
	{
		const bool bFirst = FMath::FRand() < 0.5;
		for (const double Sgn : { bFirst ? 1.0 : -1.0, bFirst ? -1.0 : 1.0 })
		{
			FVector Q = FromP + Dir * (Dist * Sgn);
			if (D.Settle) D.Settle(Q);
			if (!D.Standable || D.Standable(Q.X, Q.Z)) return Q;
		}
	}
	return FromP;
}

// ---------- what is left ----------

void FS47Abduction::Ring(const FVector& At, double Scale)
{
	// (patchy, soft at both edges: seen from the grass, lying in it, as well as standing)
	TUniquePtr<FS47Group> G = MakeUnique<FS47Group>(D.Web, TEXT("abduct:ring"));
	UMaterialInstanceDynamic* M = G->Basic(S47Three::Hex(0xc8d4dc), TEXT("M_Web_Unlit_Trans_2S"));
	S47Figures::SetMap(M, S47Abduct::Frost());
	S47Figures::SetOpacity(M, 0.32);
	// RingGeometry(1.3 s, 2.7 s) turned flat: its frost lies only in the ring (the texture's alpha), on a
	// square as wide as the ring, as its flat UVs have the texture
	G->Plane(&G->Root(), 2 * 2.7 * Scale, 2 * 2.7 * Scale, M, FVector::ZeroVector, FVector(-UE_DOUBLE_HALF_PI, 0, 0));
	G->Root().Position = FVector(At.X, At.Y + 0.03, At.Z); G->Root().Apply();
	G->AddToScene();
	Rings.Add(MoveTemp(G));
}

void FS47Abduction::Record(const FVector& At, const FString& Scene, bool bNone)
{
	FRecord R;
	R.Fig = MakeUnique<FS47JacketWalker>(D.Web, TEXT("abduct:record"));
	FS47Node& G = R.Fig->G->Root();
	G.Position = At; G.Rotation.Y = Rnd(0, UE_DOUBLE_PI * 2); G.Apply();
	R.Fig->G->SetVisible(false); R.Fig->G->AddToScene();
	R.Scene = Scene; R.bNone = bNone;
	R.Home = At; R.HomeYaw = G.Rotation.Y;
	Records.Add(MoveTemp(R));
}

TArray<FS47Abduction::FShown> FS47Abduction::RecordsShown() const
{
	TArray<FShown> Out;
	for (int32 i = 0; i < Records.Num(); ++i) if (Records[i].Fig->G->Visible() && !Records[i].bHeld) Out.Add({ i + 1, Records[i].Fig->G->Root().Position });
	return Out;
}

TArray<FVector> FS47Abduction::RingsShown() const
{
	TArray<FVector> Out;
	for (const TUniquePtr<FS47Group>& R : Rings) if (R->Visible() && R->InScene()) Out.Add(R->Root().Position);
	return Out;
}

void FS47Abduction::RecordsStep()
{
	// the recordings stand where they were taken; walk up to one and it was not there
	const FVector C = PlayerPos();
	const FS47Cam Cam = Camera();
	for (FRecord& R : Records)
	{
		if (R.bHeld || R.bNone) continue;
		const FVector P = R.Fig->G->Root().Position;
		const bool bFar = R.Scene == SceneId && FMath::Sqrt(FMath::Square(P.X - C.X) + FMath::Square(P.Z - C.Z)) > 45;
		const FVector Head(P.X, P.Y + 1.2, P.Z);
		// (back where it stands after it walked: only once that place has been out of the view)
		if (R.bHide && (!bFar || !S47Facing(Cam, Head, 1000, 50))) R.bHide = false;
		R.Fig->G->SetVisible(bFar && !R.bHide);
		if (R.Fig->G->Visible()) R.Fig->Shine(D.Shine ? D.Shine(Head) : 0, &Cam);
	}
}

TArray<FS47Abduction::FRec> FS47Abduction::Recordings()
{
	TArray<FRec> Out;
	for (int32 i = 0; i < Records.Num(); ++i)
	{
		if (!Data.Spots.IsValidIndex(i) || Data.Spots[i].bCab) continue;
		const FS47AbductSpot& Sp = Data.Spots[i];
		FRec R;
		R.N = i + 1; R.Scene = Records[i].Scene; R.Take = Sp.Take; R.Path = Sp.Path; R.Walked = Sp.Walked; R.Clock = Sp.Clock; R.Fig = Records[i].Fig.Get();
		R.MarkWalked = [this, i](TOptional<double> C) { if (Data.Spots.IsValidIndex(i)) Data.Spots[i].Walked = C; };
		Out.Add(MoveTemp(R));
	}
	return Out;
}

void FS47Abduction::HoldRecord(int32 N, bool bHeld)
{
	if (!Records.IsValidIndex(N - 1)) return;
	FRecord& R = Records[N - 1];
	R.bHeld = bHeld;
	if (!bHeld)
	{
		FS47Node& G = R.Fig->G->Root();
		G.Position = R.Home; G.Rotation = FVector(0, R.HomeYaw, 0); G.Apply();
		R.Fig->Stand();
		R.bHide = true; R.Fig->G->SetVisible(false);
	}
}

TOptional<FS47Pix> FS47Abduction::FromAbove(const FVector& At, bool bMe, bool bTruck)
{
	// yourself lying in the grass, from above (the frame and the extra exposure)
	const double Y = D.Ground ? D.Ground(At.X, At.Z) : 0;
	if (bMe) { Lying->Root().Position = FVector(At.X, Y + 0.12, At.Z); Lying->Root().Rotation.Y = Rnd(0, UE_DOUBLE_PI * 2); Lying->Root().Apply(); Lying->SetVisible(true); }
	FS47ShotCam Cam;
	Cam.Fov = 40; Cam.Aspect = 1.6; Cam.Near = 0.5; Cam.Far = 400;
	// (the truck: higher, the whole of it and the ring round it in the frame)
	Cam.Pos = FVector(At.X + 0.3, Y + (bTruck ? 13 : bMe ? 7.5 : 5.5), At.Z + 0.2);
	Cam.LookAt(FVector(At.X, Y, At.Z));
	// their light, from straight above: the only time the ground here is seen like this (three's
	// PointLight(0xeef4ff, 320 or 140, 40 or 30, decay 1.6); Unreal's falls off as the square, as the
	// web world's other point lights do here)
	UPointLightComponent* L = nullptr;
	if (D.Web)
	{
		L = NewObject<UPointLightComponent>(D.Web);
		L->SetupAttachment(D.Web->GetRootComponent());
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(bTruck ? 320.f : 140.f);
		L->SetLightColor(S47Three::Hex(0xeef4ff));
		L->SetAttenuationRadius(bTruck ? 4000.f : 3000.f);
		L->SetCastShadows(true);
		L->SetWorldLocation(S47Three::ToUE(At.X, Y + (bTruck ? 14 : 9), At.Z));
		L->RegisterComponent();
	}
	TArray<FColor> Px;
	const bool bOk = D.Shoot && D.Shoot(Cam, Px) && Px.Num() == 640 * 400;
	if (L) L->DestroyComponent();
	Lying->SetVisible(false);
	if (!bOk) return {};
	return S47Abduct::Grain(FS47Pix::FromColors(Px, 640, 400), 7, bMe ? 0.9 : 1.3);
}

void FS47Abduction::MakeFrames()
{
	Frames.Reset();
	TArray<FS47Pix> Out;
	// the light: painted, with your legs in it and the shadow going from under them; else the light
	// drawn here and the grass from above with nobody in it
	const TOptional<FS47Pix> LightF = S47Abduct::Painted(TEXT("abductLight"), 0.25, 3);
	Out.Add(LightF ? *LightF : S47Abduct::FrameLight(3));
	// (at the wheel: the truck, not where the player's body was left when they got in)
	FS47Truck* Truck = bCab && bCabIn && D.Cab && D.Cab->Group ? D.Cab->Group() : nullptr;
	if (!LightF) { if (TOptional<FS47Pix> Grass = FromAbove(Truck ? Truck->Position : PlayerPos(), false)) Out.Add(*Grass); }
	FS47Pix Under;
	const bool bUnder = S47Raster::Art(TEXT("abductLight"), Under);
	Out.Add(S47Abduct::FrameHand(11, bUnder ? &Under : nullptr));
	struct FP { const TCHAR* Id; double Y, Seed; };
	for (const FP& F : { FP{ TEXT("abductTruck"), 0.5, 13 }, FP{ TEXT("abductFigures"), 0.45, 17 } }) if (TOptional<FS47Pix> P = S47Abduct::Painted(F.Id, F.Y, F.Seed)) Out.Add(*P);
	// (at the wheel: the truck from above, with the player in it)
	TOptional<FS47Pix> Me = Truck ? FromAbove(Truck->Position, false, true) : FromAbove(PlayerPos(), true);
	if (!Me) Me = S47Abduct::Painted(TEXT("abductAbove"), 0.5, 19, 0.9);
	if (Me) Out.Add(*Me);
	for (const FS47Pix& P : Out) Frames.Emplace(S47Image::FromPixels(P.Colors(), P.W, P.H));
}

// ---------- the tone ----------

void FS47Abduction::ToneOn()
{
	const TSharedPtr<FS47AudioContext> Ctx = D.Audio ? D.Audio->Ctx : nullptr;
	if (!Ctx) return;
	FS47AudioContext::FBatch Bt(*Ctx);
	FTone T;
	const TSharedRef<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValue(0);
	const TSharedRef<FS47BiquadFilterNode> Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(160);
	for (const double F : { 37.0, 52.5, 74.0 }) { const TSharedRef<FS47OscillatorNode> X = Ctx->CreateOscillator(); X->Frequency.SetValue(F); X->Connect(Lp); X->Start(); T.Osc.Add(X); }
	Lp->Connect(G); G->Connect(D.Audio->SfxBus);
	// without the bought pressure (a build without the licensed folder) all of the tone is below what a
	// phone's speaker plays, and :31 would pass unheard there: a little of its upper partials over it, a
	// hair off so they beat slowly
	if (!Bed)
	{
		const TSharedRef<FS47GainNode> Up = Ctx->CreateGain(); Up->Gain.SetValue(0.16); Up->Connect(G);
		for (const double F : { 222.3, 296.5, 369.4 }) { const TSharedRef<FS47OscillatorNode> X = Ctx->CreateOscillator(); X->Frequency.SetValue(F); X->Connect(Up); X->Start(); T.Osc.Add(X); }
	}
	T.Gain = G;
	Tone = T;
}

void FS47Abduction::ToneOff()
{
	if (!Tone) return;
	if (D.Audio && D.Audio->Ctx)
	{
		FS47AudioContext::FBatch Bt(*D.Audio->Ctx);
		for (const TSharedPtr<FS47OscillatorNode>& X : Tone->Osc) if (X) X->Stop();
		if (Tone->Gain) Tone->Gain->Disconnect();
	}
	Tone.Reset();
}

// ---------- the case ----------

void FS47Abduction::Restore(const S47J::FObj& V)
{
	Clear();
	Data = V.IsValid() ? FS47AbductSave::FromJson(V) : FS47AbductSave();
	for (const FS47AbductSpot& Sp : Data.Spots)
	{
		Ring(Sp.Back, Sp.bCab && FMath::Sqrt(FMath::Square(Sp.Back.X - Sp.Take.X) + FMath::Square(Sp.Back.Z - Sp.Take.Z)) > 100 ? 2.05 : 1);
		Record(Sp.Take, Sp.Scene, Sp.bCab);
	}
}

void FS47Abduction::Halt()
{
	if (Stage == TEXT("calm")) return;
	if (D.Flashlight) D.Flashlight->Flicker(0);
	if (D.Hush) D.Hush(0.1, 1, {}, {});
	if (D.Glitch) D.Glitch(0);
	Clear();
}

void FS47Abduction::Refile() { for (const FS47AbductSpot& Sp : Data.Spots) if (Sp.Doc && D.File) D.File(*Sp.Doc); }

void FS47Abduction::Clear()
{
	ToneOff(); Bed.Stop(); Bed = FS47Pressure(); bSilent = false; Stage = TEXT("calm"); St = 0; Beat = 0; Pressure = 0; Top = Rnd(0.85, 1.15); BackAt.Reset();
	bForced = false; PathV.Reset(); bCab = false; bCabIn = false; bAwakeCab = false;
	if (D.Screen.Hide) D.Screen.Hide();
	if (D.Screen.Light) D.Screen.Light({});
	if (D.Cab && D.Cab->Light) D.Cab->Light(0);
	OverDrop();
	Rings.Reset();
	Records.Reset();
}

// ---------- the figures and the frames ----------

namespace S47Abduct
{
	FS47Pix Grain(const FS47Pix& Src, double Seed, double Dark, double Grey, double Contrast)
	{
		FS47Pix G(640, 400);
		G.Filter = { { FS47Pix::EFilter::Grayscale, Grey }, { FS47Pix::EFilter::Contrast, Contrast }, { FS47Pix::EFilter::Brightness, Dark } };
		G.DrawImage(Src, 0, 0, 640, 400);
		G.Filter.Reset();
		S47Raster::FRng R(Seed);
		for (int32 i = 0; i < 9000; ++i)
		{
			const double A = R() * 0.08;
			G.SetFill(0xffffff, A);
			const double X = R() * 640, Y = R() * 400;
			G.FillRect(X, Y, 1, 1);
		}
		FS47Pix::FPaint V = FS47Pix::Radial(320, 200, 90, 320, 200, 400);
		V.Stop(0, 0x000000, 0).Stop(1, 0x000000, 0.9);
		G.SetFill(V); G.FillRect(0, 0, 640, 400);
		for (int32 Y = 0; Y < 400; Y += 3) { G.SetFill(0x000000, 0.18); G.FillRect(0, Y, 640, 1); }
		return G;
	}

	TOptional<FS47Pix> Painted(const FString& Id, double Y, double Seed, double Dark)
	{
		// one of the painted frames (4:3), cut to the frames' 16:10 (y: where the cut sits, 0 the top) and
		// given the same grain and lines as the drawn ones, a little less grey
		FS47Pix Im;
		if (!S47Raster::Art(Id, Im)) return {};
		FS47Pix C(640, 400);
		const double Sh = Im.W / 1.6;
		C.DrawImage(Im, 0, (Im.H - Sh) * Y, Im.W, Sh, 0, 0, 640, 400);
		return Grain(C, Seed, Dark, 0.45, 1.15);
	}

	FS47Pix FrameLight(double Seed)
	{
		FS47Pix G(640, 400);
		FS47Pix::FPaint L = FS47Pix::Radial(320, 40, 10, 320, 160, 420);
		L.Stop(0, 0xffffff).Stop(0.35, 0xd8e0e4).Stop(1, 0x101214);
		G.SetFill(L); G.FillRect(0, 0, 640, 400);
		return Grain(G, Seed, 1);
	}

	FS47Pix FrameHand(double Seed, const FS47Pix* Under)
	{
		// a hand with fingers too long at the right-hand edge, against the light: over the grass of the
		// painted light frame, close, or over the light drawn here
		FS47Pix G(640, 400);
		if (Under)
		{
			// the lit grass beside the boots, nearer: the lower left of the picture
			G.DrawImage(*Under, 0, Under->H * 0.56, Under->W * 0.44, Under->W * 0.44 / 1.6, 0, 0, 640, 400);
			G.SetFill(0xebf2f6, 0.18); G.FillRect(0, 0, 640, 400);
		}
		else
		{
			FS47Pix::FPaint L = FS47Pix::Radial(260, 120, 20, 300, 200, 420);
			L.Stop(0, 0xe8eef0).Stop(1, 0x202426);
			G.SetFill(L); G.FillRect(0, 0, 640, 400);
		}
		// out of focus, too near the lens: the palm in from the edge, four long jointed fingers and a thumb
		G.SetFill(0x060607); G.Filter = { { FS47Pix::EFilter::Blur, 2.5 } };
		G.BeginPath(); G.Ellipse(668, 252, 88, 64, -0.35, 0, 7); G.Fill();
		auto Finger = [&G](double X, double Y, double A, double Len, double W, double Bend)
		{
			// three joints, each a little narrower and bent a little more
			double Px = X, Py = Y, Ang = A, Wd = W;
			G.BeginPath();
			TArray<FVector2D> Left, Right;
			for (const double K : { 0.42, 0.33, 0.25 })
			{
				const double Nx = Px + FMath::Cos(Ang) * Len * K, Ny = Py + FMath::Sin(Ang) * Len * K, O = Ang + UE_DOUBLE_HALF_PI;
				Left.Add(FVector2D(Px + FMath::Cos(O) * Wd, Py + FMath::Sin(O) * Wd)); Right.Add(FVector2D(Px - FMath::Cos(O) * Wd, Py - FMath::Sin(O) * Wd));
				Px = Nx; Py = Ny; Ang += Bend; Wd *= 0.78;
			}
			G.MoveTo(Left[0].X, Left[0].Y);
			for (int32 i = 1; i < Left.Num(); ++i) G.LineTo(Left[i].X, Left[i].Y);
			G.LineTo(Px + FMath::Cos(Ang) * Wd * 1.2, Py + FMath::Sin(Ang) * Wd * 1.2);
			for (int32 i = Right.Num() - 1; i >= 0; --i) G.LineTo(Right[i].X, Right[i].Y);
			G.ClosePath(); G.Fill();
		};
		struct FF { double A, Len, B; };
		for (const FF& F : { FF{ -2.25, 250, -0.1 }, FF{ -2.55, 300, -0.08 }, FF{ -2.85, 310, -0.05 }, FF{ -3.1, 270, 0.02 } }) Finger(612, 238, F.A, F.Len, 10, F.B);
		Finger(640, 300, -2.6, 150, 11, 0.18);
		G.Filter.Reset();
		return Grain(G, Seed, 1, Under ? 0.45 : 0.7, Under ? 1.15 : 1.3);
	}

	TArray<FColor> FrostPixels()
	{
		// the frost on the flattened grass: thick in the middle of the ring, thinning out at both edges in
		// patches, as frost lies (white where the alpha is; the ring's own UVs are flat, so a square)
		const int32 N = 256;
		uint32 Sd = 47;
		auto R = [&Sd] { Sd = Sd * 1664525u + 1013904223u; return Sd / 4294967296.0; };
		// a coarse noise of a few octaves, round the ring
		struct FCells { int32 N; TArray<double> V; };
		TArray<FCells> Cells;
		for (int32 o = 0; o < 3; ++o) { FCells C; C.N = 8 << o; for (int32 i = 0; i < C.N * C.N; ++i) C.V.Add(R()); Cells.Add(MoveTemp(C)); }
		auto Noise = [&Cells](double X, double Y)
		{
			double T = 0, W = 0;
			for (int32 o = 0; o < Cells.Num(); ++o)
			{
				const int32 Nn = Cells[o].N;
				const TArray<double>& V = Cells[o].V;
				const double Fx = X * (Nn - 1), Fy = Y * (Nn - 1);
				const int32 Ix = FMath::FloorToInt(Fx), Iy = FMath::FloorToInt(Fy);
				const double Ax = Fx - Ix, Ay = Fy - Iy;
				auto At = [&](int32 I, int32 J) { return V[FMath::Min(Nn - 1, J) * Nn + FMath::Min(Nn - 1, I)]; };
				const double K = 1.0 / (1 + o);
				T += K * ((At(Ix, Iy) * (1 - Ax) + At(Ix + 1, Iy) * Ax) * (1 - Ay) + (At(Ix, Iy + 1) * (1 - Ax) + At(Ix + 1, Iy + 1) * Ax) * Ay); W += K;
			}
			return T / W;
		};
		TArray<FColor> Px;
		Px.SetNumUninitialized(N * N);
		for (int32 Y = 0; Y < N; ++Y) for (int32 X = 0; X < N; ++X)
		{
			const double U = (X + 0.5) / N * 2 - 1, V = (Y + 0.5) / N * 2 - 1, Dm = FMath::Sqrt(U * U + V * V) * 2.7;   // metres from the middle
			const double Band = Smooth(Dm, 1.25, 1.75) * (1 - Smooth(Dm, 2.0, 2.72));
			// patches, and in them the grain of the frost on each blade
			const double A = Band * FMath::Clamp((Noise((X + 0.5) / N, (Y + 0.5) / N) - 0.32) * 2.4, 0.0, 1.0) * (0.55 + 0.45 * R());
			Px[Y * N + X] = FColor(255, 255, 255, FS47Pix::Clamp8(255 * A));
		}
		return Px;
	}

	UTexture2D* Frost()
	{
		static TStrongObjectPtr<UTexture2D> Tex;
		if (!Tex.IsValid()) Tex.Reset(S47Image::FromPixels(FrostPixels(), 256, 256));
		return Tex.Get();
	}
}

// ---------- the self test ----------

bool FS47Abduction::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s abduction: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the pressure on a line, the warning's beats and the corner clock (no world: no player, no sound)
	FS47AbductDeps D;
	TArray<FString> Notes;
	D.Note = [&Notes](const FString& T) { Notes.Add(T); };
	FS47Abduction A(D);
	FS47AbductIn In;
	In.bOk = true; In.Line = TEXT("B"); In.Along = FVector(0, 0, 1); In.Clock = 3 * 3600; In.NextFixed = 4 * 3600 + 35 * 60;
	A.Update(1, In);
	Check(A.Stage == TEXT("calm") && A.Pressure > 0.002 && A.Pressure < 0.01, TEXT("the pressure rises alone on a line"));
	In.May = false; A.Pressure = 2; A.Update(0.1, In);
	Check(A.Stage == TEXT("calm") && A.Pressure <= 1.2, TEXT("the curve keeps it at the top"));
	In.May.Reset(); A.Update(0.1, In);
	Check(A.Stage == TEXT("warning") && A.WarnClock() && A.WarnClock()->Hm == TEXT("03:00") && A.WarnClock()->Ss == TEXT("00"), TEXT("the warning begins with the corner clock at :00"));
	for (int32 i = 0; i < 171; ++i) A.Update(0.1, In);
	Check(A.bSilent && A.WarnClock()->Ss == TEXT("17"), TEXT(":16 takes the footsteps"));
	FS47AbductIn Off = In; Off.Line.Reset();
	A.Update(0.1, Off);
	Check(A.Stage == TEXT("calm") && FMath::IsNearlyEqual(A.Pressure, 0.35) && !A.bSilent, TEXT("off the line, it passes"));
	A.Force(Off);
	A.Update(0.1, Off);
	Check(A.Stage == TEXT("warning") && A.bForced, TEXT("STATION 01's cannot be got away from"));
	FS47AbductIn TooLate = In; TooLate.NextFixed = In.Clock + 600;
	Check(!A.MayBegin(TooLate), TEXT("no room for the lost time before the next fixed point"));
	// the save keeps every field of a spot
	FS47AbductSave S;
	S.Count = 1; S.Last = 9000;
	FS47AbductSpot Sp; Sp.Scene = TEXT("saro"); Sp.Take = FVector(1, 2, 3); Sp.Back = FVector(4, 5, 6); Sp.Clock = 8000; Sp.Over = 1; Sp.Path = TArray<double>{ 1, 2, 0.5 }; Sp.bCab = true;
	FS47Doc Dc; Dc.Id = TEXT("away1"); Dc.Kind = TEXT("photo"); Dc.Title = TEXT("PHOTO"); Sp.Doc = Dc;
	S.Spots.Add(Sp); S.Station = TEXT("taken");
	const FS47AbductSave R = FS47AbductSave::FromJson(S47J::Read(S47J::Write(S.ToJson())));
	Check(R.Count == 1 && R.Last == 9000 && R.Spots.Num() == 1 && R.Spots[0].Take == FVector(1, 2, 3) && R.Spots[0].Over == 1 && R.Spots[0].Path->Num() == 3 && R.Spots[0].bCab && !R.Spots[0].bStation
		&& R.Spots[0].Doc->Id == TEXT("away1") && R.Station == TEXT("taken"), TEXT("the save keeps its spots"));
	Check(S47J::Write(FS47AbductSave().ToJson()) == TEXT("{\"count\":0,\"last\":-1000000000,\"spots\":[]}"), TEXT("a night without one saves as the web version's"));
	// the frost: a ring, nothing in the middle and nothing at the corners
	const TArray<FColor> F = S47Abduct::FrostPixels();
	int32 Ring = 0;
	for (int32 X = 0; X < 256; ++X) Ring += F[128 * 256 + X].A > 0 ? 1 : 0;
	Check(F[128 * 256 + 128].A == 0 && F[0].A == 0 && Ring > 20, TEXT("the frost lies in a ring"));
	return bOk;
}
