// story/archivePhone.ts
#include "S47ArchivePhone.h"

#include "S47Annex.h"
#include "S47ArchiveDishes.h"
#include "S47Audio.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47WebAudio.h"

namespace
{
	/** Seconds after the steps on the roof are over before the phone can ring (they are the build). */
	constexpr double BUILD = 12;
	/** Still reading: this long after the steps and settled in the room before it rings. */
	constexpr double BUILD_READING = 50;
	constexpr double QUIET = 2.5, RING_MAX = 40, TAIL = 30;
	constexpr double COUNT = 47;
	/** The tipping is over by then, whatever the dishes do (seconds from :47). */
	constexpr double MOMENT_MAX = 32;
	/** The call, from the pick-up (the lift adds LIFT before it). */
	struct FCallT { double Tube, Thump, Drawer, Roll, Fall, Hum, End; };
	constexpr FCallT CALL = { 0.35, 2.6, 3.1, 0.55, 4.05, 4.6, 6.3 };
	constexpr double LIFT = 1.7;
	/** The box thrown off the shelf: pushed, tipping over the edge, falling, on the floor. */
	struct FFallT { double Push, Tip, Land, Settle; };
	constexpr FFallT FALL = { 0.12, 0.42, 0.86, 1.0 };
	struct FShelf { double Edge, Y, Z, X0; };
	constexpr FShelf SHELF = { -5.555, 1.135, 8.6, -5.62 };
	/** Where it ends: on its side on the floor in front of the shelf, its label to the room (sideways). */
	struct FFloor { double X, Y, Z, Yaw; };
	constexpr FFloor FLOOR = { -5.33, 0.17, 8.5, 0.34 };
	FQuat Q_FLOOR() { return S47Three::Euler(UE_DOUBLE_PI / 2, FLOOR.Yaw, 0, TEXT("YXZ")); }
	FQuat Q_TIP() { return S47Three::Euler(0, 0, -0.95); }

	FString DAY(double C) { return TEXT("04/14 ") + S47Text::ClockText(C, false); }
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); } // Math.round

	// ---------- the board ----------
	const TMap<FString, FS47BookNote>& NOTES()
	{
		static const TMap<FString, FS47BookNote> N = {
			{ TEXT("n.r47.cut"), { TEXT("A cable through cutters, on a reel from 1979. The cable at STATION 01 was cut clean, on purpose.") } },
			{ TEXT("n.r47.gap"), { TEXT("The 1947 log: the carrier ceased at 02:17:00, and 47 seconds later the relay, and the reference moved. Reel 047 has the same 47 seconds, and the same relay.") } },
			{ TEXT("n.r47.her"), { TEXT("She broke the connection herself in 1947, with Tomás in the reference. Reel 047 has the cut on it, thirty-two years later.") } },
			{ TEXT("n.r47.crank"), { TEXT("The telephone in the hut is rung by its handle. On reel 047 somebody rings one.") } },
		};
		return N;
	}
	TArray<FS47BookRule> RULES()
	{
		auto Rule = [](const TCHAR* A, const TCHAR* B, const TCHAR* Note, const FString& Text) { FS47BookRule R; R.A = A; R.B = B; R.Note = Note; R.Text = Text; return R; };
		const auto& N = NOTES();
		return {
			Rule(TEXT("r47.cut"), TEXT("frame04"), TEXT("n.r47.cut"), N[TEXT("n.r47.cut")].Text),
			Rule(TEXT("r47.gap"), TEXT("e10.relay"), TEXT("n.r47.gap"), N[TEXT("n.r47.gap")].Text),
			Rule(TEXT("r47.gap"), TEXT("e10.carrier"), TEXT("n.r47.gap"), N[TEXT("n.r47.gap")].Text),
			Rule(TEXT("r47.cut"), TEXT("e12.cut"), TEXT("n.r47.her"), N[TEXT("n.r47.her")].Text),
			Rule(TEXT("r47.crank"), TEXT("ol.answer"), TEXT("n.r47.crank"), N[TEXT("n.r47.crank")].Text),
			// (thin: the player can do the sum or not)
			Rule(TEXT("r47.range"), TEXT("rx.dist"), TEXT(""), TEXT("-32 on reel 047, 10 August 1979. -39 on tonight's printout.")),
		};
	}
	const TCHAR* LINK = TEXT("a47:");

	/** /^<prefix>(-?\d+)$/ on a key: the number, if it is one */
	TOptional<double> KeyNumber(const FString& K, const TCHAR* Prefix)
	{
		if (!K.StartsWith(Prefix, ESearchCase::CaseSensitive)) return {};
		const FString Rest = K.Mid(FCString::Strlen(Prefix));
		int32 I = Rest.StartsWith(TEXT("-")) ? 1 : 0;
		if (I >= Rest.Len()) return {};
		for (int32 J = I; J < Rest.Len(); ++J) if (!FChar::IsDigit(Rest[J])) return {};
		return FCString::Atod(*Rest);
	}
	/** the night's order: the evening before midnight comes first */
	double Night(double V) { const double C = FMath::Fmod(FMath::Fmod(V, 86400.0) + 86400.0, 86400.0); return C >= 12 * 3600 ? C - 86400 : C; }
}

FS47ArchivePhone::FS47ArchivePhone(FS47ArcDeps InD) : D(MoveTemp(InD))
{
	LineEnd = CALL.End + 0.1;
	US47Annex* A = D.Annex.Get();
	if (US47Inter* I = D.Inter.Get(); I && A)
	{
		FS47Interactable Dr;
		Dr.Id = TEXT("arc:drawer"); Dr.Object = A->Arc.Hit.Drawer; Dr.Range = 2.6;
		Dr.Label = [this] { return !DrawerOut() ? FString() : Has(TEXT("arc:reel")) ? FString(TEXT("Fire drawer")) : FString(TEXT("Take reel 047")); };
		Dr.Use = [this] { UseDrawer(); };
		Dr.Examine = [this]
		{
			return !DrawerOut() ? FString() : Has(TEXT("arc:reel")) ? FString(TEXT("The fire drawer, out of the wall on its rusted runners. Empty now."))
				: FString(TEXT("A steel fire drawer let into the wall behind the shelf, out on its runners. Rust on the runners. In the tray, a reel of tape in a grey box: 047."));
		};
		I->Add(Dr);
		FS47Interactable Rc;
		Rc.Id = TEXT("arc:recorder"); Rc.Object = A->Arc.Hit.Recorder; Rc.Range = 2.4;
		Rc.Label = [this] { return bShownOk ? FString(TEXT("Call recorder")) : FString(); };
		Rc.Use = [this] { ReadStrip(); };
		Rc.Examine = [this] { return bShownOk ? FString(TEXT("The call recorder on its bracket under the switchboard: a strip printer that puts down a line for every call on SARO's lines.")) : FString(); };
		I->Add(Rc);
		FS47Interactable Ks;
		Ks.Id = TEXT("arc:ksu"); Ks.Object = A->Arc.Hit.Ksu; Ks.Label = [] { return FString(); }; Ks.Use = [] {}; Ks.bLook = true; Ks.ExamineRange = 4;
		Ks.Examine = [this] { return bShownOk ? FString(TEXT("SARO's key service unit: lamps for the outside line and the supervisor line, and a typed card. EXT 201 CONTROL ROOM, EXT 214 RECORDS, EXT 220 OPERATIONS, EXT 230 PHOTO LAB.")) : FString(); };
		I->Add(Ks);
	}
	FS47BookSpec O;
	O.Key = TEXT("ch2");
	O.Owns = [](const FString& N) { return N.StartsWith(TEXT("r47.")) || N == TEXT("r47"); };
	O.Notes = NOTES(); O.Rules = RULES();
	O.Links = [this]() -> TArray<FString>& { return Links; };
	O.Wrong = [](const FString&, const FS47BookRule&) { return TOptional<FS47Result>(); };
	O.Save = [this] { KeepLinks(); if (D.Save) D.Save(); };
	O.Cards = [this] { return Cards(); };
	O.Empty = TEXT("Those two do not say anything about each other.");
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

// ---------- what main.ts asks ----------

bool FS47ArchivePhone::Has(const FString& K) const { return D.Seen && D.Seen->Contains(K); }
void FS47ArchivePhone::Add(const FString& K) const { if (D.Seen) D.Seen->Add(K); }

bool FS47ArchivePhone::Busy() const
{
	return Phase == TEXT("ringing") || Phase == TEXT("line") || Phase == TEXT("count") || Phase == TEXT("moment") || (D.Deck.Playing && D.Deck.Playing());
}

bool FS47ArchivePhone::Racing() const { return Busy() || (D.Dishes && D.Dishes->Moving()); }

void FS47ArchivePhone::ArcLamp(const FString& Line, double K) const
{
	if (const US47Annex* A = D.Annex.Get()) A->Arc.Lamp(Line, K);
}

// ---------- per frame ----------

void FS47ArchivePhone::Update(double Dt, const FS47ArcIn& S)
{
	bShownOk = !S.Stage.IsEmpty() || Has(TEXT("arc:done")) || Has(TEXT("arc:call"));
	if (D.Dishes) D.Dishes->bMuffled = S.Where != TEXT("out");
	if (Tail > 0) Tail = FMath::Max(0.0, Tail - Dt);
	// the steps on the roof are the build: count from when they are over
	if (S.bRoofBusy) Since = -1;
	else if (bWasRoof || (S.bRoofDone && Since < 0)) { Since = 0; InRoom = 0; }
	bWasRoof = S.bRoofBusy;
	if (Since >= 0) Since += Dt;
	// (the cues run on the event's own time: they hold with the 47, a paper open over it)
	if (!(Phase == TEXT("count") && S.bHold)) CueT += Dt;
	while (Cues.Num() && Cues[0].At <= CueT) { TFunction<void()> Fn = MoveTemp(Cues[0].Fn); Cues.RemoveAt(0); if (Fn) Fn(); }
	if (Phase == TEXT("idle")) Idle(Dt, S);
	else if (Phase == TEXT("ringing")) Ringing(Dt, S);
	else if (Phase == TEXT("line")) { T += Dt; if (T >= LineEnd) Dead(S); }
	else if (Phase == TEXT("count")) { if (!S.bHold) { T += Dt; Count(S); } }
	else if (Phase == TEXT("moment")) { T += Dt; Moment(Dt, S); }
	else if (Phase == TEXT("done")) Found(S);
	FallStep(Dt);
	PrintStep(Dt);
	ArrayStep(Dt, S);
	if (D.Deck.Playing && D.Deck.Playing() && D.Deck.Update) D.Deck.Update(Dt);
}

void FS47ArchivePhone::Found(const FS47ArcIn& S)
{
	// not there when it came: the first time back in the records room after it, what is left
	if (S.Where != TEXT("records") || Has(TEXT("arc:found")) || !Has(TEXT("arc:done")) || Phase == TEXT("moment")) return;
	Add(TEXT("arc:found"));
	Say(TEXT("The STATION 01 box is on the floor, its papers out of it. Behind where it stood, a steel drawer has come out of the wall."), 4.6f);
	if (D.Note) D.Note(FString::Printf(TEXT("%s. The records room. The STATION 01 box is on the floor with its papers out of it. Behind where it stood on the shelf a steel drawer has come out of the wall: the drawer from the phone."), *S47Text::ClockText(S.Clock, false)));
	if (D.Save) D.Save();
}

void FS47ArchivePhone::Idle(double Dt, const FS47ArcIn& S)
{
	if (!Pending() || !S.bOk || S.Where != TEXT("records")) { Q = 0; return; }
	if (S.bQuiet && S.bFree && !S.bRoofBusy) Q += Dt; else Q = 0;
	if (Since >= 0) InRoom += Dt;
	if (Rings >= 2 || Since < 0) return;
	// the room's last quiet moment (the key holder next), or still reading, a while after the steps
	const bool bDue = S.Stage == TEXT("call-nora") ? Since >= BUILD && Q >= QUIET
		: S.Stage == TEXT("records") && S.bRead && Since >= BUILD_READING && InRoom >= BUILD_READING * 0.6 && Q >= 4;
	if (bDue && (Rings == 0 || Since >= 90)) Ring(S);
}

bool FS47ArchivePhone::PhoneUse(const FString& Where, const FS47ArcIn& S)
{
	if (Phase == TEXT("ringing")) { Answer(Where); return true; }
	if (Where != TEXT("room")) return false;
	if (Phase == TEXT("line")) return true;
	if (Phase == TEXT("count") || Phase == TEXT("moment")) { Say(TEXT("The line is dead. Not even a dial tone."), 3); return true; }
	if (!Pending() || !S.bOk) return false;
	auto DeadLine = [this]
	{
		Say(TEXT("You lift the handset. The line is dead. Not even a dial tone."), 3.4f);
		if (US47Audio* A = D.Audio.Get()) { FS47Sound O; O.Gain = 0.45f; O.At = D.At.RecPhone(); A->Sfx(TEXT("click"), O); }
	};
	// something on the roof, or something else of the night's going: the line is dead (and the
	// steps go on, not held for a call; nothing of this starts over the other thing)
	if (S.bRoofBusy || S.bOthers) { DeadLine(); return true; }
	// the key holder next and the steps not come yet (the player went straight to the phone from
	// the board): the line is dead, and they come now. The build is always before the peak.
	if (S.Stage == TEXT("call-nora") && !S.bRoofDone && D.RoofNow && D.RoofNow()) { DeadLine(); return true; }
	// to call the key holder (or before): no dial tone, the line is already open
	if (S.Stage == TEXT("call-nora") || (S.Stage == TEXT("records") && S.bRoofDone && S.bRead)) { Lift(S); return true; }
	return false;
}

// ---------- the ring ----------

void FS47ArchivePhone::Ring(const FS47ArcIn& S)
{
	Phase = TEXT("ringing"); T = 0; Rings++; Shown.Reset();
	PrintIn(S.Clock);
	Cue(1.1, [this]
	{
		US47Audio* A = D.Audio.Get();
		if (Phase != TEXT("ringing") || !A || !A->Ctx.IsValid()) return;
		FS47Sound R; R.bLoop = true; R.Gain = 1.0f; R.At = D.At.RecPhone();
		const FS47Play Room = A->Sfx(TEXT("phoneRing"), R);
		// through the wall, the empty control room's, on the same line
		FS47Sound K; K.bLoop = true; K.Gain = 0.32f; K.Lp = 1300.f; K.At = D.At.Desk(); K.Rate = 0.985f;
		const FS47Play Desk = A->Sfx(TEXT("phoneRing"), K);
		for (const FS47Play& H : { Room, Desk }) if (H) RingHandles.Add({ H.Src, H.Gain });
	});
	Cue(1.6, [this] { if (Phase == TEXT("ringing")) Say(TEXT("The wall phone. And through the wall, the control room's, on the same line."), 3.8f); });
	Say(TEXT("By the wall phone, the call recorder prints a line. Nobody is on the phones."), 3.6f);
}

void FS47ArchivePhone::Ringing(double Dt, const FS47ArcIn& S)
{
	T += Dt;
	Lamp += Dt;
	ArcLamp(TEXT("trk1"), FMath::FloorToInt(Lamp * 2.4) % 2 ? 1 : 0.15);
	const bool bNear = S.Where == TEXT("records") || S.Where == TEXT("annex") || S.Where == TEXT("room");
	if (T > RING_MAX || !bNear)
	{
		StopRing();
		Phase = TEXT("idle"); Since = 0.001; Q = 0;
		ArcLamp(TEXT("trk1"), 0);
		if (bNear) Say(TEXT("The ringing stops."), 2.4f);
	}
}

void FS47ArchivePhone::StopRing()
{
	US47Audio* A = D.Audio.Get();
	if (A && A->Ctx.IsValid())
	{
		FS47AudioContext::FBatch B(*A->Ctx);
		const double Now = A->Ctx->CurrentTime();
		for (const FRingHandle& H : RingHandles) { H.Gain->Gain.SetTargetAtTime(0, Now, 0.02); H.Src->Stop(Now + 0.15); }
	}
	RingHandles.Reset();
}

// ---------- the call ----------

void FS47ArchivePhone::Answer(const FString& Where)
{
	StopRing();
	How = TEXT("ring"); Via = Where;
	if (US47Audio* A = D.Audio.Get()) { FS47Sound O; O.Gain = 0.6f; O.At = Where == TEXT("room") ? D.At.RecPhone() : D.At.Desk(); A->Sfx(TEXT("click"), O); }
	Call(0);
}

void FS47ArchivePhone::Lift(const FS47ArcIn& S)
{
	How = TEXT("lift"); Via = TEXT("room"); Rings = FMath::Max(Rings, 1); Shown.Reset();
	if (US47Audio* A = D.Audio.Get()) { FS47Sound O; O.Gain = 0.5f; O.At = D.At.RecPhone(); A->Sfx(TEXT("click"), O); }
	PrintIn(S.Clock);
	Say(TEXT("You lift the handset to dial. No dial tone. The line is already open."), 3.2f);
	Call(LIFT);
}

void FS47ArchivePhone::Call(double Lead)
{
	Phase = TEXT("line"); T = 0; LineEnd = Lead + CALL.End + 0.1;
	Add(TEXT("arc:call"));
	if (How == TEXT("lift")) Add(TEXT("arc:lift"));
	ArcLamp(TEXT("trk1"), 1);
	if (D.Cinematic) D.Cinematic(true);
	LineSound(Lead);
	auto Cap = [this](double At, const FString& Text, float Secs) { Cue(At, [this, Text, Secs] { Say(Text, Secs); }); };
	Cap(Lead + 0.3, TEXT("Static. Then a room, very quiet. A fluorescent tube ticks somewhere in it."), 2.6f);
	Cap(Lead + CALL.Thump, TEXT("A deep thump, through concrete."), 1.6f);
	Cap(Lead + CALL.Drawer + 0.05, TEXT("A drawer rolls out on its runners, all the way, and slams."), 1.9f);
	Cap(Lead + CALL.Fall + 0.05, TEXT("Something heavy falls. Paper, all over the floor."), 2.4f);
}

void FS47ArchivePhone::LineSound(double Lead)
{
	// what the line has: the records room before it happens
	US47Audio* A = D.Audio.Get();
	if (!A || !A->Ctx.IsValid() || !A->NoiseBuffer().IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	const TSharedPtr<FS47AudioBuffer> Noise = A->NoiseBuffer();
	FS47AudioContext::FBatch B(*Ctx);
	const FS47PhoneLine Line = A->PhoneLine();
	const double T0 = Ctx->CurrentTime() + Lead, End = T0 + CALL.End;
	LineNodes.Append(Line.Nodes);
	auto Src = [&](const TSharedPtr<FS47AudioBuffer>& Buf, double Gain, const TCHAR* FType, double Freq) -> TSharedPtr<FS47GainNode>
	{
		auto S = Ctx->CreateBufferSource(); S->Buffer = Buf; S->bLoop = true;
		auto G = Ctx->CreateGain(); G->Gain.SetValue(Gain);
		if (FType) { auto Bq = Ctx->CreateBiquadFilter(); Bq->SetType(FType); Bq->Frequency.SetValue(Freq); S->Connect(Bq); Bq->Connect(G); LineNodes.Add(Bq); }
		else S->Connect(G);
		G->Connect(Line.Input); S->Start(T0 - Lead); S->Stop(End + 0.05);
		LineSrcs.Add(S); LineNodes.Add(G);
		return G;
	};
	// the line's own hiss from the lift; the room under it once it opens
	Src(Noise, 0.04, nullptr, 0);
	const TSharedPtr<FS47GainNode> Room = Src(Noise, 0, TEXT("lowpass"), 520);
	Room->Gain.SetValueAtTime(0, T0); Room->Gain.LinearRampToValueAtTime(0.32, T0 + CALL.Tube + 0.3);
	// the tube's ballast and its ticks, the way the tired one goes
	auto Hum = Ctx->CreateOscillator(); Hum->Frequency.SetValue(240);
	auto Hg = Ctx->CreateGain(); Hg->Gain.SetValueAtTime(0, T0); Hg->Gain.LinearRampToValueAtTime(0.012, T0 + 0.6);
	Hum->Connect(Hg); Hg->Connect(Line.Input); Hum->Start(T0); Hum->Stop(End);
	LineSrcs.Add(Hum); LineNodes.Add(Hg);
	for (const double W : { 0.85, 1.7, 1.95, 3.0, 4.9, 5.5 }) { FS47Sound C; C.Gain = 0.09f; C.Rate = 2.5f; C.When = float(Lead + W); C.DestNode = Line.Input; A->Sfx(TEXT("click"), C); }
	// a thump through concrete: every brake in the array at once, heard down through the floor
	A->Boom(0.55f, Line.Input, float(Lead + CALL.Thump));
	{ FS47Sound Dm; Dm.Gain = 0.45f; Dm.Rate = 0.42f; Dm.Lp = 900.f; Dm.When = float(Lead + CALL.Thump + 0.02); Dm.DestNode = Line.Input; A->Sfx(A->Or(TEXT("door_heavy"), TEXT("doorMetal")), Dm); }
	// the drawer, and the box off the shelf
	DrawerRoll(A, Line.Input, Lead + CALL.Drawer, 1, CALL.Roll);
	BoxFall(A, Line.Input, Lead + CALL.Fall, 1);
	// under it, something low begins to turn: the gears of the drives
	auto Gear = Ctx->CreateOscillator(); Gear->SetType(TEXT("square")); Gear->Frequency.SetValueAtTime(180, T0 + CALL.Hum); Gear->Frequency.LinearRampToValueAtTime(236, End);
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(236); Bp->Q.SetValue(3);
	auto Gg = Ctx->CreateGain(); Gg->Gain.SetValueAtTime(0, T0 + CALL.Hum); Gg->Gain.LinearRampToValueAtTime(0.06, End - 0.05); Gg->Gain.LinearRampToValueAtTime(0, End);
	Gear->Connect(Bp); Bp->Connect(Gg); Gg->Connect(Line.Input); Gear->Start(T0 + CALL.Hum); Gear->Stop(End + 0.02);
	LineSrcs.Add(Gear); LineNodes.Add(Bp); LineNodes.Add(Gg);
	// and the click: the line is dead
	{ FS47Sound C; C.Gain = 0.8f; C.When = float(Lead + CALL.End); A->Sfx(TEXT("click"), C); }
}

void FS47ArchivePhone::ReleaseLine(double Fade)
{
	US47Audio* A = D.Audio.Get();
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Srcs = MoveTemp(LineSrcs);
	TArray<TSharedPtr<FS47AudioNode>> Nodes = MoveTemp(LineNodes);
	LineSrcs.Reset(); LineNodes.Reset();
	TFunction<void()> Free = [Srcs, Nodes]
	{
		for (const TSharedPtr<FS47ScheduledSourceNode>& S : Srcs) { S->Stop(); S->Disconnect(); }
		for (const TSharedPtr<FS47AudioNode>& N : Nodes) N->Disconnect();
	};
	if (A && A->Ctx.IsValid()) A->Ctx->SetTimeout(Fade + 0.4, MoveTemp(Free)); else Free();
}

void FS47ArchivePhone::Dead(const FS47ArcIn& S)
{
	// click. The line is dead: and the 47 from now
	ReleaseLine();
	if (D.Cinematic) D.Cinematic(false);
	ArcLamp(TEXT("trk1"), 0);
	Phase = TEXT("count"); T = 0; Beats.Reset(); Beats47.Reset();
	DeadAt = JsRound(S.Clock);
	Add(FString::Printf(TEXT("arc@%d"), int32(DeadAt)));
	if (US47Audio* A = D.Audio.Get()) { FS47Sound O; O.Gain = 0.5f; O.At = Via == TEXT("desk") ? D.At.Desk() : D.At.RecPhone(); A->Sfx(A->Or(TEXT("handset_down"), TEXT("thudSoft")), O); }
	Say(TEXT("Click. The line is dead."), 2.6f);
	const TOptional<double> First = D.FirstCall ? D.FirstCall() : TOptional<double>();
	const FString At = First ? S47Text::ClockText(*First, false) : FString(TEXT("02:14"));
	// (answered in the control room, on the same line: the room on it was the records room)
	const bool bDesk = Via == TEXT("desk");
	Cue(2.4, [this, bDesk, At]
	{
		Say(bDesk ? FString::Printf(TEXT("Like the call at %s, on this same phone. Only this time the room on the line was the records room."), *At)
			: FString::Printf(TEXT("Like the call at %s. Only this time the room was this one."), *At), 4.2f);
	});
	const FString FirstAt = First ? FString::Printf(TEXT("at %s, "), *At) : FString();
	if (D.Note) D.Note(bDesk
		? FString::Printf(TEXT("%s. The control room's phone, ringing with the records room's on the same line. No voice. The records room, very quiet, with the tube ticking: a thump through concrete, a drawer rolling out on its runners and slamming, something heavy falling, paper. Then nothing. I have heard a call like this once before tonight, %son this phone. Forty-seven seconds after that one the floor shook."), *S47Text::ClockText(DeadAt, false), *FirstAt)
		: FString::Printf(TEXT("%s. The wall phone in the records room. No voice. This room, very quiet, with the tube ticking: a thump through concrete, a drawer rolling out on its runners and slamming, something heavy falling, paper. Then nothing. I have heard a call like this once before tonight, %sin the control room. Forty-seven seconds after that one the floor shook."), *S47Text::ClockText(DeadAt, false), *FirstAt));
	if (D.Log) D.Log(TEXT("STATION CLOCK  WWVB REFERENCE LOST"), DeadAt);
	// K3 at once: it trips before, not after (bank 3 out, and 47 seconds to put it back)
	const bool bTripped = D.Relay ? D.Relay() : false;
	if (US47Audio* A = D.Audio.Get()) A->RelayClack(D.At.Rack(), S.Where != TEXT("room"));
	const FString Where = S.Where;
	Cue(0.5, [this, Where, bTripped]
	{
		Say(Where == TEXT("room") ? TEXT("On the rack, K3 trips.")
			: bTripped ? TEXT("Through the wall, in the empty control room: the relay. Bank 3 is out again.") : TEXT("Through the wall, in the empty control room: the relay, with nothing left to trip."), 3.8f);
	});
	Beat(TEXT("relay"));
	Clock();
}

void FS47ArchivePhone::Count(const FS47ArcIn& S)
{
	const double Tt = T;
	Clock();
	US47Audio* A = D.Audio.Get();
	if (Tt >= 16 && !HasBeat(TEXT("beep"))) { Beat(TEXT("beep")); if (A) { A->Beep(52, 2.8f, 0.09f, 0, TEXT("sine")); A->Beep(1046, 0.05f, 0.035f, 0, TEXT("sine")); } }
	if (Tt >= 31 && !HasBeat(TEXT("print"))) { Beat(TEXT("print")); PrintAgain(S); }
	if (Tt >= 40 && !HasBeat(TEXT("tube")))
	{
		Beat(TEXT("tube"));
		if (US47Annex* An = D.Annex.Get()) An->HoldTubes(TEXT("out"));
		if (A) { FS47Sound O; O.Gain = 0.18f; O.Rate = 0.55f; O.At = D.At.Tube; A->Sfx(TEXT("click"), O); A->HoldRoomTone(0.45f, 0.3f); }
		if (S.Where == TEXT("records")) Say(TEXT("The tube that has flickered all night goes out."), 3.2f);
	}
	if (Tt >= COUNT) At47Now(S);
}

void FS47ArchivePhone::Beat(const FString& What) { Beats.Add({ What, JsRound(T * 10) / 10 }); Beats47.Add(What); }

void FS47ArchivePhone::Clock()
{
	const double Ss = FMath::Min(COUNT, FMath::FloorToDouble(T));
	ClockShow = FClockShow{ S47Text::ClockText(DeadAt, false), S47Text::Pad(Ss) };
}

// ---------- :47 ----------

void FS47ArchivePhone::At47Now(const FS47ArcIn& S)
{
	US47Audio* A = D.Audio.Get();
	US47Annex* An = D.Annex.Get();
	Phase = TEXT("moment"); T = 0;
	MomentAt = JsRound(S.Clock);
	At47 = S.Where;
	ClockShow = FClockShow{ S47Text::ClockText(DeadAt, false), TEXT("47") };
	Add(TEXT("arc:done"));
	// a thump up through the floor, and the building takes it
	if (A) { A->Boom(0.7f); A->HoldRoomTone(1, 0.2f); }
	const bool bCalm = D.Calm && D.Calm();
	if (D.Shake) D.Shake(bCalm ? 0.15 : 0.42);
	if (An)
	{
		An->HoldTubes(!bCalm ? TEXT("stutter") : TEXT(""));
		for (const FVector2D& P : { FVector2D(-3.0, 7.6), FVector2D(-4.4, 9.4), FVector2D(-5.2, 8.2) }) An->Sift(P.X, P.Y, 26, 0.5, 1.4);
	}
	// every drive at once, down to the horizon along 026
	const bool bTipped = D.Dishes && D.Dishes->Tip();
	if (bTipped) { Add(TEXT("arc:tip")); if (D.Log) D.Log(TEXT("ARRAY  S-01 TO S-27 MOTION  NO COMMAND"), MomentAt); }
	if (D.Log) D.Log(TEXT("STATION CLOCK  WWVB REFERENCE RESTORED  HOLDOVER 00:00:47"), MomentAt);
	// the drawer from the phone, and the box off the shelf
	if (A && An)
	{
		const TSharedPtr<FS47AudioNode> Dest = A->Spatial(An->Arc.DrawerAt, 1.6f);
		if (Dest.IsValid()) DrawerRoll(A, Dest, 0.12, 1.3, 0.5);
	}
	FallT = 0;
	// the printer in the control room, by itself
	PrintKind = S.bBankIn ? TEXT("same") : TEXT("blank");
	Add(S.bBankIn ? TEXT("arc:in") : TEXT("arc:out"));
	const TOptional<double> Solve = D.SolveAt ? D.SolveAt() : TOptional<double>();
	if (S.bBankIn && Solve) { if (D.Log) D.Log(TEXT("SOLVE  RANGE NEGATIVE"), *Solve); }
	else if (!S.bBankIn) { if (D.Log) D.Log(TEXT("PRINTER  FORM FEED"), MomentAt); }
	Cue(0.9, [this] { PrintStart(); });
	// what is said depends on where the player is
	const FString& W = S.Where;
	auto CueSay = [this](double At, const FString& Text, float Secs) { Cue(At, [this, Text, Secs] { Say(Text, Secs); }); };
	if (W == TEXT("records"))
	{
		CueSay(0.25, TEXT("A thump, up through the floor. Every drive in the array has started at once."), 3.4f);
		CueSay(1.5, TEXT("Behind the STATION 01 box a drawer has rolled out of the wall. The box is on the floor."), 4.4f);
	}
	else if (W == TEXT("room"))
	{
		CueSay(0.25, TEXT("The floor shakes. Out in the array every drive starts at once, and the dishes come down."), 4);
		CueSay(1.5, TEXT("The printer starts by itself. From the records room, a slam, and something heavy falling."), 4.4f);
	}
	else if (W == TEXT("annex"))
	{
		CueSay(0.25, TEXT("A thump, up through the floor. Every drive in the array has started at once."), 3.4f);
		CueSay(1.5, TEXT("In the records room, a slam, and something heavy falling."), 3.6f);
	}
	else if (W == TEXT("out"))
	{
		CueSay(0.25, TEXT("Every drive in the array starts at once. The dishes are coming down."), 3.8f);
	}
	if (D.Save) D.Save();
}

void FS47ArchivePhone::Moment(double Dt, const FS47ArcIn& S)
{
	(void)Dt;
	US47Annex* An = D.Annex.Get();
	if (T >= 2.6 && ClockShow) ClockShow.Reset();
	if (T >= 2.6 && An && An->TubeMode() == TEXT("out")) An->HoldTubes(TEXT(""));
	// over when the dishes are down (or never tipped), and the box and the paper have settled
	const bool bDown = D.Dishes && D.Dishes->Down(), bHolds = D.Dishes && D.Dishes->Holds();
	if ((bDown || !bHolds || T >= MOMENT_MAX) && T >= 3) Finish(S);
}

void FS47ArchivePhone::Finish(const FS47ArcIn& S)
{
	Phase = TEXT("done"); T = 0; Tail = TAIL; ClockShow.Reset();
	if (US47Annex* An = D.Annex.Get(); An && !An->TubeMode().IsEmpty()) An->HoldTubes(TEXT(""));
	const FString Hm = S47Text::ClockText(MomentAt, false);
	const FString& W = At47;
	if (W == TEXT("records"))
	{
		Add(TEXT("arc:found"));
		if (D.Note) D.Note(FString::Printf(TEXT("%s. Forty-seven seconds after the line went dead, a thump came up through the floor and every drive in the array started at once. A drawer I never knew was there rolled out of the wall behind the STATION 01 box, the drawer from the phone, and threw the box on the floor."), *Hm));
	}
	else if (W == TEXT("annex") || W == TEXT("room"))
	{
		if (D.Note) D.Note(FString::Printf(TEXT("%s. Forty-seven seconds after the line went dead, the floor shook and every drive in the array started at once. In the records room something slammed and fell."), *Hm));
	}
	else if (W == TEXT("out"))
	{
		if (D.Note) D.Note(FString::Printf(TEXT("%s. Forty-seven seconds after the line went dead, every drive in the array started at once, and they all came down out of 32 together."), *Hm));
	}
	if (D.Dishes && D.Dishes->Down() && D.Log) D.Log(TEXT("ARRAY  27 DRIVES  AZ 026 EL 02  NO COMMAND"), JsRound(S.Clock));
	if (D.Save) D.Save();
}

// ---------- the box, the drawer ----------

void FS47ArchivePhone::FallStep(double Dt)
{
	if (FallT < 0) return;
	US47Annex* An = D.Annex.Get();
	if (!An) { FallT = -1; return; }
	const double Ft = (FallT += Dt);
	FS47Obj Fb = An->Obj(TEXT("fieldBoxModel"));
	const double DrawerK = FMath::Clamp((Ft - 0.12) / 0.5, 0.0, 1.0);
	An->Arc.SetDrawer(DrawerK * DrawerK * (3 - 2 * DrawerK));
	// pushed off the shelf by the drawer's front as it comes, then over the edge and down
	const double Tt = Ft - 0.12 - 0.38;   // from when the drawer's front reaches the box
	if (Tt < 0) return;
	if (Tt < FALL.Push)
	{
		Fb.SetX(SHELF.X0 + 0.11 * (Tt / FALL.Push));
	}
	else if (Tt < FALL.Tip)
	{
		const double K = (Tt - FALL.Push) / (FALL.Tip - FALL.Push);
		TipPose(-0.95 * K * K);
	}
	else if (Tt < FALL.Land)
	{
		// off the edge: it turns over on its way down and lands on its side
		const double K = (Tt - FALL.Tip) / (FALL.Land - FALL.Tip);
		const FVector2D From = TipAt(-0.95);
		Fb.SetPosition(FVector(FMath::Lerp(From.X, FLOOR.X, K), FMath::Lerp(From.Y, FLOOR.Y, K * K), FMath::Lerp(SHELF.Z, FLOOR.Z, K)));
		Fb.SetQuaternion(FQuat::Slerp(Q_TIP(), Q_FLOOR(), FMath::Min(1.0, K * 1.08)));
	}
	else
	{
		if (FallT >= 0 && !An->Arc.Papers.Visible())
		{
			FS47Obj Papers = An->Arc.Papers; Papers.SetVisible(true);
			if (US47Audio* A = D.Audio.Get())
			{
				const TSharedPtr<FS47AudioNode> Dest = A->Spatial(FVector(FLOOR.X, 0.2, FLOOR.Z), 1.6f);
				if (Dest.IsValid()) BoxFall(A, Dest, 0, 1.3);
			}
			for (const FVector2D& P : { FVector2D(-5.1, 8.6), FVector2D(-4.9, 8.8) }) An->Sift(P.X, P.Y, 10, 0.3, 0.4);
		}
		const double K = FMath::Min(1.0, (Tt - FALL.Land) / (FALL.Settle - FALL.Land));
		FloorPose(0.08 * (1 - K));
		if (K >= 1) { FallT = -1; BoxDown(); }
	}
}

void FS47ArchivePhone::TipPose(double A)
{
	// the box tipping about the shelf's front edge, by angle a (radians, negative: forward)
	US47Annex* An = D.Annex.Get();
	if (!An) return;
	FS47Obj Fb = An->Obj(TEXT("fieldBoxModel"));
	const FVector2D P = TipAt(A);
	Fb.SetPosition(FVector(P.X, P.Y, SHELF.Z));
	Fb.SetQuaternion(S47Three::Euler(0, 0, A, TEXT("YXZ")));
}

FVector2D FS47ArchivePhone::TipAt(double A) const
{
	// the box's origin is its bottom centre; it pivots on the edge, 0.045 behind its centre once pushed
	const double Off = (SHELF.X0 + 0.11) - SHELF.Edge;
	return FVector2D(SHELF.Edge + Off * FMath::Cos(A), SHELF.Y + Off * FMath::Sin(A));
}

void FS47ArchivePhone::FloorPose(double Lift)
{
	US47Annex* An = D.Annex.Get();
	if (!An) return;
	FS47Obj Fb = An->Obj(TEXT("fieldBoxModel"));
	Fb.SetPosition(FVector(FLOOR.X, FLOOR.Y + Lift, FLOOR.Z));
	Fb.SetQuaternion(S47Three::Euler(UE_DOUBLE_PI / 2 - Lift * 1.5, FLOOR.Yaw, 0, TEXT("YXZ")));
}

void FS47ArchivePhone::BoxDown()
{
	// on the floor for good: its hit area there, and in the way
	US47Annex* An = D.Annex.Get();
	if (!An) return;
	FloorPose();
	FS47Obj Box = An->Obj(TEXT("fieldBox"));
	Box.SetPosition(FVector(FLOOR.X, 0.2, FLOOR.Z + 0.12));
	if (TArray<FS47Rect>* C = D.Colliders ? D.Colliders() : nullptr)
	{
		if (!C->ContainsByPredicate([](const FS47Rect& R) { return R.Id == TEXT("arc:box"); }))
		{
			FS47Rect R; R.MinX = -5.55; R.MaxX = -5.1; R.MinZ = 8.45; R.MaxZ = 8.82; R.Id = TEXT("arc:box");
			C->Add(R);
		}
	}
	FS47Obj Papers = An->Arc.Papers; Papers.SetVisible(true);
}

void FS47ArchivePhone::BoxBack()
{
	US47Annex* An = D.Annex.Get();
	if (!An) return;
	FS47Obj Fb = An->Obj(TEXT("fieldBoxModel"));
	Fb.SetPosition(FVector(SHELF.X0, SHELF.Y, SHELF.Z)); Fb.SetQuaternion(FQuat::Identity);
	FS47Obj Box = An->Obj(TEXT("fieldBox"));
	Box.SetPosition(FVector(-5.55, 1.28, 8.6));
	if (TArray<FS47Rect>* C = D.Colliders ? D.Colliders() : nullptr) C->RemoveAll([](const FS47Rect& R) { return R.Id == TEXT("arc:box"); });
	FS47Obj Papers = An->Arc.Papers; Papers.SetVisible(false);
}

void FS47ArchivePhone::UseDrawer()
{
	if (!DrawerOut()) return;
	if (Has(TEXT("arc:reel"))) { Say(TEXT("The fire drawer. Empty now."), 2.4f); return; }
	Add(TEXT("arc:reel"));
	US47Annex* An = D.Annex.Get();
	if (An) { FS47Obj R = An->Arc.Reel; R.SetVisible(false); }
	if (US47Audio* A = D.Audio.Get()) { FS47Sound O; O.Gain = 0.3f; O.Rate = 1.4f; O.At = An ? An->Arc.DrawerAt : FVector::ZeroVector; A->Sfx(TEXT("paper0"), O); }
	if (D.File) D.File(ReelDoc());
	Say(TEXT("Reel 047, 10-AUG-79. NOT FOR CATALOGUE, in pencil. It fits the deck on the signal processor."), 4.4f);
	if (D.Note) D.Note(FString::Printf(TEXT("%s. The drawer behind the STATION 01 box: a steel fire drawer let into the wall, shut so long its runners have rusted. In it, alone, a reel of tape. 047, 10-AUG-79. In pencil: NOT FOR CATALOGUE."), *S47Text::ClockText(NowClock, false)));
	if (D.Save) D.Save();
}

// ---------- the call recorder ----------

void FS47ArchivePhone::PrintIn(double Clock)
{
	// the recorder puts down the call coming in, at its clock: IN TRK1 EXT 201
	Add(FString::Printf(TEXT("arcin@%d"), int32(JsRound(Clock))));
	Chatter([this] { Strip(); });
}

void FS47ArchivePhone::PrintAgain(const FS47ArcIn& S)
{
	// at :31, by itself: the night's first call again, and where it came from
	Add(TEXT("arc:31"));
	Chatter([this] { Strip(); });
	if (S.Where == TEXT("records")) Say(TEXT("The call recorder prints another line by itself. Nobody has called."), 3.4f);
}

void FS47ArchivePhone::Chatter(TFunction<void()> Then)
{
	// the recorder's print head, a line's worth (the dot matrix of the control room, small and quick)
	US47Audio* A = D.Audio.Get();
	const US47Annex* An = D.Annex.Get();
	const FVector At = An ? An->Arc.RecorderAt : FVector::ZeroVector;
	if (A)
	{
		FS47Sound P; P.Gain = 0.26f; P.Rate = 1.75f; P.At = At; P.Dur = 1.05f; A->Sfx(TEXT("printer"), P);
		FS47Sound C; C.Gain = 0.14f; C.Rate = 1.9f; C.At = At; C.When = 1.08f; A->Sfx(TEXT("click"), C);
	}
	Cue(1.1, MoveTemp(Then));
}

TArray<FString> FS47ArchivePhone::Lines() const
{
	TArray<FString> Out = { TEXT("CALL RECORD  SARO  KSU-12"), TEXT("- - - - - - - - - - - - - -") };
	const TOptional<double> First = D.FirstCall ? D.FirstCall() : TOptional<double>(), Ward = D.WardCall ? D.WardCall() : TOptional<double>();
	TArray<TPair<double, FString>> Rec;
	if (First) Rec.Add({ *First, FString::Printf(TEXT("%s  IN   TRK1  EXT 201  0:04"), *DAY(*First)) });
	if (Ward) Rec.Add({ *Ward, FString::Printf(TEXT("%s  IN   TRK2  EXT 201  0:11"), *DAY(*Ward)) });
	TArray<double> RingsAt;
	if (D.Seen)
	{
		for (const FString& K : D.Seen->Keys)
		{
			if (const TOptional<double> M = KeyNumber(K, TEXT("arcin@"))) { Rec.Add({ *M, FString::Printf(TEXT("%s  IN   TRK1  EXT 201"), *DAY(*M)) }); RingsAt.Add(*M); }
			if (const TOptional<double> N = KeyNumber(K, TEXT("arcnora@"))) Rec.Add({ *N, FString::Printf(TEXT("%s  OUT  TRK1  EXT 214  555 0119  0:14"), *DAY(*N)) });
		}
	}
	Rec.StableSort([](const TPair<double, FString>& X, const TPair<double, FString>& Y) { return Night(X.Key) < Night(Y.Key); });
	// (the one it put down by itself at :31 goes in where it was printed: after the call that came at the ring)
	const FString Again = Has(TEXT("arc:31")) && First ? FString::Printf(TEXT("%s  IN   EXT 201  FROM EXT 201"), *DAY(*First)) : FString();
	RingsAt.StableSort([](double X, double Y) { return Night(X) < Night(Y); });
	const TOptional<double> Ring = RingsAt.Num() ? TOptional<double>(RingsAt.Last()) : TOptional<double>();
	for (const TPair<double, FString>& R : Rec)
	{
		Out.Add(R.Value);
		if (!Again.IsEmpty() && Ring && R.Key == *Ring) Out.Add(Again);
	}
	return Out;
}

void FS47ArchivePhone::Strip() { if (const US47Annex* An = D.Annex.Get()) An->Arc.SetStrip(Lines()); }

FS47Doc FS47ArchivePhone::StripDoc() const
{
	const TArray<FString> L = Lines();
	const bool bAgain = Has(TEXT("arc:31"));
	bool bIn = false;
	if (D.Seen) for (const FString& K : D.Seen->Keys) bIn |= K.StartsWith(TEXT("arcin@"));
	FS47Doc Doc;
	Doc.Id = TEXT("callstrip"); Doc.Title = TEXT("Call recorder strip"); Doc.Kind = TEXT("typed"); Doc.Stamp = TEXT("CALL RECORD");
	Doc.Page = FString::Join(L, TEXT("\n"));
	Doc.Sheets = { { Doc.Page, TEXT("fanfold1986"), TEXT("true"), TEXT("true") } };
	Doc.Transcript = FString(TEXT("The paper strip from the call recorder under the switchboard in the records room. It puts down a line for every call on SARO's lines: the date and the time, in or out, the line, the extension. The card on the switchboard: EXT 201 is the control room, EXT 214 the records room."))
		+ (bIn ? TEXT("\n\nThe line IN TRK1 EXT 201 came out before the phone rang. Nobody was on the phones.") : TEXT(""))
		+ (bAgain ? TEXT("\n\nThe line under it was printed by itself while that line was dead, a quarter of a minute before the floor shook: the night's first call again, the one that played the control room. This time the recorder has where it came from. EXT 201, to EXT 201.") : TEXT(""));
	return Doc;
}

void FS47ArchivePhone::ReadStrip()
{
	const FS47Doc Doc = StripDoc();
	if (D.File) D.File(Doc);
	if (!D.Open) return;
	D.Open(Doc, [this]
	{
		if (Has(TEXT("arc:31")) && !Has(TEXT("arc:strip")))
		{
			Add(TEXT("arc:strip"));
			const TOptional<double> First = D.FirstCall ? D.FirstCall() : TOptional<double>();
			if (D.Note) D.Note(FString::Printf(TEXT("%s. The call recorder's strip. The call at %s is on it twice: once as it came in on the outside line, and once more, printed by itself tonight, from EXT 201 to EXT 201. The control room called itself."),
				*S47Text::ClockText(NowClock, false), First ? *S47Text::ClockText(*First, false) : TEXT("02:14")));
			if (D.Save) D.Save();
		}
	});
}

// ---------- the printer in the control room ----------

void FS47ArchivePhone::PrintStart()
{
	US47Audio* A = D.Audio.Get();
	PrintT = 0;
	if (!A) return;
	FS47Sound P; P.At = D.At.Printer();
	if (PrintKind == TEXT("same")) { P.Gain = 0.8f; P.Dur = 5.4f; }
	else { P.Gain = 0.32f; P.Rate = 0.72f; P.Dur = 1.7f; }
	A->Sfx(TEXT("printer"), P);
}

void FS47ArchivePhone::PrintStep(double Dt)
{
	if (PrintT < 0) return;
	PrintT += Dt;
	const double Len = PrintKind == TEXT("same") ? 5.2 : 1.6;
	if (D.Paper) D.Paper(PrintKind, FMath::Min(1.0, PrintT / Len));
	if (PrintT >= Len) PrintT = -1;
}

FString FS47ArchivePhone::Sheet() const
{
	return Has(TEXT("arc:done")) && !Has(TEXT("arc:torn")) ? (Has(TEXT("arc:in")) ? TEXT("same") : TEXT("blank")) : FString();
}

bool FS47ArchivePhone::PrinterUse()
{
	const FString K = Sheet();
	if (K.IsEmpty() || PrintT >= 0) return false;
	Add(TEXT("arc:torn"));
	if (D.Paper) D.Paper(FString(), 0);
	if (US47Audio* A = D.Audio.Get()) { FS47Sound O; O.Gain = 0.35f; O.Rate = 1.6f; O.At = D.At.Printer(); A->Sfx(TEXT("thudSoft"), O); }
	const FS47Doc Doc = ReprintDoc(K, D.SolveAt ? D.SolveAt() : TOptional<double>(), MomentAt ? MomentAt : At47Clock());
	if (D.File) D.File(Doc);
	if (D.Open) D.Open(Doc, [this, K]
	{
		const FString At = S47Text::ClockText(MomentAt ? MomentAt : At47Clock(), false);
		const TOptional<double> Solve = D.SolveAt ? D.SolveAt() : TOptional<double>();
		if (D.Note) D.Note(K == TEXT("same")
			? FString::Printf(TEXT("%s. The printer ran by itself at %s, with nobody at the console. It is the solve from %s, line for line, its time on it too. Nobody ran a solve."), *S47Text::ClockText(NowClock, false), *At, Solve ? *S47Text::ClockText(*Solve) : TEXT("earlier tonight"))
			: FString::Printf(TEXT("%s. The printer ran by itself at %s and fed out one sheet with nothing on it. Not even the header. Bank 3 was out."), *S47Text::ClockText(NowClock, false), *At));
		if (D.Save) D.Save();
	});
	return true;
}

double FS47ArchivePhone::At47Clock() const
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (const TOptional<double> M = KeyNumber(K, TEXT("arc@"))) return *M + COUNT;
	return NowClock;
}

// ---------- the array ----------

void FS47ArchivePhone::ArrayStep(double Dt, const FS47ArcIn& S)
{
	NowClock = S.Clock;
	FS47ArrayTip* Dishes = D.Dishes;
	if (!Dishes) return;
	const FString R = Dishes->Update(Dt);
	if (R == TEXT("home"))
	{
		Add(TEXT("arc:up"));
		if (D.Log) D.Log(TEXT("ARRAY  27 DRIVES  AZ 026 EL 32  NO COMMAND"), JsRound(S.Clock));
		if (bWatchUp) Add(TEXT("arc:upseen"));
		bWatchUp = false;
	}
	// seen, down: once
	if (Dishes->Holds() && S.bView && !Has(TEXT("arc:tipseen")) && FacingArray(S.Cam))
	{
		Add(TEXT("arc:tipseen"));
		if (D.Note) D.Note(FString::Printf(TEXT("%s. Every dish in the array has come down out of 32: all 27 on 026, tipped down to the horizon, as if something stood out there. Their red lamps blink together. Nobody sent a command."), *S47Text::ClockText(S.Clock, false)));
	}
	if (Dishes->Mode == TEXT("raising") && S.bView && FacingArray(S.Cam)) bWatchUp = true;
	// nobody at SARO: they go back up unseen
	if (Dishes->Down() && !S.bSaro)
	{
		Dishes->Home();
		Add(TEXT("arc:up")); Add(TEXT("arc:unseen"));
		if (D.Log) D.Log(TEXT("ARRAY  27 DRIVES  AZ 026 EL 32  NO COMMAND"), JsRound(S.Clock));
		if (D.Save) D.Save();
	}
	// back at SARO, the first look at them after
	if (Has(TEXT("arc:unseen")) && !Has(TEXT("arc:upnote")) && S.bSaro && S.bView && !Dishes->Holds() && FacingArray(S.Cam))
	{
		Add(TEXT("arc:upnote"));
		if (D.Note) D.Note(FString::Printf(TEXT("%s. The dishes are back up on 026 and 32, every one. I did not see them go."), *S47Text::ClockText(S.Clock, false)));
	}
}

bool FS47ArchivePhone::FacingArray(const FS47Cam& Cam) const
{
	// the first rows of the array, from where the player is: any of three points across it
	for (const double X : { -40.0, -1.0, 40.0 }) if (S47Facing(Cam, FVector(X, 9, -80), 420, 34)) return true;
	return false;
}

// ---------- reel 047 on the signal processor ----------

FString FS47ArchivePhone::DeckLabel() const
{
	if (D.Deck.Playing && D.Deck.Playing()) return TEXT("Signal processor (reel 047)");
	return Reel() == TEXT("kit") ? TEXT("Thread reel 047 on the deck") : FString();
}

bool FS47ArchivePhone::DeckUse()
{
	if (D.Deck.Playing && D.Deck.Playing()) { Say(TEXT("Reel 047 is on the deck. Let it run."), 2.4f); return true; }
	if (Reel() != TEXT("kit")) return false;
	const FString Why = D.DeckReady ? D.DeckReady() : FString();
	if (!Why.IsEmpty()) { Say(Why, 3.2f); return true; }
	Shown.Reset();
	if (!D.Deck.Play) { UE_LOG(LogTemp, Warning, TEXT("S47: reel 047's deck (story/reel047.ts) is not hooked to the archive's phone")); return true; }
	TSharedRef<bool> bRaised = MakeShared<bool>(false);
	D.Deck.Play([this, bRaised](const FString& Kind)
	{
		if (Kind == TEXT("thread")) Say(TEXT("You thread reel 047 on the deck and start it."), 2.6f);
		else if (Kind == TEXT("pulse")) Say(TEXT("Tape hiss. Then the pulse group, four and seven, the way it came in tonight."), 3.8f);
		else if (Kind == TEXT("crank")) Say(TEXT("A handle turned, and a bell: somebody ringing a field telephone."), 3.6f);
		else if (Kind == TEXT("cut")) Say(TEXT("A snap, metal through metal. Like a cable going through cutters."), 3.4f);
		else if (Kind == TEXT("hole")) Cue(0.8, [this] { Say(TEXT("Then nothing. Not even the hiss. The reels go on turning."), 4); });
		else if (Kind == TEXT("relay"))
		{
			Say(TEXT("A relay, loud and close, on the tape."), 2.8f);
			// and out in the array, all at once, back up to 32 (E10: "relay impact and reference motion observed")
			if (D.Dishes && D.Dishes->Raise())
			{
				*bRaised = true;
				if (D.Log) D.Log(TEXT("ARRAY  S-01 TO S-27 MOTION  NO COMMAND"), JsRound(NowClock));
				Cue(1.2, [this] { Say(TEXT("Out in the array, every drive starts at once."), 3.2f); });
			}
		}
		else if (Kind == TEXT("range")) Say(TEXT("On the processor's display, from the tape's data track: SOURCE DISTANCE -32 LY."), 4.4f);
		else if (Kind == TEXT("end"))
		{
			Add(TEXT("arc:heard"));
			if (*bRaised) Add(TEXT("arc:raised"));
			const FString Hm = S47Text::ClockText(NowClock, false);
			if (D.Note) D.Note(FString::Printf(TEXT("%s. Reel 047, 10 August 1979, on the signal processor. The pulse group, four and seven. A crank, like a field telephone being rung by its handle. A cut, metal through metal. Then nothing, not even the hiss, for 47 seconds. Then a relay, close. The data track ends on a solve: -32 LY.%s"),
				*Hm, *bRaised ? TEXT(" When the relay went on the tape, the array went back up, all 27 together, to 026 and 32.") : TEXT("")));
			if (D.Refresh) D.Refresh();
			if (D.Save) D.Save();
		}
	});
	return true;
}

// ---------- the board ----------

TArray<FS47BoardCard> FS47ArchivePhone::Cards() const
{
	if (!Has(TEXT("arc:heard"))) return {};
	FS47BoardCard C;
	C.Id = TEXT("r47"); C.Doc = TEXT("reel047"); C.Stamp = TEXT("REEL 047"); C.Title = TEXT("COMMISSIONING, 10-AUG-79"); C.Kind = TEXT("thing"); C.bHave = true; C.W = 300;
	C.Text = TEXT("From the fire drawer behind the STATION 01 box. NOT FOR CATALOGUE.");
	C.Lines = {
		{ TEXT("r47.pulse"), TEXT("The pulse group, four and seven, as on tape B tonight."), FString() },
		{ TEXT("r47.crank"), TEXT("A handle turned, and a bell: a field telephone being rung."), FString() },
		{ TEXT("r47.cut"), TEXT("A snap of metal through metal: a cable through cutters."), FString() },
		{ TEXT("r47.gap"), TEXT("Then nothing, not even the hiss, for 47 seconds. Then a relay."), FString() },
		{ TEXT("r47.range"), TEXT("On the data track: SOURCE DISTANCE -32 LY."), FString() },
	};
	return { C };
}

void FS47ArchivePhone::KeepLinks()
{
	if (!D.Seen) return;
	for (const FString& K : D.Seen->Array()) if (K.StartsWith(LINK)) D.Seen->Remove(K);
	for (const FString& K : Links) D.Seen->Add(LINK + K);
}

// ---------- small things ----------

void FS47ArchivePhone::Say(const FString& Text, float Secs) { Shown.Add(Text); if (D.Toast) D.Toast(Text, Secs); }

void FS47ArchivePhone::Cue(double After, TFunction<void()> Fn)
{
	Cues.Add({ CueT + After, MoveTemp(Fn) });
	Cues.StableSort([](const FCue& A, const FCue& B) { return A.At < B.At; });
}

// ---------- a night opened, the developer menu, the tests ----------

void FS47ArchivePhone::Halt()
{
	StopRing();
	ReleaseLine(0);
	if (Phase == TEXT("line") && D.Cinematic) D.Cinematic(false);
	if ((Phase == TEXT("count") || Phase == TEXT("moment")) && D.Audio.IsValid()) D.Audio->HoldRoomTone(1, 0.1f);
	if (D.Deck.Reset) D.Deck.Reset();   // (and not finished: the processor's display is the processor's own again)
	Cues.Reset(); ClockShow.Reset(); FallT = -1; PrintT = -1;
	if (US47Annex* An = D.Annex.Get()) { if (!An->TubeMode().IsEmpty()) An->HoldTubes(TEXT("")); }
	ArcLamp(TEXT("trk1"), 0); ArcLamp(TEXT("trk2"), 0);
	Beats47.Reset();
	Phase = TEXT("idle"); T = 0; How.Empty(); Via.Empty(); Rings = 0; Q = 0; Since = -1; InRoom = 0; bWasRoof = false; Tail = 0;
}

void FS47ArchivePhone::Restore()
{
	// a night put back from the town's part of the case: the drawer, the box, the strip, the paper in
	// the printer, the dishes, the papers in the journal. A night saved in the middle of it (it cannot
	// be: no save while it goes) is put back as if it had not begun.
	Halt();
	if (D.Seen && Has(TEXT("arc:call")) && !Has(TEXT("arc:done")))
	{
		D.Seen->Remove(TEXT("arc:call")); D.Seen->Remove(TEXT("arc:lift"));
		for (const FString& K : D.Seen->Array()) if (K.StartsWith(TEXT("arc@"))) D.Seen->Remove(K);
	}
	const bool bDone = Has(TEXT("arc:done"));
	Phase = bDone ? TEXT("done") : TEXT("idle");
	if (US47Annex* An = D.Annex.Get())
	{
		An->Arc.SetDrawer(bDone ? 1 : 0);
		FS47Obj R = An->Arc.Reel; R.SetVisible(!Has(TEXT("arc:reel")));
	}
	if (bDone) BoxDown(); else BoxBack();
	Strip();
	// (only a sheet of its own: the prologue has the printer's paper until then, on its own paper
	// again, not a blank one left from another night opened before in this session)
	if (D.Paper) { const FString Sh = Sheet(); if (!Sh.IsEmpty()) D.Paper(Sh, 1); else D.Paper(FString(), -1); }
	// the dishes: down and at rest if they were saved down, else as the night left them
	if (D.Dishes) D.Dishes->Restore(Has(TEXT("arc:tip")) && !Has(TEXT("arc:up")));
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
	if (Has(TEXT("arc:reel")) && D.File) D.File(ReelDoc());
	if (Has(TEXT("arc:torn")) && D.File) D.File(ReprintDoc(Has(TEXT("arc:in")) ? TEXT("same") : TEXT("blank"), D.SolveAt ? D.SolveAt() : TOptional<double>(), At47Clock()));
	if (Has(TEXT("arc:strip")) && D.File) D.File(StripDoc());
}

void FS47ArchivePhone::CalledOut(double Clock)
{
	if (D.Seen) for (const FString& K : D.Seen->Keys) if (K.StartsWith(TEXT("arcnora@"))) return;
	Add(FString::Printf(TEXT("arcnora@%d"), int32(JsRound(Clock))));
	Chatter([this] { Strip(); });
}

bool FS47ArchivePhone::Now(const FString& InHow, const FS47ArcIn& S)
{
	if (Phase != TEXT("idle") || !Pending() || S.Where != TEXT("records")) return false;
	if (InHow == TEXT("ring")) Ring(S); else Lift(S);
	return true;
}

// ---------- the papers ----------

FS47Doc FS47ArchivePhone::ReelDoc()
{
	FS47Doc D;
	D.Id = TEXT("reel047"); D.Title = TEXT("Reel 047 / The box label"); D.Kind = TEXT("typed"); D.Stamp = TEXT("REEL 047");
	D.Page = TEXT("SARO  RAW TAPE\nREEL  047\nDATE  10-AUG-79\nFREQ  1420.405 MHz\nSURVEY / COMMISSIONING\n\n~ NOT FOR CATALOGUE");
	D.Transcript = TEXT("A grey reel box from the fire drawer behind the STATION 01 box. The label is typed like every other reel SARO keeps. The pencil under it is not Dale's.\n\nThe tape catalogue on the operations terminal goes from 046 to 048.");
	return D;
}

FS47Doc FS47ArchivePhone::ReprintDoc(const FString& Kind, TOptional<double> Solve, double Printed)
{
	if (Kind == TEXT("blank"))
	{
		FS47Doc D;
		D.Id = TEXT("reprint"); D.Title = TEXT("A blank sheet from the printer"); D.Kind = TEXT("printout"); D.Page = TEXT("");
		D.Transcript = FString::Printf(TEXT("Fan-fold paper from the printer by RX bank 3, torn off below the perforation. The printer ran by itself at %s and fed it out.\n\nNothing is printed on it. Not even the header."), *S47Text::ClockText(Printed, false));
		return D;
	}
	// (the night's own printout, the same lines, its time with them: Prologue.printout)
	FS47Doc D = S47Docs::Printout(Solve.Get(0));
	D.Id = TEXT("reprint"); D.Title = TEXT("Direction solve printout, again");
	D.Transcript = FString::Printf(TEXT("Fan-fold paper, still warm. The printer ran by itself at %s, with nobody at the console.\n\nIt is the solve from earlier tonight, line for line, down to the time on it: %s."), *S47Text::ClockText(Printed, false), *S47Text::ClockText(Solve.Get(0)));
	return D;
}

// ---------- the sounds ----------

void FS47ArchivePhone::DrawerRoll(US47Audio* A, const TSharedPtr<FS47AudioNode>& Dest, double When, double K, double Dur)
{
	// a steel drawer rolling out on its runners and slamming at the end of its travel
	if (!A || !A->Ctx.IsValid() || !A->NoiseBuffer().IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch B(*Ctx);
	const double T = Ctx->CurrentTime() + When;
	// the runners: a band of noise rippling with the ball bearings, quicker as it goes
	auto N = Ctx->CreateBufferSource(); N->Buffer = A->NoiseBuffer(); N->bLoop = true;
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Q.SetValue(1.6);
	Bp->Frequency.SetValueAtTime(300, T); Bp->Frequency.LinearRampToValueAtTime(640, T + Dur);
	auto Am = Ctx->CreateGain(); Am->Gain.SetValue(0.55);
	auto Lfo = Ctx->CreateOscillator(); Lfo->Frequency.SetValueAtTime(16, T); Lfo->Frequency.LinearRampToValueAtTime(48, T + Dur);
	auto Lg = Ctx->CreateGain(); Lg->Gain.SetValue(0.45); Lfo->Connect(Lg); Lg->Connect(Am->Gain);
	auto G = Ctx->CreateGain(); G->Gain.SetValueAtTime(0, T); G->Gain.LinearRampToValueAtTime(0.32 * K, T + 0.07); G->Gain.LinearRampToValueAtTime(0.5 * K, T + Dur); G->Gain.SetTargetAtTime(0, T + Dur, 0.015);
	N->Connect(Bp); Bp->Connect(Am); Am->Connect(G); G->Connect(Dest);
	N->Start(T, FMath::FRand() * 2); N->Stop(T + Dur + 0.2); Lfo->Start(T); Lfo->Stop(T + Dur + 0.2);
	TArray<TSharedPtr<FS47AudioNode>> All = { N, Bp, Am, Lfo, Lg, G };
	Ctx->SetTimeout(When + Dur + 1, [All] { for (const TSharedPtr<FS47AudioNode>& X : All) X->Disconnect(); });
	// the slam: steel on steel, and the wall taking it; a rattle after
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.75 * K); S.Rate = 1.35f; S.Lp = 4200.f; S.When = float(When + Dur); A->Sfx(A->Or(TEXT("door_heavy"), TEXT("doorMetal")), S); }
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.55 * K); S.Rate = 1.1f; S.When = float(When + Dur + 0.005); A->Sfx(TEXT("thudSoft"), S); }
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.3 * K); S.Rate = 0.7f; S.When = float(When + Dur + 0.09); A->Sfx(TEXT("click"), S); }
}

void FS47ArchivePhone::BoxFall(US47Audio* A, const TSharedPtr<FS47AudioNode>& Dest, double When, double K)
{
	// a heavy archive box off a shelf onto the floor, and its papers going
	if (!A) return;
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.95 * K); S.Rate = 0.6f; S.Lp = 2400.f; S.When = float(When); A->Sfx(TEXT("thudSoft"), S); }
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.4 * K); S.Rate = 0.9f; S.When = float(When + 0.11); A->Sfx(TEXT("thudSoft"), S); }
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.55 * K); S.Rate = 0.85f; S.When = float(When + 0.04); A->Sfx(TEXT("paper0"), S); }
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.4 * K); S.Rate = 1.1f; S.When = float(When + 0.3); A->Sfx(TEXT("paper1"), S); }
	{ FS47Sound S; S.DestNode = Dest; S.Gain = float(0.2 * K); S.Rate = 0.6f; S.When = float(When + 0.18); A->Sfx(TEXT("click"), S); }
}

// ---------- the self test ----------

bool FS47ArchivePhone::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s archive phone: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47TownSeen Seen;
	bool bRoofNow = false;
	TArray<FString> Notes, Logged;
	FS47ArrayTipDeps Td;
	FS47ArrayTip Tip(Td);
	FS47ArcDeps Dp;
	Dp.Seen = &Seen; Dp.Dishes = &Tip;
	Dp.Note = [&Notes](const FString& N) { Notes.Add(N); };
	Dp.Log = [&Logged](const FString& L, double) { Logged.Add(L); };
	Dp.Relay = [] { return true; };
	Dp.FirstCall = [] { return TOptional<double>(2.0 * 3600 + 14 * 60 + 40 - 5); };
	Dp.WardCall = [] { return TOptional<double>(); };
	Dp.SolveAt = [] { return TOptional<double>(2.0 * 3600 + 14 * 60 + 30); };
	Dp.RoofNow = [&bRoofNow] { bRoofNow = true; return true; };
	Dp.At.Rack = Dp.At.Desk = Dp.At.Printer = Dp.At.RecPhone = [] { return FVector::ZeroVector; };
	FS47ArchivePhone P(Dp);
	FS47ArcIn S;
	S.bOk = true; S.Stage = TEXT("call-nora"); S.bRead = true; S.bQuiet = true; S.bFree = true; S.Where = TEXT("records"); S.Clock = 3 * 3600; S.bSaro = true; S.bBankIn = true;
	// the build: the steps on the roof first, then BUILD seconds and the room quiet
	P.Update(1, S);
	Check(P.Phase == TEXT("idle"), TEXT("not before the steps on the roof"));
	S.bRoofDone = true;
	for (int32 I = 0; I < 11; ++I) P.Update(1, S);
	Check(P.Phase == TEXT("idle"), TEXT("not within the build's 12 seconds"));
	for (int32 I = 0; I < 3; ++I) P.Update(1, S);
	Check(P.Phase == TEXT("ringing") && P.PhoneLabel() == TEXT("Answer the phone") && Seen.Contains(TEXT("arcin@10800")), TEXT("the wall phone rings at the room's last quiet moment, the recorder first"));
	// answered: the line, then dead, then the 47
	Check(P.PhoneUse(TEXT("room"), S) && P.Phase == TEXT("line") && P.How == TEXT("ring") && Seen.Contains(TEXT("arc:call")), TEXT("answered, the line opens"));
	for (int32 I = 0; I < 7; ++I) P.Update(1, S);
	Check(P.Phase == TEXT("count") && P.ClockShow && P.ClockShow->Hm == TEXT("03:00") && Logged.Contains(TEXT("STATION CLOCK  WWVB REFERENCE LOST")), TEXT("dead, and the corner clock counts"));
	Check(Notes.Num() == 1 && Notes[0].Contains(TEXT("at 02:14, in the control room")), TEXT("the note has the first call"));
	for (int32 I = 0; I < 48; ++I) P.Update(1, S);
	Check(P.Beats.Num() == 4 && P.Beats[1].What == TEXT("beep") && P.Beats[2].What == TEXT("print") && P.Beats[3].What == TEXT("tube"), TEXT("the beep at :16, the print at :31, the tube at :40"));
	Check(Seen.Contains(TEXT("arc:done")) && Seen.Contains(TEXT("arc:tip")) && Seen.Contains(TEXT("arc:in")) && P.At47 == TEXT("records"), TEXT(":47: the drawer, the dishes, the printer"));
	const TArray<FString> L = P.Lines();
	Check(L.Num() == 5 && L[2] == TEXT("04/14 02:14  IN   TRK1  EXT 201  0:04") && L[3] == TEXT("04/14 03:00  IN   TRK1  EXT 201") && L[4] == TEXT("04/14 02:14  IN   EXT 201  FROM EXT 201"), TEXT("the strip: the first call, the ring, and the first call again from EXT 201"));
	Check(P.Sheet() == TEXT("same") && P.PrinterLabel().IsEmpty(), TEXT("a sheet in the printer, not torn while it prints"));
	for (int32 I = 0; I < 5; ++I) P.Update(1, S);
	Check(P.Phase == TEXT("done") && P.Resting() && Seen.Contains(TEXT("arc:found")), TEXT("over once the dishes are down"));
	// a lift, the reprint
	const FS47Doc R = ReprintDoc(TEXT("same"), TOptional<double>(2.0 * 3600 + 14 * 60 + 30), 3.0 * 3600 + 54);
	Check(R.Id == TEXT("reprint") && R.Page.Contains(TEXT("-39 LY")) && R.Transcript.EndsWith(TEXT("down to the time on it: 02:14:30.")), TEXT("the reprint is the night's own solve"));
	(void)bRoofNow;
	return bOk;
}
