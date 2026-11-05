#include "S47Score.h"
#include "S47CosmicAmbient.h"

#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47DecoderDesk.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Media.h"
#include "S47Story.h"
#include "S47Walker.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace S47Score
{
	const TMap<FString, FS47ScoreTrack> TRACKS = {
		{ TEXT("shadows"), { TEXT("shadows_and_dust.mp3"), TEXT("Shadows and Dust"), TEXT("Scott Buckley"), 0.5 } },
		{ TEXT("decoherence"), { TEXT("decoherence.mp3"), TEXT("Decoherence"), TEXT("Scott Buckley"), 0.45 } },
		{ TEXT("oldOnes"), { TEXT("the_old_ones.mp3"), TEXT("The Old Ones"), TEXT("Scott Buckley"), 0.45 } },
		{ TEXT("solitude"), { TEXT("in_search_of_solitude.mp3"), TEXT("In Search of Solitude"), TEXT("Scott Buckley"), 0.42 } },
		{ TEXT("neon"), { TEXT("neon.mp3"), TEXT("Neon"), TEXT("Scott Buckley"), 0.36 } },
		{ TEXT("hymn"), { TEXT("hymn_to_the_dawn.mp3"), TEXT("Hymn to the Dawn"), TEXT("Scott Buckley"), 0.45 } },
		{ TEXT("cylOne"), { TEXT("cylinder_one.mp3"), TEXT("Cylinder One"), TEXT("Chris Zabriskie"), 0.42 } },
		{ TEXT("cylTwo"), { TEXT("cylinder_two.mp3"), TEXT("Cylinder Two"), TEXT("Chris Zabriskie"), 0.42 } },
		{ TEXT("cylEight"), { TEXT("cylinder_eight.mp3"), TEXT("Cylinder Eight"), TEXT("Chris Zabriskie"), 0.4 } },
		{ TEXT("floating"), { TEXT("floating_cities.mp3"), TEXT("Floating Cities"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("ossuary1"), { TEXT("ossuary_1.mp3"), TEXT("Ossuary 1 - A Beginning"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("ossuary5"), { TEXT("ossuary_5.mp3"), TEXT("Ossuary 5 - Rest"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("longNote"), { TEXT("long_note_four.mp3"), TEXT("Long Note Four"), TEXT("Kevin MacLeod"), 0.5 } },
		{ TEXT("lightless"), { TEXT("lightless_dawn.mp3"), TEXT("Lightless Dawn"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("phantom"), { TEXT("phantom_from_space.mp3"), TEXT("Phantom from Space"), TEXT("Kevin MacLeod"), 0.4 } },
		{ TEXT("docks"), { TEXT("night_on_the_docks.mp3"), TEXT("Night on the Docks (Sax)"), TEXT("Kevin MacLeod"), 0.42 } },
		{ TEXT("nowhere"), { TEXT("signs_to_nowhere.mp3"), TEXT("Signs To Nowhere"), TEXT("Shane Ivers"), 0.38 } },
	};
	const TMap<FString, TArray<FString>> CUES = {
		{ TEXT("night"), { TEXT("cylEight"), TEXT("floating"), TEXT("ossuary1"), TEXT("longNote"), TEXT("lightless"), TEXT("shadows"), TEXT("phantom"), TEXT("cylTwo") } },
		{ TEXT("eerie"), { TEXT("ossuary5"), TEXT("cylOne"), TEXT("decoherence"), TEXT("longNote") } },
		{ TEXT("tension"), { TEXT("oldOnes"), TEXT("longNote"), TEXT("lightless"), TEXT("phantom") } },
		{ TEXT("motel"), { TEXT("solitude"), TEXT("ossuary5"), TEXT("cylOne") } },
		{ TEXT("drive"), { TEXT("neon"), TEXT("cylTwo"), TEXT("floating"), TEXT("cylEight") } },
		{ TEXT("town"), { TEXT("nowhere"), TEXT("docks") } },
		{ TEXT("dawn"), { TEXT("hymn"), TEXT("cylEight"), TEXT("floating") } },
	};
}

namespace
{
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	bool On(const TFunction<bool()>& Fn) { return Fn && Fn(); }
	double Value(const TFunction<double()>& Fn) { return Fn ? Fn() : 0; }
	FString PoolFor(const FString& Cue)
	{
		if (Cue == TEXT("calm")) return TEXT("night");
		if (Cue == TEXT("action")) return TEXT("tension");
		if (Cue == TEXT("recovery")) return TEXT("motel");
		return Cue;
	}
	bool KnownCue(const FString& Cue) { return S47Score::CUES.Contains(PoolFor(Cue)); }
	void SilenceDeck(TSharedPtr<FS47MediaSource>& Source, TSharedPtr<FS47GainNode>& Gain)
	{
		if (Source) { Source->OnEnded = nullptr; Source->OnError = nullptr; Source->Pause(); Source->Stop(); Source->Disconnect(); }
		if (Gain) Gain->Disconnect();
		Source.Reset(); Gain.Reset();
	}
}

FS47Score::FS47Score(US47Main* InM, const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& InDest)
	: M(InM), Ctx(InCtx), Dest(InDest), Rest(Rnd(6, 14)) {}

FS47Score::~FS47Score()
{
	*bAlive = false;
	Stop();
}

// The current imported tracks have no verified BPM/key/stems. Their transitions use a
// shared audio clock and real two-deck fades; quantization is opt-in per verified track.
void FS47Score::RequestMood(const FString& Cue, double Severity)
{
	RequestedMood = KnownCue(Cue) ? Cue : FString();
	RequestedSeverity = FMath::IsFinite(Severity) ? FMath::Clamp(Severity, 0.0, 1.0) : 0;
}

bool FS47Score::SetTrackTiming(const FString& InTrack, double Bpm, int32 BeatsPerBar, int32 PhraseBars)
{
	if (!S47Score::TRACKS.Contains(InTrack) || !FMath::IsFinite(Bpm) || Bpm < 20 || Bpm > 300 || BeatsPerBar < 1 || BeatsPerBar > 16 || PhraseBars < 1 || PhraseBars > 16) return false;
	Timings.Add(InTrack, { Bpm, BeatsPerBar, PhraseBars }); return true;
}

double FS47Score::TransitionAt(bool bUrgent) const
{
	const double Now = Ctx->CurrentTime();
	const FTrackTiming* Timing = Timings.Find(Track);
	if (!El || !Timing) return Now;
	const double Beat = 60.0 / Timing->Bpm;
	const double Unit = bUrgent ? Beat : Beat * Timing->BeatsPerBar;
	const double Position = El->CurrentTime();
	double Wait = FMath::CeilToDouble((Position + 0.00001) / Unit) * Unit - Position;
	if (!bUrgent && Timing->PhraseBars > 1)
	{
		const double Phrase = Unit * Timing->PhraseBars;
		const double PhraseWait = FMath::CeilToDouble((Position + 0.00001) / Phrase) * Phrase - Position;
		if (PhraseWait <= 5) Wait = PhraseWait;
	}
	// A real threat cannot wait through a long musical bar. This cap deliberately releases
	// quantization when necessary; no bar alignment is claimed for that urgent fallback.
	return Now + (bUrgent ? FMath::Min(0.35, Wait) : Wait);
}

void FS47Score::Update(double Dt, const FString& InWant, bool bHeld)
{
	if (bPaused)
	{
		if (Cosmic) Cosmic->Pause(true);
		RequestedMood.Empty(); RequestedSeverity = 0; return;
	}
	// One persistent texture under the same music destination and volume control.
	// No hidden event data is read. A receiver always gets clean listening space,
	// including builds that duck rather than fully hold their music at the PC.
	if (!Cosmic && !InWant.IsEmpty() && !bHeld)
	{
		FS47AudioContext::FBatch Batch(*Ctx);
		Cosmic = MakeShared<FS47CosmicAmbient>(Ctx); Cosmic->Connect(Dest);
		if (M && M->Audio)
		{
			for (const TCHAR* Key : { TEXT("nasa_earth_chorus"), TEXT("nasa_cassini_jupiter"), TEXT("space_nebula") })
				Cosmic->AddReference(M->Audio->SoundSource(Key));
		}
	}
	if (Cosmic) Cosmic->Set(InWant, bHeld || bFading || (M && M->Audio && M->Audio->ReceiverMonitorOn()));
	Dt = FMath::IsFinite(Dt) ? FMath::Max(0.0, Dt) : 0;
	T += Dt;
	const double Severity = RequestedSeverity;
	RequestedMood.Empty(); RequestedSeverity = 0;
	if (Outgoing && Ctx->CurrentTime() >= OutgoingUntil) RetireOutgoing();
	const bool bNewHold = bHeld && !bHeldNow;
	bHeldNow = bHeld;
	if (bHeld)
	{
		Pending.Empty(); Candidate.Empty(); bRecovering = false; ActionUntil = RecoveryUntil = 0;
		if (bNewHold) bFading = false; // A listening hold preempts even an existing slow fade.
		if (!Playing.IsEmpty()) Fade(0.35, 4);
		else Rest = FMath::Max(Rest, 4.0);
		return;
	}
	FString Want = KnownCue(InWant) ? InWant : FString();
	if (Want.IsEmpty())
	{
		Pending.Empty(); Candidate.Empty(); bRecovering = false; ActionUntil = RecoveryUntil = 0;
		if (!Playing.IsEmpty()) Fade(2, 4); else Rest = FMath::Max(Rest, 4.0);
		return;
	}
	const bool bUrgent = Want == TEXT("action") || Severity >= 0.75;
	if (Want == TEXT("action")) { ActionUntil = T + 6; bRecovering = false; }
	else if (Playing == TEXT("action") || bRecovering)
	{
		if (T < ActionUntil) Want = TEXT("action");
		else
		{
			if (!bRecovering) { bRecovering = true; RecoveryUntil = T + 8; }
			if (T < RecoveryUntil) Want = TEXT("recovery"); else bRecovering = false;
		}
	}
	if (bFading) return; // Listening holds and explicit developer fades own their whole fade.
	if (Want == Playing) { Pending.Empty(); Candidate.Empty(); return; }
	if (Playing.IsEmpty())
	{
		if (!bUrgent) { Rest -= Dt; if (Rest > 0) return; }
		Start(Want); return;
	}
	if (Want != Candidate)
	{
		Candidate = Want; CandidateSince = T; Pending.Empty();
	}
	const double StableFor = bUrgent ? 0 : Want == TEXT("recovery") ? 0.1 : 2.0;
	if (T - CandidateSince < StableFor) return;
	if (Pending != Want) { Pending = Want; PendingAt = TransitionAt(bUrgent); }
	if (Ctx->CurrentTime() < PendingAt) return;
	const FString Next = Pending; Pending.Empty(); Candidate.Empty();
	Begin(Next, Pick(Next), bUrgent ? 2.0 : 4.0);
}

// Score.pick: not the one just played, and the one heard longest ago.
FString FS47Score::Pick(const FString& Cue) const
{
	// Keep slow ambient beds for unease. The percussion-led sci-fi cue gives
	// pursuit its own identity instead of choosing another ambient drone.
	static const TArray<FString> Action = { TEXT("phantom") };
	static const TArray<FString> Calm = { TEXT("solitude"), TEXT("cylTwo") };
	const TArray<FString>* Found = Cue == TEXT("action") ? &Action
		: Cue == TEXT("calm") ? &Calm : S47Score::CUES.Find(PoolFor(Cue));
	if (!Found || Found->IsEmpty()) return FString();
	const TArray<FString>& List = *Found;
	TArray<FString> Fresh;
	for (const FString& K : List) if (K != Track) Fresh.Add(K);
	const TArray<FString>& Pool = Fresh.IsEmpty() ? List : Fresh;
	TArray<double> Ages;
	for (const FString& K : Pool)
	{
		const double* At = Last.Find(K);
		Ages.Add(T - (At ? *At : -1e9));
	}
	int32 Best = 0;
	for (int32 I = 1; I < Pool.Num(); ++I) if (Ages[I] + Rnd(0, 120) > Ages[Best] + Rnd(0, 120)) Best = I;
	return Pool[Best];
}

// Start remains the named developer entry point; normal state changes use the same two decks.
void FS47Score::Start(const FString& Cue, const FString& InTrack)
{
	if (!S47Score::bScoreOn) return;
	const FString Next = InTrack.IsEmpty() ? Pick(Cue) : InTrack;
	if (!S47Score::TRACKS.Contains(Next) || !KnownCue(Cue)) return;
	if (El && Track == Next) Stop(); // Explicit named replay; adaptive Begin keeps its position.
	Pending.Empty(); Candidate.Empty();
	Begin(Cue, Next, El ? 3.0 : 6.0);
}

void FS47Score::RetireOutgoing()
{
	FS47AudioContext::FBatch B(*Ctx);
	SilenceDeck(Outgoing, OutgoingGain); OutgoingUntil = 0;
}

void FS47Score::Begin(const FString& Cue, const FString& InTrack, double FadeSeconds)
{
	const FS47ScoreTrack* C = S47Score::TRACKS.Find(InTrack);
	if (!C || !Dest) return;
	// A piece may suit two moods. Recategorize it without replaying its intro.
	// Explicit developer Start still restarts because it calls Stop first.
	if (El && Track == InTrack) { Playing = Cue; return; }
	const TSharedPtr<FS47MediaSource> NewEl = FS47MediaSource::Open(Ctx, TEXT("music/") + C->File);
	if (!NewEl) { if (!El) Rest = 120; return; }
	FS47AudioContext::FBatch B(*Ctx);
	// An interrupted transition keeps the louder deck under the incoming music, instead of
	// abruptly dropping the old bed while its replacement is still near zero. Never three streams.
	const double Now = Ctx->CurrentTime();
	if (Outgoing && OutgoingGain && Gain && OutgoingGain->Gain.ValueAt(Now) > Gain->Gain.ValueAt(Now))
	{
		Swap(El, Outgoing); Swap(Gain, OutgoingGain);
	}
	RetireOutgoing(); ++Generation; bFading = false;
	if (El)
	{
		Outgoing = El; OutgoingGain = Gain;
		Outgoing->OnEnded = nullptr; Outgoing->OnError = nullptr;
		OutgoingGain->Gain.CancelAndHoldAtTime(Now);
		OutgoingGain->Gain.LinearRampToValueAtTime(0, Now + FadeSeconds);
		OutgoingUntil = Now + FadeSeconds;
	}
	const TSharedPtr<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValue(0);
	NewEl->Connect(G); G->Connect(Dest);
	G->Gain.SetValueAtTime(0, Now); G->Gain.LinearRampToValueAtTime(C->Gain, Now + FadeSeconds);
	RampUntil = Now + FadeSeconds; TargetGain = C->Gain;
	const TWeakPtr<FS47MediaSource> WeakEl = NewEl;
	NewEl->OnEnded = [this, Alive = bAlive, WeakEl] { if (*Alive && El == WeakEl.Pin()) Done(Rnd(4, 15)); };
	NewEl->OnError = [this, Alive = bAlive, WeakEl] { if (*Alive && El == WeakEl.Pin()) Done(120); };
	El = NewEl; Gain = G; Playing = Cue; Track = InTrack; Last.Add(InTrack, T);
	if (!NewEl->Play() && El == NewEl) Done(30);
	else if (bPaused) NewEl->Pause();
}

// Explicit fades and listening holds fade both decks. The generation protects replacements.
void FS47Score::Fade(double Secs, double Quiet)
{
	if (Cosmic) Cosmic->Set(FString(), true);
	if (!El || !Gain || bFading) return;
	Secs = FMath::IsFinite(Secs) ? FMath::Clamp(Secs, 0.01, 30.0) : 0.35;
	bFading = true; FadeQuiet = Quiet; Pending.Empty(); Candidate.Empty();
	const uint64 Token = ++Generation;
	const TSharedPtr<FS47MediaSource> OldEl = El;
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime();
	Gain->Gain.CancelAndHoldAtTime(Now); Gain->Gain.LinearRampToValueAtTime(0, Now + Secs);
	if (OutgoingGain) { OutgoingGain->Gain.CancelAndHoldAtTime(Now); OutgoingGain->Gain.LinearRampToValueAtTime(0, Now + Secs); OutgoingUntil = Now + Secs; }
	RampUntil = Now + Secs; TargetGain = 0;
	Ctx->SetTimeout(Secs + 0.1, [this, Alive = bAlive, OldEl, Token, Quiet]
	{
		if (*Alive && Generation == Token && El == OldEl && !bPaused) Done(Quiet);
	});
}

void FS47Score::Stop()
{
	if (Cosmic) { FS47AudioContext::FBatch Batch(*Ctx); Cosmic->Disconnect(); Cosmic.Reset(); }
	FS47AudioContext::FBatch B(*Ctx);
	++Generation;
	SilenceDeck(El, Gain); RetireOutgoing();
	Playing.Empty(); Pending.Empty(); Candidate.Empty(); RequestedMood.Empty(); RequestedSeverity = 0;
	bFading = false; bPaused = false; bRecovering = false; ActionUntil = RecoveryUntil = 0;
}

// Freeze both stream positions and their gain ramps, even when the context itself keeps running.
void FS47Score::Pause(bool bOn)
{
	if (bOn == bPaused) return;
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime();
	bPaused = bOn;
	if (Cosmic) Cosmic->Pause(bOn);
	if (bOn)
	{
		PauseAt = Now; PausedRampLeft = FMath::Max(0.0, RampUntil - Now); PausedOutgoingLeft = FMath::Max(0.0, OutgoingUntil - Now);
		if (El) El->Pause(); if (Outgoing) Outgoing->Pause();
		if (Gain) Gain->Gain.CancelAndHoldAtTime(Now); if (OutgoingGain) OutgoingGain->Gain.CancelAndHoldAtTime(Now);
	}
	else
	{
		PendingAt += Now - PauseAt; RampUntil = Now + PausedRampLeft; OutgoingUntil = Now + PausedOutgoingLeft;
		if (Gain) Gain->Gain.LinearRampToValueAtTime(TargetGain, Now + FMath::Max(0.001, PausedRampLeft));
		if (OutgoingGain) OutgoingGain->Gain.LinearRampToValueAtTime(0, Now + FMath::Max(0.001, PausedOutgoingLeft));
		if (El) El->Play(); if (Outgoing) Outgoing->Play();
		if (bFading)
		{
			const uint64 Token = ++Generation; const auto Current = El;
			Ctx->SetTimeout(PausedRampLeft + 0.1, [this, Alive = bAlive, Token, Current] { if (*Alive && Generation == Token && El == Current && !bPaused) Done(FadeQuiet); });
		}
	}
}

void FS47Score::Done(double Quiet)
{
	// A natural track gap keeps the same texture, seed and reference timeline.
	auto KeepCosmic = MoveTemp(Cosmic);
	Stop(); Cosmic = MoveTemp(KeepCosmic); Rest = Quiet;
}

// main.ts musicWant: keep these tests in the same order as the original.
FString FS47Score::MusicWant() const
{
	if (!M || !M->World) return FString();
	const bool Free = M->IsFreeform();
	if (!Free && M->Story && M->Story->ChapterKey == TEXT("ch6")) return FString();
	if (!RequestedMood.IsEmpty()) return RequestedMood;
	if (M->World->bDriving) return TEXT("drive");
	if (M->World->Area == TEXT("station01")) return TEXT("tension");
	if (M->World->Area == TEXT("room6")) return TEXT("motel");
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	if (M->World->Area == TEXT("saro") && M->World->AtMotel(P)) return TEXT("motel");
	// The town keeps its own musical identity, also during Freeform mornings.
	if (M->World->Area == TEXT("diner")) return M->World->RoadStopAt(P) ? TEXT("eerie") : TEXT("town");
	const double Clock = Free && M->Freeform ? M->Freeform->Clock() : M->Story ? M->Story->Clock() : 0;
	const double Hour = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400, 86400.0) / 3600;
	if (Hour >= 5 && Hour < 12) return TEXT("dawn");
	return M->Space == TEXT("yard") ? TEXT("night") : TEXT("eerie");
}

// main.ts musicHeld: the unported modules' properties are supplied through Held.
bool FS47Score::MusicHeld() const
{
	if (!M) return false;
	const AS47Story* Game = M->Story;
	const bool Free = M->IsFreeform();
	bool bPhaseHeld = false;
	if (!Free && Game && Game->ChapterKey == TEXT("prologue"))
	{
		const ES47Phase P = Game->Phase();
		bPhaseHeld = P == ES47Phase::Ringing || P == ES47Phase::Call || P == ES47Phase::Countdown || P == ES47Phase::Event || P == ES47Phase::Turning || P == ES47Phase::End;
	}
	// The processor owns this listening space. TAPE A keeps SARO quiet after its answer,
	// including Continue; its canonical night fact is already saved by Story/Flow.
	// The motel shares the SARO area, and driving has its own score: neither is held here.
	const bool bSaroListening = M->World && M->World->Area == TEXT("saro") && !M->World->bDriving
		&& !(M->Player && M->World->AtMotel(M->Player->Pos));
	const bool bDecoderHeld = Game && bSaroListening && ((!Free && Game->Night().bTapeA)
		|| (Game->Decoder && (Game->Decoder->Playing() || Game->Decoder->Answering())));
	const bool bFreeformListening = Free && M->Freeform && M->Freeform->AudioPlaying();
	const bool bReferenceListening = M->Audio && M->Audio->SoundMixPlaying(TEXT("receiver_reference"));
	// These Story layers do not advance in Freeform. Their saved quiet timers must
	// not suppress its outdoor score indefinitely. Audible sources still own holds.
	const bool bStoryHeld = !Free && ((Game && (Game->bConsoleOpen || Game->IsCinematic())) || bPhaseHeld
		|| On(Held.RoadsideSilent) || On(Held.MotelTvHeard) || On(Held.AbductionBusy) || On(Held.RecWalkBusy)
		|| Value(Held.FaceQuiet) > 0 || Value(Held.NinthQuiet) > 0 || On(Held.RoofStepsBusy) || On(Held.ReelTapeBusy)
		|| On(Held.SaroDarkBusy) || On(Held.OfficePhoneBusy) || On(Held.ArchiveBusy) || On(Held.ArrayBackBusy)
		|| On(Held.TruckBRacing) || On(Held.MorningBusy));
	return bDecoderHeld || bFreeformListening || bReferenceListening || bStoryHeld
		|| (M->Audio && M->Audio->ReceiverMonitorOn())
		|| (M->Audio && M->Audio->RadioOn() && M->Space != TEXT("yard")) || M->bRadioHeard
		|| (M->Audio && M->Audio->Night && M->Audio->Night->Breathing().IsSet());
}

bool FS47Score::SelfTest(TArray<FString>& Lines)
{
	bool bOk = FS47CosmicAmbient::SelfTest(Lines);
	auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s score: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bPass; };
	const TSharedRef<FS47AudioContext> C = FS47AudioContext::Make();
	FS47Score S(nullptr, C, C->Destination);
	Check(S47Score::TRACKS.Num() == 17 && S47Score::CUES.Num() == 7, TEXT("17 tracks and seven cues"));
	Check(S47Score::CUES.FindChecked(TEXT("night"))[0] == TEXT("cylEight") && S47Score::CUES.FindChecked(TEXT("town"))[1] == TEXT("docks"), TEXT("cue order"));
	Check(S.Rest >= 6 && S.Rest <= 14, TEXT("initial quiet 6 to 14 seconds"));
	{
		FS47Score Layer(nullptr, C, C->Destination);
		Layer.Update(.01, TEXT("night"), false);
		const auto First = Layer.Cosmic;
		Check(First.IsValid() && Layer.HasAudio() && Layer.Playing.IsEmpty(), TEXT("cosmic texture is counted as audio during a track gap"));
		Layer.Done(5);
		Check(Layer.Cosmic == First && Layer.Rest == 5, TEXT("natural deck completion preserves the cosmic texture"));
		Layer.Update(.01, TEXT("night"), false);
		Check(Layer.Cosmic == First, TEXT("repeated score frames reuse one cosmic node"));
		Layer.Stop();
		Check(!Layer.HasAudio() && !Layer.Cosmic, TEXT("full score stop releases cosmic audio even without a deck"));
	}
	S.Track = TEXT("nowhere");
	Check(S.Pick(TEXT("town")) == TEXT("docks"), TEXT("does not immediately repeat a piece"));
	S.Track.Empty(); S.Last.Add(TEXT("nowhere"), -1000); S.Last.Add(TEXT("docks"), 0);
	Check(S.Pick(TEXT("town")) == TEXT("nowhere"), TEXT("longest unheard wins beyond the random 120 seconds"));
	S.Rest = 1; S.Update(2, TEXT("night"), true);
	Check(S.Rest == 4 && S.T == 2 && S.Playing.IsEmpty(), TEXT("held music waits at least four seconds"));
	S.Rest = 2; S.Update(1, FString(), false);
	Check(S.Rest == 4 && S.T == 3, TEXT("no wanted cue also holds the quiet"));
	S.Done(15);
	Check(S.Rest == 15 && S.Track.IsEmpty() && S.Playing.IsEmpty(), TEXT("done sets the next quiet"));
	S.Track = TEXT("docks"); S.Stop();
	Check(S.Track == TEXT("docks") && S.Rest == 15, TEXT("stop keeps the last track and quiet"));
	bool bAllTracks = true;
	for (const auto& Cue : S47Score::CUES) for (const FString& K : Cue.Value) bAllTracks &= S47Score::TRACKS.Contains(K);
	Check(bAllTracks, TEXT("every cue references an existing track"));
	Check(!S.Pick(TEXT("calm")).IsEmpty() && !S.Pick(TEXT("action")).IsEmpty() && !S.Pick(TEXT("recovery")).IsEmpty(), TEXT("adaptive aliases reuse existing credited tracks"));
	Check(S.Pick(TEXT("unknown")).IsEmpty(), TEXT("unknown cues are rejected without a checked lookup"));
	Check(S.Timings.IsEmpty() && !S.SetTrackTiming(TEXT("neon"), 0) && !S.SetTrackTiming(TEXT("unknown"), 120), TEXT("timing defaults unknown and rejects fabricated invalid metadata"));
	US47Main* Main = NewObject<US47Main>(); Main->World = NewObject<US47World>(); Main->Space = TEXT("yard");
	FS47Score State(Main, C, C->Destination);
	Check(State.MusicWant() == TEXT("night"), TEXT("SARO yard chooses night"));
	Main->Space = TEXT("room");
	Check(State.MusicWant() == TEXT("eerie"), TEXT("SARO indoors chooses eerie"));
	Main->World->Area = TEXT("station01");
	Check(State.MusicWant() == TEXT("tension"), TEXT("station overrides the acoustic space"));
	Main->World->Area = TEXT("room6");
	Check(State.MusicWant() == TEXT("motel"), TEXT("room six chooses motel"));
	Main->World->bDriving = true;
	Check(State.MusicWant() == TEXT("drive"), TEXT("driving takes precedence over the area"));
	Main->World->bDriving = false; Main->World->Area = TEXT("diner");
	Check(State.MusicWant() == TEXT("town"), TEXT("diner chooses town away from a road stop"));
	Check(!State.MusicHeld(), TEXT("ordinary play leaves the music free"));
	State.RequestMood(TEXT("action"), 1);
	Check(State.MusicWant() == TEXT("action"), TEXT("observable requested danger overrides the location cue"));
	State.Update(0, FString(), true);
	Check(State.MusicWant() == TEXT("town"), TEXT("mood requests expire after one update"));
	Main->bRadioHeard = true;
	Check(State.MusicHeld(), TEXT("an audible receiver holds the score"));
	Main->bRadioHeard = false; State.Held.FaceQuiet = [] { return 2.0; };
	Check(State.MusicHeld(), TEXT("the album's face quiet hook holds the score"));
	State.Held.FaceQuiet = [] { return 0.0; }; State.Held.MorningBusy = [] { return true; };
	Check(State.MusicHeld(), TEXT("the morning's quiet hook holds the score"));
	State.Held.MorningBusy = [] { return false; };
	Main->SessionMode=ES47SessionMode::Freeform;Main->Freeform=MakeShared<FS47FreeformSession>(Main);
	Main->World->Area=TEXT("saro");Main->Space=TEXT("room");
	Check(State.MusicWant()==TEXT("dawn")&&!State.MusicHeld(),TEXT("Freeform uses its own morning clock and keeps ambient music enabled"));
	State.Held.FaceQuiet=[] { return 47.; };State.Held.RoadsideSilent=[] { return true; };State.Held.MorningBusy=[] { return true; };
	Main->Space=TEXT("yard");
	Check(State.MusicWant()==TEXT("dawn")&&!State.MusicHeld(),TEXT("Freeform outdoors ignores inactive Story silence timers"));
	Main->World->Area=TEXT("diner");
	Check(State.MusicWant()==TEXT("town")&&!State.MusicHeld(),TEXT("Freeform town music takes precedence over morning ambience"));
	Main->World->bDriving=true;
	Check(State.MusicWant()==TEXT("drive")&&!State.MusicHeld(),TEXT("Freeform driving without audible radio keeps its score"));
	Main->bRadioHeard=true;
	Check(State.MusicHeld(),TEXT("Freeform audible car radio still holds the score"));
	Main->bRadioHeard=false;
	Check(!State.MusicHeld(),TEXT("Freeform radio off releases the driving score"));
	Main->SessionMode=ES47SessionMode::Story;
	Check(State.MusicHeld(),TEXT("Story retains its narrative silence hooks"));
	// Optional content-backed checks use the real imported recording through the context, with
	// half a second rendered in total. Import-free self tests keep the checks above unchanged.
	const FString MediaPath = FPaths::ProjectContentDir() / TEXT("Audio/Raw/media/music/neon.wav");
	if (IFileManager::Get().FileExists(*MediaPath))
	{
		const TSharedRef<FS47AudioContext> MediaCtx = FS47AudioContext::Make();
		FS47Score Recorded(nullptr, MediaCtx, MediaCtx->Destination);
		Recorded.Start(TEXT("drive"), TEXT("neon"));
		Check(Recorded.El.IsValid() && Recorded.Gain.IsValid() && Recorded.Playing == TEXT("drive") && Recorded.Track == TEXT("neon"), TEXT("imported recording starts through the score graph"));
		if (Recorded.El && Recorded.Gain)
		{
			const double Now = MediaCtx->CurrentTime();
			Check(FMath::Abs(Recorded.Gain->Gain.ValueAt(Now)) < 1e-9 && FMath::Abs(Recorded.Gain->Gain.ValueAt(Now + 3) - 0.18) < 1e-9 && FMath::Abs(Recorded.Gain->Gain.ValueAt(Now + 6) - 0.36) < 1e-9, TEXT("start ramps from zero to the track gain in exactly six seconds"));
			TArray<float> Samples; Samples.SetNumZeroed(24000);
			MediaCtx->Render(Samples.GetData(), 11904);
			const double At = Recorded.El->CurrentTime();
			Recorded.Pause(true); MediaCtx->Render(Samples.GetData(), 6016);
			Check(At > 0 && Recorded.El->Paused() && FMath::Abs(Recorded.El->CurrentTime() - At) < 1e-9, TEXT("pause holds the exact media position while the context runs"));
			Recorded.Pause(false); MediaCtx->Render(Samples.GetData(), 6016);
			Check(!Recorded.El->Paused() && FMath::Abs(Recorded.El->CurrentTime() - At - 6016.0 / MediaCtx->SampleRate) < 1e-9, TEXT("resume continues from the held media position"));
			const TSharedPtr<FS47MediaSource> First = Recorded.El;
			const double FadeAt = MediaCtx->CurrentTime();
			Recorded.Fade(2.5, 6); Recorded.Fade(0.1, 999);
			Check(Recorded.bFading && Recorded.Gain->Gain.ValueAt(FadeAt + 0.1) > 0 && FMath::Abs(Recorded.Gain->Gain.ValueAt(FadeAt + 2.5)) < 1e-9, TEXT("fade is guarded once and keeps its first duration"));
			MediaCtx->Tick(2.59);
			Check(Recorded.El == First && Recorded.Playing == TEXT("drive"), TEXT("fade retains the source through its 100 ms completion margin"));
			MediaCtx->Tick(0.02);
			Check(!Recorded.El && Recorded.Playing.IsEmpty() && !Recorded.bFading && Recorded.Rest == 6 && Recorded.Track == TEXT("neon"), TEXT("fade completion stops the source and sets the requested quiet"));
			Recorded.Start(TEXT("drive"), TEXT("neon"));
			const TSharedPtr<FS47MediaSource> Old = Recorded.El;
			Recorded.Fade(0.2, 123);
			Recorded.Start(TEXT("drive"), TEXT("neon"));
			const TSharedPtr<FS47MediaSource> Replacement = Recorded.El;
			MediaCtx->Tick(0.31);
			Check(Replacement.IsValid() && Replacement != Old && Recorded.El == Replacement && Recorded.Playing == TEXT("drive") && !Recorded.bFading && Recorded.Rest == 6, TEXT("a stale fade cannot stop the replacement piece or replace its quiet"));
		}
		// Actual two-deck streaming, not just a gain estimate. Optional when music is imported.
		const FString ActionPath = FPaths::ProjectContentDir() / TEXT("Audio/Raw/media/music/the_old_ones.wav");
		if (IFileManager::Get().FileExists(*ActionPath) && Recorded.El && Recorded.Gain)
		{
			Recorded.Gain->Gain.CancelScheduledValues(MediaCtx->CurrentTime()); Recorded.Gain->Gain.SetValue(0.36);
			Recorded.Start(TEXT("action"), TEXT("oldOnes"));
			const double At = MediaCtx->CurrentTime();
			Check(Recorded.ActiveDecks() == 2 && Recorded.Outgoing && Recorded.Gain && Recorded.OutgoingGain
				&& Recorded.Gain->Gain.ValueAt(At + 1.5) > 0 && Recorded.OutgoingGain->Gain.ValueAt(At + 1.5) > 0,
				TEXT("crossfade overlaps two actual recording decks with both gains positive"));
			Recorded.Pause(true);
			const double InAt = Recorded.El->CurrentTime(), OutAt = Recorded.Outgoing->CurrentTime();
			TArray<float> Samples; Samples.SetNumZeroed(12032);
			MediaCtx->Render(Samples.GetData(), 6016);
			Check(Recorded.El->Paused() && Recorded.Outgoing->Paused() && Recorded.El->CurrentTime() == InAt && Recorded.Outgoing->CurrentTime() == OutAt,
				TEXT("pause freezes both overlapping media positions"));
			Recorded.Pause(false); MediaCtx->Render(Samples.GetData(), 6016);
			bool bFinite = true; double Energy = 0;
			for (float V : Samples) { bFinite &= FMath::IsFinite(V); Energy += V * V; }
			Check(bFinite && Energy > 0, TEXT("overlapping imported media produces actual finite nonzero samples"));
			const auto SamePiece = Recorded.El;
			Recorded.Begin(TEXT("eerie"), TEXT("oldOnes"), 2);
			Check(Recorded.El == SamePiece && Recorded.Playing == TEXT("eerie"), TEXT("same recording changes mood without restarting its intro"));
			Check(Recorded.TransitionAt(false) == MediaCtx->CurrentTime(), TEXT("unverified tracks transition in free time"));
			Recorded.SetTrackTiming(TEXT("oldOnes"), 120, 4, 4); Recorded.El->Seek(0.25);
			Check(FMath::Abs(Recorded.TransitionAt(false) - MediaCtx->CurrentTime() - 1.75) < 1e-9
				&& FMath::Abs(Recorded.TransitionAt(true) - MediaCtx->CurrentTime() - 0.25) < 1e-9,
				TEXT("isolated verified-timing fixture schedules next bar or urgent beat"));
			Recorded.Timings.Empty(); // Fixture values are not claimed as this recording's BPM.
			Recorded.Update(0.1, TEXT("action"), false); const auto ActionDeck = Recorded.El;
			Recorded.Update(5.9, TEXT("calm"), false);
			Check(Recorded.El == ActionDeck && Recorded.Playing == TEXT("action"), TEXT("danger exit is held for six quiet seconds"));
			Recorded.Update(0.2, TEXT("calm"), false); Recorded.Update(0.11, TEXT("calm"), false);
			Check(Recorded.Playing == TEXT("recovery") && Recorded.ActiveDecks() == 2, TEXT("sustained safety crossfades action into recovery"));
			const auto RecoveryDeck = Recorded.El; Recorded.Update(7, TEXT("calm"), false);
			Check(Recorded.El == RecoveryDeck && Recorded.Playing == TEXT("recovery"), TEXT("recovery holds eight seconds instead of chattering between states"));
			Recorded.RequestMood(TEXT("action"), 1); Recorded.Update(0.01, TEXT("action"), true);
			const double HeldAt = MediaCtx->CurrentTime();
			Check(Recorded.Pending.IsEmpty() && Recorded.bFading && Recorded.Gain->Gain.ValueAt(HeldAt + 0.35) == 0
				&& (!Recorded.OutgoingGain || Recorded.OutgoingGain->Gain.ValueAt(HeldAt + 0.35) == 0), TEXT("listening hold cancels pending danger and rapidly fades both decks"));
			MediaCtx->Tick(0.46);
			Check(Recorded.ActiveDecks() == 0 && Recorded.Playing.IsEmpty(), TEXT("held score cannot resurrect a scheduled incoming cue"));
		}

		Recorded.Stop(); MediaCtx->Close();
	}
	C->Close();
	return bOk;
}
