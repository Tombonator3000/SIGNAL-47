// SIDEBAND-20261113, channels: the nine own mixes as stems on one position, let in and out by tension on the piece's bar lines.
#include "S47ScoreStems.h"

#include "S47Json.h"
#include "S47Media.h"
#include "S47Score.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Stems, Log, All);

namespace S47Stems
{
	// The stems are Tools/music/build_music.py's (STEMS there: which loops make which). Tension is what the player can perceive
	// (S47Sideband::Drive): 0 routine, about 0.35 something heard coming, 0.4 to 0.7 near, 0.7 seen, 1 hunted.
	//   "Carrier Lock" is the tension pool's own piece: its pulse and its arpeggios come IN as it tightens (the prototype Tom heard).
	//   The quiet beds have no pulse to bring in: they thin instead, the upper and warmer layers leaving as something comes, down
	//   to the cold ground, which is where FLARES' bed and BROKEN come in (and the night's own sounds have dropped by then).
	//   The pieces of a place (the drive, the motel, the dawn) lose their drums and their top first; the piece itself is already
	//   going under by then (FS47Score::DreadDuck).
	// In/Out are bars from the bar line (a pad a bar, a drum kit a twentieth); in free time, seconds.
	const TArray<FPiece>& Pieces()
	{
		static const TArray<FPiece> All = {
			{ TEXT("s47_night_freight"), 80., { { TEXT("bed"), 0., 1., 1., 1. }, { TEXT("low"), 0., 1., .5, 1. }, { TEXT("arps"), 0., .6, 1., 1. }, { TEXT("drums"), 0., .35, .05, .1 } } },
			{ TEXT("s47_cold_dish"), 100., { { TEXT("ground"), 0., 1., 1., 1. }, { TEXT("pads"), 0., .75, 2., 2. }, { TEXT("high"), 0., .45, 2., 2. } } },
			{ TEXT("s47_carrier_lock"), 100., { { TEXT("bed"), 0., 1., 1., 1. }, { TEXT("pulse"), .3, 1., .5, .1 }, { TEXT("arp"), .55, 1., 1., .5 } } },
			{ TEXT("s47_county_road"), 124., { { TEXT("bed"), 0., 1., 1., 1. }, { TEXT("low"), 0., 1., .5, 1. }, { TEXT("arps"), 0., .6, 1., 1. }, { TEXT("drums"), 0., .35, .05, .1 } } },
			{ TEXT("s47_third_shift"), 0., { { TEXT("ground"), 0., 1., 6., 6. }, { TEXT("inside"), 0., .6, 6., 6. } } },
			{ TEXT("s47_dry_lake"), 0., { { TEXT("ground"), 0., 1., 6., 6. }, { TEXT("lights"), 0., .45, 6., 6. }, { TEXT("low"), 0., 1., 6., 6. } } },
			{ TEXT("s47_vacancy"), 0., { { TEXT("ground"), 0., 1., 6., 6. }, { TEXT("chords"), 0., .6, 6., 6. } } },
			{ TEXT("s47_grey_hour"), 0., { { TEXT("ground"), 0., 1., 6., 6. }, { TEXT("pads"), 0., .7, 6., 6. }, { TEXT("top"), 0., .4, 6., 6. } } },
			{ TEXT("s47_standing_wave"), 0., { { TEXT("ground"), 0., 1., 6., 6. }, { TEXT("low"), 0., 1., 6., 6. }, { TEXT("far"), 0., .5, 6., 6. } } },
		};
		return All;
	}

	const FPiece* Find(const FString& TrackFile)
	{
		const FString Base = FPaths::GetBaseFilename(TrackFile);
		for (const FPiece& P : Pieces()) if (Base == P.File) return &P;
		return nullptr;
	}

	bool Sounds(const FRow& Row, double Tension, bool bWasIn)
	{
		const double T = FMath::IsFinite(Tension) ? FMath::Clamp(Tension, 0., 1.) : 0.;
		if (bWasIn) return T >= Row.From - Hysteresis && T <= Row.To + Hysteresis;
		return T >= Row.From + (Row.From > 0 ? Hysteresis : 0.) && T <= Row.To - (Row.To < 1 ? Hysteresis : 0.);
	}

	double ToBarLine(double Bpm, double Position)
	{
		if (!(Bpm > 0) || !FMath::IsFinite(Position)) return 0.;
		const double Bar = 240. / Bpm;
		return FMath::CeilToDouble((FMath::Max(0., Position) + 1e-5) / Bar) * Bar - FMath::Max(0., Position);
	}

	FString WebName(const FPiece& Piece, const FRow& Row)
	{
		return FString::Printf(TEXT("music/stems/%s__%s.mp3"), Piece.File, Row.Name);
	}

	struct FStaged { double Level = 0, Lowpass = 0; };
	const FStaged* StagedRow(const FString& BaseName)
	{
		static TMap<FString, FStaged> Levels;
		static bool bRead = false;
		if (!bRead)
		{
			bRead = true;
			FString Text;
			if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Audio/Raw/media/music/stems/stems.json"))))
			{
				const S47J::FObj All = S47J::Obj(S47J::Read(Text), TEXT("pieces"));
				if (All.IsValid()) for (const auto& Row : All->Values)
				{
					const TSharedPtr<FJsonObject>* O = nullptr;
					if (Row.Value.IsValid() && Row.Value->TryGetObject(O) && O) Levels.Add(FString(Row.Key.ToView()), { S47J::Num(*O, TEXT("level"), 0.), S47J::Num(*O, TEXT("lowpass_hz"), 0.) });
				}
			}
			UE_LOG(LogS47Stems, Display, TEXT("S47: stems staged for %d of the own mixes (Content/Audio/Raw/media/music/stems)"), Levels.Num());
		}
		return Levels.Find(BaseName);
	}

	double StagedLevel(const FString& BaseName)
	{
		const FStaged* Found = StagedRow(BaseName);
		return Found && FMath::IsFinite(Found->Level) && Found->Level > 0 ? Found->Level : 0.;
	}

	double StagedLowpass(const FString& BaseName)
	{
		const FStaged* Found = StagedRow(BaseName);
		return Found && FMath::IsFinite(Found->Lowpass) && Found->Lowpass >= 1000. && Found->Lowpass <= 20000. ? Found->Lowpass : 0.;
	}

	bool Enabled()
	{
		static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("S47NoStems"));
		return !bOff;
	}
}

TSharedPtr<FS47ScoreStems> FS47ScoreStems::Open(const TSharedRef<FS47AudioContext>& InCtx, const FString& TrackFile, double Tension)
{
	const S47Stems::FPiece* P = S47Stems::Enabled() ? S47Stems::Find(TrackFile) : nullptr;
	const double Level = P ? S47Stems::StagedLevel(P->File) : 0.;
	if (!P || Level <= 0) return nullptr;
	// every stem, or the mix: a piece never plays with a part of itself missing
	for (const S47Stems::FRow& Row : P->Stems) if (!S47Score::MediaPresent(S47Stems::WebName(*P, Row))) return nullptr;
	TArray<TSharedPtr<FS47MediaSource>> Opened;
	for (const S47Stems::FRow& Row : P->Stems)
	{
		const TSharedPtr<FS47MediaSource> D = FS47MediaSource::Open(InCtx, S47Stems::WebName(*P, Row));
		if (!D.IsValid()) { for (const TSharedPtr<FS47MediaSource>& O : Opened) O->Stop(); return nullptr; }
		Opened.Add(D);
	}
	return Make(InCtx, *P, Opened, Level, Tension, S47Stems::StagedLowpass(P->File));
}

TSharedPtr<FS47ScoreStems> FS47ScoreStems::Make(const TSharedRef<FS47AudioContext>& InCtx, const S47Stems::FPiece& InPiece, const TArray<TSharedPtr<FS47MediaSource>>& InDecks, double Level, double Tension, double Lowpass)
{
	if (InDecks.Num() != InPiece.Stems.Num() || !(Level > 0) || !FS47MediaSource::Group(InDecks))
	{
		UE_LOG(LogS47Stems, Warning, TEXT("S47: the stems of %s do not make one group (lengths or rates differ): the mix plays instead"), InPiece.File);
		return nullptr;
	}
	TSharedPtr<FS47ScoreStems> S = MakeShared<FS47ScoreStems>();
	S->Ctx = InCtx; S->Piece = &InPiece; S->Decks = InDecks;
	FS47AudioContext::FBatch B(*InCtx);
	// the mix's own tape curve, once over the sum, at the mix's working level (the curve is laid over twice the range)
	S->Sum = InCtx->CreateGain(); S->Sum->Gain.SetValue(1. / (2. * Level));
	S->Tape = InCtx->CreateWaveShaper();
	S->Tape->Curve.SetNumUninitialized(4097);
	for (int32 I = 0; I < 4097; ++I) S->Tape->Curve[I] = float(FMath::Tanh(1.6 * 2. * (-1. + 2. * I / 4096.)) / 1.6);
	S->Post = InCtx->CreateGain(); S->Post->Gain.SetValue(Level);
	// stems staged at half the rate: a fourth-order low-pass over their sum, against the images the interpolation leaves above
	// 12 kHz (what it takes below that, and its phase, the recipe has given back in the stems themselves)
	TSharedPtr<FS47AudioNode> Last = S->Sum;
	if (Lowpass > 0) for (int32 I = 0; I < 2; ++I)
	{
		S->Lp[I] = InCtx->CreateBiquadFilter(); S->Lp[I]->SetType(TEXT("lowpass"));
		S->Lp[I]->Frequency.SetValue(Lowpass); S->Lp[I]->Q.SetValue(S47Stems::LowpassQ[I]);
		Last->Connect(S->Lp[I]); Last = S->Lp[I];
	}
	Last->Connect(S->Tape); S->Tape->Connect(S->Post);
	for (int32 I = 0; I < InDecks.Num(); ++I)
	{
		const S47Stems::FRow& Row = InPiece.Stems[I];
		const bool bSounds = Tension >= Row.From && Tension <= Row.To;
		const TSharedRef<FS47GainNode> G = InCtx->CreateGain(); G->Gain.SetValue(bSounds ? 1. : 0.);
		InDecks[I]->Connect(G); G->Connect(S->Sum);
		S->Gains.Add(G); S->bIn.Add(bSounds); S->Change.Add(0.);
	}
	return S;
}

FS47ScoreStems::~FS47ScoreStems() { Close(); }

void FS47ScoreStems::Close()
{
	if (!Ctx.IsValid()) return;
	FS47AudioContext::FBatch B(*Ctx);
	if (Decks.Num() && Decks[0].IsValid()) Decks[0]->Stop(); // (the first of a group stops all of it)
	for (const TSharedPtr<FS47MediaSource>& D : Decks) if (D.IsValid()) { D->OnEnded = nullptr; D->OnError = nullptr; D->Disconnect(); }
	for (const TSharedPtr<FS47GainNode>& G : Gains) if (G.IsValid()) G->Disconnect();
	if (Sum.IsValid()) Sum->Disconnect();
	for (TSharedPtr<FS47BiquadFilterNode>& F : Lp) if (F.IsValid()) { F->Disconnect(); F.Reset(); }
	if (Tape.IsValid()) Tape->Disconnect();
	if (Post.IsValid()) Post->Disconnect();
	Decks.Reset(); Gains.Reset(); Sum.Reset(); Tape.Reset(); Post.Reset(); Ctx.Reset();
}

int32 FS47ScoreStems::Wanted() const
{
	int32 N = 0;
	for (bool b : bIn) N += b;
	return N;
}

void FS47ScoreStems::Step(double Tension)
{
	if (!Ctx.IsValid() || Decks.IsEmpty() || !Piece) return;
	const double Now = Ctx->CurrentTime(), Bar = Piece->Bpm > 0 ? 240. / Piece->Bpm : 0.;
	double Wait = -1;
	for (int32 I = 0; I < Gains.Num(); ++I)
	{
		if (Now < Change[I]) continue; // one change at a time
		const S47Stems::FRow& Row = Piece->Stems[I];
		const bool bWant = S47Stems::Sounds(Row, Tension, bIn[I]);
		if (bWant == bIn[I]) continue;
		// on the piece's next bar line (every stem that changes this frame, on the same one)
		if (Wait < 0) Wait = Decks[0]->Paused() ? 0. : S47Stems::ToBarLine(Piece->Bpm, Decks[0]->CurrentTime());
		const double At = Now + Wait, Takes = FMath::Max(.02, (bWant ? Row.In : Row.Out) * (Bar > 0 ? Bar : 1.));
		FS47AudioContext::FBatch B(*Ctx);
		FS47AudioParam& P = Gains[I]->Gain;
		const double V0 = bIn[I] ? 1. : 0.;
		P.CancelScheduledValues(Now); P.SetValueAtTime(V0, Now); P.SetValueAtTime(V0, At); P.LinearRampToValueAtTime(bWant ? 1. : 0., At + Takes);
		Change[I] = At + Takes; bIn[I] = bWant;
	}
}

bool S47Stems::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s stems: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bPass; };
	// the table: the nine own mixes, each two to four stems, the first always in, every window inside 0 to 1
	bool bNine = Pieces().Num() == 9, bRows = true, bOwn = true;
	for (const FPiece& P : Pieces())
	{
		bool bTrack = false;
		for (const auto& Tr : S47Score::TRACKS) bTrack |= FPaths::GetBaseFilename(Tr.Value.File) == P.File && Tr.Value.bOptional;
		bOwn &= bTrack && FString(P.File).StartsWith(TEXT("s47_")) && Find(FString(P.File) + TEXT(".mp3")) == &P;
		bRows &= P.Stems.Num() >= 2 && P.Stems.Num() <= 4 && P.Stems[0].From == 0 && P.Stems[0].To == 1 && P.Bpm >= 0;
		TSet<FString> Names;
		for (const FRow& R : P.Stems)
		{
			bRows &= R.From >= 0 && R.To <= 1 && R.From < R.To && R.In > 0 && R.Out > 0 && !Names.Contains(R.Name);
			Names.Add(R.Name);
			bRows &= WebName(P, R) == FString::Printf(TEXT("music/stems/%s__%s.mp3"), P.File, R.Name);
		}
	}
	Check(bNine && bOwn, TEXT("the nine own mixes have stems, each a piece of the score's table, found by its file"));
	Check(bRows, TEXT("two to four stems a piece, the first always in, every window inside 0 to 1, names once"));
	Check(Find(TEXT("neon.mp3")) == nullptr && Find(TEXT("s47_no_such.mp3")) == nullptr, TEXT("no stems for a piece that is not ours"));
	// in and out with tension, and no chatter at an edge
	const FRow Thin = { TEXT("high"), 0., .45, 2., 2. }, Rise = { TEXT("pulse"), .3, 1., .5, .1 }, Always = { TEXT("bed"), 0., 1., 1., 1. };
	Check(Sounds(Always, 0, false) && Sounds(Always, 1, false) && Sounds(Always, .5, true), TEXT("the ground is in at every tension"));
	Check(Sounds(Thin, 0, false) && Sounds(Thin, .45, true) && Sounds(Thin, .48, true) && !Sounds(Thin, .5, true) && !Sounds(Thin, .43, false) && Sounds(Thin, .4, false),
		TEXT("a layer that thins leaves just past its edge and comes back only well under it"));
	Check(!Sounds(Rise, 0, false) && !Sounds(Rise, .32, false) && Sounds(Rise, .35, false) && Sounds(Rise, .27, true) && !Sounds(Rise, .25, true) && Sounds(Rise, 1, true),
		TEXT("a layer that comes in with tension enters just past its edge and stays until well under it"));
	Check(!Sounds(Rise, NAN, true) && Sounds(Thin, -4, false) && !Sounds(Thin, 7, true), TEXT("a tension that is no number counts as none, and it is kept inside 0 to 1"));
	// the bar grid
	Check(FMath::IsNearlyEqual(ToBarLine(100., 0.25), 2.15, 1e-6) && FMath::IsNearlyEqual(ToBarLine(100., 2.4), 2.4, 1e-4) && FMath::IsNearlyEqual(ToBarLine(80., 2.9), .1, 1e-6)
		&& ToBarLine(0., 12.) == 0 && ToBarLine(100., NAN) == 0, TEXT("the next bar line: 2.4 s bars at 100 BPM, 3 s at 80; at once in free time"));
	Check(StagedLevel(TEXT("s47_no_such")) == 0 && StagedLowpass(TEXT("s47_no_such")) == 0, TEXT("a piece that is not staged as stems has no level and no low-pass, and plays as its mix"));
	{
		// stems.json as staged on this machine, if it is: every piece's stems at one rate with the low-pass that rate needs
		bool bStagedOk = true; int32 Staged = 0;
		for (const FPiece& P : Pieces())
		{
			if (StagedLevel(P.File) <= 0) continue;
			++Staged;
			bStagedOk &= StagedLevel(P.File) > .3 && StagedLevel(P.File) < 1.5 && (StagedLowpass(P.File) == 0 || (StagedLowpass(P.File) >= 9000 && StagedLowpass(P.File) <= 12000));
		}
		Lines.Add(FString::Printf(TEXT("INFO stems: %d of the nine pieces are staged as stems here"), Staged));
		Check(bStagedOk, TEXT("every staged piece has a working level, and a low-pass between 9 and 12 kHz or none"));
	}
	// decks as stems: the group, the tape curve over the sum, a change waiting for its bar line
	const FString Path = FPaths::ProjectSavedDir() / TEXT("S47StemsSelfTest.wav");
	{
		constexpr int32 Frames = 48000 * 4;
		TArray<uint8> Bytes; Bytes.SetNumZeroed(44 + Frames * 2);
		FMemory::Memcpy(Bytes.GetData(), "RIFF", 4); FMemory::Memcpy(Bytes.GetData() + 8, "WAVEfmt ", 8); FMemory::Memcpy(Bytes.GetData() + 36, "data", 4);
		const auto Set16 = [&Bytes](int32 At, uint16 V) { Bytes[At] = uint8(V); Bytes[At + 1] = uint8(V >> 8); };
		const auto Set32 = [&Set16](int32 At, uint32 V) { Set16(At, uint16(V)); Set16(At + 2, uint16(V >> 16)); };
		Set32(4, Bytes.Num() - 8); Set32(16, 16); Set16(20, 1); Set16(22, 1); Set32(24, 48000); Set32(28, 96000); Set16(32, 2); Set16(34, 16); Set32(40, Frames * 2);
		for (int32 I = 0; I < Frames; ++I) Set16(44 + I * 2, uint16(int16(3277))); // a steady 0.1
		FFileHelper::SaveArrayToFile(Bytes, *Path);
	}
	const TSharedRef<FS47AudioContext> C = FS47AudioContext::Make();
	const FString Full = FPaths::ConvertRelativePathToFull(Path);
	const FPiece Test = { TEXT("test"), 100., { Always, Rise } };
	TArray<TSharedPtr<FS47MediaSource>> Two = { FS47MediaSource::Open(C, Full), FS47MediaSource::Open(C, Full) };
	// (with the low-pass that half-rate stems get: a steady level passes it as it is)
	const TSharedPtr<FS47ScoreStems> S = Two[0].IsValid() && Two[1].IsValid() ? FS47ScoreStems::Make(C, Test, Two, .5, 0., 11000.) : nullptr;
	Check(S.IsValid() && S->Num() == 2 && S->Leader() == Two[0] && S->In(0) && !S->In(1) && S->Wanted() == 1 && S->Filtered(), TEXT("decks of one length open as the stems of a piece; at no tension only the ground is in"));
	if (S.IsValid())
	{
		S->Out()->Connect(C->Destination);
		S->Leader()->Play();
		TArray<float> Pcm; Pcm.SetNumZeroed(48000 * 2);
		// (a quantum at a time with a breath now and then: the reader's thread brings the next chunks in)
		const auto Render = [&C, &Pcm](int32 N)
		{
			for (int32 Done = 0, K = 0; Done < N; Done += S47Quantum, ++K)
			{
				C->Render(Pcm.GetData(), S47Quantum, 2);
				if (K % 8 == 7) FPlatformProcess::Sleep(0.001f);
			}
			C->Tick(double(N) / 48000);
			return double(Pcm[(S47Quantum - 1) * 2]);
		};
		// one stem of 0.1 through the curve at a level of 0.5: tanh(1.6 * 0.2) / 1.6 * 0.5
		const double One = FMath::Tanh(1.6 * .1 / .5) / 1.6 * .5, Both = FMath::Tanh(1.6 * .2 / .5) / 1.6 * .5;
		const double Got = Render(4800);
		Check(FMath::Abs(Got - One) < 2e-4, TEXT("the sum passes the mix's own tape curve at its working level"));
		S->Step(.6); // the pulse is wanted: on the next bar line (2.4 s), over half a bar
		Check(S->In(1) && S->Wanted() == 2 && FMath::Abs(S->ChangeAt(1) - (2.4 + 1.2)) < .01, TEXT("a stem wanted in is set for the piece's next bar line, half a bar long"));
		const double Before = Render(48000 * 2);          // 2.1 s: not yet
		S->Step(.6);
		const double Rising = Render(int32(48000 * .9));  // 3.0 s: half way up
		const double After = Render(int32(48000 * .8));   // 3.8 s: in
		Check(FMath::Abs(Before - One) < 2e-4 && Rising > One + .005 && Rising < Both - .005 && FMath::Abs(After - Both) < 3e-4,
			TEXT("it waits for the bar line, comes in over its fade, and both then pass the curve together"));
		Check(Two[0]->CurrentTime() == Two[1]->CurrentTime() && Two[0]->CurrentTime() > 3., TEXT("the stems are on one position"));
		S->Step(.1);
		Check(!S->In(1) && S->ChangeAt(1) > C->CurrentTime(), TEXT("and it is set to leave when the tension has gone"));
		S->Close();
		Check(S->Num() == 0 && Two[0]->Paused(), TEXT("closing stops every deck"));
	}
	{
		// a one-channel stem beside a two-channel one (the staged form may fold a stem whose sides are far under its middle), and
		// a file at another rate
		const FString WidePath = FPaths::ProjectSavedDir() / TEXT("S47StemsSelfTestWide.wav"), SlowPath = FPaths::ProjectSavedDir() / TEXT("S47StemsSelfTestSlow.wav");
		const auto Tone = [](const FString& To, int32 Channels, int32 Rate)
		{
			constexpr int32 Frames = 48000 * 4; // (the same count of frames in each, whatever the rate)
			TArray<uint8> Bytes; Bytes.SetNumZeroed(44 + Frames * 2 * Channels);
			FMemory::Memcpy(Bytes.GetData(), "RIFF", 4); FMemory::Memcpy(Bytes.GetData() + 8, "WAVEfmt ", 8); FMemory::Memcpy(Bytes.GetData() + 36, "data", 4);
			const auto Set16 = [&Bytes](int32 At, uint16 V) { Bytes[At] = uint8(V); Bytes[At + 1] = uint8(V >> 8); };
			const auto Set32 = [&Set16](int32 At, uint32 V) { Set16(At, uint16(V)); Set16(At + 2, uint16(V >> 16)); };
			Set32(4, Bytes.Num() - 8); Set32(16, 16); Set16(20, 1); Set16(22, uint16(Channels)); Set32(24, Rate); Set32(28, Rate * 2 * Channels); Set16(32, uint16(2 * Channels)); Set16(34, 16);
			Set32(40, Frames * 2 * Channels);
			for (int32 I = 0; I < Frames * Channels; ++I) Set16(44 + I * 2, uint16(int16(3277))); // a steady 0.1 in every channel
			FFileHelper::SaveArrayToFile(Bytes, *To);
		};
		Tone(WidePath, 2, 48000); Tone(SlowPath, 1, 24000);
		const FPiece Flat = { TEXT("test"), 0., { Always, { TEXT("wide"), 0., 1., 1., 1. } } };
		TArray<TSharedPtr<FS47MediaSource>> Mixed = { FS47MediaSource::Open(C, Full), FS47MediaSource::Open(C, FPaths::ConvertRelativePathToFull(WidePath)) };
		const TSharedPtr<FS47ScoreStems> Fold = Mixed[0].IsValid() && Mixed[1].IsValid() ? FS47ScoreStems::Make(C, Flat, Mixed, .5, 0., 11000.) : nullptr;
		Check(Fold.IsValid() && Fold->Wanted() == 2 && Mixed[0]->Grouped() && Mixed[1]->Grouped(), TEXT("a one-channel stem and a two-channel one of the same length make one group"));
		if (Fold.IsValid())
		{
			Fold->Out()->Connect(C->Destination);
			Fold->Leader()->Play();
			TArray<float> Pcm; Pcm.SetNumZeroed(S47Quantum * 2);
			for (int32 K = 0; K < 40; ++K) { C->Render(Pcm.GetData(), S47Quantum, 2); if (K % 8 == 7) FPlatformProcess::Sleep(0.001f); }
			// 0.1 from the one-channel deck and 0.1 from the two-channel one in each speaker: both pass the curve as 0.2. (Under a
			// pan law the one-channel deck would come 3 dB short.)
			const double Sum2 = FMath::Tanh(1.6 * .2 / .5) / 1.6 * .5;
			const double Left = Pcm[(S47Quantum - 1) * 2], Right = Pcm[(S47Quantum - 1) * 2 + 1];
			Check(FMath::Abs(Left - Sum2) < 3e-4 && FMath::Abs(Right - Sum2) < 3e-4 && Mixed[0]->CurrentTime() == Mixed[1]->CurrentTime() && Mixed[0]->CurrentTime() > 0,
				TEXT("a one-channel stem sits in the middle at the level of its two-channel form (no pan law), on the same position as the others"));
			Fold->Close();
		}
		TArray<TSharedPtr<FS47MediaSource>> Rates = { FS47MediaSource::Open(C, Full), FS47MediaSource::Open(C, FPaths::ConvertRelativePathToFull(SlowPath)) };
		Check(Rates[0].IsValid() && Rates[1].IsValid() && !FS47MediaSource::Group(Rates) && !Rates[0]->Grouped() && !Rates[1]->Grouped(),
			TEXT("decks of two rates are not grouped (one position in frames holds for one rate): such a piece plays as its mix"));
		if (Rates[1].IsValid() && Rates[0].IsValid())
		{
			// a deck at half the context's rate is as long in seconds as its frames say: the bar grid is in seconds
			Check(FMath::IsNearlyEqual(Rates[1]->Duration(), 8., 1e-9) && FMath::IsNearlyEqual(Rates[0]->Duration(), 4., 1e-9), TEXT("a deck's length and position are in seconds of its own rate"));
		}
		IFileManager::Get().Delete(*WidePath); IFileManager::Get().Delete(*SlowPath);
	}
	C->Close();
	IFileManager::Get().Delete(*Path);
	return bOk;
}
