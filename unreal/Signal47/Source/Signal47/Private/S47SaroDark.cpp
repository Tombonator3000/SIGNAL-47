// story/saroDark.ts
#include "S47SaroDark.h"

#include "S47Audio.h"
#include "S47Docs.h"
#include "S47SaroPower.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	const double T0 = 2 * 3600 + 29 * 60;              // 02:29, the telex's minute (KAPITLER.md)
	const double T1 = 2 * 3600 + 40 * 60;              // after this it does not come
	const double YARD_FROM = T0 + 150, ROOM_FROM = T0 + 420;   // the lab first; the yard from 02:31:30; the room from 02:36
	const double RELAY = 40, SAG = 31, DARK = 47;
	const double FIG_FROM = 6.5, FIG_ANY = 20;          // the figure: out of the middle of the view from 6.5 s, anywhere seen from 20 s
	const double FIG_GROUND = 12;                       // on the ground only from 12 s (a roof first, if one comes into view by then)
	const double TELEX_AFTER = 2.6, CPS = 10.5;         // the teleprinter: seconds after the light, characters a second
	const double SODIUM = 28;                           // the sodium lamps, from dim to full after the light
	const double STRIKE = 0.45;                         // the tubes and safelights stutter as they strike
	struct { double Low, High, Secs; } const ADAPT = { 0.32, 1.3, 14 };   // the starlight as the eyes open (the sky light's share)
	/** How far from the eye a spot may be (m, along the ground): further than this it is too small to
	 *  be anything (and up on a roof, the near edge decides how close: overEdge). */
	const double REACH_ROOF[2] = { 5, 30 }, REACH_GROUND[2] = { 11, 60 };
	/** Silhouette.headY standing (core/roadside.ts HEAD_MID). */
	const double HEAD_Y = 1.1;
	/** black against the night, with the faintest cold edge of the starlight on it */
	const uint32 EDGE = 0x1d2433;

	// What the eyes can see through at SARO: the buildings' footprints (xz), each with the openings it
	// has (a door, when it is open; the control room's windows on the north side).
	struct FHole { TCHAR Side; double A, B; const TCHAR* Door; };
	struct FBuilding { double MinX, MaxX, MinZ, MaxZ; TArray<FHole> Holes; };
	const TArray<FBuilding>& BUILDINGS()
	{
		static const TArray<FBuilding> B = {
			{ -6.3, 6.3, -4.75, 4.75, { { 'n', -5.4, 1.8, nullptr }, { 'e', 1.1, 2.1, TEXT("east") } } },   // the control room
			{ -6.6, 2.6, 4.75, 11.1, {} },                                                                // the annex
			{ 12.6, 19.2, -6.0, 1.0, { { 'w', -1.7, -0.7, TEXT("lab") } } },                              // the photo lab
			{ 20.2, 39.0, -5.9, 5.9, {} },                                                                // the east wing
		};
		return B;
	}

	/** Whether, from the eye, a figure up on a roof shows over the roof's near edge down to its
	 *  shoulders (close under the wall the edge hides it). */
	bool OverEdge(const FVector& Eye, const FS47DarkSpot& Spot, double Dist)
	{
		if (!Spot.bRoof) return true;
		const FBuilding& R = BUILDINGS()[Spot.On];
		const double Dx = Spot.X - Eye.X, Dz = Spot.Z - Eye.Z;
		double T0e = 0;   // where the line from the eye first gets over the footprint
		const double PQ[4][2] = { { -Dx, Eye.X - R.MinX }, { Dx, R.MaxX - Eye.X }, { -Dz, Eye.Z - R.MinZ }, { Dz, R.MaxZ - Eye.Z } };
		for (const auto& X : PQ) if (X[0] < 0) T0e = FMath::Max(T0e, X[1] / X[0]);
		const double Edge = FMath::Max(0.01, T0e * Dist), Up = Spot.RoofY - Eye.Y;
		return (Up + 0.7) / Dist > Up / Edge;
	}

	double Day(double Clock) { return FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0); }
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }
	double AngleDeg(const FVector& A, const FVector& B)
	{
		const double L = A.Size() * B.Size();
		return L < 1e-12 ? 90 : FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B) / L, -1.0, 1.0)));
	}
}

const double FS47SaroDark::FILED = T0;

const TArray<FS47DarkSpot>& S47DarkSpots()
{
	// Up on a roof first, where it can be (critic, 20 October: on a phone the figure out on the ground
	// was a dark stick of a dozen pixels against the dark): black against the stars, which is the one
	// thing that stays light when SARO goes dark. The control room's roof at its north-east corner, over
	// the empty room where K3 went (seen from the lab's doorway, the walk, the apron and B-12, 7 to 15 m;
	// in chapter two the steps go over the records room's roof next to it: story/roofSteps.ts), and the
	// east wing's at its north-west corner (from the apron and B-12, over the lab). Else out on the
	// grounds on the windows side, towards the highway, whose lamps are not SARO's and still light the
	// ground beyond: seen from the walk north of the lab, from B-12, and through the control room's
	// windows, what stands there is black against that light.
	static const TArray<FS47DarkSpot> S = {
		{ TEXT("roof"), 5.3, -3.9, TEXT("up on the roof of the control room, at the corner, against the stars"), true, 3.6, 0 },
		{ TEXT("wing"), 21.3, -4.9, TEXT("up on the roof of the east wing, against the stars"), true, 3.6, 3 },
		{ TEXT("grounds"), -6.5, -12.5, TEXT("out on the grounds by the windows, at the edge of the highway's light") },
		{ TEXT("fence"), -9.0, -17.5, TEXT("out by the fence towards the highway, at the edge of its light") },
	};
	return S;
}

bool S47ClearAt(const FVector& A, const FVector& B, const TFunction<bool(const FString&)>& Open, const TArray<FS47DarkRect>& More, int32 On)
{
	const double Dx = B.X - A.X, Dz = B.Z - A.Z;
	TArray<FBuilding> Boxes = BUILDINGS();
	for (const FS47DarkRect& M : More) Boxes.Add({ M.MinX, M.MaxX, M.MinZ, M.MaxZ, {} });
	for (int32 I = 0; I < Boxes.Num(); ++I)
	{
		const FBuilding& R = Boxes[I];
		if (I == On) { if (A.X > R.MinX && A.X < R.MaxX && A.Z > R.MinZ && A.Z < R.MaxZ) return false; continue; }
		// the segment's part inside the box (Liang-Barsky)
		double T0s = 0, T1s = 1;
		TCHAR Side0 = 0, Side1 = 0;
		auto Clip = [&](double P, double Q, TCHAR Enter, TCHAR Leave)
		{
			if (FMath::Abs(P) < 1e-9) return Q >= 0;
			const double T = Q / P;
			if (P < 0) { if (T > T1s) return false; if (T > T0s) { T0s = T; Side0 = Enter; } }
			else { if (T < T0s) return false; if (T < T1s) { T1s = T; Side1 = Leave; } }
			return true;
		};
		if (!Clip(-Dx, A.X - R.MinX, 'w', 'w') || !Clip(Dx, R.MaxX - A.X, 'e', 'e') || !Clip(-Dz, A.Z - R.MinZ, 'n', 'n') || !Clip(Dz, R.MaxZ - A.Z, 's', 's')) continue;
		if (T1s <= T0s) continue;
		// from inside (the eye in the room or the lab): out through an opening, or not at all
		auto Through = [&](double T, TCHAR Side)
		{
			if (!Side) return false;
			const double X = A.X + Dx * T, Z = A.Z + Dz * T, Along = Side == 'n' || Side == 's' ? X : Z;
			return R.Holes.ContainsByPredicate([&](const FHole& H) { return H.Side == Side && Along >= H.A && Along <= H.B && (!H.Door || (Open && Open(H.Door))); });
		};
		const bool bFromIn = T0s <= 0 && Side0 == 0, bToIn = T1s >= 1 && Side1 == 0;
		if (bFromIn && !bToIn) { if (!Through(T1s, Side1)) return false; continue; }
		if (!bFromIn && bToIn) { if (!Through(T0s, Side0)) return false; continue; }
		if (bFromIn && bToIn) continue;
		return false;   // straight across a building
	}
	return true;
}

FS47SaroDark::FS47SaroDark(FS47DarkDeps InD) : D(MoveTemp(InD))
{
	// black against the night, with the faintest cold edge of the starlight on it (the export's
	// Silhouette('saroDark:figure', 0x1d2433, 'stand'), hidden, under the scene)
	Fig = S47SceneObject(D.Web.Get(), TEXT("saroDark:figure"));
	SetFigVisible(false);
}

void FS47SaroDark::SetFigVisible(bool bOn)
{
	bFigVisible = bOn;
	if (Fig) Fig.SetVisible(bOn);
}

bool FS47SaroDark::Busy() const
{
	return Phase == TEXT("relay") || Phase == TEXT("dark") || (Phase == TEXT("back") && Printed < Chars.Num());
}

bool FS47SaroDark::Window(double Clock) { const double C = Day(Clock); return C >= T0 && C < T1; }
bool FS47SaroDark::Pending(double Clock) { return Day(Clock) < T1; }

void FS47SaroDark::Update(double Dt, const FS47DarkIn& S)
{
	NowClock = S.Clock;
	SodiumStep(Dt);
	AdaptStep(Dt);
	if (Phase == TEXT("idle"))
	{
		if (S.bOk && !S.bHold && Due(S)) Begin(S);
		return;
	}
	if (Phase == TEXT("done")) return;
	if (S.bHold && Phase != TEXT("back")) return;
	T += Dt;
	if (Phase == TEXT("relay")) RelayStep(S);
	else if (Phase == TEXT("dark")) DarkStep(S);
	else BackStep();
}

bool FS47SaroDark::Due(const FS47DarkIn& S) const
{
	const double C = Day(S.Clock);
	if (C < T0 || C >= T1) return false;
	return S.Where == TEXT("lab") || (S.Where == TEXT("yard") && C >= YARD_FROM) || (S.Where == TEXT("room") && C >= ROOM_FROM);
}

bool FS47SaroDark::Now(const FS47DarkIn& S)
{
	if (Phase != TEXT("idle") || S.Where == TEXT("away")) return false;
	Begin(S);
	return true;
}

void FS47SaroDark::Begin(const FS47DarkIn& S)
{
	US47Audio* A = D.Audio.Get();
	Phase = TEXT("relay"); T = 0; bSagged = false; bNoted = false;
	At = JsRound(S.Clock); From = S.Where; Figure.Reset();
	bWasOpen = D.Relay ? !D.Relay() : false;
	if (A) A->RelayClack(D.Rack ? D.Rack() : FVector::ZeroVector, S.Where != TEXT("room"));
	Toast(S.Where == TEXT("room") ? TEXT("On the rack, K3 trips.") : TEXT("Through the wall, in the empty control room: the relay."), 3.6);
	ClockNow();
}

void FS47SaroDark::RelayStep(const FS47DarkIn& S)
{
	US47Audio* A = D.Audio.Get();
	ClockNow();
	// :31, a sag: everything dims for a moment and the hum with it
	if (!bSagged && T >= SAG)
	{
		bSagged = true; SagT = T;
		if (D.Power) { D.Power->Cut(); D.Power->Set(0.35, 0.5); }
		if (A) A->HumLevel(0.3f, 0.03f);
	}
	if (SagT >= 0 && T >= SagT + 0.34) { SagT = -1; if (D.Power) D.Power->Restore(); if (A) A->HumLevel(1, 0.15f); }
	if (T >= RELAY) GoDark(S);
}

void FS47SaroDark::GoDark(const FS47DarkIn& S)
{
	US47Audio* A = D.Audio.Get();
	Phase = TEXT("dark"); T = 0; SagT = -1;
	bSawOut = S.Where == TEXT("yard") || S.Where == TEXT("room"); bDoorway = false;
	if (D.Power) D.Power->Cut();
	const FVector Rack = D.Rack ? D.Rack() : FVector::ZeroVector;
	if (A)
	{
		A->Contactor(Rack, S.Where == TEXT("room") ? 1.f : 1.3f);
		A->PowerDown(Rack);
		A->HumLevel(0, 0.05f);
		A->HoldRoomTone(0, 0.05f);
	}
	if (D.PowerOut) D.PowerOut(true);
	if (D.Log) D.Log(TEXT("LINE POWER  FAIL"), JsRound(S.Clock));
	// a breath later the night outside goes quiet too, and stays so until the light
	HushAt = 1.4;
	Adapt = ADAPT.Low; AdaptT = 0;
	Toast(S.Where == TEXT("lab") ? TEXT("The safelight dies, and the hum with it. Black.") : S.Where == TEXT("room") ? TEXT("Every light in the room goes out at once.") : TEXT("Every lamp at SARO goes out at once. The red lamps on the dishes too."), 3.6);
	ClockShow = FClockShow{ ClockShow ? ClockShow->Hm : S47Text::ClockText(At, false), TEXT("40") };
}

void FS47SaroDark::DarkStep(const FS47DarkIn& S)
{
	if (HushAt >= 0 && T >= HushAt) { HushAt = -1; if (D.Hush) D.Hush(600, 0, 0.9, 0.2); }
	if (S.Where == TEXT("yard") || S.Where == TEXT("room")) bSawOut = true;
	// in the black of the lab, the one thing left to see is the way out (and from its doorway the
	// control room's roof against the stars)
	if (!bDoorway && T >= 6 && S.Where == TEXT("lab") && S.Open && S.Open(TEXT("lab"))) { bDoorway = true; Toast(TEXT("Only the doorway, grey with starlight."), 3.4); }
	Eye = S.Cam.Pos;
	if (bFigVisible) Watch(S);
	else if (!Figure && T >= FIG_FROM) Place(S);
	if (T >= DARK) GoBack();
}

void FS47SaroDark::Place(const FS47DarkIn& S)
{
	// It stands where the player can see it from here, out of the middle of the view (from FIG_ANY,
	// anywhere they can see it).
	const FVector F = S.Cam.Forward;
	const FS47DarkSpot* Best = nullptr;
	FVector BestP;
	double BestScore = -1e18;
	for (const FS47DarkSpot& Spot : S47DarkSpots())
	{
		const FVector P(Spot.X, Spot.bRoof ? Spot.RoofY : (D.Ground ? D.Ground(Spot.X, Spot.Z) : 0), Spot.Z);
		const FVector HeadP(P.X, P.Y + HEAD_Y, P.Z);
		const double Dist = FMath::Sqrt(FMath::Square(HeadP.X - Eye.X) + FMath::Square(HeadP.Z - Eye.Z));
		const double* Reach = Spot.bRoof ? REACH_ROOF : REACH_GROUND;
		if ((!Spot.bRoof && T < FIG_GROUND) || Dist < Reach[0] || Dist > Reach[1] || !OverEdge(Eye, Spot, FMath::Sqrt(FMath::Square(Spot.X - Eye.X) + FMath::Square(Spot.Z - Eye.Z)))
			|| !S47ClearAt(Eye, HeadP, S.Open, S.Blocks, Spot.bRoof ? Spot.On : -1)) continue;
		const double Off = AngleDeg(F, HeadP - Eye);
		if (Off < 38 && T < FIG_ANY) continue;
		// (a roof where there is one: against the stars it reads, on a phone too)
		const double Score = Off + (Spot.bRoof ? 1000 : 0);
		if (!Best || Score > BestScore) { Best = &Spot; BestP = P; BestScore = Score; }
	}
	if (!Best) return;
	FigPos = BestP;
	if (Fig)
	{
		Fig.SetPosition(BestP);
		Fig.SetRotation(FVector(0, FMath::Atan2(Eye.X - BestP.X, Eye.Z - BestP.Z), 0));
		// sil.wake(); sil.pose('stand'); sil.rim(0x1d2433): the shell is the rim
		const TArray<FS47Obj> Parts = Fig.Children();
		if (Parts.Num() > 1) Parts[1].SetColor(S47Three::Hex(EDGE));
	}
	Head = FVector(BestP.X, BestP.Y + HEAD_Y, BestP.Z);
	SetFigVisible(true);
	Figure = FFigure{ Best->Id, false, FString() };
}

void FS47SaroDark::Watch(const FS47DarkIn& S)
{
	// Looked at, it stays. A closer look, the flashlight on it, or walking up to it, and it was not there.
	FFigure& Fg = *Figure;
	const double Dist = FVector::Dist(Eye, Head);
	if (S47Facing(S.Cam, Head, 90, 22)) Fg.bLooked = true;
	FString Gone;
	const FS47DarkSpot* Spot = S47DarkSpots().FindByPredicate([&](const FS47DarkSpot& X) { return Fg.Id == X.Id; });
	if (S.Zoom > 0.25 && S47Facing(S.Cam, Head, 120, 14)) Gone = TEXT("zoom");
	else if (S.bTorch && Dist < 70 && S47Facing(S.Cam, Head, 70, 40)) Gone = TEXT("flashlight");
	// (up on a roof: only right under it, where its edge hides it anyway; walk past and look back
	// and it is still there)
	else if (Spot && Spot->bRoof ? FMath::Sqrt(FMath::Square(Head.X - Eye.X) + FMath::Square(Head.Z - Eye.Z)) < 3 : Dist < 9) Gone = TEXT("near");
	if (!Gone.IsEmpty()) { Fg.Gone = Gone; SetFigVisible(false); }
}

void FS47SaroDark::GoBack()
{
	US47Audio* A = D.Audio.Get();
	Phase = TEXT("back"); T = 0;
	if (bFigVisible) { SetFigVisible(false); if (Figure) Figure->Gone = TEXT("light"); }
	// the tubes, the safelights and the screens strike at once (a stutter), the sodium lamps dim
	if (D.Power) D.Power->Set(1, 0.06);
	SodiumT = 0;
	const FVector Rack = D.Rack ? D.Rack() : FVector::ZeroVector;
	if (A)
	{
		A->Contactor(Rack, 0.8f);
		A->PowerUp(Rack);
		A->HumLevel(1, 0.4f);
		A->HoldRoomTone(1, 0.35f);
	}
	if (D.PowerOut) D.PowerOut(false);
	if (D.Hush) D.Hush(0, 1, 0.05, 0.12);   // the crickets all at once
	HushAt = -1;
	Adapt = 1; AdaptT = -1;
	if (D.Log) D.Log(TEXT("LINE POWER  RESTORED  00:00:47"), JsRound(NowClock));
	ClockShow.Reset();
	// the teleprinter: the message as it prints, at its own pace, a line at a time
	Chars.Reset(); Printed = 0;
	double Tc = TELEX_AFTER + 0.6;
	for (const FString& Line : S47Text::Split(S47Docs::TelexHondo(At), TEXT("\n")))
	{
		for (const TCHAR Ch : Line) { if (Ch != ' ') Chars.Add({ Tc, false }); Tc += 1 / CPS; }
		Chars.Add({ Tc, true }); Tc += 0.32;
	}
	Toast(TEXT("The lights come back."), 2.6);
}

void FS47SaroDark::BackStep()
{
	US47Audio* A = D.Audio.Get();
	if (!bMotor && T >= TELEX_AFTER)
	{
		bMotor = true;
		if (A) A->TelexMotor(D.Teleprinter ? D.Teleprinter() : FVector::ZeroVector);
		Toast(TEXT("In the control room, the telex starts up."), 3.2);
	}
	const FVector At3 = D.Teleprinter ? D.Teleprinter() : FVector::ZeroVector;
	while (Printed < Chars.Num() && T >= Chars[Printed].T) { if (A) A->TypeBar(At3, Chars[Printed].bCr); Printed++; }
	if (bMotor && Printed >= Chars.Num())
	{
		bMotor = false;
		if (A) A->TelexMotor(TOptional<FVector>());
		if (D.Telex) D.Telex(At);
		Finish();
	}
}

void FS47SaroDark::Finish()
{
	Phase = TEXT("done");
	if (bNoted) return;
	bNoted = true;
	const FString Hm = S47Text::ClockText(At, false);
	// (what was seen of it: from the lab all along only the safelight and the hum went)
	const FString Went = bSawOut ? TEXT("every light at SARO went out, the red lamps on the dishes too") : TEXT("the power went: the safelight, the hum, everything");
	if (D.Note) D.Note(bWasOpen
		? FString::Printf(TEXT("%s. A clack from the rack, in the empty control room: K3, with nothing left to trip. It tripped anyway. Forty seconds later %s. For 47 s. The telex ran when the power came back."), *Hm, *Went)
		: FString::Printf(TEXT("%s. In the empty control room K3 tripped. Forty seconds later %s. For 47 s. The telex ran when the power came back."), *Hm, *Went));
	const FS47DarkSpot* Spot = Figure ? S47DarkSpots().FindByPredicate([&](const FS47DarkSpot& X) { return Figure->Id == X.Id; }) : nullptr;
	if (Figure && Figure->bLooked && Spot && D.Note)
		D.Note(FString::Printf(TEXT("%s. In the dark, %s: someone small, standing quite still. %s"), *Hm, Spot->Where, Figure->Gone == TEXT("light") ? TEXT("Not there when the lights came back.") : TEXT("Then not there.")));
}

void FS47SaroDark::SodiumStep(double Dt)
{
	// the stutter as the power strikes, and the sodium lamps warming up after
	if (SodiumT < 0) return;
	SodiumT += Dt;
	const double K = FMath::Min(1.0, SodiumT / SODIUM), E = 0.06 + 0.94 * K * K * (3 - 2 * K);
	// the strike: on, two short drop-outs, on (the same every time and at any frame rate: not a
	// strobe); with Flashing images on slow fades, straight on
	const double Ts = SodiumT;
	const bool bDrop = !(D.Calm && D.Calm()) && ((Ts > 0.07 && Ts < 0.16) || (Ts > 0.25 && Ts < 0.31));
	if (D.Power)
	{
		if (SodiumT < STRIKE) D.Power->Set(bDrop ? 0.12 : 1, E);
		else { D.Power->Settle(); D.Power->Set(1, E); }
	}
	if (K >= 1) { SodiumT = -1; if (D.Power) D.Power->Restore(); }
}

void FS47SaroDark::AdaptStep(double Dt)
{
	if (AdaptT < 0) return;
	AdaptT += Dt;
	const double K = FMath::Min(1.0, AdaptT / ADAPT.Secs);
	Adapt = ADAPT.Low + (ADAPT.High - ADAPT.Low) * K * K * (3 - 2 * K);
}

void FS47SaroDark::ClockNow()
{
	const double Ss = FMath::Min(40.0, FMath::FloorToDouble(T));
	ClockShow = FClockShow{ S47Text::ClockText(At, false), S47Text::Pad(Ss) };
}

void FS47SaroDark::Halt()
{
	US47Audio* A = D.Audio.Get();
	if (bMotor) { bMotor = false; if (A) A->TelexMotor(TOptional<FVector>()); }
	if (D.Power && D.Power->Out()) D.Power->Restore();
	if (Phase == TEXT("relay") || Phase == TEXT("dark") || Phase == TEXT("back"))
	{
		if (A) { A->HumLevel(1, 0.1f); A->HoldRoomTone(1, 0.1f); }
		if (D.PowerOut) D.PowerOut(false);
		if (D.Hush) D.Hush(0, 1, 0.05, 0.1);
	}
	SetFigVisible(false);
	Phase = TEXT("idle"); T = 0; SodiumT = -1; SagT = -1; Adapt = 1; AdaptT = -1; HushAt = -1;
	ClockShow.Reset(); Chars.Reset(); Printed = 0; Figure.Reset();
}

bool FS47SaroDark::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s saro dark: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the eyes and the buildings
	auto Shut = [](const FString&) { return false; };
	auto Open = [](const FString&) { return true; };
	Check(!S47ClearAt(FVector(-10, 1.6, 0), FVector(10, 1.6, 0), Shut), TEXT("straight across the control room: not seen"));
	Check(S47ClearAt(FVector(0, 1.6, -2), FVector(0, 1.6, -12), Shut), TEXT("from inside the room out through its windows (north): seen"));
	Check(!S47ClearAt(FVector(0, 1.6, 2), FVector(0, 1.6, 12), Shut), TEXT("from inside the room through the south wall: not seen"));
	Check(!S47ClearAt(FVector(16, 1.6, -1.2), FVector(8, 1.6, -1.2), Shut) && S47ClearAt(FVector(16, 1.6, -1.2), FVector(8, 1.6, -1.2), Open), TEXT("from the lab through its door: only while it is open"));
	Check(S47ClearAt(FVector(10, 1.6, -10), FVector(5.3, 4.7, -3.9), Shut, {}, 0), TEXT("a figure on the control room's roof, seen from the walk"));
	Check(!S47ClearAt(FVector(8, 1.6, -10), FVector(-6.5, 0.5, -12.5), Shut, { { 0, 2, -14, -8 } }), TEXT("the truck in the way"));
	Check(Window(2 * 3600 + 30 * 60) && !Window(2 * 3600 + 41 * 60) && Pending(2 * 3600) && !Pending(3 * 3600), TEXT("its window, 02:29 to 02:40"));
	// a run, without the world: the relay, the sag, the dark, the light, the telex
	TArray<FString> Logs, Notes;
	double Filed = -1;
	FS47DarkDeps Dp;
	Dp.Relay = [] { return false; };
	Dp.Log = [&Logs](const FString& S, double) { Logs.Add(S); };
	Dp.Telex = [&Filed](double F) { Filed = F; };
	Dp.Note = [&Notes](const FString& S) { Notes.Add(S); };
	Dp.Ground = [](double, double) { return -0.6; };
	FS47SaroDark Dk(Dp);
	FS47DarkIn S;
	S.bOk = true; S.Where = TEXT("lab"); S.Clock = 2 * 3600 + 28 * 60; S.Open = Shut;
	Dk.Update(0.1, S);
	Check(Dk.Phase == TEXT("idle"), TEXT("not before 02:29"));
	S.Clock = T0 + 5;
	Dk.Update(0.1, S);
	Check(Dk.Phase == TEXT("relay") && Dk.bWasOpen && Dk.ClockShow && Dk.ClockShow->Ss == TEXT("00"), TEXT("from the lab at 02:29: the relay, K3 with nothing left to trip, the clock in seconds"));
	S.bHold = true;
	Dk.Update(10, S);
	Check(Dk.T == 0, TEXT("a paper open holds it where it is"));
	S.bHold = false;
	for (int32 I = 0; I < 41; ++I) Dk.Update(1, S);
	Check(Dk.Phase == TEXT("dark") && Logs.Contains(TEXT("LINE POWER  FAIL")) && Dk.Dark() && Dk.Adapt < 0.5, TEXT("at 40 s every light goes, and the eyes have to open"));
	for (int32 I = 0; I < 48; ++I) Dk.Update(1, S);
	Check(Dk.Phase == TEXT("back") && Dk.Busy() && !Dk.ClockShow, TEXT("47 s later the light, and the telex still to print"));
	for (int32 I = 0; I < 40 && Dk.Busy(); ++I) Dk.Update(1, S);
	Check(Dk.Phase == TEXT("done") && Filed == JsRound(T0 + 5) && Notes.Num() == 1 && Notes[0].Contains(TEXT("K3, with nothing left to trip")) && Notes[0].Contains(TEXT("the power went: the safelight")),
		TEXT("the telex printed, filed at K3's minute, and the note in the player's words"));
	return bOk;
}
