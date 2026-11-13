#include "S47Score.h"
#include "S47CosmicAmbient.h"

#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47DecoderDesk.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Media.h"
#include "S47ScoreStems.h"
#include "S47Story.h"
#include "S47Taken.h"
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
		{ TEXT("cylTwo"), { TEXT("cylinder_two.mp3"), TEXT("Cylinder Two"), TEXT("Chris Zabriskie"), 0.42 } },
		{ TEXT("cylEight"), { TEXT("cylinder_eight.mp3"), TEXT("Cylinder Eight"), TEXT("Chris Zabriskie"), 0.4 } },
		{ TEXT("floating"), { TEXT("floating_cities.mp3"), TEXT("Floating Cities"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("ossuary1"), { TEXT("ossuary_1.mp3"), TEXT("Ossuary 1 - A Beginning"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("ossuary5"), { TEXT("ossuary_5.mp3"), TEXT("Ossuary 5 - Rest"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("longNote"), { TEXT("long_note_four.mp3"), TEXT("Long Note Four"), TEXT("Kevin MacLeod"), 0.5 } },
		{ TEXT("lightless"), { TEXT("lightless_dawn.mp3"), TEXT("Lightless Dawn"), TEXT("Kevin MacLeod"), 0.45 } },
		{ TEXT("phantom"), { TEXT("phantom_from_space.mp3"), TEXT("Phantom from Space"), TEXT("Kevin MacLeod"), 0.4 } },
		// MUSIC-20261113 (Tom, 13 Nov: take the jazz out, and the piano piece at night too): "Night on the Docks (Sax)" (docks, the
		// town's) and "Cylinder One" (cylOne, eerie and the motel: the one solo keyboard piece, single ringing notes in slow broken
		// chords) are out of the table; their files stay where they are. In their place, all optional (never in Git, staged by
		// Tools/music/build_music.py at -21.5 LUFS like the rest): steel guitar for the town and the road, two analog drones,
		// Tom's own "Intro Drive", and four pieces arranged for this game from the bought 1984 Retro and ABYSS loops.
		{ TEXT("lookBack"), { TEXT("look_back_steelguitar.mp3"), TEXT("Look Back Steelguitar Organ (F 35)"), TEXT("Lobo Loco"), 0.4, true } },
		{ TEXT("stumping"), { TEXT("stumping_slow_deltablues.mp3"), TEXT("Stumping Slow Deltablues - Guitar Steelguitar (F 30)"), TEXT("Lobo Loco"), 0.4, true } },
		{ TEXT("standby"), { TEXT("standby_mode.mp3"), TEXT("Standby Mode"), TEXT("Uuriter"), 0.45, true } },
		{ TEXT("routine"), { TEXT("routine_drone.mp3"), TEXT("Routine Drone"), TEXT("Uuriter"), 0.45, true } },
		{ S47Score::IntroDriveId, { TEXT("signal47_intro_drive.mp3"), TEXT("Intro Drive"), TEXT("Tom"), 0.38, true } },
		{ TEXT("nightFreight"), { TEXT("s47_night_freight.mp3"), TEXT("Night Freight"), TEXT("SIGNAL / 47"), 0.38, true } },
		{ TEXT("coldDish"), { TEXT("s47_cold_dish.mp3"), TEXT("Cold Dish"), TEXT("SIGNAL / 47"), 0.45, true } },
		{ TEXT("carrierLock"), { TEXT("s47_carrier_lock.mp3"), TEXT("Carrier Lock"), TEXT("SIGNAL / 47"), 0.42, true } },
		{ TEXT("countyRoad"), { TEXT("s47_county_road.mp3"), TEXT("County Road"), TEXT("SIGNAL / 47"), 0.36, true } },
		// (Tom, 13 Nov, after hearing those four: "Prøv å mikse sammen de andre musikk-loopene med 1980s-loopene, noe ambient,
		// skummelt men rolig.") Five beds without drums: the 1984 bundle's synth loops far inside the dark ones (ABYSS, Ambience
		// SciFi). All at -21.5 LUFS; the gains put each at its pool's median level (file loudness plus gain, measured 13 Nov:
		// eerie and the motel -28.4 LUFS, the night -28.7, the dawn -28.8), the still one a little under.
		{ TEXT("thirdShift"), { TEXT("s47_third_shift.mp3"), TEXT("Third Shift"), TEXT("SIGNAL / 47"), 0.45, true } },
		{ TEXT("dryLake"), { TEXT("s47_dry_lake.mp3"), TEXT("Dry Lake"), TEXT("SIGNAL / 47"), 0.43, true } },
		{ TEXT("vacancy"), { TEXT("s47_vacancy.mp3"), TEXT("Vacancy"), TEXT("SIGNAL / 47"), 0.45, true } },
		{ TEXT("greyHour"), { TEXT("s47_grey_hour.mp3"), TEXT("Grey Hour"), TEXT("SIGNAL / 47"), 0.43, true } },
		{ TEXT("standingWave"), { TEXT("s47_standing_wave.mp3"), TEXT("Standing Wave"), TEXT("SIGNAL / 47"), 0.42, true } },
	};
	const TMap<FString, TArray<FString>> CUES = {
		// (every pool starts with the pieces whose files are in Git; the optional ones follow)
		{ TEXT("night"), { TEXT("cylEight"), TEXT("floating"), TEXT("ossuary1"), TEXT("longNote"), TEXT("lightless"), TEXT("shadows"), TEXT("phantom"), TEXT("cylTwo"), TEXT("standby"), TEXT("coldDish"), TEXT("dryLake") } },
		// ("Standing Wave" is the still one, made for the archive and for waiting: SARO indoors is this pool, the archive with it.
		// The title screen keeps Tom's own piece, and a pause freezes the score where it is.)
		{ TEXT("eerie"), { TEXT("ossuary5"), TEXT("decoherence"), TEXT("longNote"), TEXT("standby"), TEXT("routine"), TEXT("coldDish"), TEXT("thirdShift"), TEXT("standingWave") } },
		{ TEXT("tension"), { TEXT("oldOnes"), TEXT("longNote"), TEXT("lightless"), TEXT("phantom"), TEXT("routine"), TEXT("carrierLock") } },
		{ TEXT("motel"), { TEXT("solitude"), TEXT("ossuary5"), TEXT("cylTwo"), TEXT("vacancy") } },
		{ TEXT("drive"), { TEXT("neon"), TEXT("cylTwo"), TEXT("floating"), TEXT("cylEight"), TEXT("lookBack"), TEXT("stumping"), S47Score::IntroDriveId, TEXT("countyRoad"), TEXT("nightFreight") } },
		// Tom 13 November (d67): "Signs To Nowhere" was jazz too and is gone from the town. Without the staged steel guitar the town
		// has "Neon" alone, with the score's own quiet between its turns.
		{ TEXT("town"), { TEXT("neon"), TEXT("lookBack"), TEXT("stumping") } },
		{ TEXT("dawn"), { TEXT("hymn"), TEXT("cylEight"), TEXT("floating"), TEXT("greyHour") } },
	};
	bool MediaPresent(const FString& WebName)
	{
		return IFileManager::Get().FileExists(*(FPaths::ProjectContentDir() / TEXT("Audio/Raw/media") / FPaths::ChangeExtension(WebName, TEXT("wav"))));
	}
	FString MediaOr(const FString& WebName, const FString& Fallback) { return MediaPresent(WebName) ? WebName : Fallback; }
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
	: M(InM), Ctx(InCtx), Dest(InDest), Rest(Rnd(6, 14))
{
	FS47AudioContext::FBatch Batch(*Ctx);
	Bus = Ctx->CreateGain(); Bus->Gain.SetValue(1); Bus->Connect(Dest);
	Mix = Ctx->CreateGain(); Mix->Connect(Bus);
}

void FS47Score::Insert(const TSharedPtr<FS47AudioNode>& In, const TSharedPtr<FS47AudioNode>& Out)
{
	if (!Mix || !Bus) return;
	FS47AudioContext::FBatch Batch(*Ctx);
	if (InsertIn) Mix->Disconnect(InsertIn);
	if (InsertOut) InsertOut->Disconnect(Bus);
	Mix->Disconnect(Bus);
	InsertIn = In.IsValid() && Out.IsValid() ? In : nullptr; InsertOut = InsertIn ? Out : nullptr;
	if (InsertIn) { Mix->Connect(InsertIn); InsertOut->Connect(Bus); }
	else Mix->Connect(Bus);
}

void FS47Score::SetStems(bool bOn, double InTension)
{
	bStemsOn = bOn; Tension = FMath::IsFinite(InTension) ? FMath::Clamp(InTension, 0., 1.) : 0.;
	// (with the stems switched off while a piece plays as stems, all of them come back in: it is the mix again)
	if (Stems && !bPaused) Stems->Step(bStemsOn ? Tension : 0.);
}

int32 FS47Score::StemsPlaying() const { return Stems.IsValid() ? Stems->Num() : 0; }

FString FS47Score::PlayingFile() const
{
	const FS47ScoreTrack* C = El.IsValid() ? S47Score::TRACKS.Find(Track) : nullptr;
	return C ? C->File : FString();
}

FS47Score::~FS47Score()
{
	*bAlive = false;
	StopNow();
	Insert(nullptr, nullptr);
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

FString FS47Score::NowPlaying() const
{
	const FS47ScoreTrack* C = El && !Track.IsEmpty() ? S47Score::TRACKS.Find(Track) : nullptr;
	return C ? FString::Printf(TEXT("%s (%s)"), *C->Title, *C->By) : FString();
}

FString FS47Score::TrackOf(const FString& Piece)
{
	if (S47Score::TRACKS.Contains(Piece)) return Piece;
	const FString Want = FPaths::GetBaseFilename(Piece);
	for (const TPair<FString, FS47ScoreTrack>& T : S47Score::TRACKS) if (FPaths::GetBaseFilename(T.Value.File) == Want) return T.Key;
	return FString();
}

FString FS47Score::CueOf(const FString& InTrack)
{
	for (const TCHAR* Cue : { TEXT("night"), TEXT("eerie"), TEXT("tension"), TEXT("motel"), TEXT("drive"), TEXT("town"), TEXT("dawn") })
		if (const TArray<FString>* Pool = S47Score::CUES.Find(Cue); Pool && Pool->Contains(InTrack)) return Cue;
	return FString();
}

bool FS47Score::Own(const FString& Piece)
{
	if (Piece.IsEmpty())
	{
		if (!OwnTrack.IsEmpty()) UE_LOG(LogTemp, Display, TEXT("S47: score: the render lets go of %s"), *OwnTrack);
		OwnTrack.Empty(); OwnCue.Empty(); bOwnStemsTried = false;
		return true;
	}
	const FString Key = TrackOf(Piece), Cue = CueOf(Key);
	if (Key.IsEmpty() || Cue.IsEmpty() || !Present(Key) || !S47Score::bScoreOn)
	{
		UE_LOG(LogTemp, Warning, TEXT("S47: score: the render asked for %s, which cannot play (%s)"), *Piece, Key.IsEmpty() ? TEXT("no such piece") : TEXT("its file is not staged"));
		return false;
	}
	OwnTrack.Empty(); // (so that the stop below is not held)
	if (bPaused) Pause(false);
	if (El && Track == Key && Playing == Cue && !(bStemsOn && !Stems)) { OwnTrack = Key; OwnCue = Cue; return true; }
	StopNow();
	Begin(Cue, Key, .05);
	OwnTrack = Key; OwnCue = Cue; bOwnStemsTried = bStemsOn;
	UE_LOG(LogTemp, Display, TEXT("S47: score: the render owns the score: %s (%s), %s"), *Key, *Cue, Stems ? TEXT("as its stems") : TEXT("as its mix"));
	return El.IsValid();
}

void FS47Score::Update(double Dt, const FString& InWant, bool bHeld)
{
	if (!OwnTrack.IsEmpty())
	{
		// SIDEBAND-20261113: the render's piece, whatever the night asks; once more through the stems if they came on after it began
		if (bPaused) Pause(false);
		if (!El || Track != OwnTrack || (bStemsOn && !Stems && !bOwnStemsTried))
		{
			bOwnStemsTried = bStemsOn;
			const FString Key = OwnTrack; OwnTrack.Empty(); StopNow(); Begin(OwnCue, Key, .05); OwnTrack = Key;
			UE_LOG(LogTemp, Display, TEXT("S47: score: the render's piece %s again, %s"), *Key, Stems ? TEXT("as its stems") : TEXT("as its mix"));
		}
		RequestedMood.Empty(); RequestedSeverity = 0;
		return;
	}
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
	// MUSIC-20261113: an optional piece is in the pool only while its file is there
	TArray<FString> List;
	for (const FString& K : *Found) if (Present(K)) List.Add(K);
	if (List.IsEmpty()) return FString();
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

bool FS47Score::Present(const FString& InTrack) const
{
	const FS47ScoreTrack* C = S47Score::TRACKS.Find(InTrack);
	if (!C) return false;
	if (!C->bOptional) return true;
	return HasFile ? HasFile(*C) : S47Score::MediaPresent(TEXT("music/") + C->File);
}

// Start remains the named developer entry point; normal state changes use the same two decks.
void FS47Score::Start(const FString& Cue, const FString& InTrack)
{
	if (!S47Score::bScoreOn) return;
	if (!OwnTrack.IsEmpty()) { UE_LOG(LogTemp, Display, TEXT("S47: score: %s held off: the render owns the score"), *Cue); return; }
	const FString Next = InTrack.IsEmpty() ? Pick(Cue) : InTrack;
	if (!S47Score::TRACKS.Contains(Next) || !KnownCue(Cue)) return;
	if (El && Track == Next) Stop(); // Explicit named replay; adaptive Begin keeps its position.
	Pending.Empty(); Candidate.Empty();
	Begin(Cue, Next, El ? 3.0 : 6.0);
}

void FS47Score::RetireOutgoing()
{
	FS47AudioContext::FBatch B(*Ctx);
	SilenceDeck(Outgoing, OutgoingGain); OutgoingUntil = 0; OutgoingCue.Empty();
	if (OutgoingStems) { OutgoingStems->Close(); OutgoingStems.Reset(); }
}

void FS47Score::Begin(const FString& Cue, const FString& InTrack, double FadeSeconds)
{
	const FS47ScoreTrack* C = S47Score::TRACKS.Find(InTrack);
	if (!C || !Dest) return;
	// A piece may suit two moods. Recategorize it without replaying its intro.
	// Explicit developer Start still restarts because it calls Stop first.
	if (El && Track == InTrack) { Playing = Cue; return; }
	// SIDEBAND-20261113: an own mix as its stems when they are staged (the first of them leads, as El); else the mix file
	const TSharedPtr<FS47ScoreStems> NewStems = bStemsOn ? FS47ScoreStems::Open(Ctx, C->File, Tension) : nullptr;
	const TSharedPtr<FS47MediaSource> NewEl = NewStems ? NewStems->Leader() : FS47MediaSource::Open(Ctx, TEXT("music/") + C->File);
	if (!NewEl) { if (!El) Rest = 120; return; }
	FS47AudioContext::FBatch B(*Ctx);
	// An interrupted transition keeps the louder deck under the incoming music, instead of
	// abruptly dropping the old bed while its replacement is still near zero. Never three streams.
	const double Now = Ctx->CurrentTime();
	if (Outgoing && OutgoingGain && Gain && OutgoingGain->Gain.ValueAt(Now) > Gain->Gain.ValueAt(Now))
	{
		Swap(El, Outgoing); Swap(Gain, OutgoingGain); Swap(Playing, OutgoingCue); // Playing is set again below
		Swap(Stems, OutgoingStems);
	}
	RetireOutgoing(); ++Generation; bFading = false;
	if (El)
	{
		Outgoing = El; OutgoingGain = Gain; OutgoingCue = Playing; OutgoingStems = Stems;
		Outgoing->OnEnded = nullptr; Outgoing->OnError = nullptr;
		OutgoingGain->Gain.CancelAndHoldAtTime(Now);
		OutgoingGain->Gain.LinearRampToValueAtTime(0, Now + FadeSeconds);
		OutgoingUntil = Now + FadeSeconds;
	}
	const TSharedPtr<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValue(0);
	if (NewStems) NewStems->Out()->Connect(G); else NewEl->Connect(G);
	G->Connect(Mix);
	Stems = NewStems;
	G->Gain.SetValueAtTime(0, Now); G->Gain.LinearRampToValueAtTime(C->Gain, Now + FadeSeconds);
	RampUntil = Now + FadeSeconds; TargetGain = C->Gain;
	const TWeakPtr<FS47MediaSource> WeakEl = NewEl;
	NewEl->OnEnded = [this, Alive = bAlive, WeakEl] { if (*Alive && El == WeakEl.Pin()) Done(Rnd(4, 15)); };
	NewEl->OnError = [this, Alive = bAlive, WeakEl] { if (*Alive && El == WeakEl.Pin()) Done(120); };
	El = NewEl; Gain = G; Playing = Cue; Track = InTrack; Last.Add(InTrack, T);
	// MUSIC-20261113 (Tom heard a soft piano at night and could not name it): every piece change, with the clock and the cue
	{
		const double Clock = M && M->IsFreeform() && M->Freeform ? M->Freeform->Clock() : M && M->Story ? M->Story->Clock() : -1;
		const double Hour = Clock >= 0 ? FMath::Fmod(Clock, 86400.) / 3600. : -1;
		UE_LOG(LogTemp, Display, TEXT("S47: score: now playing \"%s\" by %s (%s, cue %s%s), game clock %s"), *C->Title, *C->By, *InTrack, *Cue, NewStems ? TEXT(", as its stems") : TEXT(""),
			Hour < 0 ? TEXT("none") : *FString::Printf(TEXT("%02d:%02d"), int32(Hour), int32(FMath::Fmod(Hour * 60., 60.))));
	}
	if (!NewEl->Play() && El == NewEl) Done(30);
	else if (bPaused) NewEl->Pause();
}

// Explicit fades and listening holds fade both decks. The generation protects replacements.
void FS47Score::Fade(double Secs, double Quiet)
{
	if (!OwnTrack.IsEmpty()) { UE_LOG(LogTemp, Display, TEXT("S47: score: a fade of %.1f s held off: the render owns the score"), Secs); return; }
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
	if (!OwnTrack.IsEmpty()) { UE_LOG(LogTemp, Display, TEXT("S47: score: a stop held off: the render owns the score")); return; }
	StopNow();
}

void FS47Score::StopNow()
{
	if (Cosmic) { FS47AudioContext::FBatch Batch(*Ctx); Cosmic->Disconnect(); Cosmic.Reset(); }
	FS47AudioContext::FBatch B(*Ctx);
	++Generation;
	SilenceDeck(El, Gain); RetireOutgoing();
	if (Stems) { Stems->Close(); Stems.Reset(); }
	Playing.Empty(); Pending.Empty(); Candidate.Empty(); RequestedMood.Empty(); RequestedSeverity = 0;
	bFading = false; bPaused = false; bRecovering = false; ActionUntil = RecoveryUntil = 0;
}

// Freeze both stream positions and their gain ramps, even when the context itself keeps running.
void FS47Score::Pause(bool bOn)
{
	if (bOn == bPaused) return;
	if (bOn && !OwnTrack.IsEmpty()) { UE_LOG(LogTemp, Display, TEXT("S47: score: a pause held off: the render owns the score")); return; }
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
	const bool WasPaused = bPaused;
	auto KeepCosmic = MoveTemp(Cosmic);
	StopNow(); Cosmic = MoveTemp(KeepCosmic); bPaused = WasPaused; Rest = Quiet;
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
double FS47Score::MusicDuck() const
{
	// Tom 5 Nov: the music stays faintly there at the receiver PC, under the search and signal.
	if (!M || !M->IsFreeform() || !M->Freeform) return 1.;
	return M->Freeform->TerminalOnGlass() && M->Freeform->ActiveTerminalScreen() == 1 ? .22 : 1.;
}

bool FS47Score::PlaceCue(const FString& Cue)
{
	return Cue == TEXT("town") || Cue == TEXT("dawn") || Cue == TEXT("motel") || Cue == TEXT("drive");
}

bool FS47Score::PlacePieceSounding() const
{
	return (El.IsValid() && PlaceCue(Playing)) || (Outgoing.IsValid() && PlaceCue(OutgoingCue));
}

double FS47Score::DreadDuck(bool bPlacePiece, double Dread, double Heard)
{
	// Critics, 10 Nov: the bought ground under a meeting is the music then (Tom's idea); the score stands back to a third.
	// D45-20261110: a place's own piece goes all but under it, and already as something is heard coming.
	const double D = FMath::IsFinite(Dread) ? FMath::Clamp(Dread, 0., 1.) : 0.;
	const double H = FMath::IsFinite(Heard) ? FMath::Clamp(Heard, 0., 1.) : 0.;
	return FMath::Clamp(bPlacePiece ? 1. - FMath::Max(.95 * D, .8 * H) : 1. - FMath::Max(.66 * D, .5 * H), 0., 1.); // (both close, so the end of a crossfade does not swell)
}

bool FS47Score::TakenHolds(bool bFree, int32 Stage)
{
	return bFree && (Stage == int32(FS47Taken::EStage::Flood) || Stage == int32(FS47Taken::EStage::Lift));
}

void FS47Score::SetDuck(double Level)
{
	if (!Bus) return;
	FS47AudioContext::FBatch Batch(*Ctx);
	Bus->Gain.SetTargetAtTime(FMath::Clamp(Level, 0., 1.), Ctx->CurrentTime(), .25);
}

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
	// D45-20261110: the take owns the room in its flood and its lift: the score fades in a third of a second and waits.
	const bool bTakenHeld = Free && M->Freeform && M->Freeform->Taken && TakenHolds(Free, int32(M->Freeform->Taken->Stage()));
	const bool bReferenceListening = M->Audio && M->Audio->SoundMixPlaying(TEXT("receiver_reference"));
	// These Story layers do not advance in Freeform. Their saved quiet timers must
	// not suppress its outdoor score indefinitely. Audible sources still own holds.
	const bool bStoryHeld = !Free && ((Game && (Game->bConsoleOpen || Game->IsCinematic())) || bPhaseHeld
		|| On(Held.RoadsideSilent) || On(Held.MotelTvHeard) || On(Held.AbductionBusy) || On(Held.RecWalkBusy)
		|| Value(Held.FaceQuiet) > 0 || Value(Held.NinthQuiet) > 0 || On(Held.RoofStepsBusy) || On(Held.ReelTapeBusy)
		|| On(Held.SaroDarkBusy) || On(Held.OfficePhoneBusy) || On(Held.ArchiveBusy) || On(Held.ArrayBackBusy)
		|| On(Held.TruckBRacing) || On(Held.MorningBusy));
	return bDecoderHeld || bFreeformListening || bTakenHeld || bReferenceListening || bStoryHeld
		|| (!Free && M->Audio && M->Audio->ReceiverMonitorOn())
		|| (M->Audio && M->Audio->RadioOn() && M->Space != TEXT("yard")) || M->bRadioHeard
		|| (M->Audio && M->Audio->Night && M->Audio->Night->Breathing().IsSet());
}

bool FS47Score::SelfTest(TArray<FString>& Lines)
{
	bool bOk = FS47CosmicAmbient::SelfTest(Lines);
	auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s score: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bPass; };
	const TSharedRef<FS47AudioContext> C = FS47AudioContext::Make();
	FS47Score S(nullptr, C, C->Destination);
	Check(S47Score::TRACKS.Num() == 28 && S47Score::CUES.Num() == 7 && !S47Score::TRACKS.Contains(TEXT("nowhere")), TEXT("28 tracks and seven cues; the town's jazz is not among them"));
	// SIDEBAND-20261113: the render names its piece by key or by file, and plays it in its first pool
	Check(TrackOf(TEXT("s47_cold_dish")) == TEXT("coldDish") && TrackOf(TEXT("coldDish")) == TEXT("coldDish") && TrackOf(TEXT("nothing_such")).IsEmpty() && CueOf(TEXT("coldDish")) == TEXT("night")
		&& !S.Owned() && !S.Own(TEXT("nothing_such")) && !S.Owned(), TEXT("a render's piece by key or file name, in its first pool; an unknown one is refused and owns nothing"));
	Check(S47Score::CUES.FindChecked(TEXT("night"))[0] == TEXT("cylEight") &&S47Score::CUES.FindChecked(TEXT("town"))[0] == TEXT("neon") && S47Score::CUES.FindChecked(TEXT("town"))[1] == TEXT("lookBack"), TEXT("cue order"));
	// MUSIC-20261113: the jazz and the night's solo keyboard are gone from the table and every pool; the pools stand without the optional files
	{
		bool bGone = !S47Score::TRACKS.Contains(TEXT("docks")) && !S47Score::TRACKS.Contains(TEXT("cylOne")), bTwo = true, bInGit = true, bFilesOnce = true, bOptionalLast = true;
		TSet<FString> Files;
		for (const auto& Tr : S47Score::TRACKS)
		{
			bGone &= !Tr.Value.File.Contains(TEXT("night_on_the_docks")) && !Tr.Value.File.Contains(TEXT("cylinder_one"));
			bFilesOnce &= !Files.Contains(Tr.Value.File) && Tr.Value.Gain > 0.2 && Tr.Value.Gain <= 0.5 && !Tr.Value.Title.IsEmpty() && !Tr.Value.By.IsEmpty();
			Files.Add(Tr.Value.File);
		}
		for (const auto& Cue : S47Score::CUES)
		{
			int32 InGit = 0; bool bSeenOptional = false; TSet<FString> Seen;
			for (const FString& K : Cue.Value)
			{
				const FS47ScoreTrack* Tr = S47Score::TRACKS.Find(K);
				bGone &= K != TEXT("docks") && K != TEXT("cylOne");
				bFilesOnce &= !Seen.Contains(K); Seen.Add(K);
				if (!Tr) continue; // (the missing-id check below names it)
				if (Tr->bOptional) bSeenOptional = true; else { ++InGit; bOptionalLast &= !bSeenOptional; }
			}
			bTwo &= Cue.Value.Num() >= 2; bInGit &= InGit >= 1;
		}
		Check(bGone, TEXT("Night on the Docks and Cylinder One are in no pool and not in the table"));
		Check(bTwo && bInGit && bOptionalLast, TEXT("every pool has at least two pieces, one or more with its file in Git, the optional ones after them"));
		Check(bFilesOnce, TEXT("every piece has its own file, a title, a credit and a gain like the others; no pool names a piece twice"));
		const FS47ScoreTrack* Own = S47Score::TRACKS.Find(S47Score::IntroDriveId);
		Check(Own && Own->bOptional && FString(S47Score::IntroDriveId) == TEXT("signal47_intro_drive") && FString(S47Score::IntroDriveFile) == TEXT("music/") + Own->File
			&& S47Score::CUES.FindChecked(TEXT("drive")).Contains(S47Score::IntroDriveId), TEXT("Tom's Intro Drive is signal47_intro_drive, optional, in the drive's pool, and its media name is its file"));
		Check(S47Score::MediaOr(TEXT("music/s47_no_such_piece.mp3"), TEXT("radio/bama_country.mp3")) == TEXT("radio/bama_country.mp3"), TEXT("a staged piece that is not there gives its fallback"));
		// The five quiet mixes of 13 Nov: each optional, a file of its own in the s47_ scheme, and only in the pools of the places it was made for
		{
			const TMap<FString, TArray<FString>> Quiet = {
				{ TEXT("thirdShift"), { TEXT("eerie") } }, { TEXT("dryLake"), { TEXT("night") } }, { TEXT("vacancy"), { TEXT("motel") } },
				{ TEXT("greyHour"), { TEXT("dawn") } }, { TEXT("standingWave"), { TEXT("eerie") } },
			};
			bool bQuiet = true;
			for (const auto& Q : Quiet)
			{
				const FS47ScoreTrack* Tr = S47Score::TRACKS.Find(Q.Key);
				bQuiet &= Tr && Tr->bOptional && Tr->File.StartsWith(TEXT("s47_")) && Tr->File.EndsWith(TEXT(".mp3")) && Tr->By == TEXT("SIGNAL / 47") && Tr->Gain >= 0.4 && Tr->Gain <= 0.45;
				for (const auto& Cue : S47Score::CUES) bQuiet &= Cue.Value.Contains(Q.Key) == Q.Value.Contains(Cue.Key);
			}
			Check(bQuiet, TEXT("the five quiet mixes are optional, s47_ files, and each only in the pool of its place: SARO indoors, the land, the motel, the dawn"));
			FS47Score Room(nullptr, C, C->Destination);
			Room.HasFile = [](const FS47ScoreTrack& Tr) { return Tr.File == TEXT("s47_vacancy.mp3"); };
			Room.Last.Add(TEXT("solitude"), 0); Room.Last.Add(TEXT("ossuary5"), 0); Room.Last.Add(TEXT("cylTwo"), 0); Room.Last.Add(TEXT("vacancy"), -1000);
			Check(Room.Pick(TEXT("motel")) == TEXT("vacancy") && !Room.Present(TEXT("greyHour")) && Room.Pick(TEXT("recovery")) == TEXT("vacancy"),
				TEXT("a staged quiet mix is picked in its pool (the motel's, and recovery with it), and the unstaged ones are not there"));
		}
	}
	Check(S.Rest >= 6 && S.Rest <= 14, TEXT("initial quiet 6 to 14 seconds"));
	{
		FS47Score Layer(nullptr, C, C->Destination);
		Layer.Update(.01, TEXT("night"), false);
		const auto First = Layer.Cosmic;
		Check(First.IsValid() && Layer.HasAudio() && Layer.Playing.IsEmpty(), TEXT("cosmic texture is counted as audio during a track gap"));
		Layer.Pause(true); Layer.Done(5);
		Check(Layer.Cosmic == First && Layer.Rest == 5 && Layer.bPaused, TEXT("natural deck completion preserves cosmic texture and a pending pause"));
		Layer.Pause(false);
		Layer.Update(.01, TEXT("night"), false);
		Check(Layer.Cosmic == First, TEXT("repeated score frames reuse one cosmic node"));
		Layer.Stop();
		Check(!Layer.HasAudio() && !Layer.Cosmic, TEXT("full score stop releases cosmic audio even without a deck"));
	}
	// (the picks below are the same on a machine with the staged files and on one without: the test says which are there)
	S.HasFile = [](const FS47ScoreTrack&) { return true; };
	S.Track = TEXT("neon"); S.Last.Add(TEXT("lookBack"), 0);
	Check(S.Pick(TEXT("town")) == TEXT("stumping"), TEXT("does not immediately repeat a piece"));
	S.Track.Empty(); S.Last.Add(TEXT("neon"), -1000); S.Last.Add(TEXT("stumping"), 0);
	Check(S.Pick(TEXT("town")) == TEXT("neon"), TEXT("longest unheard wins beyond the random 120 seconds"));
	{
		bool bEveryPiece = true;
		for (const auto& Cue : S47Score::CUES) for (const FString& K : Cue.Value) bEveryPiece &= S.Present(K);
		Check(bEveryPiece && !S.Present(TEXT("docks")) && !S.Present(TEXT("cylOne")), TEXT("with the staged files every pool's pieces can be picked, and the removed ones cannot"));
		S.HasFile = [](const FS47ScoreTrack&) { return false; };
		bool bNoOptional = true;
		for (int32 I = 0; I < 40; ++I) for (const auto& Cue : S47Score::CUES) { const FS47ScoreTrack* Tr = S47Score::TRACKS.Find(S.Pick(Cue.Key)); bNoOptional &= Tr && !Tr->bOptional; }
		Check(bNoOptional && !S.Present(TEXT("lookBack")) && S.Present(TEXT("neon")), TEXT("without the staged files no pool picks an optional piece, and none is left empty"));
		S.Track = TEXT("neon");
		Check(S.Pick(TEXT("town")) == TEXT("neon"), TEXT("a pool down to one piece plays it again rather than nothing"));
		S.Track.Empty();
		S.HasFile = [](const FS47ScoreTrack& Tr) { return Tr.File == TEXT("look_back_steelguitar.mp3"); };
		S.Last.Add(TEXT("neon"), 0); S.Last.Add(TEXT("lookBack"), -1000);
		Check(S.Pick(TEXT("town")) == TEXT("lookBack") && S.Present(TEXT("lookBack")) && !S.Present(TEXT("stumping")), TEXT("a staged optional piece joins its pool, and only that one"));
		S.HasFile = [](const FS47ScoreTrack&) { return false; };
	}
	S.Rest = 1; S.Update(2, TEXT("night"), true);
	Check(S.Rest == 4 && S.T == 2 && S.Playing.IsEmpty(), TEXT("held music waits at least four seconds"));
	S.Rest = 2; S.Update(1, FString(), false);
	Check(S.Rest == 4 && S.T == 3, TEXT("no wanted cue also holds the quiet"));
	S.Done(15);
	Check(S.Rest == 15 && S.Track.IsEmpty() && S.Playing.IsEmpty(), TEXT("done sets the next quiet"));
	S.Track = TEXT("neon"); S.Stop();
	Check(S.Track == TEXT("neon") && S.Rest == 15, TEXT("stop keeps the last track and quiet"));
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
	// D45-20261110: the feed store. Something heard coming takes the town's piece to eerie, and it comes back after.
	Main->World->bDriving=false;
	State.RequestMood(TEXT("eerie"), .3);
	Check(State.MusicWant()==TEXT("eerie"),TEXT("Freeform town gives way to eerie while something is heard coming"));
	State.Update(0, FString(), true);
	Check(State.MusicWant()==TEXT("town")&&!State.MusicHeld(),TEXT("Freeform town comes back once nothing is heard, and nothing holds it"));
	Main->World->bDriving=true;
	Check(FMath::IsNearlyEqual(DreadDuck(true, 1), .05) && FMath::IsNearlyEqual(DreadDuck(false, 1), .34) && DreadDuck(true, 0) == 1 && FMath::IsNearlyEqual(DreadDuck(false, 0, 1), .5),
		TEXT("under the full ground a place's piece is ducked to a twentieth, an adaptive cue to a third, and nothing ducks without it"));
	Check(FMath::IsNearlyEqual(DreadDuck(true, .1, .5), .6) && FMath::IsNearlyEqual(DreadDuck(true, .9, .2), 1 - .95 * .9),
		TEXT("a place's piece already stands back as something is heard coming"));
	Check(PlaceCue(TEXT("town")) && PlaceCue(TEXT("dawn")) && PlaceCue(TEXT("motel")) && PlaceCue(TEXT("drive")) && !PlaceCue(TEXT("eerie")) && !PlaceCue(TEXT("tension")) && !PlaceCue(TEXT("action")) && !PlaceCue(TEXT("night")),
		TEXT("town, dawn, the motel and the drive are a place's own pieces; the adaptive cues are not"));
	Check(!State.PlacePieceSounding(), TEXT("no deck, no place piece sounding"));
	Check(TakenHolds(true, int32(FS47Taken::EStage::Flood)) && TakenHolds(true, int32(FS47Taken::EStage::Lift)) && !TakenHolds(true, int32(FS47Taken::EStage::Dread))
		&& !TakenHolds(true, int32(FS47Taken::EStage::None)) && !TakenHolds(false, int32(FS47Taken::EStage::Flood)),
		TEXT("Freeform: the take's flood and lift hold the score; its dread before them does not, nor does Story"));
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
