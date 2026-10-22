#include "S47Score.h"

#include "S47Ambience.h"
#include "S47Audio.h"
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
}

FS47Score::FS47Score(US47Main* InM, const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& InDest)
	: M(InM), Ctx(InCtx), Dest(InDest), Rest(Rnd(6, 14)) {}

FS47Score::~FS47Score()
{
	*bAlive = false;
	Stop();
}

// Score.update: every frame while a night runs.
void FS47Score::Update(double Dt, const FString& Want, bool bHeld)
{
	T += Dt;
	if (!Playing.IsEmpty())
	{
		if (bHeld) Fade(2.5, 6);
		else if (Want != Playing && !bFading) Fade(5, Rnd(3, 8));
		return;
	}
	if (bHeld || Want.IsEmpty()) { Rest = FMath::Max(Rest, 4.0); return; }
	Rest -= Dt;
	if (Rest > 0) return;
	Start(Want);
}

// Score.pick: not the one just played, and the one heard longest ago.
FString FS47Score::Pick(const FString& Cue) const
{
	const TArray<FString>& List = S47Score::CUES.FindChecked(Cue);
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

// Score.start: a place's music, or one piece by name (the developer menu).
void FS47Score::Start(const FString& Cue, const FString& InTrack)
{
	if (!S47Score::bScoreOn) return;
	const FString Next = InTrack.IsEmpty() ? Pick(Cue) : InTrack;
	Stop();
	const FS47ScoreTrack& C = S47Score::TRACKS.FindChecked(Next);
	// HTMLMediaElementSource is the same graph input, backed here by bounded PCM chunks.
	const TSharedPtr<FS47MediaSource> NewEl = FS47MediaSource::Open(Ctx, TEXT("music/") + C.File);
	if (!NewEl)
	{
		// A missing file is HTMLAudioElement's asynchronous error event in the web version.
		Playing = Cue; Track = Next; bFading = false; Last.Add(Next, T);
		Done(120); return;
	}
	FS47AudioContext::FBatch B(*Ctx);
	const TSharedPtr<FS47GainNode> G = Ctx->CreateGain(); G->Gain.SetValue(0);
	NewEl->Connect(G);
	G->Connect(Dest);
	const double Now = Ctx->CurrentTime();
	G->Gain.SetValueAtTime(0, Now); G->Gain.LinearRampToValueAtTime(C.Gain, Now + 6);
	const TWeakPtr<FS47MediaSource> WeakEl = NewEl;
	NewEl->OnEnded = [this, Alive = bAlive, WeakEl] { if (*Alive && El == WeakEl.Pin()) Done(Rnd(4, 15)); };
	NewEl->OnError = [this, Alive = bAlive, WeakEl] { if (*Alive && El == WeakEl.Pin()) Done(120); };
	El = NewEl; Gain = G; Playing = Cue; Track = Next; bFading = false;
	Last.Add(Next, T);
	if (!NewEl->Play() && El == NewEl) Done(30);
}

// Score.fade: fade out over secs, then quiet seconds before the next.
void FS47Score::Fade(double Secs, double Quiet)
{
	if (!El || !Gain || bFading) return;
	bFading = true;
	const TSharedPtr<FS47MediaSource> OldEl = El;
	const TSharedPtr<FS47GainNode> G = Gain;
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime();
	G->Gain.CancelScheduledValues(Now); G->Gain.SetValueAtTime(G->Gain.Value(), Now); G->Gain.LinearRampToValueAtTime(0, Now + Secs);
	Ctx->SetTimeout(Secs + 0.1, [this, Alive = bAlive, OldEl, Quiet] { if (*Alive && El == OldEl) Done(Quiet); });
}

// Score.stop: at once, leaving the night, the title, the end.
void FS47Score::Stop()
{
	FS47AudioContext::FBatch B(*Ctx);
	if (El) { El->OnEnded = nullptr; El->OnError = nullptr; El->Pause(); El->Stop(); El->Disconnect(); }
	if (Gain) Gain->Disconnect();
	El.Reset(); Gain.Reset(); Playing.Empty(); bFading = false;
}

// Score.pause: the pause menu, stop where it is and go on after.
void FS47Score::Pause(bool bOn) { if (El) { if (bOn) El->Pause(); else El->Play(); } }

void FS47Score::Done(double Quiet)
{
	Stop();
	Rest = Quiet;
}

// main.ts musicWant: keep these tests in the same order as the original.
FString FS47Score::MusicWant() const
{
	if (!M || !M->World) return FString();
	if (M->Story && M->Story->ChapterKey == TEXT("ch6")) return FString();
	if (M->World->bDriving) return TEXT("drive");
	if (M->World->Area == TEXT("station01")) return TEXT("tension");
	if (M->World->Area == TEXT("room6")) return TEXT("motel");
	const FVector P = M->Player ? M->Player->Pos : FVector::ZeroVector;
	if (M->World->Area == TEXT("saro") && M->World->AtMotel(P)) return TEXT("motel");
	const double Clock = M->Story ? M->Story->Clock() : 0;
	const double Hour = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400, 86400.0) / 3600;
	if (Hour >= 5 && Hour < 12) return TEXT("dawn");
	if (M->World->Area == TEXT("diner")) return M->World->RoadStopAt(P) ? TEXT("eerie") : TEXT("town");
	return M->Space == TEXT("yard") ? TEXT("night") : TEXT("eerie");
}

// main.ts musicHeld: the unported modules' properties are supplied through Held.
bool FS47Score::MusicHeld() const
{
	if (!M) return false;
	const AS47Story* Game = M->Story;
	bool bPhaseHeld = false;
	if (Game && Game->ChapterKey == TEXT("prologue"))
	{
		const ES47Phase P = Game->Phase();
		bPhaseHeld = P == ES47Phase::Ringing || P == ES47Phase::Call || P == ES47Phase::Countdown || P == ES47Phase::Event || P == ES47Phase::Turning || P == ES47Phase::End;
	}
	return (Game && Game->IsCinematic()) || bPhaseHeld || (M->Audio && M->Audio->RadioOn() && M->Space != TEXT("yard"))
		|| M->bRadioHeard || On(Held.RoadsideSilent) || On(Held.MotelTvHeard) || On(Held.AbductionBusy) || On(Held.RecWalkBusy)
		|| Value(Held.FaceQuiet) > 0 || Value(Held.NinthQuiet) > 0 || On(Held.RoofStepsBusy) || On(Held.ReelTapeBusy)
		|| On(Held.SaroDarkBusy) || On(Held.OfficePhoneBusy) || On(Held.ArchiveBusy) || On(Held.ArrayBackBusy) || On(Held.TruckBRacing)
		|| (M->Audio && M->Audio->Night && M->Audio->Night->Breathing().IsSet()) || On(Held.MorningBusy);
}

bool FS47Score::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s score: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bPass; };
	const TSharedRef<FS47AudioContext> C = FS47AudioContext::Make();
	FS47Score S(nullptr, C, C->Destination);
	Check(S47Score::TRACKS.Num() == 17 && S47Score::CUES.Num() == 7, TEXT("17 tracks and seven cues"));
	Check(S47Score::CUES.FindChecked(TEXT("night"))[0] == TEXT("cylEight") && S47Score::CUES.FindChecked(TEXT("town"))[1] == TEXT("docks"), TEXT("cue order"));
	Check(S.Rest >= 6 && S.Rest <= 14, TEXT("initial quiet 6 to 14 seconds"));
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
	Main->bRadioHeard = true;
	Check(State.MusicHeld(), TEXT("an audible receiver holds the score"));
	Main->bRadioHeard = false; State.Held.FaceQuiet = [] { return 2.0; };
	Check(State.MusicHeld(), TEXT("the album's face quiet hook holds the score"));
	State.Held.FaceQuiet = [] { return 0.0; }; State.Held.MorningBusy = [] { return true; };
	Check(State.MusicHeld(), TEXT("the morning's quiet hook holds the score"));
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
		Recorded.Stop(); MediaCtx->Close();
	}
	C->Close();
	return bOk;
}
