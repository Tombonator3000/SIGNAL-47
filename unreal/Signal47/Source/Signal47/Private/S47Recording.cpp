// story/recording.ts
#include "S47Recording.h"

#include "S47Figures.h"

using namespace S47Recording;

namespace
{
	/** At the end, standing still as long as the light took; then not there for a moment. */
	const double STAND = 3, GONE = 1.6;
	/** It can be made out (counts as looked at): the band lit by a beam from near the eye this much,
	 *  or this near (metres), against whatever is behind it. */
	const double CLEAR_LIT = 0.25, CLEAR_D = 60;
	/** A walk the player went away from before ever making it out comes again, this many times in all. */
	const int32 TRIES = 2;
	/** Less than this much ground covered in the 47 s, and it would only stand: it does not walk. */
	const double MOVED = 5;

	double LerpAngle(double A, double B, double K) { return A + FMath::Atan2(FMath::Sin(B - A), FMath::Cos(B - A)) * K; }
	double Smoothstep(double X, double Min, double Max) { if (X <= Min) return 0; if (X >= Max) return 1; X = (X - Min) / (Max - Min); return X * X * (3 - 2 * X); }
}

// ---------- the path ----------

double S47Recording::PathLen(const TArray<double>& P) { return FMath::Max(0.0, (P.Num() / 3 - 1) * PATH_DT); }

double S47Recording::PathAt(const TArray<double>& P, double T, FVector& Out)
{
	const int32 Nn = P.Num() / 3;
	if (Nn < 1) { Out = FVector::ZeroVector; return 0; }
	const double F = FMath::Clamp(T / PATH_DT, 0.0, double(Nn - 1));
	const int32 I = FMath::Min(Nn - 2, FMath::FloorToInt(F));
	const double K = Nn > 1 ? F - I : 0;
	auto At = [&](int32 J, int32 C) { return P[FMath::Max(0, FMath::Min(Nn - 1, J)) * 3 + C]; };
	// Catmull-Rom through the quarter seconds, so a turn is a turn and not a corner
	auto Cr = [&](int32 C)
	{
		const double P0 = At(I - 1, C), P1 = At(I, C), P2 = At(I + 1, C), P3 = At(I + 2, C), K2 = K * K, K3 = K2 * K;
		return 0.5 * (2 * P1 + (-P0 + P2) * K + (2 * P0 - 5 * P1 + 4 * P2 - P3) * K2 + (-P0 + 3 * P1 - 3 * P2 + P3) * K3);
	};
	Out = FVector(Cr(0), 0, Cr(1));
	return LerpAngle(At(I, 2), At(I + 1, 2), K);
}

double S47Recording::PathMoved(const TArray<double>& P)
{
	double Dd = 0;
	for (int32 I = 3; I + 2 < P.Num(); I += 3) Dd += FMath::Sqrt(FMath::Square(P[I] - P[I - 3]) + FMath::Square(P[I + 1] - P[I - 2]));
	return Dd;
}

// ---------- RecordingWalk ----------

FS47RecordingWalk::FS47RecordingWalk(FS47RecordingDeps InD) : D(MoveTemp(InD)) {}

TOptional<FVector> FS47RecordingWalk::At() const
{
	return Rec.IsValid() && Rec->Fig.IsValid() && Rec->Fig->G->Visible() && bClear ? TOptional<FVector>(Head) : TOptional<FVector>();
}

TSharedPtr<FS47Rec> FS47RecordingWalk::Candidate(const FS47RecordingIn& S) const
{
	for (const TSharedPtr<FS47Rec>& R : D.Recs ? D.Recs() : TArray<TSharedPtr<FS47Rec>>())
	{
		if (R->Walked || !R->Path.Num() || R->Scene != S.Scene || R->Path.Num() < 36 || PathMoved(R->Path) < MOVED) continue;
		const double Dist = FMath::Sqrt(FMath::Square(R->Take.X - S.Pos.X) + FMath::Square(R->Take.Z - S.Pos.Z));
		if (Dist >= FROM_D && Dist <= TO_D) return R;
	}
	return nullptr;
}

void FS47RecordingWalk::Update(double Dt, const FS47RecordingIn& S)
{
	if (Phase == TEXT("idle"))
	{
		const TSharedPtr<FS47Rec> C = S.bOk && S.bOut ? Candidate(S) : nullptr;
		bWaiting = C.IsValid();
		if (!C.IsValid() || !S.bMay) return;
		// it comes where the player is not looking: they turn, and someone is walking there
		PathAt(C->Path, 0, V);
		V.Y = (D.Ground ? D.Ground(V.X, V.Z) : 0) + 1.2;
		if (S47Facing(Cam(), V, 600, 40) || FMath::Sqrt(FMath::Square(V.X - S.Pos.X) + FMath::Square(V.Z - S.Pos.Z)) <= NEAR) return;
		Begin(C, S.Clock);
		return;
	}
	bWaiting = false;
	FS47Rec& R = *Rec;
	// (held while a cinematic or the like has the player; it does not end for it)
	if (!S.bOk && S.Scene == R.Scene) return;
	if (S.Scene != R.Scene || FMath::Sqrt(FMath::Square(R.Take.X - S.Pos.X) + FMath::Square(R.Take.Z - S.Pos.Z)) > GIVE_UP) { End(TEXT("left")); return; }
	const double Len = PathLen(R.Path);
	if (Phase == TEXT("walk"))
	{
		T += Dt;
		// now and then the tape it is on drops a few frames, and catches up
		if (Hitch > 0) Hitch -= Dt;
		else if ((NextHitch -= Dt) <= 0) { Hitch = 0.12 + FMath::FRand() * 0.18; NextHitch = 6 + FMath::FRand() * 5; }
		if (Hitch <= 0) Shown = T;
		if (T >= Len) { Phase = TEXT("stand"); St = 0; Shown = Len; }
	}
	else if (Phase == TEXT("stand"))
	{
		if ((St += Dt) >= STAND) { Phase = TEXT("gone"); St = 0; }
	}
	else if ((St += Dt) >= GONE)
	{
		// and it starts over, where the player was when the crickets stopped
		Loops++;
		if (Looked > 0.5) bSawRestart = true;
		// twice through; a third time if it was never looked at
		if (Loops >= (Looked > 0.5 ? 2 : 3)) { End(TEXT("done")); return; }
		Phase = TEXT("walk"); T = 0; Shown = 0; St = 0;
		PathAt(R.Path, 0, Prev);
	}
	Place(Dt, R);
	const FVector Gp = R.Fig.IsValid() ? R.Fig->G->Root().Position : V;
	const bool bNear = FMath::Sqrt(FMath::Square(Gp.X - S.Pos.X) + FMath::Square(Gp.Z - S.Pos.Z)) <= NEAR;
	// walked up to: it was not there
	if (bNear && Phase != TEXT("gone")) { End(TEXT("near")); return; }
	// g.visible = this.phase !== 'gone'
	if (R.Fig.IsValid()) R.Fig->G->SetVisible(Phase != TEXT("gone"));
	if (Phase != TEXT("gone") && bClear && S47Facing(Cam(), Head, 320, 18)) Looked += Dt;
}

void FS47RecordingWalk::Place(double Dt, FS47Rec& R)
{
	// where it is on the path and how it is carried there
	const TArray<double>& P = R.Path;
	const double ViewYaw = PathAt(P, Shown, V);
	PathAt(P, FMath::Min(PathLen(P), Shown + 0.15), W);
	const FVector Ahead = W - V;
	PathAt(P, FMath::Max(0.0, Shown - 0.15), W);
	const FVector Vel = (V - W + Ahead) / 0.3;
	const double Speed = Phase == TEXT("walk") ? FMath::Sqrt(Vel.X * Vel.X + Vel.Z * Vel.Z) : 0;
	// (the player walks at 1.9 m/s and runs at 3: player/Player.ts)
	const double Run = Smoothstep(Speed, 2.0, 2.9);
	// facing the way it goes while it goes, else the way the player was looking then
	const double Want = Speed > 0.45 ? FMath::Atan2(-Vel.X, -Vel.Z) : ViewYaw;
	const double Turn = FMath::Min(1.0, Dt * (Speed > 0.45 ? 7 : 3.5));
	YawNow = LerpAngle(YawNow, Want, Turn);
	// the stride from the ground covered (frozen with the tape)
	if (Hitch <= 0) PhaseStep += FMath::Sqrt(FMath::Square(V.X - Prev.X) + FMath::Square(V.Z - Prev.Z)) / FMath::Lerp(1.3, 2.2, Run) * UE_DOUBLE_PI * 2;
	Prev = V;
	const double Wide = FMath::Min(1.0, Speed / 1.1) * FMath::Lerp(0.62, 1.0, Run);
	const double Sw = FMath::Sin(PhaseStep);
	// the stumbling run: forward over the feet, and off balance now and then
	const double Tt = Shown;
	const double Lurch = Run * FMath::Pow(FMath::Max(0.0, FMath::Sin(Tt * 1.7 + 1.3 * FMath::Sin(Tt * 0.37))), 3);
	const double Lean = 0.03 + 0.17 * Run + 0.12 * Lurch;
	const double Roll = Run * (0.06 * FMath::Sin(Tt * 2.3 + 2 * FMath::Sin(Tt * 0.71)) + 0.05 * Lurch * FMath::Sign(FMath::Sin(Tt * 0.9)));
	const double GroundY = D.Ground ? D.Ground(V.X, V.Z) : 0;
	if (R.Fig.IsValid())
	{
		R.Fig->Pose(Sw, Wide, Lean, Roll, FMath::Abs(FMath::Cos(PhaseStep)) * 0.035 * Wide);
		if (Phase != TEXT("walk")) R.Fig->Stand();
		// g.position.set(v.x, ground, v.z); g.rotation.set(0, yaw, 0)
		FS47Node& G = R.Fig->G->Root();
		G.Position = FVector(V.X, GroundY, V.Z); G.Rotation = FVector(0, YawNow, 0); G.Apply();
	}
	Head = FVector(V.X, GroundY + 1.45, V.Z);
	W = FVector(V.X, GroundY + 1.2, V.Z);
	const FS47Cam C = Cam();
	if (R.Fig.IsValid()) R.Fig->Shine(D.Shine ? D.Shine(W) : 0, &C);
	bClear = (R.Fig.IsValid() ? R.Fig->Shone() : 0) >= CLEAR_LIT || FMath::Sqrt(FMath::Square(C.Pos.X - V.X) + FMath::Square(C.Pos.Z - V.Z)) <= CLEAR_D;
}

void FS47RecordingWalk::Begin(const TSharedPtr<FS47Rec>& R, double Clock)
{
	Rec = R; N = R->N; Phase = TEXT("walk"); T = 0; Shown = 0; St = 0; Loops = 0; Looked = 0;
	Hitch = 0; NextHitch = 5 + FMath::FRand() * 4; PhaseStep = 0; bNoted = false; bSawRestart = false; Ended.Reset();
	PathAt(R->Path, 0, Prev);
	YawNow = R->Path.Num() > 2 ? R->Path[2] : 0;
	if (D.Hold) D.Hold(R->N, true);
	Tries.FindOrAdd(R->N)++;
	R->Walked = FMath::FloorToDouble(Clock + 0.5);
	if (R->MarkWalked) R->MarkWalked(R->Walked);
	Place(0, *R);
	if (R->Fig.IsValid()) R->Fig->G->SetVisible(true);
	if (D.Save) D.Save();
}

void FS47RecordingWalk::End(const FString& Why)
{
	const TSharedPtr<FS47Rec> R = Rec;
	// seen walking for a while: written down, in the player's own words
	if (!bNoted && Looked >= 2.5)
	{
		bNoted = true;
		if (D.Note) D.Note(FString::Printf(TEXT("Someone in a SARO jacket, out where I was taken at %s. It went the way I went in those 47 seconds: it stopped where I stopped, ran where I ran, looked where I looked, and stood still at the end.%s"),
			*(D.ClockText ? D.ClockText(R->Clock) : FString()), bSawRestart ? TEXT(" Then it was not there, and it was back where I had been when the crickets stopped, doing it again.") : TEXT("")));
	}
	if (R->Fig.IsValid()) R->Fig->Stand();
	if (D.Hold) D.Hold(R->N, false);
	// the player went away before ever making it out (drove on, say): it was not seen, and it walks
	// again another time they are back within sight of the place (once more)
	if (Why == TEXT("left") && Looked <= 0 && Tries.FindRef(R->N) < TRIES) { R->Walked.Reset(); if (R->MarkWalked) R->MarkWalked({}); }
	Phase = TEXT("idle"); Ended = Why; Rec.Reset(); N = 0; bClear = false;
	if (D.Save) D.Save();
}

void FS47RecordingWalk::Reset()
{
	if (Rec.IsValid()) { if (Rec->Fig.IsValid()) Rec->Fig->Stand(); if (D.Hold) D.Hold(Rec->N, false); }
	Phase = TEXT("idle"); Rec.Reset(); N = 0; bWaiting = false; Ended.Reset(); bClear = false;
	Tries.Reset();
}

bool FS47RecordingWalk::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s recording: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// a path: 47 s walking north at 1.9 m/s, looking ahead (yaw 0)
	TArray<double> P;
	for (int32 I = 0; I < 189; ++I) P.Append({ 100.0, -10 - I * PATH_DT * 1.9, 0.0 });
	FVector At;
	Check(FMath::IsNearlyEqual(PathLen(P), 47.0) && FMath::IsNearlyEqual(PathMoved(P), 47 * 1.9, 1e-6), TEXT("47 s of path, its ground"));
	PathAt(P, 10.1, At);
	Check(At.Equals(FVector(100, 0, -10 - 10.1 * 1.9), 1e-6), TEXT("Catmull-Rom on a straight walk is the walk"));
	// it walks once the player is within sight and not looking that way; walked up to, it was not there
	const TSharedPtr<FS47Rec> R = MakeShared<FS47Rec>();
	R->N = 1; R->Scene = TEXT("saro"); R->Take = FVector(100, 0, -10); R->Path = P; R->Clock = 3 * 3600;
	R->Fig = MakeShared<FS47JacketWalker>(nullptr, TEXT("test"));
	TOptional<double> Marked; int32 Held = 0;
	R->MarkWalked = [&Marked](TOptional<double> C) { Marked = C; };
	FS47RecordingDeps Dp;
	Dp.Recs = [R] { return TArray<TSharedPtr<FS47Rec>>{ R }; };
	Dp.Hold = [&Held](int32, bool bH) { Held += bH ? 1 : -1; };
	FS47Cam Cam; Cam.Pos = FVector(0, 1.6, 30); Cam.Forward = FVector(0, 0, 1);   // looking south, away from it
	Dp.Camera = [&Cam] { return Cam; };
	FS47RecordingWalk Wk(Dp);
	FS47RecordingIn S; S.bOk = true; S.bOut = true; S.Scene = TEXT("saro"); S.Pos = FVector(0, 0, 30); S.Clock = 4 * 3600;
	Wk.Update(0.1, S);
	Check(Wk.bWaiting && !Wk.Busy(), TEXT("within sight, it waits for the curve"));
	S.bMay = true;
	Wk.Update(0.1, S);
	Check(Wk.Busy() && Wk.N == 1 && Marked == 4 * 3600 && Held == 1, TEXT("it walks, kept as walked, the one standing held"));
	for (int32 I = 0; I < 100; ++I) Wk.Update(0.1, S);
	Check(Wk.Spot() && Wk.Spot()->Z < -20, TEXT("along the player's own way"));
	S.Pos = *Wk.Spot();
	Wk.Update(0.1, S);
	Check(!Wk.Busy() && Wk.Ended == TEXT("near") && Held == 0, TEXT("walked up to: it was not there"));
	return bOk;
}
