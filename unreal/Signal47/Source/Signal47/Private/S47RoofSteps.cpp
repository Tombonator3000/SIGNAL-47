// story/roofSteps.ts
#include "S47RoofSteps.h"

#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Inter.h"
#include "S47WebAudio.h"

namespace
{
	// the annex's roof deck (Annex.ts: the slab 2.8 to 3.2 over x -6.3 to 2.3, z 4.8 to 10.8; the control
	// room's taller wall along its north side), and where the steps come from and go to
	struct FRoof { double X0, X1, Z0, Z1, Y; };
	constexpr FRoof ROOF = { -6.1, 2.1, 5.0, 10.6, 3.22 };
	const FVector2D FROM(1.9, 5.3);                  // over the corridor's east end, by the restrooms
	FVector2D CORNER(const FString& C) { return C == TEXT("sw") ? FVector2D(-6.12, 10.62) : FVector2D(2.12, 10.62); }
	constexpr double STRIDE = 0.3, SETTLE = 0.8, BEAT = 10.5, AHEAD = 0.12;
	constexpr int32 SLOTS = 8;
	constexpr double SLOT = 0.3, SLOT_LEN = 0.28;    // sfx/roof_steps.mp3: slot i from i * 0.3 s
	const TCHAR* KENNEY[] = { TEXT("stepGravel0"), TEXT("stepGravel1"), TEXT("stepGravel2"), TEXT("stepGravel3"), TEXT("stepGravel4") };
	/** Levels, measured in the game (tools/horror_roof.py, K-weighted): right overhead some 6 dB over the
	 *  player's own step on the records room's vinyl and 5 over the room's hum (which is as loud as that
	 *  step, and low; held 8 dB down while they come, so they stand 13 over it), the first ones across
	 *  the roof some 11 dB under the last, coming up out of the hum. */
	struct FLevel { double Lic, Kenney, Thump, Ring; };
	constexpr FLevel LEVEL = { 0.245, 0.217, 0.079, 0.1 };
	// under the deck: the corridor and the records room (Annex.ts), where dust can come down through the view
	struct FBox2 { double X0, X1, Z0, Z1; };
	constexpr FBox2 ROOMS = { -6.0, -1.0, 4.85, 10.5 };
	// the roof unit over the records room (Annex.ts), its top
	struct FUnit { double X0, X1, Z0, Z1, Y; };
	constexpr FUnit UNIT = { -5.0, -3.4, 8.05, 9.15, 4.06 };

	const TCHAR* UNIT_PLAIN = TEXT("The unit on the records room roof, its louvres furred with dust.");
	const TCHAR* UNIT_FROST = TEXT("The unit on the records room roof is white with frost, thickest along the top edge and down the side over the table. Nothing else up there has any. It is not cold enough for frost tonight.");
	const TCHAR* LIP_FROST = TEXT("A crust of frost on the lip of the roof at this corner. Nowhere else along the edge.");
	// the captions keep time with what is heard (the game captions its sounds as toasts; on a phone
	// with the sound down they are the steps): while they come, when they stop, when they go
	const TCHAR* CAPTION_COME = TEXT("Footsteps on the roof. Light and quick, closer together than anyone walks.");
	const TCHAR* CAPTION_STOP = TEXT("They stop, right over you.");
	const TCHAR* CAPTION_2 = TEXT("Again, further off, out to the edge of the roof. Then nothing.");

	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	/** three's Vector2.normalize: divided by its length, or left as it is when that is zero */
	FVector2D Norm(const FVector2D& V) { const double L = V.Size(); return L > 0 ? V / L : V; }
}

FS47RoofSteps::FS47RoofSteps(FS47RoofDeps InD) : D(MoveTemp(InD))
{
	US47Inter* I = D.Inter.Get();
	if (!I) return;
	FS47Interactable U;
	U.Id = TEXT("look:roofUnit"); U.Object = D.Hit.Unit; U.Label = [] { return FString(); }; U.Use = [] {}; U.bLook = true; U.ExamineRange = 32;
	U.Examine = [this] { return !(D.Outside && D.Outside()) ? FString() : Corner.IsEmpty() ? FString(UNIT_PLAIN) : (FrostSeen(), FString(UNIT_FROST)); };
	I->Add(U);
	for (const TCHAR* C : { TEXT("sw"), TEXT("se") })
	{
		const FString Cs = C;
		FS47Interactable L;
		L.Id = TEXT("look:roofLip") + Cs.ToUpper(); L.Object = Cs == TEXT("sw") ? D.Hit.Sw : D.Hit.Se; L.Label = [] { return FString(); }; L.Use = [] {}; L.bLook = true; L.ExamineRange = 26;
		L.Examine = [this, Cs] { return D.Outside && D.Outside() && Corner == Cs ? (FrostSeen(), FString(LIP_FROST)) : FString(); };
		I->Add(L);
	}
}

FVector FS47RoofSteps::Overhead() const { return FVector(Target.X, ROOF.Y, Target.Y); }

void FS47RoofSteps::Reset()
{
	Cancel();
	FS47TownSeen* S = D.Seen;
	Corner = S && S->Contains(TEXT("roof:sw")) ? TEXT("sw") : S && S->Contains(TEXT("roof:se")) ? TEXT("se") : TEXT("");
	Phase = S && S->Contains(TEXT("roof:steps")) ? TEXT("done") : TEXT("idle");
	// saved while they were still coming (a paper filed mid-way autosaves): they will not come
	// again, so what they left is up there all the same
	if (Phase == TEXT("done") && Corner.IsEmpty()) { Corner = TEXT("sw"); if (S) S->Add(TEXT("roof:sw")); }
	if (D.Frost) D.Frost(Corner);
	Room = 0; Quiet = 0; Still = 0; Last = FVector(1e9, 0, 0);
}

void FS47RoofSteps::Cancel()
{
	for (const TSharedPtr<FS47ScheduledSourceNode>& S : Sources) { S->OnEnded = nullptr; S->Stop(); S->Disconnect(); }
	Sources.Reset();
	Queue.Reset(); Cues.Reset();
	if (US47Audio* A = D.Audio.Get()) A->HoldRoomTone(1, 0.05f);
	if (D.Tubes) D.Tubes(FString());
}

bool FS47RoofSteps::Due(const FS47RoofIn& S) const
{
	// can they start now (the waiting is over)?
	if (!S.bOk || !S.bInRoom || !S.bQuiet) return false;
	// settled in, a paper read and put back, standing still: the first quiet moment
	if (S.bRead && Room >= 20 && Quiet >= 4 && Still >= 1.2) return true;
	// the board done and the key holder next: the room's last quiet moment, before the call
	if (S.bLastQuiet && Quiet >= 3.5) return true;
	// never standing still: still once, a while in
	return Room >= 75 && Quiet >= 6;
}

void FS47RoofSteps::Update(double Dt, const FS47RoofIn& S)
{
	const double Moved = Last.X > 1e8 ? 0 : FMath::Sqrt(FMath::Square(S.Pos.X - Last.X) + FMath::Square(S.Pos.Z - Last.Z));
	Last = S.Pos;
	if (Phase == TEXT("idle"))
	{
		// the time settled in the room counts only from when the steps could come (chapter two's records)
		if (S.bOk && S.bInRoom && S.bQuiet) { Room += Dt; Quiet += Dt; } else Quiet = 0;
		if (Moved > 0.004) Still = 0; else Still += Dt;
		if (Due(S)) Start(S);
		return;
	}
	if (!Busy()) return;
	if (!S.bChapter) { Cancel(); Phase = TEXT("done"); return; }
	bMuffled = S.bMuffled;
	if (S.bHold) return;             // a call or a cinematic: the rest waits for it
	T += Dt;
	Target = RoofOver(S.Pos);
	Plan();
	// play what falls due in the next moment, at its exact time
	US47Audio* A = D.Audio.Get();
	const bool bCtx = A && A->Ctx.IsValid();
	while (Queue.Num() && Queue[0].At <= T + AHEAD)
	{
		const FStep St = Queue[0];
		Queue.RemoveAt(0);
		const double Dd = FMath::Sqrt(FMath::Square(St.X - S.Pos.X) + FMath::Square(St.Z - S.Pos.Z));
		Log.Add({ St.At, St.X, St.Z, Dd, St.Burst });
		if (bCtx) Play(St, FMath::Max(0.0, St.At - T));
		// over the rooms and near enough to see: a few motes shaken out of the ceiling under it
		// (the last of the first ones and the trickle where it stops are cued in Stopped())
		if (S.bInRoom && St.X > ROOMS.X0 && St.X < ROOMS.X1 && St.Z > ROOMS.Z0 && St.Z < ROOMS.Z1 && Dd < 4.5)
		{
			const int32 N = St.Burst == 1 ? (Dd < 1.6 ? 7 : 4) : 3;
			Cue(St.At, [this, St, N] { if (D.Puff) D.Puff(St.X, St.Z, N); });
		}
		// the caption keeps time with them: once they are near enough to be plain, while they come
		if (St.Burst == 1 && !bTold && (Dd < 3.2 || Walk.N >= 9))
		{
			bTold = true;
			Cue(St.At + 0.15, [this] { if (D.Toast) D.Toast(CAPTION_COME, 4.2f); });
		}
	}
	while (Cues.Num() && Cues[0].At <= T) { TFunction<void()> Fn = MoveTemp(Cues[0].Fn); Cues.RemoveAt(0); if (Fn) Fn(); }
}

bool FS47RoofSteps::Now(const FS47RoofIn& S)
{
	if (Phase != TEXT("idle") || !S.bInRoom) return false;
	Start(S);
	return true;
}

void FS47RoofSteps::Start(const FS47RoofIn& S)
{
	Phase = TEXT("come"); T = 0; Log.Reset(); Queue.Reset(); Cues.Reset(); bTold = false;
	if (D.Seen) D.Seen->Add(TEXT("roof:steps"));   // never twice, even if the night is left in the middle
	US47Audio* A = D.Audio.Get();
	bBought = A && A->Lic(TEXT("roof_steps")).IsValid();
	ClockAt = D.Clock ? D.Clock() : 0;
	bMuffled = S.bMuffled;
	Target = RoofOver(S.Pos);
	// the hum sinks a little as they come (as when you stop to listen), and goes when they stop
	if (A) A->HoldRoomTone(0.4f, 0.6f);
	FWalk& W = Walk;
	W.Pos = FROM; W.N = 0; W.Next = 0.05; W.Settle = 0; W.bHesitated = false; W.bPlanned = false;
}

FVector2D FS47RoofSteps::RoofOver(const FVector& P)
{
	// the roof right over a point (clamped to the deck); within half a metre of the roof unit, up on
	// the unit (a player at the work table stands just off its edge: it steps up onto it, over them)
	const double X = FMath::Clamp(P.X, ROOF.X0, ROOF.X1), Z = FMath::Clamp(P.Z, ROOF.Z0, ROOF.Z1), M = 0.12;
	const double Ux = FMath::Clamp(X, UNIT.X0 + M, UNIT.X1 - M), Uz = FMath::Clamp(Z, UNIT.Z0 + M, UNIT.Z1 - M);
	return FMath::Sqrt(FMath::Square(X - Ux) + FMath::Square(Z - Uz)) < 0.5 + M ? FVector2D(Ux, Uz) : FVector2D(X, Z);
}

// ---------- the steps, a little ahead of when they are heard ----------

void FS47RoofSteps::Plan()
{
	FWalk& W = Walk;
	while (Phase == TEXT("come") && !W.bPlanned && W.Next <= T + AHEAD + 0.05)
	{
		// coming: a short stride and quick, a little longer every other step (two feet), each foot a
		// little to its side; one hesitation early on; then three slower ones that close the gap
		const FVector2D To = Target;
		const double Dd = FVector2D::Distance(W.Pos, To);
		const FVector2D Dir = Norm(To - W.Pos), Side(-Dir.Y, Dir.X);
		double Gap;
		FVector2D At;
		if (W.Settle == 0 && Dd > SETTLE && W.N < 40)
		{
			W.Pos += Dir * FMath::Min(STRIDE, Dd - SETTLE * 0.6);
			At = W.Pos + Side * (W.N % 2 ? 0.07 : -0.07);
			Gap = (W.N % 2 ? 0.205 : 0.155) + Rnd(-0.012, 0.012);
			if (W.N == 4 && !W.bHesitated) { W.bHesitated = true; Gap += 0.62; }
		}
		else
		{
			W.Settle++;
			const double K = W.Settle == 1 ? 0.45 : W.Settle == 2 ? 0.75 : 1.0;
			W.Pos += (To - W.Pos) * K;
			At = W.Settle == 3 ? To : W.Pos + Side * (W.Settle % 2 ? 0.05 : -0.05);
			Gap = W.Settle == 1 ? 0.27 : W.Settle == 2 ? 0.36 : 0.0;
		}
		const double Near = 1 - FMath::Min(1.0, FVector2D::Distance(W.Pos, To) / 6);
		Queue.Add({ W.Next, At.X, At.Y, Slot(), (0.92 + 0.08 * Near) * Rnd(0.85, 1.05) * (W.Settle == 3 ? 1.12 : 1), 1 });
		W.N++;
		if (W.Settle == 3) { W.bPlanned = true; Stopped(W.Next, To); break; }
		W.Next += Gap;
	}
}

int32 FS47RoofSteps::Slot()
{
	const int32 N = bBought ? SLOTS : UE_ARRAY_COUNT(KENNEY);
	int32 I = FMath::Min(N - 1, int32(FMath::FRand() * N));
	if (I == SlotLast) I = (I + 1 + FMath::Min(N - 2, int32(FMath::FRand() * (N - 1)))) % N;
	return (SlotLast = I);
}

void FS47RoofSteps::Stopped(double At, const FVector2D& Where)
{
	// right over the player: the beat, then once more further off, then the hum back
	Cue(At, [this] { Phase = TEXT("still"); });
	// the corner it goes off at: the south-west, unless that is right over the player
	const FString C = FVector2D::Distance(Where, CORNER(TEXT("sw"))) >= 3.2 ? TEXT("sw") : TEXT("se");
	Corner = C;
	if (D.Seen) D.Seen->Add(TEXT("roof:") + C);
	Cue(At + 0.02, [this, C] { if (D.Frost) D.Frost(C); });   // up there now; nobody outside to see it come
	// (where: the target itself, as it is when the dust falls)
	Cue(At + 0.05, [this] { if (D.Dust) D.Dust(Target.X, Target.Y); });
	Cue(At + 0.9, [this]
	{
		US47Audio* A = D.Audio.Get();
		if (A) A->HoldRoomTone(0, 0.09f);
		if (D.Tubes) D.Tubes(TEXT("steady"));
		if (A && A->Night.IsValid()) A->Night->Hush(BEAT + 4, 0);
	});
	Cue(At + 0.45, [this] { if (D.Toast) D.Toast(CAPTION_STOP, 4.5f); });
	// once more, further off: from where it stands now (it moved without a sound) to the lip
	const FVector2D Cc = CORNER(C), Away = Cc - Where;
	const double Len = Away.Size();
	const FVector2D Dir = Norm(Away);
	const FVector2D B0 = Where + Dir * FMath::Clamp(Len - 1.8, 1.6, Len - 0.6);
	const int32 N = FMath::Max(5, FMath::Min(8, int32(FMath::FloorToDouble(FVector2D::Distance(B0, Cc) / 0.27 + 0.5)) + 1));
	double T2 = At + BEAT;
	for (int32 i = 0; i < N; ++i)
	{
		const double K = N == 1 ? 1 : double(i) / (N - 1);
		FVector2D P = B0 + (Cc - B0) * K;
		const FVector2D Side(-Dir.Y, Dir.X);
		if (i < N - 1) P += Side * (i % 2 ? 0.06 : -0.06);
		Queue.Add({ T2, P.X, P.Y, Slot(), 0.62 * Rnd(0.85, 1.05) * (1 - 0.25 * K), 2 });
		T2 += FMath::Max(0.12, (i % 2 ? 0.175 : 0.15) - i * 0.006) + Rnd(-0.01, 0.01);
	}
	const double Gone = T2;
	Cue(At + BEAT - 0.01, [this] { Phase = TEXT("off"); });
	Cue(Gone + 0.6, [this] { if (D.Toast) D.Toast(CAPTION_2, 4); });
	HumBack = Gone + 2.5;
	Cue(Gone + 0.05, [this] { Phase = TEXT("after"); });
	Cue(HumBack, [this]
	{
		US47Audio* A = D.Audio.Get();
		if (A) A->HoldRoomTone(1, 0.35f);
		if (D.Tubes) D.Tubes(TEXT("stutter"));
		if (A) { FS47Sound O; O.Gain = 0.14f; O.Rate = 0.62f; O.At = D.TubeAt; A->Sfx(TEXT("click"), O); }
	});
	Cue(HumBack + 0.4, [this]
	{
		Phase = TEXT("done");
		if (D.Note) D.Note(FString::Printf(TEXT("%s. Records room. Steps on the roof, light and quick, closer together than anyone walks. They came across and stopped right over me, and stayed there. The tubes kept burning and the hum went out. Then a few more, further off, out to the edge of the roof, and nothing."), *S47Text::ClockText(ClockAt, false)));
	});
}

void FS47RoofSteps::Cue(double At, TFunction<void()> Fn)
{
	Cues.Add({ At, MoveTemp(Fn) });
	Cues.StableSort([](const FCue& A, const FCue& B) { return A.At < B.At; });
}

void FS47RoofSteps::FrostSeen()
{
	FS47TownSeen* S = D.Seen;
	if (!S || S->Contains(TEXT("roof:frost"))) return;
	S->Add(TEXT("roof:frost"));
	const FString Where = Corner == TEXT("se") ? TEXT("south-east") : TEXT("south-west");
	if (D.Note) D.Note(FString::Printf(TEXT("%s. Frost on the unit on the records room roof, white along the top, and a crust of it on the lip of the roof at the %s corner. Nothing else out here has any."), *S47Text::ClockText(D.Clock ? D.Clock() : 0, false), *Where));
}

// ---------- one step, through the slab ----------

void FS47RoofSteps::Play(const FStep& St, double When)
{
	US47Audio* A = D.Audio.Get();
	if (!A || !A->Ctx.IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch B(*Ctx);
	const double T0 = Ctx->CurrentTime() + When;
	const TSharedPtr<FS47AudioBuffer> Lic = A->Lic(TEXT("roof_steps"));
	const TSharedPtr<FS47AudioBuffer> Buf = Lic.IsValid() ? Lic : A->Buf.FindRef(KENNEY[St.Slot % UE_ARRAY_COUNT(KENNEY)]);
	if (!Buf.IsValid()) return;
	// on the roof unit's sheet-metal top (over the table) a step is higher up, and the metal rings
	const bool bOnUnit = St.X > UNIT.X0 && St.X < UNIT.X1 && St.Z > UNIT.Z0 && St.Z < UNIT.Z1;
	const double Y = bOnUnit ? UNIT.Y : ROOF.Y;
	auto Pan = Ctx->CreatePanner();
	Pan->PanningModel = TEXT("HRTF"); Pan->DistanceModel = ES47DistanceModel::Inverse; Pan->RefDistance = 1.2; Pan->RolloffFactor = 0.7; Pan->MaxDistance = 60;
	Pan->SetPosition(St.X, Y, St.Z);
	Pan->Connect(A->SfxBus);
	auto Src = Ctx->CreateBufferSource(); Src->Buffer = Buf;
	// small and quick: the bought hits a little up in pitch, Kenney's a good deal (they are a man's)
	Src->PlaybackRate.SetValue((Lic.IsValid() ? 1.12 : 1.42) * Rnd(0.95, 1.06));
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Q.SetValue(0.6);
	const double Far = FMath::Min(1.0, FMath::Sqrt(FMath::Square(St.X - Target.X) + FMath::Square(St.Z - Target.Y)) / 6);
	Lp->Frequency.SetValue(bMuffled ? 2400 - 900 * Far : 5200);
	auto Hp = Ctx->CreateBiquadFilter(); Hp->SetType(TEXT("highpass")); Hp->Frequency.SetValue(70);
	auto G = Ctx->CreateGain(); G->Gain.SetValue(St.Gain * (Lic.IsValid() ? LEVEL.Lic : LEVEL.Kenney) * (bMuffled ? 1 : 0.7));
	Src->Connect(Hp); Hp->Connect(Lp); Lp->Connect(G); G->Connect(Pan);
	if (Lic.IsValid()) Src->Start(T0, St.Slot * SLOT, SLOT_LEN); else Src->Start(T0);
	Track(Src);
	if (bOnUnit) Ring(Ctx, Pan, T0 + 0.004, St.Gain);
	// the slab taking it: a short low thump under each step, felt more than heard
	if (bMuffled)
	{
		auto O = Ctx->CreateOscillator(); O->SetType(TEXT("sine"));   // (on the unit too: its curb stands on the slab)
		O->Frequency.SetValueAtTime(Rnd(64, 78), T0); O->Frequency.ExponentialRampToValueAtTime(46, T0 + 0.09);
		auto Og = Ctx->CreateGain(); Og->Gain.SetValueAtTime(0, T0);
		Og->Gain.LinearRampToValueAtTime(LEVEL.Thump * St.Gain, T0 + 0.004); Og->Gain.ExponentialRampToValueAtTime(0.0008, T0 + 0.11);
		O->Connect(Og); Og->Connect(Pan); O->Start(T0); O->Stop(T0 + 0.13);
		Track(O);
	}
}

void FS47RoofSteps::Ring(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Out, double T0, double K)
{
	// a light foot on thin sheet metal: a dull, short ring (three damped partials, the panel's own),
	// through the same panner
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(bMuffled ? 1100 : 3000);
	Lp->Connect(Out);
	const double Base = Rnd(176, 196);
	const double PARTS[3][3] = { { 1, 1, 0.16 }, { 1.73, 0.55, 0.11 }, { 2.91, 0.3, 0.07 } };
	for (const auto& P : PARTS)
	{
		const double M = P[0], Am = P[1], Dec = P[2];
		auto O = Ctx->CreateOscillator(); O->Frequency.SetValue(Base * M * Rnd(0.99, 1.01));
		auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T0);
		G->Gain.LinearRampToValueAtTime(LEVEL.Ring * Am * K, T0 + 0.003); G->Gain.ExponentialRampToValueAtTime(0.0005, T0 + Dec);
		O->Connect(G); G->Connect(Lp); O->Start(T0); O->Stop(T0 + Dec + 0.02);
		Track(O);
	}
}

void FS47RoofSteps::Track(const TSharedPtr<FS47ScheduledSourceNode>& N)
{
	Sources.Add(N);
	TWeakPtr<FS47ScheduledSourceNode> W = N;
	N->OnEnded = [this, W]
	{
		const TSharedPtr<FS47ScheduledSourceNode> P = W.Pin();
		if (!P.IsValid()) return;
		Sources.Remove(P);
		P->Disconnect();
	};
}

bool FS47RoofSteps::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s roof steps: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47TownSeen Seen;
	TArray<FString> Notes, Toasts;
	FString Frost, Tubes;
	FS47RoofDeps Dp;
	Dp.Seen = &Seen;
	Dp.Note = [&Notes](const FString& N) { Notes.Add(N); };
	Dp.Toast = [&Toasts](const FString& T, float) { Toasts.Add(T); };
	Dp.Frost = [&Frost](const FString& C) { Frost = C; };
	Dp.Tubes = [&Tubes](const FString& M) { Tubes = M; };
	Dp.Clock = [] { return 3.0 * 3600; };
	FS47RoofSteps R(Dp);
	R.Reset();
	// the records room: settled in, a paper read, standing still
	FS47RoofIn S;
	S.bOk = true; S.bQuiet = true; S.bInRoom = true; S.bMuffled = true; S.Pos = FVector(-4.2, 0, 9.6); S.bRead = true; S.bChapter = true;
	for (int32 I = 0; I < 19; ++I) R.Update(1, S);
	Check(R.Phase == TEXT("idle"), TEXT("not before twenty seconds settled in"));
	for (int32 I = 0; I < 2; ++I) R.Update(1, S);
	Check(R.Phase == TEXT("come") && Seen.Contains(TEXT("roof:steps")), TEXT("they come at the first quiet moment"));
	// right over the player: up on the roof unit, the player at the table under it
	Check(R.Overhead().Equals(FVector(-4.2, 3.22, 9.03), 1e-9), TEXT("they make for the roof unit over the table"));
	for (int32 I = 0; I < 40 * 20; ++I) R.Update(0.05, S);
	Check(R.Phase == TEXT("done") && R.Corner == TEXT("se") && Seen.Contains(TEXT("roof:se")) && Frost == TEXT("se") && Tubes == TEXT("stutter"), TEXT("over: the frost at the south-east lip, the tube stutters"));
	Check(Notes.Num() == 1 && Notes[0].StartsWith(TEXT("03:00. Records room. Steps on the roof")), TEXT("the note in the player's words"));
	Check(Toasts.Num() == 3 && Toasts[0] == CAPTION_COME && Toasts[1] == CAPTION_STOP && Toasts[2] == CAPTION_2, TEXT("three captions, in time with them"));
	int32 First = 0, Second = 0;
	for (const FLog& L : R.Log) (L.Burst == 1 ? First : Second)++;
	Check(First >= 8 && Second >= 5 && Second <= 8, TEXT("the steps across, then five to eight further off"));
	R.Reset();
	Check(R.Phase == TEXT("done") && R.Corner == TEXT("se"), TEXT("a night put back: done, the frost where it was"));
	return bOk;
}
