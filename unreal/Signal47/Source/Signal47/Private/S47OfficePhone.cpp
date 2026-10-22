// story/officePhone.ts
#include "S47OfficePhone.h"

#include "HAL/PlatformTime.h"
#include "Internationalization/Regex.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47PhoneScripts.h"
#include "S47Web.h"
#include "S47WebAudio.h"

namespace
{
	/** Seconds after coming to the office before it rings; and, when something else of the night's was
	 *  going as the player came (or the case was just opened there), seconds of nothing going first. */
	constexpr double DELAY = 1.4, AFTER = 8;
	/** Seconds it rings for if nobody picks it up (five turns of the pattern: from the far side of the
	 *  lot to the telephone is some fifteen seconds' walk). */
	constexpr double RING_MAX = 36;
	/** Within this far of the telephone the player is in the office with it (the toast says which). */
	constexpr double BY_IT = 6;
	/** How far from the telephone the cord lets the handset go (metres, on the floor). */
	constexpr double CORD = 2.8;
	/** Seconds nothing else of the night's may start after it (one at a time). */
	constexpr double TAIL = 10;
	/** The gap between the two shutters: as it was up to REAL seconds; a longer one drawn in towards CAP. */
	constexpr double REAL = 8, CAP = 14, MIN_GAP = 0.7, DRAW = 90;
	/** Nora's first two lines on the hut's telephone, and how long they last without her recording. */
	const FString& NORA_FIRST() { return S47PhoneScripts::FieldNora()[0].Text; }
	const FString& NORA_NEXT() { return S47PhoneScripts::FieldNora()[1].Text; }
	constexpr double VOICE_SECS = 1.4, VOICE2_SECS = 4.4;
	/** The office's bell: one turn of the pattern, seconds, and the bursts in it (the handle turned). */
	constexpr double BELL_LOOP = 7.2;
	constexpr double BURSTS[3][2] = { { 0, 1.4 }, { 1.95, 2.6 }, { 3.1, 4.4 } };
	constexpr double STEP = 0.56;

	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }

	// ---------- the board ----------
	const TMap<FString, FS47BookNote>& NOTES()
	{
		static const TMap<FString, FS47BookNote> N = {
			{ TEXT("n.ol.frames"), { TEXT("The two shutters on the office line are my two frames at STATION 01, as far apart as I took them.") } },
			{ TEXT("n.ol.rx"), { TEXT("The carrier under the steps is the receiver I left on in the hut.") } },
			{ TEXT("n.ol.room6"), { TEXT("Room 6 is on the same line. Asked about the key, it gives back his correction from 1947. Nobody asked it about me, and it gave me back.") } },
		};
		return N;
	}
	TArray<FS47BookRule> RULES()
	{
		auto Rule = [](const TCHAR* A, const TCHAR* B, const TCHAR* Note) { FS47BookRule R; R.A = A; R.B = B; R.Note = Note; R.Text = NOTES()[Note].Text; return R; };
		return {
			Rule(TEXT("ol.shutters"), TEXT("frame03"), TEXT("n.ol.frames")),
			Rule(TEXT("ol.shutters"), TEXT("frame04"), TEXT("n.ol.frames")),
			Rule(TEXT("ol.boards"), TEXT("hut.rx"), TEXT("n.ol.rx")),
			Rule(TEXT("ol.answer"), TEXT("r6.line"), TEXT("n.ol.room6")),
		};
	}
	const TCHAR* LINK = TEXT("ol:");   // a thread on the board, kept in the town's part of the case

	/** The bell, made once (the module's bellBuf). */
	TSharedPtr<FS47AudioBuffer> BellBuf;
}

// ---------- the line ----------

double S47OfficeLine::ShutterGap(double Dt)
{
	const double Dd = FMath::Abs(Dt);
	if (!FMath::IsFinite(Dd)) return REAL;
	if (Dd <= REAL) return FMath::Max(MIN_GAP, Dd);
	return REAL + (CAP - REAL) * (1 - FMath::Exp(-(Dd - REAL) / DRAW));
}

S47OfficeLine::FPlan S47OfficeLine::PlanLine(TOptional<double> F3, TOptional<double> F4, double Voice, double Voice2)
{
	FPlan P;
	auto Cue = [&P](double At, const TCHAR* Kind, TOptional<double> Gain = {}, TOptional<double> Dur = {}) { P.Cues.Add({ At, Kind, Gain, Dur }); };
	auto Cap = [&P](double At, const FString& Text, double Secs) { P.Caps.Add({ At, Text, Secs }); };
	Cue(0, TEXT("open"));
	Cap(0.5, TEXT("Nobody on the line. A carrier, very weak, under the hiss."), 3.6);
	// in the hut: across the boards from the telephone to the door, and out
	const double First = 2.2;
	for (int32 i = 0; i < 6; ++i) Cue(First + i * STEP, TEXT("board"), 0.72 - i * 0.06);
	Cue(First + 1.9, TEXT("creak"));
	Cap(First + 0.25, TEXT("Boards creak out there. Someone is walking on a wooden floor."), 3.4);
	const double Door = First + 6 * STEP + 0.2;
	Cue(Door, TEXT("door"));
	Cue(Door + 0.55, TEXT("out"), 0.36); Cue(Door + 1.15, TEXT("out"), 0.26);
	Cap(Door + 0.1, TEXT("A door moves on its hinges. The steps go out."), 3.2);
	double T = Door + 2.3;
	if (F3 && F4 && FMath::IsFinite(*F3) && FMath::IsFinite(*F4))
	{
		P.Delta = FMath::Abs(*F4 - *F3);
		P.Gap = ShutterGap(*P.Delta);
		P.Shutters = { T, T + *P.Gap };
		Cue(T, TEXT("shutter")); Cap(T, TEXT("Outside, not far off: a shutter."), 2.8);
		Cue(T + *P.Gap, TEXT("shutter")); Cap(T + *P.Gap, TEXT("Another."), 2.4);
		T += *P.Gap + 1.8;
	}
	else T += 2.6;   // nothing out there but the carrier, a while
	// the telephone in the hut rings; under the second ring steps cross the boards right up to it,
	// and it is lifted (the bell is cut short)
	const double Ring = T;
	Cue(Ring, TEXT("ring"), {}, 1.75);
	Cap(Ring, TEXT("A bell, right at the mouthpiece: the telephone in the hut is ringing."), 3.6);
	const double Ring2 = Ring + 3.0, InAt = Ring + 2.4;
	for (int32 i = 0; i < 4; ++i) Cue(InAt + i * STEP, TEXT("board"), 0.5 + i * 0.09);
	Cap(Ring2, TEXT("It rings again. Steps cross the boards, right up to it."), 3.4);
	const double Lift = InAt + 4 * STEP + 0.35;
	// (seconds of the recording: it is played at 0.82, as the hut's bell was)
	Cue(Ring2, TEXT("ring"), {}, FMath::Min(1.75, (Lift - Ring2) * 0.82));
	Cue(Lift, TEXT("lift")); Cap(Lift, TEXT("Someone picks it up."), 2.4);
	const double V = Lift + 0.9;
	Cue(V, TEXT("voice"));
	Cap(V, FString::Printf(TEXT("Very faint, from the earpiece out there, a woman: \"%s\""), *NORA_FIRST()), FMath::Max(3.0, Voice + 1.6));
	// her next line, fainter still: about this telephone, heard on it, in the motel office
	const double V2 = V + Voice + 0.7;
	Cue(V2, TEXT("voice2"));
	Cap(V2, FString::Printf(TEXT("She goes on: \"%s\""), *NORA_NEXT()), FMath::Max(3.6, Voice2 + 1.2));
	const double Listen = V2 + Voice2 + 0.8;
	Cap(Listen, TEXT("Nobody answers her. Someone is listening."), 3.2);
	const double Down = Listen + 2.8;
	Cue(Down, TEXT("down")); Cap(Down, TEXT("The handset goes down out there. Only the carrier."), 3.2);
	P.End = Down + 2.4;
	Cue(P.End, TEXT("end"));
	P.Cues.StableSort([](const FCue& A, const FCue& B) { return A.At < B.At; });
	return P;
}

TSharedRef<FS47AudioBuffer> S47OfficeLine::CrankBell(const TSharedRef<FS47AudioContext>& Ctx)
{
	const int32 Sr = 16000, Len = int32(JsRound(Sr * BELL_LOOP));   // (the top partial is near 6.1 kHz)
	TSharedRef<FS47AudioBuffer> Buf = Ctx->CreateBuffer(1, Len, float(Sr));
	float* Out = Buf->GetChannelData(0);
	for (int32 i = 0; i < Len; ++i) Out[i] = 0.f;
	// [ratio, level, seconds to fall to 1/e]: a small hemispherical gong
	const double PARTS[5][3] = { { 1, 1, 0.5 }, { 2.04, 0.5, 0.32 }, { 2.83, 0.38, 0.24 }, { 4.16, 0.22, 0.15 }, { 5.43, 0.13, 0.1 } };
	struct FPart { double C1, C2, A, Y1, Y2; };
	TArray<FPart> Gongs[2];
	const double F0[2] = { 1047, 1123 };
	for (int32 g = 0; g < 2; ++g)
	{
		for (const auto& Pt : PARTS)
		{
			const double W = 2 * UE_DOUBLE_PI * F0[g] * Pt[0] / Sr, R = FMath::Exp(-1 / (Pt[2] * Sr));
			Gongs[g].Add({ 2 * R * FMath::Cos(W), -R * R, Pt[1] * FMath::Sin(W), 0, 0 });
		}
	}
	// where the clapper strikes, gong by gong (a little uneven: a hand on a crank)
	TArray<float> Hits;
	Hits.Init(0.f, Len * 2);
	int32 Side = 0;
	int64 Seed = 7;
	auto Rnd = [&Seed] { Seed = (Seed * 16807) % 2147483647; return double(Seed) / 2147483647; };
	for (const auto& Bu : BURSTS)
	{
		const double A = Bu[0], B = Bu[1];
		for (double T = A; T < B;)
		{
			const double K = (T - A) / (B - A), F = 14 + 6 * FMath::Sin(UE_DOUBLE_PI * FMath::Min(1.0, K * 1.15));
			const int32 I = int32(JsRound(T * Sr));
			if (I < Len) Hits[I * 2 + Side] = float(0.75 + 0.25 * Rnd());
			Side ^= 1;
			T += 1 / (2 * F);
		}
	}
	double Peak = 0;
	for (int32 i = 0; i < Len; ++i)
	{
		double Y = 0;
		for (int32 g = 0; g < 2; ++g)
		{
			const double X = Hits[i * 2 + g];
			for (FPart& P : Gongs[g])
			{
				const double V = P.C1 * P.Y1 + P.C2 * P.Y2 + X * P.A;
				P.Y2 = P.Y1; P.Y1 = V; Y += V;
			}
			// the clapper's own tick on the metal
			if (X != 0) for (int32 j = 0; j < 24 && i + j < Len; ++j) Out[i + j] += float((Rnd() * 2 - 1) * 0.35 * X * (1 - j / 24.0));
		}
		Out[i] += float(Y);
		Peak = FMath::Max(Peak, double(FMath::Abs(Out[i])));
	}
	if (Peak > 0) for (int32 i = 0; i < Len; ++i) Out[i] = float(Out[i] * (0.85 / Peak));
	return Buf;
}

// ---------- the telephone ----------

FS47OfficePhone::FS47OfficePhone(FS47OfficePhoneDeps InD) : D(MoveTemp(InD))
{
	FS47BookSpec O;
	O.Key = TEXT("ch4");
	O.Owns = [](const FString& N) { return N.StartsWith(TEXT("ol.")); };
	O.Notes = NOTES(); O.Rules = RULES();
	O.Links = [this]() -> TArray<FString>& { return Links; };
	O.Wrong = [](const FString&, const FS47BookRule&) { return TOptional<FS47Result>(); };
	O.Save = [this] { KeepLinks(); if (D.Save) D.Save(); };
	O.Cards = [this] { return Cards(); };
	O.Empty = TEXT("Those two do not say anything about each other.");
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

double FS47OfficePhone::LineTime() const
{
	// the line's clock: the sound's own while there is sound (a pause suspends it and holds the call),
	// else the page's. Chosen when the call is picked up, and kept for it.
	return Ctx.IsValid() ? Ctx->CurrentTime() : FPlatformTime::Seconds();
}

FVector FS47OfficePhone::PhoneAt() const
{
	AS47Web* W = D.Web.Get();
	const FS47Obj P = W ? W->Obj(TEXT("world.motel.proxies.officePhone")) : FS47Obj();
	return P ? P.WorldPosition() : FVector(0, -999, 0);
}

void FS47OfficePhone::Handset(TOptional<bool> Visible, TOptional<double> RotZ) const
{
	AS47Web* W = D.Web.Get();
	FS47Obj H = W ? W->Obj(TEXT("world.motel.objs.officePhoneHandset")) : FS47Obj();
	if (!H) return;
	if (Visible) H.SetVisible(*Visible);
	if (RotZ) H.SetRotZ(*RotZ);
}

// ---------- the interaction (story/Chapter4.ts asks first) ----------

FString FS47OfficePhone::Label() const
{
	if (State == TEXT("ringing")) return TEXT("Answer the field telephone");
	if (State == TEXT("line")) return TEXT("Hang up");
	return FString();
}

bool FS47OfficePhone::Use()
{
	if (State == TEXT("ringing")) { Answer(); return true; }
	if (State == TEXT("line")) { HangUp(TEXT("You put the handset back on its hook.")); return true; }
	if (Heard()) { ListenAgain(); return true; }
	return false;
}

// ---------- per frame ----------

void FS47OfficePhone::Update(double Dt, const FS47OfficePhoneIn& S)
{
	T += Dt;
	Pos = S.Pos;
	if (Tail > 0) Tail = FMath::Max(0.0, Tail - Dt);
	const bool bEntered = S.bNear && !bWas;
	bWas = S.bNear;
	const bool bWasOpened = bOpened; bOpened = false;
	if (State == TEXT("idle"))
	{
		if (!S.bOpen || Heard()) { bArmed = false; return; }
		// the bell is made while the player is still on the way (a moment's work, not at the ring)
		US47Audio* A = D.Audio.Get();
		if (S.bHere && !BellBuf.IsValid() && A && A->Ctx.IsValid()) BellBuf = S47OfficeLine::CrankBell(A->Ctx.ToSharedRef());
		if (bEntered) { bArmed = true; bWaited = bWasOpened; Calm = 0; }
		if (!S.bNear) { bArmed = false; return; }
		if (!bArmed) return;
		// never on top of something else of the night's: it waits while that goes, and a while after
		if (!S.bOk) { Calm = 0; bWaited = true; return; }
		Calm += Dt;
		if (Calm >= (bWaited ? AFTER : DELAY)) { bArmed = false; Ring(); }
	}
	else if (State == TEXT("ringing"))
	{
		RingT += Dt;
		const double Ph = FMath::Fmod(RingT, BELL_LOOP);
		bool bOn = false;
		for (const auto& Bu : BURSTS) bOn |= Ph >= Bu[0] && Ph < Bu[1] + 0.15;
		Handset({}, bOn ? FMath::Sin(T * 70) * 0.05 : 0.0);
		if (RingT > RING_MAX || !S.bOpen || !S.bHere) Unanswered(S.bHere);
	}
	else if (State == TEXT("line"))
	{
		const FVector P = PhoneAt();
		if (FMath::Sqrt(FMath::Square(S.Pos.X - P.X) + FMath::Square(S.Pos.Z - P.Z)) > CORD) { HangUp(TEXT("The cord will not reach. The handset goes back on its hook.")); return; }
		LineStep(S.Clock);
	}
}

// ---------- the bell ----------

void FS47OfficePhone::Ring()
{
	US47Audio* A = D.Audio.Get();
	State = TEXT("ringing"); RingT = 0; Result.Empty(); Rings++;
	if (A && A->Ctx.IsValid())
	{
		const TSharedRef<FS47AudioContext> C = A->Ctx.ToSharedRef();
		if (!BellBuf.IsValid()) BellBuf = S47OfficeLine::CrankBell(C);
		// (from the two gongs over the mouthpiece; a magneto bell is loud, made to be heard out of
		// doors: across the lot through the office's open door)
		FVector At = PhoneAt(); At.Y += 0.2;
		const TSharedPtr<FS47AudioNode> Dest = A->Spatial(At, 2.5f);
		if (Dest.IsValid())
		{
			FS47AudioContext::FBatch B(*C);
			auto Src = C->CreateBufferSource(); Src->Buffer = BellBuf; Src->bLoop = true;
			auto Gain = C->CreateGain(); Gain->Gain.SetValue(0.45); Src->Connect(Gain); Gain->Connect(Dest);
			Src->Start();
			Bell = FBell{ Src, Gain };
		}
	}
	const FVector P = PhoneAt();
	const bool bBy = !Pos || FMath::Sqrt(FMath::Square(Pos->X - P.X) + FMath::Square(Pos->Z - P.Z)) <= BY_IT;
	if (D.Toast) D.Toast(bBy ? TEXT("The crank telephone in the office is ringing. At the other end of the line, someone is turning the handle.")
		: TEXT("In the motel office, a telephone is ringing: a bell in bursts, the way a crank telephone rings when someone at the other end turns the handle."), 4.6f);
}

void FS47OfficePhone::StopBell()
{
	TOptional<FBell> B = Bell;
	Bell.Reset();
	Handset({}, 0.0);
	US47Audio* A = D.Audio.Get();
	if (!B || !A || !A->Ctx.IsValid()) return;
	FS47AudioContext::FBatch L(*A->Ctx);
	B->Gain->Gain.SetTargetAtTime(0, A->Ctx->CurrentTime(), 0.03);
	B->Src->Stop(A->Ctx->CurrentTime() + 0.25);
}

void FS47OfficePhone::Unanswered(bool bHere)
{
	StopBell();
	State = TEXT("idle"); Result = TEXT("unanswered"); Tail = 4;
	if (bHere && D.Toast) D.Toast(TEXT("The ringing stops."), 2.4f);
}

// ---------- the line ----------

void FS47OfficePhone::Answer()
{
	US47Audio* A = D.Audio.Get();
	StopBell();
	const FVector At = PhoneAt();
	Handset(false, {});
	if (A)
	{
		FS47Sound C; C.At = At; C.Gain = 0.55f; A->Sfx(TEXT("click"), C);
		FS47Sound Th; Th.At = At; Th.Gain = 0.16f; Th.Rate = 1.7f; Th.When = 0.05f; A->Sfx(TEXT("thudSoft"), Th);
	}
	const TPair<TOptional<double>, TOptional<double>> Fr = D.Frames ? D.Frames() : TPair<TOptional<double>, TOptional<double>>();
	// her recordings, both or neither (never half of what she said out there spoken)
	TSharedPtr<FS47AudioBuffer> Voice = A ? A->LineBuffer(TEXT("N. VEGA"), NORA_FIRST()) : nullptr, Voice2 = A ? A->LineBuffer(TEXT("N. VEGA"), NORA_NEXT()) : nullptr;
	if (!Voice.IsValid() || !Voice2.IsValid()) { Voice.Reset(); Voice2.Reset(); }
	const S47OfficeLine::FPlan P = S47OfficeLine::PlanLine(Fr.Key, Fr.Value, Voice.IsValid() ? Voice->Duration() : VOICE_SECS, Voice2.IsValid() ? Voice2->Duration() : VOICE2_SECS);
	Plan = P; Shown.Reset(); CapAt = 0; bVoiced = false;
	State = TEXT("line");
	Schedule(P, Voice, Voice2);
	StartedAt = LineTime();
}

void FS47OfficePhone::Schedule(const S47OfficeLine::FPlan& P, const TSharedPtr<FS47AudioBuffer>& Voice, const TSharedPtr<FS47AudioBuffer>& Voice2)
{
	// every sound of the call put on the line's clock at once: a pause holds it all (the context is
	// suspended), and a frame that comes late cannot pull the steps apart
	US47Audio* A = D.Audio.Get();
	Ctx = A && A->Ctx.IsValid() && A->Ctx->State() == TEXT("running") && A->NoiseBuffer().IsValid() ? A->Ctx : nullptr;
	if (!Ctx.IsValid()) return;
	const TSharedRef<FS47AudioContext> C = Ctx.ToSharedRef();
	FS47AudioContext::FBatch Lk(*C);
	const FS47PhoneLine Line = A->PhoneLine();
	auto NewBus = C->CreateGain(); NewBus->Gain.SetValue(1); NewBus->Connect(Line.Input);
	// what is a few metres from the hut's telephone, and out of its door: duller
	auto Far = C->CreateBiquadFilter(); Far->SetType(TEXT("lowpass")); Far->Frequency.SetValue(2000); Far->Connect(NewBus);
	auto Outside = C->CreateGain(); Outside->Gain.SetValue(1.1);
	auto Ol = C->CreateBiquadFilter(); Ol->SetType(TEXT("lowpass")); Ol->Frequency.SetValue(3000); Outside->Connect(Ol); Ol->Connect(NewBus);
	Bus = NewBus;
	Nodes = { NewBus, Far, Outside, Ol };
	Nodes.Append(Line.Nodes);
	const double T0 = C->CurrentTime();
	// the line itself: its hiss, and the bought hush of an open line (the payphone has it too)
	auto Hiss = C->CreateBufferSource(); Hiss->Buffer = A->NoiseBuffer(); Hiss->bLoop = true;
	auto Hg = C->CreateGain(); Hg->Gain.SetValue(0.015); Hiss->Connect(Hg); Hg->Connect(NewBus); Hiss->Start(T0);
	Loops.Add(Hiss); Nodes.Add(Hg);
	if (const TSharedPtr<FS47AudioBuffer> Hush = A->Lic(TEXT("line_hush")))
	{
		auto H = C->CreateBufferSource(); H->Buffer = Hush; H->bLoop = true;
		auto G = C->CreateGain(); G->Gain.SetValue(0.09); H->Connect(G); G->Connect(NewBus); H->Start(T0, FMath::FRand() * Hush->Duration());
		Loops.Add(H); Nodes.Add(G);
	}
	// the receiver left on in the hut, across the room: its carrier, coming and going (Audio.radio)
	auto Car = C->CreateGain(); Car->Gain.SetValue(0);
	Car->Gain.SetValueAtTime(0, T0); Car->Gain.LinearRampToValueAtTime(0.03, T0 + 1.4);
	auto Bp = C->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1100); Bp->Q.SetValue(0.6);
	Car->Connect(Bp); Bp->Connect(NewBus);
	auto Osc = C->CreateOscillator(); Osc->Frequency.SetValue(760);
	auto Og = C->CreateGain(); Og->Gain.SetValue(0.6); Osc->Connect(Og); Og->Connect(Car);
	auto Lfo = C->CreateOscillator(); Lfo->Frequency.SetValue(0.21);
	auto Lg = C->CreateGain(); Lg->Gain.SetValue(0.35); Lfo->Connect(Lg); Lg->Connect(Og->Gain);
	Osc->Start(T0); Lfo->Start(T0);
	Loops.Add(Osc); Loops.Add(Lfo); Nodes.Add(Car); Nodes.Add(Bp); Nodes.Add(Og); Nodes.Add(Lg);
	if (const TOptional<FS47StaticSrc> St = A->StaticSrc())
	{
		auto Sg = C->CreateGain(); Sg->Gain.SetValue(0.25); St->Out->Connect(Sg); Sg->Connect(Car); Loops.Add(St->Src); Nodes.Add(Sg);
	}
	auto Pick = [](const TCHAR* Kind) { return FString::Printf(TEXT("%s%d"), Kind, FMath::Min(4, int32(FMath::FRand() * 5))); };
	for (const S47OfficeLine::FCue& Q : P.Cues)
	{
		const float When = float(Q.At);
		if (Q.Kind == TEXT("open")) { FS47Sound S; S.Gain = 0.5f; S.When = When; S.DestNode = NewBus; A->Sfx(TEXT("click"), S); }
		else if (Q.Kind == TEXT("board")) { FS47Sound S; S.Gain = float(Q.Gain.Get(1)); S.Rate = 0.95f + FMath::FRand() * 0.1f; S.Lp = 3000.f; S.When = When; S.DestNode = NewBus; A->Sfx(Pick(TEXT("stepWood")), S); }
		else if (Q.Kind == TEXT("creak")) { FS47Sound S; S.Gain = 0.16f; S.Rate = 2.3f; S.When = When; S.DestNode = Far; A->Sfx(TEXT("doorCreak0"), S); }
		else if (Q.Kind == TEXT("door")) { FS47Sound S; S.Gain = 0.35f; S.Rate = 0.9f; S.When = When; S.DestNode = Far; A->Sfx(TEXT("doorCreak1"), S); }
		else if (Q.Kind == TEXT("out")) { FS47Sound S; S.Gain = float(Q.Gain.Get(1)); S.Rate = 0.88f; S.Lp = 2400.f; S.When = When; S.DestNode = Far; A->Sfx(Pick(TEXT("stepGravel")), S); }
		else if (Q.Kind == TEXT("shutter")) A->Shutter(TSharedPtr<FS47AudioNode>(Outside), When);
		else if (Q.Kind == TEXT("ring")) { FS47Sound S; S.Gain = 0.5f; S.Rate = 0.82f; S.From = 0.f; if (Q.Dur) S.Dur = float(*Q.Dur); S.When = When; S.DestNode = NewBus; A->Sfx(TEXT("phoneRing"), S); }
		else if (Q.Kind == TEXT("lift"))
		{
			FS47Sound S; S.Gain = 0.6f; S.When = When; S.DestNode = NewBus; A->Sfx(TEXT("click"), S);
			FS47Sound Th; Th.Gain = 0.3f; Th.Rate = 1.9f; Th.When = When + 0.05f; Th.DestNode = NewBus; A->Sfx(TEXT("thudSoft"), Th);
		}
		else if (Q.Kind == TEXT("voice") || Q.Kind == TEXT("voice2"))
		{
			const TSharedPtr<FS47AudioBuffer>& Buf = Q.Kind == TEXT("voice") ? Voice : Voice2;
			if (Buf.IsValid())
			{
				// her voice in the earpiece out there, as the mouthpiece beside it heard it: small and thin
				auto S = C->CreateBufferSource(); S->Buffer = Buf;
				auto Vb = C->CreateBiquadFilter(); Vb->SetType(TEXT("bandpass")); Vb->Frequency.SetValue(1400); Vb->Q.SetValue(0.9);
				auto Vg = C->CreateGain(); Vg->Gain.SetValue(Q.Kind == TEXT("voice") ? 0.36 : 0.3);
				S->Connect(Vb); Vb->Connect(Vg); Vg->Connect(NewBus); S->Start(T0 + When);
				Loops.Add(S); Nodes.Add(Vb); Nodes.Add(Vg);
				bVoiced = true;
			}
		}
		else if (Q.Kind == TEXT("down"))
		{
			FS47Sound S; S.Gain = 0.42f; S.Rate = A->Lic(TEXT("handset_down")).IsValid() ? 1.f : 1.5f; S.When = When; S.DestNode = NewBus; A->Sfx(A->Or(TEXT("handset_down"), TEXT("thudSoft")), S);
			FS47Sound Cl; Cl.Gain = 0.55f; Cl.When = When + 0.07f; Cl.DestNode = NewBus; A->Sfx(TEXT("click"), Cl);
		}
	}
}

void FS47OfficePhone::LineStep(double Clock)
{
	if (!Plan) return;
	const double E = LineTime() - StartedAt;
	while (CapAt < Plan->Caps.Num() && Plan->Caps[CapAt].At <= E)
	{
		const S47OfficeLine::FCap& C = Plan->Caps[CapAt++];
		Shown.Add(C.Text);
		// (toast(text, secs, () => this.lineTime()): the HUD's toasts keep the game's own time)
		if (D.Toast) D.Toast(C.Text, float(C.Secs));
	}
	if (E >= Plan->End) Finish(Clock);
}

void FS47OfficePhone::StopLine(double Fade)
{
	const TSharedPtr<FS47AudioContext> C = Ctx;
	const TSharedPtr<FS47GainNode> B = Bus;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> L = MoveTemp(Loops);
	TArray<TSharedPtr<FS47AudioNode>> N = MoveTemp(Nodes);
	Bus.Reset(); Loops.Reset(); Nodes.Reset();
	if (!C.IsValid()) return;
	FS47AudioContext::FBatch Lk(*C);
	const double T1 = C->CurrentTime();
	if (B.IsValid()) B->Gain.SetTargetAtTime(0, T1, Fade);
	for (const TSharedPtr<FS47ScheduledSourceNode>& S : L) S->Stop(T1 + Fade * 8);
	C->SetTimeout(FMath::CeilToDouble(Fade * 8000) / 1000 + 0.12, [N] { for (const TSharedPtr<FS47AudioNode>& X : N) X->Disconnect(); });
}

void FS47OfficePhone::PutBack()
{
	Handset(true, 0.0);
	if (US47Audio* A = D.Audio.Get()) { FS47Sound S; S.At = PhoneAt(); S.Gain = 0.5f; A->Sfx(TEXT("click"), S); }
}

void FS47OfficePhone::HangUp(const FString& Text)
{
	// put down before the end (Use, or walked off the end of the cord): as if it had not been heard;
	// it rings again the next time the player comes to the office before room 6 is over
	if (State != TEXT("line")) return;
	StopLine();
	PutBack();
	State = TEXT("idle"); Result = TEXT("away"); Tail = TAIL;
	if (!Text.IsEmpty() && D.Toast) D.Toast(Text, 3);
}

void FS47OfficePhone::Finish(double Clock)
{
	const S47OfficeLine::FPlan P = *Plan;
	StopLine(0.3);
	PutBack();
	State = TEXT("idle"); Result = TEXT("heard"); Tail = TAIL;
	if (D.Seen) { D.Seen->Add(TEXT("ev:office")); D.Seen->Add(FString::Printf(TEXT("office@%d"), int32(JsRound(Clock)))); }
	if (D.Toast) D.Toast(TEXT("You put the handset back on its hook."), 3.2f);
	if (D.Note) D.Note(NoteText(Clock, P));
	if (D.Refresh) D.Refresh();
	if (D.Save) D.Save();
}

FString FS47OfficePhone::AnsweredAt() const
{
	// when the player answered the hut's telephone: the clock on chapter three's note of it
	const TArray<FString> Ns = D.Notes ? D.Notes() : TArray<FString>();
	for (const FString& N : Ns)
	{
		FRegexMatcher M(FRegexPattern(TEXT("^(\\d\\d:\\d\\d)\\. N\\. Vega called the field telephone")), N);
		if (M.FindNext()) return M.GetCaptureGroup(1);
	}
	return FString();
}

FString FS47OfficePhone::NoteText(double Clock, const S47OfficeLine::FPlan& P) const
{
	FString N = FString::Printf(TEXT("%s. The crank telephone in the motel office, on the line to the hut at STATION 01. Nobody on it. Someone walking on the boards, the hut door."), *S47Text::ClockText(Clock, false));
	if (P.Gap)
	{
		N += P.Delta && *P.Delta <= REAL ? TEXT(" A shutter, then another, as far apart as FRAME 03 and FRAME 04.")
			: TEXT(" A shutter, a wait, then another: the wait between FRAME 03 and FRAME 04.");
		N += TEXT(" I took two frames out there.");
	}
	const FString At = AnsweredAt();
	N += FString::Printf(TEXT(" Then the telephone in the hut rang, and someone picked it up and listened to N. Vega. That was me%s."), At.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(", at %s"), *At));
	// (what Reyes knows by now: P09 has the hut's receiver giving back T. Vega's correction; that
	// the line itself does, P12, is room 6's to show)
	N += TEXT(" The receiver out there gives back T. Vega's correction from 1947. This line has kept me.");
	return N;
}

void FS47OfficePhone::ListenAgain()
{
	// heard: lifted again, it has the carrier on it and nothing else
	US47Audio* A = D.Audio.Get();
	const FVector At = PhoneAt();
	if (A)
	{
		FS47Sound C; C.At = At; C.Gain = 0.5f; A->Sfx(TEXT("click"), C);
		if (A->Ctx.IsValid() && A->NoiseBuffer().IsValid())
		{
			const TSharedRef<FS47AudioContext> Cx = A->Ctx.ToSharedRef();
			FS47AudioContext::FBatch Lk(*Cx);
			const FS47PhoneLine Line = A->PhoneLine();
			const double T1 = Cx->CurrentTime();
			auto Nz = Cx->CreateBufferSource(); Nz->Buffer = A->NoiseBuffer(); auto Ng = Cx->CreateGain(); Ng->Gain.SetValue(0.02);
			Nz->Connect(Ng); Ng->Connect(Line.Input); Nz->Start(T1); Nz->Stop(T1 + 2.6);
			auto O = Cx->CreateOscillator(); O->Frequency.SetValue(760); auto Og = Cx->CreateGain();
			Og->Gain.SetValueAtTime(0, T1); Og->Gain.LinearRampToValueAtTime(0.018, T1 + 0.6); Og->Gain.SetValueAtTime(0.018, T1 + 2.2); Og->Gain.LinearRampToValueAtTime(0, T1 + 2.6);
			O->Connect(Og); Og->Connect(Line.Input); O->Start(T1); O->Stop(T1 + 2.7);
			FS47Sound C2; C2.At = At; C2.Gain = 0.45f; C2.When = 2.6f; A->Sfx(TEXT("click"), C2);
			const TArray<TSharedPtr<FS47AudioNode>> LN = Line.Nodes;
			Cx->SetTimeout(3.2, [LN] { for (const TSharedPtr<FS47AudioNode>& X : LN) X->Disconnect(); });
		}
	}
	if (D.Toast) D.Toast(TEXT("The carrier, very weak, and nothing on it. You put the handset back."), 3.4f);
}

// ---------- the board ----------

FString FS47OfficePhone::When() const
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(TEXT("office@"))) return S47Text::ClockText(FCString::Atod(*K.Mid(7)), false);
	return FString();
}

TArray<FS47BoardCard> FS47OfficePhone::Cards() const
{
	if (!Heard()) return {};
	const TPair<TOptional<double>, TOptional<double>> Fr = D.Frames ? D.Frames() : TPair<TOptional<double>, TOptional<double>>();
	const bool bFrames = Fr.Key.IsSet() && Fr.Value.IsSet();
	const FString Tm = When();
	FS47BoardCard C;
	C.Id = TEXT("ol"); C.Stamp = TEXT("OFFICE"); C.Title = TEXT("THE MOTEL OFFICE, TONIGHT"); C.Kind = TEXT("thing"); C.bHave = true; C.X = 14; C.Y = 820; C.W = 300;
	C.Text = FString::Printf(TEXT("The crank telephone in the office%s, on the line to the hut at STATION 01. Nobody on it."), Tm.IsEmpty() ? TEXT("") : *(TEXT(", ") + Tm));
	C.Lines.Add({ TEXT("ol.boards"), TEXT("Steps on the boards in the hut, and the hut door. A carrier under them."), FString() });
	if (bFrames) C.Lines.Add({ TEXT("ol.shutters"), TEXT("Outside, a shutter, then another."), FString() });
	C.Lines.Add({ TEXT("ol.answer"), TEXT("The telephone in the hut rings. Someone picks it up, and listens."), FString() });
	return { C };
}

void FS47OfficePhone::KeepLinks()
{
	if (!D.Seen) return;
	for (const FString& K : D.Seen->Array()) if (K.StartsWith(LINK)) D.Seen->Remove(K);
	for (const FString& K : Links) D.Seen->Add(LINK + K);
}

// ---------- a night opened, the developer menu ----------

void FS47OfficePhone::Reset()
{
	StopBell(); StopLine();
	Handset(true, 0.0);
	State = TEXT("idle"); Result.Empty(); Plan.Reset(); bArmed = false; Calm = 0; bOpened = true; Tail = 0; bWas = false; Rings = 0;
	Links.Reset();
	if (D.Seen)
	{
		const TArray<FS47BookRule> Rules = RULES();
		for (const FString& K : D.Seen->Array())
		{
			if (!K.StartsWith(LINK)) continue;
			const FString P = K.Mid(FCString::Strlen(LINK));
			if (Rules.ContainsByPredicate([&P](const FS47BookRule& R) { return S47Board::Pair(R.A, R.B) == P; })) Links.Add(P);
		}
	}
}

bool FS47OfficePhone::RingNow()
{
	if (State != TEXT("idle")) return false;
	bArmed = false;
	Ring();
	return true;
}

bool FS47OfficePhone::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s office phone: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	using namespace S47OfficeLine;
	Check(FMath::IsNearlyEqual(ShutterGap(3), 3.0) && FMath::IsNearlyEqual(ShutterGap(0.2), 0.7) && ShutterGap(30) > 8 && ShutterGap(30) < 14 && ShutterGap(1e9) <= 14, TEXT("the gap between the shutters, drawn in"));
	const FPlan P = PlanLine(8000.0, 8005.0);
	Check(P.Shutters.Num() == 2 && FMath::IsNearlyEqual(P.Shutters[0], 2.2 + 6 * 0.56 + 0.2 + 2.3) && FMath::IsNearlyEqual(*P.Gap, 5.0), TEXT("the two shutters as far apart as the frames"));
	Check(P.Cues.Num() == 24 && P.Caps.Num() == 12 && P.Cues.Last().Kind == TEXT("end") && FMath::IsNearlyEqual(P.End, P.Cues.Last().At), TEXT("the line's cues and captions"));
	Check(P.Caps[8].Text == TEXT("Very faint, from the earpiece out there, a woman: \"So you went out there.\""), TEXT("Nora's first line, in its caption"));
	const FPlan Q = PlanLine({}, {});
	Check(!Q.Gap && Q.Shutters.Num() == 0 && Q.Cues.Num() == 22 && Q.Caps.Num() == 10, TEXT("no frames: the steps, the door, the call"));
	const TSharedRef<FS47AudioContext> Ctx = FS47AudioContext::Make();
	const TSharedRef<FS47AudioBuffer> B = CrankBell(Ctx);
	double Peak = 0;
	for (int32 i = 0; i < B->Length(); ++i) Peak = FMath::Max(Peak, double(FMath::Abs(B->GetChannelData(0)[i])));
	Check(B->Length() == 115200 && B->SampleRate() == 16000.f && FMath::IsNearlyEqual(Peak, 0.85, 1e-4), TEXT("the crank bell, 7.2 s at 16 kHz, its peak at 0.85"));
	// the telephone, without the world
	FS47TownSeen Seen;
	TArray<FString> Notes;
	FS47OfficePhoneDeps Dp;
	Dp.Seen = &Seen; Dp.Note = [&Notes](const FString& N) { Notes.Add(N); };
	FS47OfficePhone O(Dp);
	O.Reset();
	FS47OfficePhoneIn S;
	S.bOk = true; S.bOpen = true; S.bNear = true; S.bHere = true; S.Pos = FVector(0, -999, 0); S.Clock = 4 * 3600;
	O.Update(0.5, S);
	Check(O.State == TEXT("idle"), TEXT("not at once after the case was opened"));
	S.bNear = false; O.Update(0.5, S); S.bNear = true;
	for (int32 i = 0; i < 3; ++i) O.Update(0.5, S);
	Check(O.State == TEXT("ringing") && O.Label() == TEXT("Answer the field telephone") && O.Busy(), TEXT("it rings when the player comes to the office"));
	S.bHere = false; O.Update(0.1, S);
	Check(O.State == TEXT("idle") && O.Result == TEXT("unanswered") && O.Busy(), TEXT("walked away: the ringing stops, a tail after it"));
	return bOk;
}
