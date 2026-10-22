#include "S47Radio.h"

#include "S47Audio.h"
#include "S47Media.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Internationalization/Regex.h"

namespace
{
	constexpr double GAP = 0.7, FRAGMENT_GAP = 0.035, CALM = 10, RECORD_MIN = 5, RECORD_FADE = 1.6, BEFORE_FIXED = 180;
	double Day(double Clock) { return FMath::Fmod(FMath::Fmod(Clock, 86400.) + 86400., 86400.); }
	FString Cap(FString S) { if (!S.IsEmpty()) S[0] = FChar::ToUpper(S[0]); return S; }
	bool Caller(const FString& Who) { return Who == TEXT("woman") || Who == TEXT("man") || Who == TEXT("old") || Who == TEXT("boy") || Who == TEXT("news"); }
	double Pitch(const FString& Who)
	{
		if (Who == TEXT("woman")) return 212;
		if (Who == TEXT("man")) return 138;
		if (Who == TEXT("roswell")) return 126;
		if (Who == TEXT("boy")) return 262;
		if (Who == TEXT("news")) return 150;
		if (Who == TEXT("old")) return 104;
		return 116;
	}
	TSharedPtr<FS47RadioSeg> Segment(const FString& Kind, double Len = 0)
	{
		auto S = MakeShared<FS47RadioSeg>(); S->Kind = Kind; S->Len = Len; return S;
	}
	TSharedPtr<FS47RadioSeg> Talk(std::initializer_list<FS47RadioLine> Lines)
	{
		auto S = Segment(TEXT("talk")); S->Lines = Lines; return S;
	}
	TSharedPtr<FS47RadioSeg> Music(const FString& Tune, double Len)
	{
		auto S = Segment(TEXT("music"), Len); S->Tune = Tune; return S;
	}
	double TalkLen(const TArray<FS47RadioLine>& Lines, double Clock)
	{
		double A = 0.4; for (const auto& L : Lines) A += S47Radio::SecsOf(L.At(Clock)) + GAP; return A;
	}
	double TalkScale(const FS47RadioSeg& S, double Clock) { return S.Duration.IsSet() ? S.Duration.GetValue() / TalkLen(S.Lines, Clock) : 1; }
	double SegLen(const FS47RadioSeg& S, double Clock)
	{
		if (S.Kind == TEXT("talk")) return S.Duration.Get(TalkLen(S.Lines, Clock));
		if (S.Kind == TEXT("music") || S.Kind == TEXT("carrier")) return S.Len;
		if (S.Kind == TEXT("anthem")) return S47Radio::ANTHEM_SECS;
		return std::numeric_limits<double>::infinity();
	}
TArray<TSharedPtr<FS47RadioSeg>> NIGHT_LINE()
{
	return {
		Talk({FS47RadioLine(TEXT("This is the Night Line on fourteen-twenty, Caliche. We are on till four."), TEXT("host")),
		FS47RadioLine([](double C) { return FString(TEXT("It's ")) + S47Radio::SayTime(C) + FString(TEXT(" in Caliche.")); }, TEXT("host"))}),
		Music(TEXT("ballad"), 78),
		Talk({FS47RadioLine(TEXT("Cold one out there. Thirty-eight on the bank sign, and the wind has laid down."), TEXT("host")),
		FS47RadioLine(TEXT("Phones are open. If you are up anyway, you might as well talk to somebody."), TEXT("host"))}),
		Music(TEXT("twostep"), 70),
		Talk({FS47RadioLine(TEXT("Earl, it's Dolores out on the Hondo road. The lights are back over the mesa. Same as last fall."), TEXT("woman"), TEXT("a caller")),
		FS47RadioLine(TEXT("Dolores, I believe you. Now turn the porch light off and go to bed."), TEXT("host"))}),
		Music(TEXT("waltz"), 64),
		Talk({FS47RadioLine(TEXT("This one goes out to the night crew up at the observatory. Somebody has to listen to the sky."), TEXT("host"))}),
		Music(TEXT("ballad"), 78),
		Talk({FS47RadioLine(TEXT("Anybody else lose the TV a little after two? Just a hum, most of a minute. Then it came back."), TEXT("man"), TEXT("a caller")),
		FS47RadioLine(TEXT("We lost the transmitter about then. Needle went to nothing for most of a minute. Ray, if you are listening, that one is yours."), TEXT("host"))}),
		Music(TEXT("twostep"), 70),
		Talk({FS47RadioLine(TEXT("Jack Nicklaus, forty-six years old, took the Masters this afternoon. Thirty on the back nine. My uncle says he called it. My uncle calls everything."), TEXT("host")),
		FS47RadioLine([](double C) { return Cap(S47Radio::SayTime(C)) + FString(TEXT(". Night Line, fourteen-twenty.")); }, TEXT("host"))}),
		Music(TEXT("waltz"), 64),
		Talk({FS47RadioLine(TEXT("Halley's comet is low in the south this week. Get out past the last streetlight and give your eyes ten minutes."), TEXT("host"))}),
		Music(TEXT("ballad"), 78),
		Talk({FS47RadioLine(TEXT("My dog won't come out from under the trailer. He's been under there since one."), TEXT("man"), TEXT("a caller")),
		FS47RadioLine(TEXT("Smart dog."), TEXT("host"))}),
		Music(TEXT("twostep"), 70)
	};
}

TArray<TSharedPtr<FS47RadioSeg>> BOY_CALL()
{
	return {
		Talk({FS47RadioLine(TEXT("Hello? Is this the radio? There's a star with a tail on it, real low in the south. A little tail."), TEXT("boy"), TEXT("a boy")),
		FS47RadioLine(TEXT("That's Halley's comet, son. It's there all week. You're up late."), TEXT("host")),
		FS47RadioLine(TEXT("I'm calling from my dad's place, out on the ranch. There's a light over the mesa too. It isn't moving."), TEXT("boy"), TEXT("a boy")),
		FS47RadioLine(TEXT("Is your dad up?"), TEXT("host")),
		FS47RadioLine(TEXT("He's out with the men. It's hot out. It's July."), TEXT("boy"), TEXT("a boy"))}),
		Music(TEXT("waltz"), 24),
		Talk({FS47RadioLine(TEXT("Folks, I want to say something. That line was dead when I picked it up. No dial tone, nothing at all."), TEXT("host")),
		FS47RadioLine(TEXT("I don't know who I was talking to."), TEXT("host"))})
	};
}

TArray<TSharedPtr<FS47RadioSeg>> SIGN_OFF_SEQ()
{
	return {
		Talk({FS47RadioLine(TEXT("That's the Night Line. Thank you for staying up with us."), TEXT("host")),
		FS47RadioLine(TEXT("This is fourteen-twenty, Caliche, New Mexico, signing off until six. Good night."), TEXT("host"))}),
		Segment(TEXT("anthem")),
		Segment(TEXT("carrier"), 75)
	};
}

TArray<TSharedPtr<FS47RadioSeg>> MORNING()
{
	return {
		Talk({FS47RadioLine(TEXT("Cattle at the sale barn steady, a dollar higher on calves."), TEXT("roswell")),
		FS47RadioLine([](double C) { return Cap(S47Radio::SayTime(C)) + FString(TEXT(" in Roswell.")); }, TEXT("roswell"))}),
		Music(TEXT("twostep"), 70),
		Talk({FS47RadioLine(TEXT("Clear today, high near seventy-four in Roswell, light winds out of the southwest."), TEXT("roswell"))}),
		Music(TEXT("ballad"), 78),
		Talk({FS47RadioLine(TEXT("Jack Nicklaus won the Masters yesterday. Forty-six years old, thirty on the back nine. Nobody saw it coming."), TEXT("roswell"))}),
		Music(TEXT("waltz"), 64),
		Talk({FS47RadioLine(TEXT("Road crews on the highway north of town all week, so give yourself a few extra minutes."), TEXT("roswell")),
		FS47RadioLine(TEXT("If you are out on the county roads this early, watch the low spots. There's deer."), TEXT("roswell"))}),
		Music(TEXT("twostep"), 70),
		Talk({FS47RadioLine(TEXT("And if you are up at this hour, Halley's comet is low in the south around midnight this week."), TEXT("roswell")),
		FS47RadioLine([](double C) { return Cap(S47Radio::SayTime(C)) + FString(TEXT(".")); }, TEXT("roswell"))}),
		Music(TEXT("ballad"), 78)
	};
}

TArray<TSharedPtr<FS47RadioSeg>> SIGN_ON_SEQ()
{
	return {
		Segment(TEXT("carrier"), 6),
		Talk({FS47RadioLine(TEXT("Good morning. This is twelve-thirty, Roswell, beginning our broadcast day."), TEXT("roswell"))}),
		Segment(TEXT("anthem"))
	};
}

TArray<TSharedPtr<FS47RadioSeg>> FAR()
{
	return {
		Music(TEXT("far"), 96)
	};
}
	struct FTrack { FString File; double Dur; };
	FTrack Track(const FString& Tune)
	{
		if (Tune == TEXT("ballad")) return { TEXT("drankin_song"), 246.4 };
		if (Tune == TEXT("twostep")) return { TEXT("bama_country"), 211.8 };
		if (Tune == TEXT("waltz")) return { TEXT("cattails"), 159.0 };
		return { TEXT("anamalie"), 208.2 };
	}
	// radio.ts pluck, steel and makeTune: the single-file/failed-recording fallback.
	constexpr int32 SR = 22050;
	double Hz(int32 N) { return 440 * FMath::Pow(2., (N - 69) / 12.); }
	void Pluck(TArray<float>& D, double F, double T0, double Dur, double Amp, double Soft = 0.5)
	{
		const int32 N = FMath::Max(2, FMath::RoundToInt(SR / F)); TArray<float> Buf; Buf.SetNumUninitialized(N);
		double Last = 0;
		for (int32 I = 0; I < N; I++) { const double W = FMath::FRand() * 2 - 1; Last = Last * Soft + W * (1 - Soft); Buf[I] = Last * 2; }
		const int32 I0 = FMath::FloorToInt(T0 * SR), Count = FMath::FloorToInt(Dur * SR); int32 K = 0;
		for (int32 I = 0; I < Count && I0 + I < D.Num(); I++)
		{
			const double A = Buf[K], B = Buf[(K + 1) % N]; Buf[K] = (A + B) * 0.4985; K = (K + 1) % N;
			D[I0 + I] += Amp * A * FMath::Min(1., (Count - I) / (SR * 0.03));
		}
	}
	void Steel(TArray<float>& D, double F, double T0, double Dur, double Amp)
	{
		const int32 I0 = FMath::FloorToInt(T0 * SR), N = FMath::FloorToInt(Dur * SR); double Ph = 0;
		for (int32 I = 0; I < N && I0 + I < D.Num(); I++)
		{
			const double T = I / double(SR), Slide = 1 - 0.055 * FMath::Exp(-T * 18), Vib = 1 + 0.006 * FMath::Sin(2 * PI * 5.2 * T) * FMath::Min(1., T * 2);
			Ph += 2 * PI * F * Slide * Vib / SR;
			const double Env = FMath::Min(1., T / 0.04) * FMath::Exp(-T * 1.1) * FMath::Min(1., (N - I) / (SR * 0.08));
			D[I0 + I] += Amp * Env * (FMath::Sin(Ph) + 0.35 * FMath::Sin(2 * Ph) + 0.12 * FMath::Sin(3 * Ph));
		}
	}
	TSharedPtr<FS47AudioBuffer> TuneBuffer(FS47AudioContext& Ctx, const FString& Id)
	{
		static TMap<FString, TSharedPtr<FS47AudioBuffer>> TuneBufs;
		if (const auto* B = TuneBufs.Find(Id)) return *B;
		const TArray<int32> G = {55,59,62}, C = {60,64,67}, D = {62,66,69}, G7 = {55,59,65}, Em = {52,55,59};
		struct FBar { int32 Root; TArray<int32> Chord, Mel; };
		TArray<FBar> Bars; double Bpm; int32 Beats;
		if (Id == TEXT("waltz") || Id == TEXT("far"))
		{
			Bpm = 100; Beats = 3;
			Bars = {{43,G,{71,74,71}},{48,C,{72,71,69}},{50,D,{69,66,62}},{43,G,{67,0,0}}, {43,G,{74,76,74}},{48,C,{72,0,76}},{50,D,{74,72,69}},{43,G,{67,0,0}}};
		}
		else if (Id == TEXT("ballad"))
		{
			Bpm = 74; Beats = 4;
			Bars = {{43,G,{67,0,71,74}},{43,G7,{77,0,74,0}},{48,C,{76,0,72,0}},{43,G,{71,0,0,0}}, {40,Em,{71,0,67,71}},{48,C,{72,0,76,0}},{50,D,{74,0,69,0}},{43,G,{67,0,0,0}}};
		}
		else
		{
			Bpm = 116; Beats = 4;
			Bars = {{43,G,{67,71,74,71}},{48,C,{72,0,69,0}},{43,G,{71,67,71,74}},{50,D,{69,0,0,0}}, {43,G,{74,76,74,71}},{48,C,{72,0,76,0}},{50,D,{74,72,69,66}},{43,G,{67,0,0,0}}};
		}
		const double Beat = 60 / Bpm, Len = Bars.Num() * Beats * Beat;
		TArray<float> Data; Data.SetNumZeroed(FMath::CeilToInt((Len + 0.2) * SR));
		for (int32 I = 0; I < Bars.Num(); I++)
		{
			const auto& B = Bars[I]; const double T0 = I * Beats * Beat;
			for (int32 K = 0; K < Beats; K++)
			{
				const double T = T0 + K * Beat;
				if (K == 0 || (Beats == 4 && K == 2)) Pluck(Data, Hz(B.Root + (K == 2 ? 7 : 0)), T, Beat * 1.6, 0.36, 0.3);
				else for (int32 J = 0; J < B.Chord.Num(); J++) Pluck(Data, Hz(B.Chord[J]), T + J * 0.012, Beat * 0.9, 0.11, 0.6);
				if (const int32 M = B.Mel[K]) { int32 Hold = 1; while (K + Hold < Beats && !B.Mel[K + Hold]) Hold++; Steel(Data, Hz(M), T, Beat * (Hold + 0.6), 0.17); }
			}
		}
		double Peak = 0; for (float V : Data) Peak = FMath::Max(Peak, double(FMath::Abs(V)));
		if (Peak > 0) for (float& V : Data) V *= 0.6 / Peak;
		auto B = Ctx.CreateBuffer(1, FMath::FloorToInt(Len * SR), SR); FMemory::Memcpy(B->GetChannelData(0), Data.GetData(), B->Length() * sizeof(float));
		TuneBufs.Add(Id, B); return B;
	}
}

const TArray<FString> S47Radio::DIAL = { TEXT("caliche"), TEXT("roswell"), TEXT("far") };
int32 S47Radio::Freq(const FString& Id) { return Id == TEXT("caliche") ? 1420 : Id == TEXT("roswell") ? 1230 : 1090; }

// radio.ts sayTime, secsOf, seed/rng/syllables and dialX.
FString S47Radio::SayTime(double Clock)
{
	static const TCHAR* Ones[] = {TEXT("zero"),TEXT("one"),TEXT("two"),TEXT("three"),TEXT("four"),TEXT("five"),TEXT("six"),TEXT("seven"),TEXT("eight"),TEXT("nine"),TEXT("ten"),TEXT("eleven"),TEXT("twelve"),TEXT("thirteen"),TEXT("fourteen"),TEXT("fifteen"),TEXT("sixteen"),TEXT("seventeen"),TEXT("eighteen"),TEXT("nineteen")};
	static const TCHAR* Tens[] = {TEXT(""),TEXT(""),TEXT("twenty"),TEXT("thirty"),TEXT("forty"),TEXT("fifty")};
	const double S = Day(Clock); const int32 Hour = FMath::FloorToInt(S / 3600) % 12, H = Hour ? Hour : 12, M = FMath::FloorToInt(S / 60) % 60;
	if (M == 0) return FString(Ones[H]) + TEXT(" o'clock");
	if (M < 10) return FString(Ones[H]) + TEXT("-oh-") + Ones[M];
	const FString Mm = M < 20 ? FString(Ones[M]) : FString(Tens[M / 10]) + (M % 10 ? FString(TEXT("-")) + Ones[M % 10] : FString());
	return FString(Ones[H]) + TEXT(" ") + Mm;
}
double S47Radio::SecsOf(const FString& Text)
{
	// JS split(/\s+/) keeps an empty first/last word.
	FRegexMatcher M(FRegexPattern(TEXT("\\s+")), Text); int32 Count = 1; while (M.FindNext()) Count++;
	return FMath::Clamp(1.1 + Count * 0.32, 2.4, 7.5);
}
TArray<FS47RadioSyllable> S47Radio::Syllables(const FString& Text, double Secs)
{
	uint32 Seed = 2166136261u; for (TCHAR C : Text) Seed = (Seed ^ uint32(C)) * 16777619u;
	auto R = [&Seed]() { Seed += 0x6d2b79f5u; uint32 V = (Seed ^ (Seed >> 15)) * (1u | Seed); V ^= V + (V ^ (V >> 7)) * (61u | V); return double(V ^ (V >> 14)) / 4294967296.; };
	TArray<FString> Words; FRegexMatcher M(FRegexPattern(TEXT("\\S+")), Text); while (M.FindNext()) Words.Add(M.GetCaptureGroup(0));
	TArray<int32> Per; int32 Pauses = 0, Total = 0;
	auto Punctuation = [](const FString& W) { return !W.IsEmpty() && FString(TEXT(",.;:!? ")).Contains(W.Right(1)) && W.Right(1) != TEXT(" "); };
	for (const auto& W : Words)
	{
		int32 Letters = 0; for (TCHAR C : W) if ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z')) Letters++;
		const int32 N = FMath::Clamp(FMath::RoundToInt(Letters / 3.), 1, 4); Per.Add(N); Total += N; if (Punctuation(W)) Pauses++;
	}
	const double D = FMath::Max(0.12, (Secs - Pauses * 0.16) / Total); TArray<FS47RadioSyllable> Out; double At = 0, F = 1;
	for (int32 I = 0; I < Words.Num(); I++)
	{
		for (int32 K = 0; K < Per[I]; K++)
		{
			F = FMath::Clamp(F + (R() - 0.5) * 0.22, 0.86, 1.22);
			const bool bEnd = I == Words.Num() - 1 && K == Per[I] - 1;
			const double P = bEnd ? (Words[I].EndsWith(TEXT("?")) ? 1.25 : 0.8) : F * (K == 0 ? 1.07 : 1);
			Out.Add({At, D * (0.7 + R() * 0.25), P}); At += D;
		}
		if (Punctuation(Words[I])) At += 0.16;
	}
	return Out;
}
double S47Radio::DialX(double Freq)
{
	const double P[][2] = {{540,88},{600,125},{700,162},{800,199},{1000,236},{1200,273},{1600,310}};
	for (int32 I = 1; I < 7; I++) if (Freq <= P[I][0]) return P[I-1][1] + (P[I][1] - P[I-1][1]) * (Freq - P[I-1][0]) / (P[I][0] - P[I-1][0]);
	return P[6][1];
}
TArray<FS47BoyCallTime> S47Radio::BoyCallTimes(FS47Radio& Radio)
{
	TArray<FS47BoyCallTime> Out;
	const auto Call = BOY_CALL();
	for (const auto& L : Call[0]->Lines)
	{
		const FString Text = L.At(0); const bool bBoy = L.Who == TEXT("boy");
		const auto Clips = bBoy ? (Radio.CallerVoice() ? Radio.CallerBank.Resolve(Text, 0, TEXT("boy")) : TOptional<TArray<FS47RadioClip>>()) : Radio.HostVoice() ? Radio.HostBank.Resolve(Text, 0) : TOptional<TArray<FS47RadioClip>>();
		double Secs = SecsOf(Text); if (Clips.IsSet()) { Secs = (Clips->Num() - 1) * FRAGMENT_GAP; for (const auto& C : *Clips) Secs += C.Duration; }
		Out.Add({Text,bBoy,Secs,GAP});
	}
	return Out;
}

// radio.ts Station, in source order.
FS47Station::FS47Station(const FString& InId, FS47Radio& InRadio) : Id(InId), Radio(InRadio)
{
	Queue = { Segment(TEXT("dead")) };
	if (Id == TEXT("caliche")) { Cfg.Loop = NIGHT_LINE(); Cfg.FixedAt = S47Radio::SIGN_OFF; Cfg.FixedSeq = SIGN_OFF_SEQ(); Cfg.Then = TEXT("dead"); }
	else if (Id == TEXT("roswell")) { Cfg.Loop = MORNING(); Cfg.FixedAt = S47Radio::SIGN_ON; Cfg.FixedSeq = SIGN_ON_SEQ(); Cfg.bOffBefore = true; }
	else Cfg.Loop = FAR();
}
FS47Station::~FS47Station() { Sleep(); }
double FS47Station::Strength(double Clock, double Time) const
{
	if (!OnAir()) return 0;
	if (Id == TEXT("caliche")) return 1;
	if (Id == TEXT("roswell")) return 0.8;
	const double Daylight = 1 - FMath::SmoothStep(S47Radio::SIGN_ON + 300, S47Radio::SIGN_ON + 1800, Day(Clock));
	const double Fade = 0.5 + 0.3 * FMath::Sin(Time * 0.37) + 0.15 * FMath::Sin(Time * 1.13 + 1);
	return FMath::Clamp(Fade, 0.08, 0.9) * Daylight;
}
void FS47Station::Reset(double Clock) { Prev.Reset(); Calm = CALM; Fixed(Clock); }
bool FS47Station::Free() const { return OnAir() && Queue.Num() == 1 && Cfg.Loop.Contains(Queue[0]) && Calm >= CALM; }
TSharedPtr<FS47RadioSeg> FS47Station::Slot(const TArray<FS47RadioLine>& Lines, double Clock)
{
	if (!Free()) return nullptr;
	const double D = Day(Clock);
	if (Cfg.FixedAt.IsSet() && After == TEXT("loop") && D < *Cfg.FixedAt && D > *Cfg.FixedAt - BEFORE_FIXED) return nullptr;
	const auto& S = Seg();
	if (S.Kind == TEXT("music"))
	{
		if (T < RECORD_MIN) return nullptr;
		if (T + RECORD_FADE < Len)
		{
			Len = T + RECORD_FADE; bFading = true;
			if (N && Radio.Audio->Ctx) { FS47AudioContext::FBatch B(*Radio.Audio->Ctx); auto& G = N->Mus->Gain; const double Now = Radio.Audio->Ctx->CurrentTime(); G.CancelScheduledValues(Now); G.SetValueAtTime(G.Value(), Now); G.LinearRampToValueAtTime(0, Now + RECORD_FADE); }
		}
		if (Cfg.Loop[LoopAt % Cfg.Loop.Num()]->Kind == TEXT("talk")) LoopAt++;
	}
	else if (S.Kind != TEXT("talk")) return nullptr;
	auto Slt = Segment(TEXT("talk")); Slt->Lines = Lines; Slt->bSlotted = true; Queue.Insert(Slt, 1); return Slt;
}
FString FS47Station::Where(const TSharedPtr<FS47RadioSeg>& X) const
{
	const int32 I = Queue.IndexOfByKey(X); return I == 0 ? TEXT("on") : I > 0 ? TEXT("waiting") : FString();
}
bool FS47Station::Unslot(const TSharedPtr<FS47RadioSeg>& X)
{
	const int32 I = Queue.IndexOfByKey(X); if (I < 1) return false; Queue.RemoveAt(I); return true;
}
double FS47Station::CutIn(const TArray<FS47RadioLine>& Lines, double Clock)
{
	if (!Free() || Interrupted.IsSet() || bHostPending || Lines.IsEmpty()) return 0;
	const double D = Day(Clock);
	if (Cfg.FixedAt.IsSet() && After == TEXT("loop") && D >= *Cfg.FixedAt - BEFORE_FIXED && D < *Cfg.FixedAt) return 0;
	FInterrupted Previous {Queue,T,Len,Sc,Said,SegNo,TrackSkip,bFading,bHostReady,Radio.HostVoice(),Radio.CallerVoice(),Spoken};
	auto S = Segment(TEXT("talk")); S->Lines = Lines; S->Duration = S47Radio::ARCHIVE_SECS;
	Start({S}, Clock); Interrupted = MoveTemp(Previous);
	Archive = FS47ArchiveProgress{++ArchiveId, bHostPending ? TEXT("loading") : TEXT("playing"),0,S47Radio::ARCHIVE_SECS};
	return S47Radio::ARCHIVE_SECS;
}
TArray<FS47Station::FSpokenLine> FS47Station::LinesFor(const FS47RadioSeg& S, double Clock)
{
	TArray<FSpokenLine> Out; if (S.Kind != TEXT("talk")) return Out;
	const double Scale = TalkScale(S, Clock);
	for (const auto& L : S.Lines)
	{
		FSpokenLine X; X.Line = L; X.Text = L.At(Clock);
		X.Bank = Id != TEXT("caliche") ? FString() : !S.Duration.IsSet() && Radio.HostVoice() && L.Who == TEXT("host") ? TEXT("host") : Radio.CallerVoice() && Caller(L.Who) ? TEXT("caller") : FString();
		X.Clips = X.Bank == TEXT("host") ? Radio.HostBank.Resolve(X.Text, Clock) : X.Bank == TEXT("caller") ? Radio.CallerBank.Resolve(X.Text, Clock, L.Who) : TOptional<TArray<FS47RadioClip>>();
		X.Secs = S47Radio::SecsOf(X.Text) * Scale;
		if (X.Clips.IsSet()) { X.Secs = (X.Clips->Num() - 1) * FRAGMENT_GAP; for (const auto& C : *X.Clips) X.Secs += C.Duration; }
		Out.Add(MoveTemp(X));
	}
	return Out;
}
void FS47Station::Layout()
{
	const auto& S = Seg(); double At = 0.2, Gap = GAP;
	if (S.Kind == TEXT("talk") && S.Duration.IsSet())
	{
		bool bRecorded = true; double Speech = 0; for (const auto& L : Spoken) { bRecorded &= L.Buffers.IsSet(); Speech += L.Secs; }
		if (bRecorded && Speech + 0.4 <= *S.Duration) Gap = (*S.Duration - 0.4 - Speech) / FMath::Max(1, Spoken.Num() - 1);
		else
		{
			const double Scale = TalkScale(S, Sc); At *= Scale; Gap *= Scale;
			for (auto& L : Spoken) { L.Secs = S47Radio::SecsOf(L.Text) * Scale; if (bRecorded) { L.Clips.Reset(); L.Buffers.Reset(); } }
		}
	}
	for (auto& L : Spoken) { L.At = At; At += L.Secs + Gap; }
}
double FS47Station::LengthOf(const FS47RadioSeg& S, double Clock)
{
	if (S.Kind != TEXT("talk")) return SegLen(S, Clock);
	if (S.Duration.IsSet()) return *S.Duration;
	double Sum = 0.4; for (const auto& L : LinesFor(S, Clock)) Sum += L.Secs + GAP; return Sum;
}
void FS47Station::Plan(double Clock)
{
	CancelHost(); Spoken = LinesFor(Seg(), Clock);
	if (Seg().Kind == TEXT("talk") && Seg().Duration.IsSet() && Spoken.ContainsByPredicate([](const FSpokenLine& L) { return !L.Clips.IsSet(); })) for (auto& L : Spoken) L.Clips.Reset();
	Layout(); bHostReady = !Spoken.ContainsByPredicate([](const FSpokenLine& L) { return L.Clips.IsSet(); });
	Len = SegLen(Seg(), Clock);
	if (Seg().Kind == TEXT("talk") && !Seg().Duration.IsSet()) { Len = 0.4; for (const auto& L : Spoken) Len += L.Secs + GAP; }
}
double FS47Station::MapPosition(const TArray<double>& Before, const TArray<double>& New) const
{
	double OldAt = 0.2, NewAt = 0.2;
	for (int32 I = 0; I < Before.Num(); I++)
	{
		if (T < OldAt) return NewAt - (OldAt - T);
		if (T < OldAt + Before[I]) return NewAt + (T - OldAt) / Before[I] * New[I];
		if (T < OldAt + Before[I] + GAP) return NewAt + New[I] + (T - OldAt - Before[I]);
		OldAt += Before[I] + GAP; NewAt += New[I] + GAP;
	}
	return NewAt + T - OldAt;
}
void FS47Station::Revoice(const FString& Kind)
{
	if (Seg().Kind != TEXT("talk")) return;
	if (!Seg().Lines.ContainsByPredicate([&](const FS47RadioLine& L) { return Kind == TEXT("host") ? L.Who == TEXT("host") : Caller(L.Who); })) return;
	if (Kind == TEXT("caller") && Seg().Duration.IsSet() && T > 0) CancelArchive();
	TArray<double> Before; for (const auto& L : Spoken) Before.Add(L.Secs);
	Plan(Sc);
	if (!Seg().Duration.IsSet()) { TArray<double> New; for (const auto& L : Spoken) New.Add(L.Secs); T = MapPosition(Before, New); }
	Sync();
}
void FS47Station::Start(const TArray<TSharedPtr<FS47RadioSeg>>& Segs, double Clock, double Skip)
{
	CancelArchive(); Interrupted.Reset(); Queue = Segs;
	T = 0; Said = -1; Sc = Clock; SegNo++; bFading = false;
	Plan(Clock); Forward(Skip, Clock, true); Sync();
}
TArray<TSharedPtr<FS47RadioSeg>> FS47Station::Refill(double Clock)
{
	if (After == TEXT("dead")) return {Segment(TEXT("dead"))};
	const double D = Day(Clock);
	if (Id == TEXT("caliche") && !Radio.bBoyCalled && D >= S47Radio::BOY_AT && D < S47Radio::SIGN_OFF - 300) { Radio.bBoyCalled = true; return BOY_CALL(); }
	const auto S = Cfg.Loop[LoopAt % Cfg.Loop.Num()]; LoopAt++; return {S};
}
void FS47Station::Fixed(double Clock)
{
	const auto Before = Prev; Prev = Clock;
	if (Before.IsSet() && Clock >= *Before - 120)
	{
		if (Cfg.FixedAt.IsSet() && *Before < *Cfg.FixedAt && Clock >= *Cfg.FixedAt) Cross(Clock - *Cfg.FixedAt, Clock);
		return;
	}
	LoopAt = FMath::FloorToInt(FMath::FRand() * Cfg.Loop.Num());
	if (!Cfg.FixedAt.IsSet() || Clock < *Cfg.FixedAt)
	{
		After = TEXT("loop");
		if (Cfg.FixedAt.IsSet() && Cfg.bOffBefore) Start({Segment(TEXT("dead"))}, Clock);
		else Start(Refill(Clock), Clock, FMath::FRand() * 20);
	}
	else Cross(Clock - *Cfg.FixedAt, Clock);
}
void FS47Station::Cross(double Since, double Clock)
{
	After = Cfg.Then; double Total = 0; for (const auto& S : Cfg.FixedSeq) Total += LengthOf(*S, Clock);
	if (Since > Total + 30) Start(Refill(Clock), Clock, Cfg.Then == TEXT("loop") ? FMath::FRand() * 20 : 0);
	else Start(Cfg.FixedSeq, Clock, Since);
}
bool FS47Station::Forward(double Dt, double Clock, bool bQuiet)
{
	T += Dt;
	if (Interrupted.IsSet() && Archive.IsSet() && Archive->State != TEXT("cancelled")) Archive->Elapsed = FMath::Min(Len, T);
	bool bChanged = false;
	while (T >= Len)
	{
		T -= Len;
		if (Interrupted.IsSet())
		{
			if (Archive.IsSet() && Archive->State != TEXT("cancelled")) { Archive->State = TEXT("complete"); Archive->Elapsed = S47Radio::ARCHIVE_SECS; }
			const auto Previous = MoveTemp(*Interrupted); Interrupted.Reset(); CancelHost();
			// radio.ts: archive overrun is still archive time; resume the held exact sample.
			Queue = Previous.Queue; T = Previous.T; Len = Previous.Len; Said = Previous.Said; Sc = Previous.Sc; SegNo = Previous.SegNo;
			TrackSkip = Previous.TrackSkip; Calm = 0; bFading = Previous.bFading; Spoken = Previous.Spoken; bHostReady = Previous.bHostReady;
			if (Seg().Kind == TEXT("talk") && ((Previous.bHostVoice != Radio.HostVoice() && Seg().Lines.ContainsByPredicate([](const FS47RadioLine& L) { return L.Who == TEXT("host"); })) || (Previous.bCallerVoice != Radio.CallerVoice() && Seg().Lines.ContainsByPredicate([](const FS47RadioLine& L) { return Caller(L.Who); }))))
			{
				TArray<double> Before; for (const auto& L : Spoken) Before.Add(L.Secs); Plan(Sc);
				TArray<double> New; for (const auto& L : Spoken) New.Add(L.Secs); T = MapPosition(Before, New);
			}
			bChanged = true; continue;
		}
		Queue.RemoveAt(0); if (Queue.IsEmpty()) Queue.Append(Refill(Clock));
		Said = -1; Sc = Clock; SegNo++; bFading = false; Plan(Clock); bChanged = true;
	}
	if (Seg().Kind == TEXT("talk") && (bQuiet || !N || bHostReady))
	{
		int32 I = 0; for (; I < Spoken.Num(); I++) if (T < Spoken[I].At + Spoken[I].Secs) break;
		if (I < Seg().Lines.Num() && T >= Spoken[I].At && I > Said)
		{
			Said = I; const auto& L = Spoken[I];
			if (!bQuiet && OnLine) OnLine(L.Text, L.Line.By, L.Buffers.IsSet() || Seg().Duration.IsSet() ? TOptional<double>(L.Secs) : TOptional<double>());
		}
	}
	return bChanged;
}
void FS47Station::Update(double Dt, double Clock)
{
	Fixed(Clock); const auto Ctx = Radio.Audio->Ctx;
	const double Advance = bHostPending ? 0 : AudioAt.IsSet() && Ctx ? FMath::Max(0., Ctx->CurrentTime() - *AudioAt - T) : Dt;
	const double ArchiveTime = Interrupted.IsSet() ? Advance : 0;
	if (!bHostPending && Forward(Advance, Clock)) Sync();
	Calm = Cfg.Loop.Contains(Queue[0]) ? Calm + Advance - ArchiveTime : 0;
}

TSharedPtr<FS47GainNode> FS47Station::EnsureBus()
{
	const auto Ctx = Radio.Audio->Ctx; if (!Ctx || Bus) return Bus; Bus = Ctx->CreateGain(); return Bus;
}
void FS47Station::Listen(bool bOn)
{
	Listeners += bOn ? 1 : -1;
	if (Listeners > 0 && !N) Wake(); else if (Listeners <= 0 && N) Sleep();
}
void FS47Station::Wake()
{
	const auto Ctx = Radio.Audio->Ctx; const auto B = EnsureBus(); if (!Ctx || !B) return;
	auto X = MakeShared<FNodes>();
	{
	FS47AudioContext::FBatch Batch(*Ctx);
	X->Horn = Ctx->CreateOscillator(); X->Horn->SetType(TEXT("sawtooth")); X->Horn->Frequency.SetValue(120);
	X->Vib = Ctx->CreateOscillator(); X->Vib->Frequency.SetValue(5.3);
	auto Vd = Ctx->CreateGain(); Vd->Gain.SetValue(2.2); X->Vib->Connect(Vd); Vd->Connect(X->Horn->Frequency);
	X->Mute = Ctx->CreateBiquadFilter(); X->Mute->SetType(TEXT("lowpass")); X->Mute->Frequency.SetValue(400); X->Mute->Q.SetValue(6.5);
	X->Voice = Ctx->CreateGain(); X->Voice->Gain.SetValue(0); X->Horn->Connect(X->Mute); X->Mute->Connect(X->Voice); X->Voice->Connect(B);
	X->Osc = Ctx->CreateOscillator(); X->Osc->Frequency.SetValue(760);
	X->Car = Ctx->CreateGain(); X->Car->Gain.SetValue(0); X->Osc->Connect(X->Car); X->Car->Connect(B);
	X->Mus = Ctx->CreateGain(); X->Mus->Gain.SetValue(1); X->Mus->Connect(B);
	X->Horn->Start(); X->Vib->Start(); X->Osc->Start(); N = X;
	}
	Sync(); AnnounceRecordedLine();
}
void FS47Station::Sleep()
{
	const auto X = N; if (!X) return; N.Reset(); CancelArchive(); CancelHost();
	if (X->Music) X->Music->Stop(); StopAnthem(*X); StopTrack(*X);
	X->Horn->Stop(); X->Vib->Stop(); X->Osc->Stop(); X->Voice->Disconnect(); X->Car->Disconnect(); X->Mus->Disconnect();
}
void FS47Station::Sync()
{
	const auto X = N; const auto Ctx = Radio.Audio->Ctx; if (!X || !Ctx) return;
	// The fallback buffer is built outside the audio lock, like Audio.ts's long buffers.
	const bool bBars = bTrackFailed || TrackSkip == SegNo;
	const auto Tune = Seg().Kind == TEXT("music") && bBars ? TuneBuffer(*Ctx, Seg().Tune) : nullptr;
	const double Now = Ctx->CurrentTime(); const auto& S = Seg(); const double At = T;
	StopHostSources(); AudioAt.Reset();
	{
		FS47AudioContext::FBatch Batch(*Ctx);
		auto& Mg = X->Mus->Gain; Mg.CancelScheduledValues(Now); Mg.SetValueAtTime(1, Now);
		if (bFading) Mg.LinearRampToValueAtTime(0, Now + FMath::Max(0.05, Len - At));
		if (X->Music) { X->Music->Stop(); X->Music->Disconnect(); X->Music.Reset(); }
	}
	if (S.Kind == TEXT("music") && !bBars) PlayTrack(*X, S.Tune, S.Len); else StopTrack(*X);
	if (Tune)
	{
		FS47AudioContext::FBatch Batch(*Ctx); auto Src = Ctx->CreateBufferSource(); Src->Buffer = Tune; Src->bLoop = true; Src->Connect(X->Mus); Src->Start(Now, FMath::Fmod(At, Tune->Duration())); X->Music = Src;
	}
	{
		FS47AudioContext::FBatch Batch(*Ctx);
		X->Horn->Frequency.CancelScheduledValues(Now); X->Mute->Frequency.CancelScheduledValues(Now); X->Voice->Gain.CancelScheduledValues(Now); X->Voice->Gain.SetValueAtTime(0, Now);
	}
	if (S.Kind == TEXT("talk") && !bHostReady) { PrepareHost(); return; }
	{
		FS47AudioContext::FBatch Batch(*Ctx);
		if (S.Kind == TEXT("talk") && bHostReady)
		{
			if (S.Duration.IsSet() || Spoken.ContainsByPredicate([](const FSpokenLine& L) { return L.Buffers.IsSet(); })) AudioAt = Now - At;
			if (Interrupted.IsSet() && Archive.IsSet() && Archive->State != TEXT("cancelled")) Archive->State = TEXT("playing");
			for (const auto& L : Spoken)
			{
				if (L.Buffers.IsSet())
				{
					double PartAt = L.At;
					for (const auto& Buffer : *L.Buffers)
					{
						if (PartAt + Buffer->Duration() > At)
						{
							auto Source = Ctx->CreateBufferSource(); Source->Buffer = Buffer; Source->Connect(Bus);
							TWeakPtr<FS47Station> Weak = AsShared(); TWeakPtr<FS47BufferSourceNode> Ws = Source;
							Source->OnEnded = [Weak, Ws] { if (auto St = Weak.Pin()) { if (auto Src = Ws.Pin()) { Src->Disconnect(); St->HostSources.Remove(Src); } } };
							HostSources.Add(Source); Source->Start(Now + FMath::Max(0., PartAt - At), FMath::Max(0., At - PartAt));
						}
						PartAt += Buffer->Duration() + FRAGMENT_GAP;
					}
				}
				else if (L.At + L.Secs > At) for (const auto& Y : S47Radio::Syllables(L.Text, L.Secs))
				{
					if (L.At + Y.At < At) continue;
					const double Ts = Now + L.At + Y.At - At, F = Pitch(L.Line.Who) * Y.F;
					auto& Fq = X->Horn->Frequency; auto& Mf = X->Mute->Frequency; auto& G = X->Voice->Gain;
					Fq.SetValueAtTime(F, Ts); Fq.LinearRampToValueAtTime(F * 0.94, Ts + Y.D);
					Mf.SetValueAtTime(330, Ts); Mf.LinearRampToValueAtTime(1350, Ts + Y.D * 0.3); Mf.LinearRampToValueAtTime(430, Ts + Y.D);
					G.SetValueAtTime(0, Ts); G.LinearRampToValueAtTime(0.42, Ts + 0.025); G.SetValueAtTime(0.42, Ts + Y.D * 0.7); G.LinearRampToValueAtTime(0, Ts + Y.D);
				}
			}
		}
	}
	if (S.Kind == TEXT("anthem")) PlayAnthem(*X); else StopAnthem(*X);
	const bool bCarrier = S.Kind == TEXT("carrier") || (S.Kind == TEXT("anthem") && bAnthemFailed);
	X->Car->Gain.SetTargetAtTime(bCarrier ? 0.03 : 0, Now, 0.05);
}
void FS47Station::StopHostSources()
{
	for (const auto& Source : HostSources) { Source->OnEnded = nullptr; Source->Stop(); Source->Disconnect(); } HostSources.Empty();
}
void FS47Station::CancelHost() { HostGeneration++; bHostPending = false; StopHostSources(); AudioAt.Reset(); }
void FS47Station::CancelArchive()
{
	if (Archive.IsSet() && (Archive->State == TEXT("loading") || Archive->State == TEXT("playing"))) Archive->State = TEXT("cancelled");
}
void FS47Station::AnnounceRecordedLine()
{
	if (!N || !bHostReady) return;
	for (int32 I = 0; I < Spoken.Num(); I++) if (Spoken[I].Buffers.IsSet() && T >= Spoken[I].At && T < Spoken[I].At + Spoken[I].Secs) { Said = I - 1; Forward(0, Radio.Clock); return; }
}
void FS47Station::PrepareHost()
{
	if (!Radio.Audio->Ctx || !N || bHostPending) return;
	bHostPending = true; const int32 Generation = ++HostGeneration;
	TArray<FS47RadioClip> HostClips, CallerClips;
	for (const auto& L : Spoken) if (L.Clips.IsSet()) (L.Bank == TEXT("caller") ? CallerClips : HostClips).Append(*L.Clips);
	if (Interrupted.IsSet() && Archive.IsSet() && Archive->State != TEXT("cancelled")) Archive->State = TEXT("loading");
	// Unreal reads short selected local WAVE clips synchronously; there is no browser fetch task.
	TMap<FString,TSharedPtr<FS47AudioBuffer>> HostBuffers, CallerBuffers;
	auto Aborted = [this, Generation] { return Generation != HostGeneration || !N; };
	bool bOk = Radio.HostBank.Load(HostClips, HostBuffers, Aborted) && Radio.CallerBank.Load(CallerClips, CallerBuffers, Aborted);
	if (Aborted()) return;
	TArray<double> Before; for (const auto& L : Spoken) Before.Add(L.Secs);
	if (bOk) for (auto& L : Spoken) if (L.Clips.IsSet())
	{
		TArray<TSharedPtr<FS47AudioBuffer>> Buffers; const auto& Map = L.Bank == TEXT("caller") ? CallerBuffers : HostBuffers;
		for (const auto& Clip : *L.Clips) { const auto* Buffer = Map.Find(Clip.Id); if (!Buffer) { bOk = false; break; } Buffers.Add(*Buffer); }
		if (!bOk) break;
		L.Buffers = Buffers; L.Secs = (Buffers.Num() - 1) * FRAGMENT_GAP; for (const auto& Buffer : Buffers) L.Secs += Buffer->Duration();
	}
	if (!bOk) for (auto& L : Spoken) { L.Clips.Reset(); L.Buffers.Reset(); L.Secs = S47Radio::SecsOf(L.Text); }
	Layout(); if (!Seg().Duration.IsSet()) { TArray<double> New; for (const auto& L : Spoken) New.Add(L.Secs); T = MapPosition(Before, New); }
	if (Seg().Duration.IsSet()) Len = *Seg().Duration;
	else { Len = 0.4; for (const auto& L : Spoken) Len += L.Secs + GAP; }
	bHostReady = true; bHostPending = false; Sync(); AnnounceRecordedLine();
	if (bOk) Forward(0, Radio.Clock);
}
FS47Station::FVoiceStatus FS47Station::VoiceStatus(const FString& Bank) const
{
	FVoiceStatus Out; Out.bLoading = bHostPending;
	for (const auto& L : Spoken)
	{
		if (!(Bank == TEXT("host") ? L.Line.Who == TEXT("host") : Caller(L.Line.Who))) continue;
		if (T >= L.At && T < L.At + L.Secs) { Out.Text = L.Text; Out.Duration = L.Secs; Out.bPlaying = L.Buffers.IsSet() && Awake() && Radio.Audio->Ctx && Radio.Audio->Ctx->State() == TEXT("running"); }
		if (L.Clips.IsSet()) for (int32 I = 0; I < L.Clips->Num(); I++) { auto Clip = (*L.Clips)[I]; if (L.Buffers.IsSet()) Clip.Duration = (*L.Buffers)[I]->Duration(); Out.Clips.Add(Clip); }
	}
	Out.BufferCount = Bank == TEXT("host") ? Radio.HostBank.BufferCount() : Radio.CallerBank.BufferCount(); return Out;
}
void FS47Station::PlayAnthem(FNodes& Nodes)
{
	if (Nodes.Anthem || bAnthemFailed) return;
	const auto Ctx = Radio.Audio->Ctx;
	auto Source = FS47MediaSource::Open(Ctx.ToSharedRef(), TEXT("radio/anthem.mp3"));
	if (!Source) { bAnthemFailed = true; return; }
	auto G = Ctx->CreateGain(); G->Gain.SetValue(0.9); Source->Connect(G); G->Connect(Nodes.Mus); Nodes.Anthem = Source; Nodes.AnthemGain = G;
	TWeakPtr<FS47Station> Weak = AsShared(); TWeakPtr<FS47MediaSource> Ws = Source;
	Source->OnError = [Weak, Ws] { if (auto St = Weak.Pin()) if (St->N && St->N->Anthem == Ws.Pin()) { St->bAnthemFailed = true; St->StopAnthem(*St->N); St->Sync(); } };
	const double At = Seg().Kind == TEXT("anthem") ? T : 0; if (At > 0.5 && At < Source->Duration()) Source->Seek(At);
	if (!Source->Play()) { bAnthemFailed = true; StopAnthem(Nodes); }
}
void FS47Station::PlayTrack(FNodes& Nodes, const FString& Tune, double Length)
{
	const auto Ctx = Radio.Audio->Ctx; const auto Tr = Track(Tune);
	// A local media source replaces HTMLAudioElement; the seek and gain timeline are unchanged.
	const auto Source = FS47MediaSource::Open(Ctx.ToSharedRef(), TEXT("radio/") + Tr.File + TEXT(".mp3"));
	if (!Source) { bTrackFailed = true; Sync(); return; }
	StopTrack(Nodes); Nodes.Track = Source; auto G = Ctx->CreateGain(); G->Gain.SetValue(0); Source->Connect(G); G->Connect(Nodes.Mus); Nodes.TrackGain = G;
	const double Room = FMath::Max(1., Tr.Dur - Length - 2), From = FMath::Fmod(SegNo * 61.7, Room); const int32 CurrentSeg = SegNo;
	Source->Seek(From + T);
	TWeakPtr<FS47Station> Weak = AsShared(); TWeakPtr<FS47MediaSource> Ws = Source;
	Source->OnError = [Weak, Ws] { if (auto St = Weak.Pin()) if (St->N && St->N->Track == Ws.Pin()) { St->bTrackFailed = true; St->StopTrack(*St->N); St->Sync(); } };
	if (!Source->Play()) { TrackSkip = CurrentSeg; StopTrack(Nodes); Sync(); return; }
	FS47AudioContext::FBatch Batch(*Ctx); const double Now = Ctx->CurrentTime(), Left = Length - T; auto& Gain = G->Gain;
	Gain.CancelScheduledValues(Now); Gain.SetValueAtTime(0, Now); Gain.LinearRampToValueAtTime(0.85, Now + FMath::Max(0.3, 1.5 - T));
	if (Left > 2) { Gain.SetValueAtTime(0.85, Now + Left - 2); Gain.LinearRampToValueAtTime(0, Now + Left); }
}
void FS47Station::StopTrack(FNodes& Nodes)
{
	if (!Nodes.Track) return;
	Nodes.Track->Pause(); if (Nodes.TrackGain) { Nodes.TrackGain->Gain.CancelScheduledValues(0); Nodes.TrackGain->Gain.SetValue(0); }
	// The port releases a stopped file handle rather than keeping an empty HTML element.
	Nodes.Track->Stop(); Nodes.Track->Disconnect(); Nodes.Track.Reset(); if (Nodes.TrackGain) Nodes.TrackGain->Disconnect(); Nodes.TrackGain.Reset();
}
void FS47Station::StopAnthem(FNodes& Nodes)
{
	if (!Nodes.Anthem) return; Nodes.Anthem->Stop(); Nodes.Anthem->Disconnect(); Nodes.Anthem.Reset(); if (Nodes.AnthemGain) Nodes.AnthemGain->Disconnect(); Nodes.AnthemGain.Reset();
}
TSharedPtr<FS47MediaSource> FS47Station::TrackEl() const { return N && N->Track && !N->Track->Paused() ? N->Track : nullptr; }
TSharedPtr<FS47MediaSource> FS47Station::AnthemEl() const { return N ? N->Anthem : nullptr; }

// radio.ts Receiver: the station and static through the same small AM speaker.
FS47Receiver::FS47Receiver(FS47Radio& InRadio, const FS47ReceiverOpts& InO) : Radio(InRadio), O(InO) {}
FS47Receiver::~FS47Receiver() { Reset(); }
void FS47Receiver::Set(bool bNewOn, const FString& NewStation)
{
	const FString St = NewStation.IsEmpty() ? Station : NewStation;
	if (St != Station) { Station = St; Sweep = 0.5; bDial = true; }
	if (bNewOn && !bOn) { Sweep = FMath::Max(Sweep, 0.25); bDial = true; }
	bOn = bNewOn;
}
FString FS47Receiver::Next()
{
	if (!bOn) { Set(true); return Station; }
	const int32 I = S47Radio::DIAL.IndexOfByKey(Station);
	if (I == S47Radio::DIAL.Num() - 1) { Set(false, S47Radio::DIAL[0]); return FString(); }
	Set(true, S47Radio::DIAL[I + 1]); return Station;
}
TSharedPtr<FS47Receiver::FNodes> FS47Receiver::Build()
{
	US47Audio* A = Radio.Audio; const auto Ctx = A->Ctx; const auto Static = A->StaticSrc(); if (!Ctx || !Static.IsSet()) return nullptr;
	FS47AudioContext::FBatch Batch(*Ctx); auto X = MakeShared<FNodes>(); X->Static = Static->Src;
	X->Sig = Ctx->CreateGain(); X->Sig->Gain.SetValue(0);
	X->Stat = Ctx->CreateGain(); X->Stat->Gain.SetValue(0); Static->Out->Connect(X->Stat);
	X->Mix = Ctx->CreateGain(); X->Sig->Connect(X->Mix); X->Stat->Connect(X->Mix);
	auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(1250); Bp->Q.SetValue(0.7);
	auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(3400);
	auto Sh = Ctx->CreateWaveShaper(); Sh->Curve.SetNumUninitialized(256); for (int32 I = 0; I < 256; I++) { const double V = I / 127.5 - 1; Sh->Curve[I] = FMath::Tanh(V * 1.6) / FMath::Tanh(1.6); }
	X->Osc = Ctx->CreateOscillator(); X->Osc->Frequency.SetValue(760); X->Car = Ctx->CreateGain(); X->Car->Gain.SetValue(0); X->Osc->Connect(X->Car); X->Car->Connect(X->Mix); X->Osc->Start();
	X->Out = Ctx->CreateGain(); X->Out->Gain.SetValue(0);
	X->Cab = Ctx->CreateBiquadFilter(); X->Cab->SetType(TEXT("lowpass")); X->Cab->Frequency.SetValue(20000); X->Cab->Q.SetValue(0.5);
	X->Mix->Connect(Bp); Bp->Connect(Lp); Lp->Connect(Sh); Sh->Connect(X->Cab); X->Cab->Connect(X->Out);
	X->Pan = StaticCastSharedPtr<FS47PannerNode>(A->Spatial(FVector::ZeroVector, O.Ref, A->RadioBus)); X->Out->Connect(X->Pan); return X;
}
void FS47Receiver::Update(double Dt)
{
	const auto Ctx = Radio.Audio->Ctx; if (!Ctx) return;
	Carry = FMath::Max(0., Carry - Dt);
	if (!N) { if (!bOn && !Carrying()) return; N = Build(); if (!N) return; }
	const auto X = N; const double Now = Ctx->CurrentTime();
	const FString Want = Live() ? Station : FString();
	if (Want != Listening)
	{
		if (!Listening.IsEmpty()) { Radio.Stations[Listening]->Listen(false); if (X->Bus) X->Bus->Disconnect(X->Sig); X->Bus.Reset(); }
		Listening = Want;
		if (!Want.IsEmpty()) { const auto St = Radio.Stations[Want]; const auto B = St->EnsureBus(); if (B) { B->Connect(X->Sig); X->Bus = B; } St->Listen(true); }
	}
	Sweep = FMath::Max(0., Sweep - Dt); const auto St = Radio.Stations[Station];
	const double S = Live() ? St->Strength(Radio.Clock, Radio.Time) : 0;
	const bool bCarrier = St->Seg().Kind == TEXT("carrier") || St->Seg().Kind == TEXT("anthem"); const double Tuning = Sweep > 0 ? 1 : 0;
	auto& Z = Fuzz;
	if (Live() && S > 0)
	{
		if (Z.Left > 0) Z.Left = FMath::Max(0., Z.Left - Dt);
		else if ((Z.Next -= Dt) <= 0) { Z.Len = Z.Left = 0.4 + FMath::FRand() * 1.2; Z.Depth = 0.3 + FMath::FRand() * 0.4; Z.Next = 20 + FMath::FRand() * 40; Crack(Z.Len, Z.Depth); }
	}
	else Z.Left = 0;
	if (bDial && Live()) Crack(0, 0.6); bDial = false;
	const double K = Z.Left > 0 ? Z.Depth * FMath::Sqrt(FMath::Max(0., FMath::Sin(PI * (1 - Z.Left / Z.Len)))) : 0; Interference = K;
	const double Crackle = K > 0 ? 0.5 + FMath::FRand() * 1.1 : 1; const bool bBare = Carrying(), bSounds = Live() || bBare;
	FS47AudioContext::FBatch Batch(*Ctx);
	X->Sig->Gain.SetTargetAtTime(Live() && !bBare ? S * (1 - Tuning) * (1 - 0.5 * K) * Power : 0, Now, bBare ? 0.008 : 0.05);
	X->Stat->Gain.SetTargetAtTime(bBare ? 0.035 * Power : Live() ? (0.03 + (bCarrier ? 0.02 : 0.36 * (1 - S)) + Tuning * 0.45 + 0.42 * K * Crackle) * Power : 0, Now, bBare ? 0.008 : K > 0 ? 0.015 : 0.04);
	X->Car->Gain.SetTargetAtTime(bBare ? 0.11 * Power : 0, Now, 0.008);
	const double M = FMath::Clamp(Muffle, 0., 1.);
	X->Out->Gain.SetTargetAtTime(bSounds ? O.Vol * (1 - 0.82 * M) : 0, Now, bSounds ? (bBare ? 0.01 : 0.08) : 0.15);
	X->Cab->Frequency.SetTargetAtTime(20000 * FMath::Pow(420. / 20000, M), Now, 0.1);
	if (Pos.IsSet() && FMath::IsFinite(Pos->X + Pos->Y + Pos->Z)) { X->Pan->PositionX.SetValue(Pos->X); X->Pan->PositionY.SetValue(Pos->Y); X->Pan->PositionZ.SetValue(Pos->Z); }
}
void FS47Receiver::Crack(double Len, double K)
{
	US47Audio* A = Radio.Audio; const FString Name = Len > 0.8 ? A->Or(TEXT("radio_crackle_long")) : A->Or(TEXT("radio_crackle_short"));
	const auto* B = A->Buf.Find(Name); if (!B || !N) return;
	FS47Sound Oa; Oa.DestNode = N->Mix; Oa.Gain = K * Power; Oa.Rate = Len ? FMath::Clamp((*B)->Duration() / Len, 0.75, 1.5) : 1; A->Sfx(Name, Oa); Cracks++;
}
bool FS47Receiver::Drift(double Secs, double Depth)
{
	if (!Live()) return false; Fuzz.Len = Fuzz.Left = Secs; Fuzz.Depth = Depth; Fuzz.Next = FMath::Max(Fuzz.Next, 20.); Crack(Secs, Depth); return true;
}
void FS47Receiver::Reset()
{
	bOn = false; Sweep = 0; bDial = false; Carry = 0;
	if (!Listening.IsEmpty()) { Radio.Stations[Listening]->Listen(false); Listening.Empty(); }
	if (N) { if (N->Bus) N->Bus->Disconnect(N->Sig); N->Static->Stop(); N->Osc->Stop(); N->Out->Disconnect(); N.Reset(); }
}

// radio.ts Radio.
FS47Radio::FS47Radio(US47Audio* InAudio) : Audio(InAudio), CallerBank(TEXT("caller"))
{
	for (const FString& Id : S47Radio::DIAL)
	{
		auto St = MakeShared<FS47Station>(Id, *this); Stations.Add(Id, St);
		St->OnLine = [this, Id](const FString& Text, const FString& By, TOptional<double> Duration) { if (OnLine) OnLine(Id, Text, By, Duration); };
	}
}
FS47Radio::~FS47Radio()
{
	for (const auto& R : Receivers) R->Reset(); Receivers.Empty();
	for (const auto& Pair : Stations) { Pair.Value->OnLine = nullptr; Pair.Value->Sleep(); }
}
void FS47Radio::SetHostVoice(bool bOn) { if (bRecordedHost == bOn) return; bRecordedHost = bOn; Stations[TEXT("caliche")]->Revoice(); }
void FS47Radio::SetCallerVoice(bool bOn) { if (bRecordedCallers == bOn) return; bRecordedCallers = bOn; Stations[TEXT("caliche")]->Revoice(TEXT("caller")); }
TSharedPtr<FS47Receiver> FS47Radio::Receiver(const FS47ReceiverOpts& O) { auto R = MakeShared<FS47Receiver>(*this, O); Receivers.Add(R); return R; }
void FS47Radio::Reset(double InClock) { Clock = InClock; for (const auto& R : Receivers) R->Reset(); for (const FString& Id : S47Radio::DIAL) Stations[Id]->Reset(Clock); }
void FS47Radio::Update(double Dt, double InClock) { Clock = InClock; Time += Dt; for (const FString& Id : S47Radio::DIAL) Stations[Id]->Update(Dt, Clock); for (const auto& R : Receivers) R->Update(Dt); }

bool FS47Radio::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true; auto Check = [&](bool Pass, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s radio: %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= Pass; };
	Check(S47Radio::SayTime(0) == TEXT("twelve o'clock") && S47Radio::SayTime(2 * 3600 + 5 * 60) == TEXT("two-oh-five") && S47Radio::SayTime(-60) == TEXT("eleven fifty-nine"), TEXT("time checks, hyphens and midnight wrapping"));
	Check(S47Radio::SecsOf(TEXT("hello")) == 2.4 && S47Radio::SecsOf(FString::ChrN(80, 'a')) == 2.4, TEXT("line length follows words, not letters"));
	Check(FMath::IsNearlyEqual(S47Radio::DialX(1420), 293.35) && FMath::IsNearlyEqual(S47Radio::DialX(1230), 275.775) && FMath::IsNearlyEqual(S47Radio::DialX(1090), 252.65), TEXT("the three AM needle positions"));
	const auto Sy = S47Radio::Syllables(TEXT("Hello, is this the radio?"), 4);
	Check(Sy.Num() == 7 && FMath::IsNearlyEqual(Sy[0].F, 1.1151394544883166, 1e-12) && FMath::IsNearlyEqual(Sy[0].D, 0.4519566089851515, 1e-12) && Sy.Last().F == 1.25, TEXT("seeded syllables match JavaScript; a question rises"));
	US47Audio* A = NewObject<US47Audio>(); FS47Radio R(A);
	const auto Car = R.Receiver({0.38,1});
	Check(Car->Next() == TEXT("caliche") && Car->Next() == TEXT("roswell") && Car->Next() == TEXT("far") && Car->Next().IsEmpty() && !Car->bOn && Car->Station == TEXT("caliche"), TEXT("dial on, 1420, 1230, 1090, off and back"));
	Car->Pos = FVector::ZeroVector; Car->Carrier(5); Car->Carrier(2);
	Check(Car->Carrying() && !Car->Live(), TEXT("carrier also lights a switched-off set")); Car->Carrier(0); Check(!Car->Carrying() && !Car->Drift(), TEXT("release carrier and ignore drift on an off set"));
	Car->Set(true); Check(Car->Drift(3,0.6), TEXT("a live set can drift")); Car->Reset(); Check(!Car->bOn && !Car->Carrying(), TEXT("reset releases the receiver"));
	R.Reset(3 * 3600); const auto C = R.Stations[TEXT("caliche")], Ro = R.Stations[TEXT("roswell")], Far = R.Stations[TEXT("far")];
	Check(C->OnAir() && !Ro->OnAir() && Far->OnAir(), TEXT("night stations before sign-off and sign-on"));
	Check(Far->Strength(5 * 3600 + 1800,0) == 0 && Far->Strength(3 * 3600,0) > 0, TEXT("skywave disappears in daylight"));
	C->Start({C->Cfg.Loop[1]}, 3 * 3600); C->T = 4;
	Check(!C->Slot({FS47RadioLine(TEXT("Test line."))}, 3 * 3600), TEXT("record minimum five seconds before a trip slot"));
	C->T = 6; const auto X = C->Slot({FS47RadioLine(TEXT("Test line."))}, 3 * 3600);
	Check(X && C->Where(X) == TEXT("waiting") && C->bFading && C->Len == 7.6, TEXT("record fades 1.6 seconds into a queued trip"));
	Check(C->Unslot(X) && C->Where(X).IsEmpty(), TEXT("unbegun trip lines can be returned"));
	C->Start({C->Cfg.Loop[0]}, 3 * 3600);
	const auto Original = C->Queue[0]; C->T = 1.75;
	const double Bulletin = C->CutIn({FS47RadioLine(TEXT("A recorded bulletin."),TEXT("news"),TEXT("the newsreader"))}, 3 * 3600);
	Check(Bulletin == 47 && C->ArchiveStatus()->State == TEXT("playing") && C->Len == 47, TEXT("bulletin keeps the fixed 47 seconds"));
	C->Update(49,3 * 3600 + 49);
	Check(C->Queue[0] == Original && C->T == 1.75 && C->ArchiveStatus()->State == TEXT("complete") && C->Calm == 0, TEXT("bulletin restores the exact programme point, even on a slow frame"));
	Check(C->CutIn({FS47RadioLine(TEXT("No."))},3 * 3600 + 50) == 0, TEXT("ten calm seconds before another interruption"));
	C->Calm = CALM; Check(!C->Slot({FS47RadioLine(TEXT("No."))}, S47Radio::SIGN_OFF - 60), TEXT("trip lines kept clear of sign-off"));
	R.Reset(S47Radio::SIGN_OFF); Check(C->Seg().Kind == TEXT("talk") && !Ro->OnAir(), TEXT("04:00 sign-off starts with the host"));
	C->Update(C->Len, S47Radio::SIGN_OFF); Check(C->Seg().Kind == TEXT("anthem"), TEXT("national anthem follows the sign-off"));
	C->Update(S47Radio::ANTHEM_SECS, S47Radio::SIGN_OFF); Check(C->Seg().Kind == TEXT("carrier"), TEXT("anthem gives way to 75 seconds of carrier"));
	C->Update(75,S47Radio::SIGN_OFF); Check(!C->OnAir(), TEXT("Caliche falls silent"));
	R.Reset(S47Radio::SIGN_ON); Check(Ro->Seg().Kind == TEXT("carrier") && Ro->Strength(S47Radio::SIGN_ON,0) == 0.8, TEXT("05:00 Roswell begins with six seconds of carrier"));
	Ro->Update(6,S47Radio::SIGN_ON); Check(Ro->Seg().Kind == TEXT("talk"), TEXT("Roswell announces the broadcast day"));
	Ro->Update(Ro->Len,S47Radio::SIGN_ON); Check(Ro->Seg().Kind == TEXT("anthem"), TEXT("Roswell anthem"));
	R.bBoyCalled = false; R.Reset(S47Radio::BOY_AT); Check(R.bBoyCalled && C->Seg().Lines[0].By == TEXT("a boy") && S47Radio::BoyCallTimes(R).Num() == 5, TEXT("boy calls once after 02:26"));
	R.Reset(S47Radio::BOY_AT); Check(C->Queue[0]->Lines.IsEmpty() || C->Queue[0]->Lines[0].By != TEXT("a boy"), TEXT("reset preserves the host's boyCalled flag"));
	A->Ctx = FS47AudioContext::Make(); C->Start({Talk({FS47RadioLine(TEXT("Hello, is this the radio?"))})},0); C->EnsureBus()->Connect(A->Ctx->Destination); C->Listen(true);
	TArray<float> Samples; Samples.SetNumZeroed(48000 * 2); A->Ctx->Render(Samples.GetData(),48000); double Peak = 0; bool bFinite = true;
	for (float V : Samples) { Peak = FMath::Max(Peak,double(FMath::Abs(V))); bFinite &= FMath::IsFinite(V); }
	Check(bFinite && Peak > 0.01, TEXT("synthesised talk renders through the graph"));
	const double Before = A->Ctx->CurrentTime(); A->Ctx->Suspend(true); A->Ctx->Render(Samples.GetData(),128); Check(A->Ctx->CurrentTime() == Before, TEXT("pause freezes the authoritative sound clock"));
	C->Listen(false); A->Ctx->Close(); A->Ctx.Reset();
	// Content-backed checks are additional to the portable fixture tests. A fresh source-only
	// checkout has no recordings; the integrator's imported content exercises these too.
	if (IFileManager::Get().FileExists(*(FPaths::ProjectContentDir() / TEXT("Audio/Raw/media/radio/anthem.wav"))))
	{
		US47Audio* Recorded = NewObject<US47Audio>(); Recorded->Ctx = FS47AudioContext::Make();
		{
			FS47Radio Air(Recorded); Air.SetHostVoice(true); Air.SetCallerVoice(true); const double Clock = 2 * 3600 + 5 * 60;
			Air.Reset(Clock); const auto St = Air.Stations[TEXT("caliche")]; St->Start({St->Cfg.Loop[0]},Clock); St->EnsureBus()->Connect(Recorded->Ctx->Destination); St->Listen(true);
			Check(St->bHostReady && St->Spoken[0].Buffers.IsSet() && St->Spoken[1].Buffers.IsSet() && St->Spoken[1].Buffers->Num() >= 3, TEXT("imported host whole line and authored time fragments decode"));
			bool bCaption = false; St->OnLine = [&bCaption](const FString&, const FString&, TOptional<double> Dur) { bCaption = Dur.IsSet(); };
			Recorded->Ctx->Render(Samples.GetData(),19200); St->Update(0.4,Clock);
			Check(bCaption && St->HostStatus().bPlaying && FMath::IsNearlyEqual(St->T,0.4,0.003), TEXT("recorded caption follows the authoritative sound clock"));
			TArray<FS47RadioLine> News = {
				{TEXT("We interrupt this program. This is a local news bulletin."),TEXT("news"),TEXT("the newsreader")},
				{TEXT("Roswell, New Mexico. Tuesday, July eighth, nineteen forty-seven."),TEXT("news"),TEXT("the newsreader")},
				{TEXT("A flying disc is reported recovered on a ranch outside town. That is the wording on the notice."),TEXT("news"),TEXT("the newsreader")},
				{TEXT("There is no description of the object. The space for the name of the person reporting it is blank."),TEXT("news"),TEXT("the newsreader")},
				{TEXT("We have received the same line twice. We are reading it just as it came through."),TEXT("news"),TEXT("the newsreader")},
				{TEXT("Please stand by. We will return to our regular program."),TEXT("news"),TEXT("the newsreader")}
			};
			Check(St->CutIn(News,Clock) == 47 && St->Spoken.Num() == 6 && St->Spoken[0].Buffers.IsSet() && St->Len == 47 && St->Spoken.Last().At + St->Spoken.Last().Secs <= 46.81, TEXT("complete imported bulletin fits 47 seconds without stretching words"));
			Recorded->Ctx->Render(Samples.GetData(),19200); St->Update(0.4,Clock); const double Held = St->T;
			Recorded->Ctx->Suspend(true); Recorded->Ctx->Render(Samples.GetData(),19200); St->Update(1,Clock);
			Check(St->T == Held && St->ArchiveStatus()->Elapsed == Held, TEXT("suspended context holds recorded bulletin and progress")); Recorded->Ctx->Suspend(false);
			Air.SetCallerVoice(false); Check(St->ArchiveStatus()->State == TEXT("cancelled"), TEXT("changing archive voice cannot count a mixed listen"));
			St->Start({Segment(TEXT("anthem"))},Clock,1.5);
			Check(St->AnthemEl() && !St->bAnthemFailed && FMath::IsNearlyEqual(St->AnthemEl()->CurrentTime(),1.5,1e-6), TEXT("imported anthem seeks into the same station music graph"));
			St->Start({Music(TEXT("ballad"),78)},Clock,6);
			const double From = FMath::Fmod(St->SegNo * 61.7,246.4 - 78 - 2) + 6;
			Check(St->TrackEl() && !St->bTrackFailed && FMath::IsNearlyEqual(St->TrackEl()->CurrentTime(),From,1e-6), TEXT("imported radio record seeks to the source segment offset"));
			St->Listen(false); Check(!St->Awake(), TEXT("last listener releases streamed files and station sources"));
		}
		Recorded->Ctx->Close(); Recorded->Ctx.Reset();
	}
	return bOk;
}

// Radio.state: the same diagnostics without making another playback path.
TMap<FString, FS47Radio::FStationState> FS47Radio::State() const
{
	TMap<FString, FStationState> Out;
	for (const FString& Id : S47Radio::DIAL)
	{
		const auto S = Stations[Id]; FStationState V;
		V.Seg = S->Seg().Kind; V.T = S->T; V.bFree = S->Free(); V.bSlotted = V.Seg == TEXT("talk") && S->Seg().bSlotted; V.bAwake = S->Awake(); V.Listeners = S->Listeners; V.Strength = S->Strength(Clock,Time);
		if (const auto Anthem = S->AnthemEl()) V.Anthem = FMediaState{Anthem->CurrentTime(),Anthem->Paused(),S->bAnthemFailed};
		V.bAnthemFailed = S->bAnthemFailed; V.Host = S->HostStatus(); V.Callers = S->CallerStatus(); V.Archive = S->ArchiveStatus(); Out.Add(Id, MoveTemp(V));
	}
	return Out;
}
