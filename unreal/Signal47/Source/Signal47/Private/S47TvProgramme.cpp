// The local TV programme: the frame pack and its sound (Scripts/prepare-tv.py), one clock per set.
#include "S47TvProgramme.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "S47Audio.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Tv, Log, All);

namespace
{
	const int32 PW = 320, PH = 240;
	const double FLAG_FPS = 12;
	// the set's little speaker: about 250 Hz to 4 kHz, a box resonance, a little saturation
	const double LOW = 250, HIGH = 4000, BOX = 1400, BOX_DB = 4, DRIVE = 1.25, SHAPE = 1.6;
	// through a wall: duller and quieter
	const double WALL_LP = 650, OPEN_LP = 18000, THROUGH = 0.4;

	struct FProgramme
	{
		TArray<uint8> Bytes;
		TArray<uint32> Offsets;
		uint32 Fps = 0;
		uint32 Read(int32 At) const
		{
			return uint32(Bytes[At]) | (uint32(Bytes[At+1]) << 8) | (uint32(Bytes[At+2]) << 16) | (uint32(Bytes[At+3]) << 24);
		}
		double Length() const { return Fps ? double(Offsets.Num() - 1) / Fps : 0; }
		FProgramme()
		{
			const FString Path = FPaths::ProjectContentDir() / TEXT("WebData/tv/unarius.s47tv");
			if (!FFileHelper::LoadFileToArray(Bytes, *Path)) return;
			if (Bytes.Num() < 28 || Bytes.Num() > 32 * 1024 * 1024 || FMemory::Memcmp(Bytes.GetData(), "S47TV001", 8)) { Bytes.Reset(); return; }
			const uint32 N = Read(20), Rate = Read(8);
			if (Rate < 1 || Rate > 30 || Read(12) != PW || Read(16) != PH || N == 0 || N > 24000 || 24 + 4 * (N + 1) > uint32(Bytes.Num())) { Bytes.Reset(); return; }
			for (uint32 I = 0; I <= N; ++I)
			{
				const uint32 Offset = Read(24 + 4 * I);
				if (Offset > uint32(Bytes.Num()) || (I == 0 && Offset != 24 + 4 * (N + 1)) || (I > 0 && (Offset <= Offsets.Last() || Offset - Offsets.Last() > 1024 * 1024)))
				{ Bytes.Reset(); Offsets.Reset(); return; }
				Offsets.Add(Offset);
			}
			if (Offsets.Last() != uint32(Bytes.Num())) { Bytes.Reset(); Offsets.Reset(); return; }
			Fps = Rate;
			UE_LOG(LogS47Tv, Display, TEXT("S47: Unarius TV programme ready: %u frames at %u fps, %.2f s, %d bytes"), N, Fps, Length(), Bytes.Num());
		}
	};
	const FProgramme& Programme() { static const FProgramme P; return P; }

	// the pictures' sound (Audio/Raw/tv), read once, used only when it is as long as the pictures
	const TSharedPtr<FS47AudioBuffer>& ProgrammeSound()
	{
		static const TSharedPtr<FS47AudioBuffer> Sound = []
		{
			const FProgramme& P = Programme();
			TArray<uint8> Bytes;
			if (!P.Fps || !FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / TEXT("Audio/Raw/tv/unarius.wav")))) return TSharedPtr<FS47AudioBuffer>();
			TSharedPtr<FS47AudioBuffer> B = S47Wa::DecodeWav(Bytes);
			if (!B.IsValid() || FMath::Abs(B->Duration() - P.Length()) > 0.05)
			{
				UE_LOG(LogS47Tv, Warning, TEXT("S47: Unarius TV sound not used: %.2f s against %.2f s of pictures (run Scripts/prepare-tv.py)"), B.IsValid() ? B->Duration() : 0.0, P.Length());
				return TSharedPtr<FS47AudioBuffer>();
			}
			UE_LOG(LogS47Tv, Display, TEXT("S47: Unarius TV sound ready: %.2f s at %.0f Hz"), B->Duration(), B->SampleRate());
			return B;
		}();
		return Sound;
	}
}

FS47TvProgramme::~FS47TvProgramme() { Silence(); }

bool FS47TvProgramme::Available() const { return bSign || Programme().Fps != 0; }
bool FS47TvProgramme::HasSound() const { return SoundBuffer().IsValid(); }
double FS47TvProgramme::Length() const { return TestLength > 0 ? TestLength : bSign ? SIGN_OFF : Programme().Length(); }
double FS47TvProgramme::Fps() const { return bSign ? FLAG_FPS : Programme().Fps; }
TSharedPtr<FS47AudioBuffer> FS47TvProgramme::SoundBuffer() const { return TestSound.IsValid() ? TestSound : bSign ? Anthem() : Available() ? ProgrammeSound() : nullptr; }

void FS47TvProgramme::Tune(bool bSignOff, double At)
{
	if (bSign == bSignOff) return;
	Silence();
	bSign = bSignOff; LastFrame = -1; Time = Wrap(At);
}

double FS47TvProgramme::Wrap(double Seconds) const
{
	const double Len = Length();
	if (Len <= 0 || !FMath::IsFinite(Seconds)) return 0;
	const double W = FMath::Fmod(Seconds, Len);
	return W < 0 ? W + Len : W;
}

int32 FS47TvProgramme::FrameIndex() const
{
	if (bSign) return int32(Wrap(Time) * FLAG_FPS);
	const FProgramme& P = Programme();
	if (!P.Fps) return -1;
	return FMath::Clamp(int32(Wrap(Time) * P.Fps), 0, P.Offsets.Num() - 2);
}

// ---------- the picture ----------

void FS47TvProgramme::Fit(const TArray<FColor>& Src, int32 Sw, int32 Sh, TArray<FColor>& Out, int32 Width, int32 Height, double Aspect)
{
	Out.Init(FColor(0, 0, 0, 255), Width * Height);
	if (Src.Num() != Sw * Sh || Width <= 0 || Height <= 0) return;
	// the picture's rectangle on the canvas, 4:3 on the screen it is shown on
	double Pw = Width, Ph = Height;
	if (Aspect > 4.0 / 3 + 1e-6) Pw = Width * (4.0 / 3) / Aspect;
	else if (Aspect < 4.0 / 3 - 1e-6) Ph = Height * Aspect / (4.0 / 3);
	const double X0 = (Width - Pw) / 2, Y0 = (Height - Ph) / 2, Kx = Sw / Pw, Ky = Sh / Ph;
	// each canvas pixel is the average of the source it covers (drawImage's smoothing); what lies
	// outside the picture is black
	struct FSpan { int32 From = 0; TArray<double, TInlineAllocator<16>> W; };
	auto Spans = [](int32 N, double Off, double K, int32 Sn)
	{
		TArray<FSpan> S; S.SetNum(N);
		for (int32 I = 0; I < N; ++I)
		{
			const double A = FMath::Clamp((I - Off) * K, 0.0, double(Sn)), B = FMath::Clamp((I + 1 - Off) * K, 0.0, double(Sn));
			S[I].From = FMath::Min(Sn - 1, FMath::FloorToInt(A));
			for (int32 J = S[I].From; J < Sn && J < B; ++J) S[I].W.Add((FMath::Min(B, J + 1.0) - FMath::Max(A, double(J))) / K);
		}
		return S;
	};
	const TArray<FSpan> Xs = Spans(Width, X0, Kx, Sw), Ys = Spans(Height, Y0, Ky, Sh);
	for (int32 Y = 0; Y < Height; ++Y) for (int32 X = 0; X < Width; ++X)
	{
		double R = 0, G = 0, B = 0;
		for (int32 J = 0; J < Ys[Y].W.Num(); ++J) for (int32 I = 0; I < Xs[X].W.Num(); ++I)
		{
			const FColor& C = Src[(Ys[Y].From + J) * Sw + Xs[X].From + I];
			const double K = Ys[Y].W[J] * Xs[X].W[I];
			R += C.R * K; G += C.G * K; B += C.B * K;
		}
		Out[Y * Width + X] = FColor(uint8(FMath::Clamp(FMath::RoundToInt(R), 0, 255)), uint8(FMath::Clamp(FMath::RoundToInt(G), 0, 255)), uint8(FMath::Clamp(FMath::RoundToInt(B), 0, 255)), 255);
	}
}

bool FS47TvProgramme::Frame(TArray<FColor>& Out, int32 Width, int32 Height, double Aspect)
{
	const FProgramme& P = Programme();
	if ((!bSign && !P.Fps) || Width <= 0 || Height <= 0) return false;
	const int32 Index = FrameIndex();
	if (bSign && Index != LastFrame)
	{
		Flag(Pixels, Wrap(Time));
		double Sum = 0;
		for (int32 Y = 0; Y < PH; Y += 2) for (int32 X = 0; X < PW; X += 2) { const FColor& C = Pixels[Y * PW + X]; Sum += 0.299 * C.R + 0.587 * C.G + 0.114 * C.B; }
		Lum = Sum / (255.0 * (PW / 2) * (PH / 2));
		LastFrame = Index;
	}
	else if (Index != LastFrame)
	{
		auto& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const auto Decoder = Module.CreateImageWrapper(EImageFormat::JPEG);
		TArray<uint8> Raw;
		if (!Decoder || !Decoder->SetCompressed(P.Bytes.GetData() + P.Offsets[Index], P.Offsets[Index+1] - P.Offsets[Index])
			|| Decoder->GetWidth() != PW || Decoder->GetHeight() != PH || !Decoder->GetRaw(ERGBFormat::BGRA, 8, Raw) || Raw.Num() < PW * PH * 4) return false;
		Pixels.SetNumUninitialized(PW * PH);
		FMemory::Memcpy(Pixels.GetData(), Raw.GetData(), Pixels.Num() * sizeof(FColor));
		double Sum = 0;
		for (int32 Y = 0; Y < PH; Y += 2) for (int32 X = 0; X < PW; X += 2) { const FColor& C = Pixels[Y * PW + X]; Sum += 0.299 * C.R + 0.587 * C.G + 0.114 * C.B; }
		Lum = Sum / (255.0 * (PW / 2) * (PH / 2));
		if (LastFrame < 0) UE_LOG(LogS47Tv, Display, TEXT("S47: Unarius TV decoded frame %d"), Index);
		LastFrame = Index;
	}
	if (Width == PW && Height == PH && FMath::IsNearlyEqual(Aspect, 4.0 / 3)) Out = Pixels;
	else Fit(Pixels, PW, PH, Out, Width, Height, Aspect);
	return true;
}

// ---------- the sound ----------

void FS47TvProgramme::Update(double Dt, US47Audio* A, const FVector& At, double InLevel, bool bInThrough)
{
	const TSharedPtr<FS47AudioContext> C = A ? A->Ctx : nullptr;
	Step(Dt, C, [A, &At]() -> TSharedPtr<FS47AudioNode> { return A ? A->Spatial(At, 0.9f) : nullptr; }, InLevel, bInThrough);
}

void FS47TvProgramme::Step(double Dt, const TSharedPtr<FS47AudioContext>& InCtx, TFunctionRef<TSharedPtr<FS47AudioNode>()> Out, double InLevel, bool bInThrough)
{
	if (Length() <= 0) return;
	// the clock: the sound's while it plays, so the picture is where the sound is; when the sound's
	// clock stands (a suspended context, no device) for half a second, the picture goes on alone
	bool bFollow = false;
	if (Src.IsValid() && Ctx.IsValid())
	{
		const double Now = Ctx->CurrentTime();
		if (Now > Heard) { Heard = Now; Stall = 0; } else Stall += FMath::Max(0.0, Dt);
		bFollow = Stall < 0.5;
		if (bFollow) Time = Wrap(From + (Now - StartedAt));
	}
	if (!bFollow) Time = Wrap(Time + FMath::Max(0.0, Dt));
	const double NewLevel = FMath::IsFinite(InLevel) ? FMath::Max(0.0, InLevel) : 0.0;
	if (Src.IsValid() && Ctx != InCtx) Silence();
	if (NewLevel < 1e-4 || !InCtx.IsValid() || !SoundBuffer().IsValid()) { Silence(); return; }
	if (!Src.IsValid())
	{
		Level = NewLevel; bThrough = bInThrough;
		FS47AudioContext::FBatch B(*InCtx);
		Start(InCtx, Out());
		return;
	}
	FS47AudioContext::FBatch B(*Ctx);
	const double Now = Ctx->CurrentTime();
	if (!FMath::IsNearlyEqual(NewLevel, Level, 1e-5) || bInThrough != bThrough) Gain->Gain.SetTargetAtTime(NewLevel * (bInThrough ? THROUGH : 1.0), Now, 0.08);
	if (bInThrough != bThrough) Wall->Frequency.SetTargetAtTime(bInThrough ? WALL_LP : OPEN_LP, Now, 0.12);
	Level = NewLevel; bThrough = bInThrough;
}

void FS47TvProgramme::Start(const TSharedPtr<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Out)
{
	if (!Out.IsValid()) return;
	FS47AudioContext& C = *InCtx;
	auto S = C.CreateBufferSource(); S->Buffer = SoundBuffer(); S->bLoop = true;
	auto Filter = [&C](const TCHAR* Type, double Freq, double Q)
	{
		auto F = C.CreateBiquadFilter(); F->SetType(Type); F->Frequency.SetValue(Freq); F->Q.SetValue(Q); return F;
	};
	// the set's little speaker: no bass and no top, a boxy middle, one channel
	auto Hp1 = Filter(TEXT("highpass"), LOW, 0.7), Hp2 = Filter(TEXT("highpass"), LOW, 0.7), Lp = Filter(TEXT("lowpass"), HIGH, 0.7);
	auto Box = Filter(TEXT("peaking"), BOX, 1.1); Box->Gain.SetValue(BOX_DB);
	// a little saturation: driven into a soft curve, the loudest moments rounded off
	auto Drive = C.CreateGain(); Drive->Gain.SetValue(DRIVE);
	auto Shape = C.CreateWaveShaper();
	for (int32 I = 0; I < 1024; ++I) { const double X = double(I) / 1023 * 2 - 1; Shape->Curve.Add(float(FMath::Tanh(SHAPE * X) / FMath::Tanh(SHAPE))); }
	auto G = C.CreateGain(); G->Gain.SetValue(0);
	auto W = Filter(TEXT("lowpass"), bThrough ? WALL_LP : OPEN_LP, 0.7);
	S->Connect(Hp1); Hp1->Connect(Hp2); Hp2->Connect(Lp); Lp->Connect(Box); Box->Connect(Drive); Drive->Connect(Shape); Shape->Connect(G); G->Connect(W); W->Connect(Out);
	const double Now = C.CurrentTime();
	G->Gain.SetTargetAtTime(Level * (bThrough ? THROUGH : 1.0), Now, 0.08);
	From = Wrap(Time); StartedAt = Now; Heard = Now; Stall = 0;
	S->Start(Now, From);
	Ctx = InCtx; Src = S; Gain = G; Wall = W;
	if (TestLength <= 0) UE_LOG(LogS47Tv, Display, TEXT("S47: Unarius TV sound playing from %.2f s, level %.3f%s"), From, Level, bThrough ? TEXT(", through the wall") : TEXT(""));
}

void FS47TvProgramme::Silence()
{
	if (!Src.IsValid()) return;
	if (Ctx.IsValid())
	{
		FS47AudioContext::FBatch B(*Ctx);
		const double Now = Ctx->CurrentTime();
		// (the context keeps the source until it has stopped; its chain goes with it)
		Gain->Gain.SetTargetAtTime(0, Now, 0.04);
		Src->Stop(Now + 0.3);
	}
	Src.Reset(); Gain.Reset(); Wall.Reset(); Ctx.Reset();
}

// ---------- the sign-off: the flag and the anthem ----------

void FS47TvProgramme::Flag(TArray<FColor>& Out, double T)
{
	Out.SetNumUninitialized(PW * PH);
	// up from black, and down to it before the card
	const double Fade = FMath::Clamp(T / 1.5, 0.0, 1.0) * FMath::Clamp((SIGN_OFF - 0.6 - T) / 2.5, 0.0, 1.0);
	const double StarX = 0.4 * 0.88 * PW / 6, StarY = 7.0 / 13 * 0.8 * PH / 9;
	for (int32 Y = 0; Y < PH; ++Y) for (int32 X = 0; X < PW; ++X)
	{
		const double U = ((X + 0.5) / PW - 0.06) / 0.88;
		// the cloth: the wave runs out from the hoist, larger toward the fly, and its folds catch the light
		const double Phase = 2 * UE_DOUBLE_PI * (1.7 * U - 0.42 * T), Amp = 0.035 * (0.25 + 0.75 * FMath::Clamp(U, 0.0, 1.0));
		const double V = ((Y + 0.5) / PH - 0.10 - Amp * FMath::Sin(Phase)) / 0.80;
		double R = 8 + 6.0 * Y / PH, G = 12 + 10.0 * Y / PH, B = 28 + 16.0 * Y / PH;   // the night behind it
		if (U >= 0 && U <= 1 && V >= 0 && V <= 1)
		{
			const bool bRed = (FMath::Min(12, int32(V * 13)) & 1) == 0;
			R = bRed ? 170 : 232; G = bRed ? 38 : 230; B = bRed ? 50 : 222;
			if (U < 0.4 && V < 7.0 / 13)
			{
				R = 44; G = 48; B = 104;
				// nine rows of stars, six and five in turn
				const double Row = V / (7.0 / 13) * 9, Col = U / 0.4 * 6;
				const int32 Ri = FMath::Min(8, int32(Row));
				const double Centre = (Ri & 1) ? FMath::Clamp(FMath::RoundToDouble(Col), 1.0, 5.0) : FMath::Clamp(FMath::FloorToDouble(Col) + 0.5, 0.5, 5.5);
				const double Dx = (Col - Centre) * StarX, Dy = (Row - (Ri + 0.5)) * StarY;
				if (Dx * Dx + Dy * Dy < 2.3 * 2.3) { R = 232; G = 230; B = 222; }
			}
			const double Shade = 0.82 + 0.2 * FMath::Cos(Phase);
			R *= Shade; G *= Shade; B *= Shade;
		}
		Out[Y * PW + X] = FColor(uint8(FMath::Clamp(FMath::RoundToInt(R * Fade), 0, 255)), uint8(FMath::Clamp(FMath::RoundToInt(G * Fade), 0, 255)), uint8(FMath::Clamp(FMath::RoundToInt(B * Fade), 0, 255)), 255);
	}
}

TSharedPtr<FS47AudioBuffer> FS47TvProgramme::Anthem()
{
	// The Star-Spangled Banner (the tune is John Stafford Smith's, about 1775; nobody's recording is used): the melody in C, each
	// note a small band of harmonics with the octave under it, as a station's worn tape through a two-inch speaker would give it.
	static const TSharedPtr<FS47AudioBuffer> Sound = []
	{
		// Tom 12 Nov: a real recording at sign-off, not the tune: the U.S. Navy Band (public domain, Tools/anthem/fetch.sh), two
		// seconds in from black and faded out before the card. Without the file, the tune below as before.
		{
			TArray<uint8> Bytes;
			if (FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / TEXT("Audio/Raw/tv/anthem.wav"))))
			{
				const TSharedPtr<FS47AudioBuffer> Rec = S47Wa::DecodeWav(Bytes);
				if (Rec.IsValid() && Rec->Duration() > 20.0 && Rec->Duration() <= SIGN_OFF - 2.5)
				{
					const float RecRate = Rec->SampleRate();
					const int32 Total = int32(SIGN_OFF * RecRate), Lead = int32(2.0 * RecRate), Len = Rec->Length(), FadeN = int32(1.2 * RecRate);
					TSharedRef<FS47AudioBuffer> Buf = MakeShared<FS47AudioBuffer>(1, Total, RecRate);
					float* Dst = Buf->GetChannelData(0);
					FMemory::Memzero(Dst, Total * sizeof(float));
					const float* Src = Rec->GetChannelData(0);
					for (int32 I = 0; I < Len && Lead + I < Total; ++I)
					{
						const float Tail = I > Len - FadeN ? float(Len - I) / float(FadeN) : 1.f;
						Dst[Lead + I] = 0.8f * Src[I] * Tail;
					}
					UE_LOG(LogS47Tv, Display, TEXT("S47: the station's sign-off: the recorded anthem, %.1f s in %.0f s"), Rec->Duration(), SIGN_OFF);
					return TSharedPtr<FS47AudioBuffer>(Buf);
				}
				UE_LOG(LogS47Tv, Warning, TEXT("S47: the recorded anthem not used (%.1f s; it must fit %.0f s); the tune instead"), Rec.IsValid() ? Rec->Duration() : 0.0, SIGN_OFF);
			}
		}
		struct FNote { int32 Midi; double Beats; };
		static const FNote A[] = { {67,.75},{64,.25},{60,1},{64,1},{67,1},{72,2},{76,.75},{74,.25},{72,1},{64,1},{66,1},{67,2},{67,.5},{67,.5},{76,1.5},{74,.5},{72,1},{71,2},{69,.75},{71,.25},{72,1},{72,1},{67,1},{64,1},{60,1} };
		static const FNote B[] = { {76,.5},{76,.5},{76,1},{77,1},{79,1},{79,2},{77,.5},{76,.5},{74,1},{76,1},{77,1},{77,2},{77,1},{76,1.5},{74,.5},{72,1},{71,2},{69,.75},{71,.25},{72,1},{64,1},{66,1},{67,2} };
		static const FNote C[] = { {67,1},{72,1},{72,1},{72,.5},{71,.5},{69,1},{69,1},{69,1},{74,1},{77,.5},{76,.5},{74,.5},{72,.5},{72,1},{71,2},{67,.5},{67,.5},{72,1.5},{74,.5},{76,.5},{77,.5},{79,3},{72,.5},{74,.5},{76,1.5},{77,.5},{74,1},{72,3} };
		const float Rate = 16000.f;
		const int32 N = int32(SIGN_OFF * Rate);
		TSharedRef<FS47AudioBuffer> Buf = MakeShared<FS47AudioBuffer>(1, N, Rate);
		float* Out = Buf->GetChannelData(0);
		FMemory::Memzero(Out, N * sizeof(float));
		double At = 2.0;
		const double Beat = 0.56;
		auto Play = [&](const FNote* Notes, int32 Count, int32 SlowFrom)
		{
			for (int32 I = 0; I < Count; ++I)
			{
				// (the last line is held back a little, as it is always played)
				const double Len = Notes[I].Beats * Beat * (I >= SlowFrom ? 1.18 : 1.0), Sounding = FMath::Max(0.06, Len - 0.035);
				const double F = 440.0 * FMath::Pow(2.0, (Notes[I].Midi - 69) / 12.0);
				const int32 From = int32(At * Rate), Samples = int32(Sounding * Rate);
				for (int32 S = 0; S < Samples && From + S < N; ++S)
				{
					const double T = S / double(Rate);
					const double Env = FMath::Min(1.0, T / 0.025) * (0.72 + 0.28 * FMath::Exp(-T / 0.22)) * FMath::Min(1.0, (Sounding - T) / 0.05);
					const double Ph = 2 * UE_DOUBLE_PI * F * T + (T > 0.15 ? 0.25 * FMath::Sin(2 * UE_DOUBLE_PI * 5.2 * T) : 0.0);
					const double Tone = FMath::Sin(Ph) + 0.55 * FMath::Sin(2 * Ph) + 0.38 * FMath::Sin(3 * Ph) + 0.2 * FMath::Sin(4 * Ph) + 0.1 * FMath::Sin(5 * Ph)
						+ 0.4 * FMath::Sin(Ph / 2) + 0.14 * FMath::Sin(Ph);
					Out[From + S] += float(0.22 * Env * Tone);
				}
				At += Len;
			}
		};
		const int32 Na = int32(UE_ARRAY_COUNT(A)), Nb = int32(UE_ARRAY_COUNT(B)), Nc = int32(UE_ARRAY_COUNT(C));
		Play(A, Na, MAX_int32); Play(A, Na, MAX_int32); Play(B, Nb, MAX_int32); Play(C, Nc, Nc - 6);
		UE_LOG(LogS47Tv, Display, TEXT("S47: the station's sign-off made: the anthem ends %.1f s into %.0f s"), At, SIGN_OFF);
		return TSharedPtr<FS47AudioBuffer>(Buf);
	}();
	return Sound;
}

// ---------- the channel ----------

namespace
{
	FS47TvProgramme::FChannel& Channel() { static FS47TvProgramme::FChannel C; return C; }
	double JamLevel = 0, JamLeft = 0;
	double DayOf(double Clock) { return FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0); }
}

void FS47TvProgramme::Advance(FChannel& C, double Clock, double Dt, bool bDay, bool bForced)
{
	Dt = FMath::IsFinite(Dt) ? FMath::Max(0.0, Dt) : 0.0;
	C.Time += Dt;
	double Was = C.Clock; C.Clock = Clock;
	// another night altogether (a load, a new game, a sleep): nothing of the old one goes on
	if (Was >= 0 && (Clock < Was - 1 || Clock - Was > 3600) && !C.bAsked) { C.Air = EAir::Snow; C.Into = -1; Was = -1; }
	if (C.bAsked && C.Air == EAir::SignOff) { if ((C.Into += Dt) < SIGN_OFF) return; C.bAsked = false; C.Air = EAir::Card; C.Into = 0; return; }
	const double Day = DayOf(FMath::IsFinite(Clock) ? Clock : 0.0);
	if (bForced || (bDay ? Day >= DAY_ON || Day < DAY_OFF : OnAir(Clock))) { C.Air = EAir::Programme; C.Into = -1; return; }
	if (!bDay) { C.Air = EAir::Snow; C.Into = -1; return; }
	const double Past = DayOf(Day - DAY_OFF), ToOn = DayOf(DAY_ON - Day);
	// the day ended in front of us (not skipped over by a sleep or a load): the station signs off
	if (C.Air == EAir::Programme && Was >= 0 && Clock >= Was && Clock - Was < 600 && Past < 1800) { C.Air = EAir::SignOff; C.Into = 0; return; }
	if (C.Air == EAir::SignOff && Past < 3 * 3600) { if ((C.Into += Dt) >= SIGN_OFF) { C.Air = EAir::Card; C.Into = 0; } return; }
	if (C.Air == EAir::Card && C.Into >= 0 && C.Into < CARD_AFTER && Past < 3 * 3600) { C.Into += Dt; return; }
	C.Air = ToOn <= 900 ? EAir::Card : EAir::Snow; C.Into = -1;
}

void FS47TvProgramme::ChannelStep(double Clock, double Dt, bool bDay)
{
	const EAir Was = Channel().Air;
	Advance(Channel(), Clock, Dt, bDay, ForcedOnAir());
	if (Channel().Air != Was)
	{
		static const TCHAR* const Names[] = { TEXT("the programme"), TEXT("the sign-off"), TEXT("the card"), TEXT("snow") };
		const int32 Day = int32(DayOf(FMath::IsFinite(Clock) ? Clock : 0.0));
		UE_LOG(LogS47Tv, Display, TEXT("S47: the station has %s at %02d:%02d (%s)"), Names[int32(Channel().Air)], Day / 3600, Day / 60 % 60, bDay ? TEXT("the valley's day") : TEXT("the late station"));
	}
	if (JamLeft > 0 && (JamLeft -= FMath::Max(0.0, Dt)) <= 0) JamLevel = 0;
}
FS47TvProgramme::EAir FS47TvProgramme::Air() { return Channel().Air; }
bool FS47TvProgramme::Staged() { return Programme().Fps != 0; }
double FS47TvProgramme::AirInto() { return FMath::Max(0.0, Channel().Into); }
double FS47TvProgramme::ChannelTime() { return Channel().Time; }
void FS47TvProgramme::SignOffNow() { FChannel& C = Channel(); C.Air = EAir::SignOff; C.Into = 0; C.bAsked = true; }
void FS47TvProgramme::JamFor(double Level, double Seconds) { JamLevel = FMath::Clamp(Level, 0.0, 1.0); JamLeft = FMath::Max(0.0, Seconds); }
double FS47TvProgramme::JamAsked() { return JamLeft > 0 ? JamLevel : 0.0; }

// ---------- the station's hours ----------

bool FS47TvProgramme::OnAir(double Clock)
{
	if (!FMath::IsFinite(Clock)) return false;
	const double Day = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	return Day >= ON_AIR || Day < OFF_AIR;
}

bool FS47TvProgramme::ForcedOnAir()
{
	static const bool bForced = []
	{
		const TCHAR* Cmd = FCommandLine::Get();
		FString Tv;
		return FParse::Param(Cmd, TEXT("S47TvOnAir")) || FParse::Param(Cmd, TEXT("S47MotelTvFilm")) || (FParse::Value(Cmd, TEXT("S47MotelTv="), Tv) && Tv.StartsWith(TEXT("programme")));
	}();
	return bForced;
}

FString FS47TvProgramme::Describe() const
{
	return FString::Printf(TEXT("picture %.2f s (frame %d), sound %s, level %.3f%s, brightness %.2f"), Time, FrameIndex(),
		Src.IsValid() && Ctx.IsValid() ? *FString::Printf(TEXT("%.2f s"), Wrap(From + Ctx->CurrentTime() - StartedAt)) : TEXT("off"), Src.IsValid() ? Level : 0.0,
		Src.IsValid() && bThrough ? TEXT(" through the wall") : TEXT(""), Lum);
}

bool FS47TvProgramme::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s tvProgramme: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the picture: 4:3 kept on every screen
	TArray<FColor> White, Out; White.Init(FColor::White, PW * PH);
	Fit(White, PW, PH, Out, 96, 72, 0.4998 / 0.36);
	Check(Out[36 * 96] == FColor(0, 0, 0, 255) && Out[36 * 96 + 95] == FColor(0, 0, 0, 255) && Out[36 * 96 + 2] == FColor::White && Out[48] == FColor::White && Out[71 * 96 + 48] == FColor::White,
		TEXT("room 6's wider screen: the 4:3 picture full height, a black bar at each side"));
	Fit(White, PW, PH, Out, 96, 72, 1.0);
	Check(Out[48] == FColor(0, 0, 0, 255) && Out[71 * 96 + 48] == FColor(0, 0, 0, 255) && Out[36 * 96] == FColor::White && Out[36 * 96 + 95] == FColor::White, TEXT("a square screen: bars above and below"));
	TArray<FColor> Ramp; Ramp.SetNum(PW * PH);
	for (int32 Y = 0; Y < PH; ++Y) for (int32 X = 0; X < PW; ++X) Ramp[Y * PW + X] = FColor(uint8(X * 255 / (PW - 1)), uint8(Y), uint8((X + Y) & 255), 255);
	Fit(Ramp, PW, PH, Out, PW, PH, 4.0 / 3);
	Check(Out == Ramp, TEXT("a 4:3 canvas the size of the frame gets it pixel for pixel"));
	Fit(Ramp, PW, PH, Out, 256, 192, 4.0 / 3);
	Check(Out[0].R <= 2 && Out[255].R >= 252 && Out[191 * 256].G >= 236, TEXT("scaled to the office set's tube, edge to edge"));
	// the sound: a 2 kHz blip 1.9 s into a 2 s loop, started at 1.5 s, comes 0.4 s later; the
	// picture's clock is the sound's while it plays, goes on alone when silent, and the sound starts
	// again where it is. (The context's clock counts whole render quanta: 128 frames of slack.)
	const TSharedRef<FS47AudioContext> C = FS47AudioContext::Make(48000.f);
	FS47TvProgramme T; T.TestLength = 2.0;
	T.TestSound = C->CreateBuffer(1, 48000, 24000.f);
	for (int32 I = 0; I < 48; ++I) T.TestSound->GetChannelData(0)[int32(1.9 * 24000) + I] = float(0.9 * FMath::Sin(2 * UE_DOUBLE_PI * 2000 * I / 24000.0));
	auto Dest = [&C]() -> TSharedPtr<FS47AudioNode> { return C->Destination; };
	TArray<float> Wav; Wav.SetNumZeroed(int32(0.75 * 48000) * 2);
	T.Time = 1.5; T.Step(0, C, Dest, 0.5, false);
	C->Render(Wav.GetData(), Wav.Num() / 2, 2);
	int32 Peak = 0; for (int32 I = 0; I < Wav.Num() / 2; ++I) if (FMath::Abs(Wav[I * 2]) > FMath::Abs(Wav[Peak * 2])) Peak = I;
	Check(T.Playing() && FMath::Abs(Peak - 0.4 * 48000) < 200 && FMath::Abs(Wav[Peak * 2]) > 0.05, FString::Printf(TEXT("the sound starts where the picture is (the click %.3f s after the start, peak %.3f)"), Peak / 48000.0, FMath::Abs(Wav[Peak * 2])));
	T.Step(0, C, Dest, 0.5, false);
	Check(FMath::IsNearlyEqual(T.Time, 0.25, 0.005), FString::Printf(TEXT("while it plays the picture follows the sound, round the loop (%.4f s)"), T.Time));
	T.Step(0, C, Dest, 0, false);
	Check(!T.Playing() && FMath::IsNearlyEqual(T.Time, 0.25, 0.005), TEXT("level 0 (the set off, the event's picture): silent"));
	C->Render(Wav.GetData(), Wav.Num() / 2, 2);
	double Tail = 0; for (int32 I = int32(0.4 * 48000); I < Wav.Num() / 2; ++I) Tail = FMath::Max(Tail, double(FMath::Abs(Wav[I * 2])));
	T.Step(0.5, C, Dest, 0, false);
	Check(Tail < 1e-4 && FMath::IsNearlyEqual(T.Time, 0.75, 0.005), FString::Printf(TEXT("the sound has stopped (%.1e) and the picture's clock goes on (%.4f s)"), Tail, T.Time));
	T.Step(0, C, Dest, 0.5, true);
	Check(T.Playing() && FMath::IsNearlyEqual(T.From, T.Time, 1e-9) && FMath::IsNearlyEqual(T.From, 0.75, 0.005) && T.bThrough, TEXT("back: the sound starts again where the picture is, through the wall"));
	C->Suspend(true);
	for (int32 I = 0; I < 4; ++I) T.Step(0.25, C, Dest, 0.5, true);
	Check(T.Time > 1.2, TEXT("a context that stands does not hold the picture"));
	C->Suspend(false); T.Silence(); C->Close();
	// the station's hours: on from 23:30 to 03:30, wrapping round midnight and any day
	Check(!OnAir(23 * 3600 + 29 * 60) && OnAir(23.5 * 3600) && OnAir(0) && OnAir(86400 + 2 * 3600) && OnAir(-1800) && !OnAir(3.5 * 3600) && !OnAir(12 * 3600) && !OnAir(-12 * 3600),
		TEXT("the programme's hours: from half past eleven to half past three, the card or snow the rest of the day"));
	// TV-CHANNEL-20261110: the one channel. Freeform's broadcast day, its sign-off in real seconds, the card and the snow after it.
	{
		FChannel Ch;
		auto Run = [&](double From, double To, double Step, bool bDay) { for (double K = From; K <= To; K += Step) Advance(Ch, K, 1.0, bDay, false); };
		Advance(Ch, 20 * 3600, 0, true, false);
		Check(Ch.Air == EAir::Programme, TEXT("channel: the programme through the evening"));
		Run(25 * 3600 + 3540, 26 * 3600 + 13, 13, true);
		Check(Ch.Air == EAir::SignOff && Ch.Into < 3, TEXT("channel: at two the station signs off in front of whoever is watching"));
		const double Programme0 = Ch.Time;
		for (int32 I = 0; I < 62; ++I) Advance(Ch, 26 * 3600 + 30 + I * 13, 1.0, true, false);
		const bool bStill = Ch.Air == EAir::SignOff;
		for (int32 I = 0; I < 22; ++I) Advance(Ch, 26 * 3600 + 900 + I * 13, 1.0, true, false); // (SIGN_OFF 82 s: the recorded anthem, Tom 12 Nov)
		Check(bStill && Ch.Air == EAir::Card && Ch.Time > Programme0 + 60, TEXT("channel: the flag and the anthem for their minute of real time, then the card"));
		for (int32 I = 0; I < int32(CARD_AFTER) + 2; ++I) Advance(Ch, 26 * 3600 + 1000 + I * 13, 1.0, true, false);
		Check(Ch.Air == EAir::Snow, TEXT("channel: then snow"));
		Advance(Ch, 29 * 3600 + 2400, 1.0, true, false); const bool bCardBefore = Ch.Air == EAir::Snow;
		Advance(Ch, 29 * 3600 + 2800, 1.0, true, false);
		Check(bCardBefore && Ch.Air == EAir::Card, TEXT("channel: the card again a quarter of an hour before sign-on"));
		Advance(Ch, 30 * 3600 + 5, 1.0, true, false);
		Check(Ch.Air == EAir::Programme, TEXT("channel: on the air again at six"));
		Advance(Ch, 49 * 3600, 1.0, true, false); Advance(Ch, 51 * 3600, 1.0, true, false);
		Check(Ch.Air == EAir::Snow, TEXT("channel: a night slept through has no sign-off waiting at the end of it"));
		Advance(Ch, 60 * 3600, 1.0, true, false); Ch.Air = EAir::SignOff; Ch.Into = 0; Ch.bAsked = true;
		Advance(Ch, 60 * 3600 + 13, 1.0, true, false);
		Check(Ch.Air == EAir::SignOff, TEXT("channel: asked for outright, the sign-off plays at any hour"));
		FChannel St;
		Advance(St, 1 * 3600, 1.0, false, false); const bool bLate = St.Air == EAir::Programme;
		Advance(St, 3.5 * 3600 + 5, 1.0, false, false); const bool bAfter = St.Air == EAir::Snow;
		Advance(St, 12 * 3600, 1.0, false, true);
		Check(bLate && bAfter && St.Air == EAir::Programme, TEXT("channel: the story keeps the late station's hours and its snow; a test can force the programme"));
	}
	{
		const TSharedPtr<FS47AudioBuffer> Song = Anthem();
		double Loud = 0, Lead = 0, End = 0;
		if (Song.IsValid())
		{
			const float* D = Song->GetChannelData(0); const int32 N = Song->Length(); const int32 R = int32(Song->SampleRate());
			for (int32 I = 0; I < N; ++I) { const double V = FMath::Abs(double(D[I])); Loud = FMath::Max(Loud, V); if (I < R) Lead = FMath::Max(Lead, V); if (I > N - R) End = FMath::Max(End, V); }
		}
		Check(Song.IsValid() && FMath::IsNearlyEqual(Song->Duration(), SIGN_OFF, 0.01) && Loud > 0.2 && Loud < 0.95 && Lead == 0 && End == 0,
			FString::Printf(TEXT("sign-off: the anthem fills its minute and no more (peak %.2f)"), Loud));
		FS47TvProgramme Set; Set.Tune(true, 20);
		TArray<FColor> Shown;
		const bool bFrame = Set.Available() && Set.Frame(Shown, PW, PH) && Shown.Num() == PW * PH;
		int32 Red = 0, Pale = 0, Blue = 0;
		if (bFrame) for (const FColor& P : Shown) { if (P.R > 110 && P.G < 70) ++Red; else if (P.R > 150 && P.G > 150 && P.B > 150) ++Pale; else if (P.B > 70 && P.R < 70) ++Blue; }
		TArray<FColor> Black; Flag(Black, 0);
		Check(bFrame && Red > 12000 && Pale > 9000 && Blue > 4000 && Set.Brightness() > 0.2 && Black[PW * 120 + 160].R == 0,
			FString::Printf(TEXT("sign-off: the flag is on the screen (red %d, white %d, blue %d), up from black"), Red, Pale, Blue));
		Set.Tune(false, 5);
		Check(!Set.SignOff() && Set.Fps() == Programme().Fps, TEXT("sign-off: tuned back, the set has the programme's feed again"));
	}
	return bOk;
}
