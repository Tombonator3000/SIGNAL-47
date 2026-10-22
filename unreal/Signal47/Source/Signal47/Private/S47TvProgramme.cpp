// The local TV programme: the frame pack and its sound (Scripts/prepare-tv.py), one clock per set.
#include "S47TvProgramme.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "S47Audio.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Tv, Log, All);

namespace
{
	const int32 PW = 320, PH = 240;
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

bool FS47TvProgramme::Available() const { return Programme().Fps != 0; }
bool FS47TvProgramme::HasSound() const { return SoundBuffer().IsValid(); }
double FS47TvProgramme::Length() const { return TestLength > 0 ? TestLength : Programme().Length(); }
double FS47TvProgramme::Fps() const { return Programme().Fps; }
TSharedPtr<FS47AudioBuffer> FS47TvProgramme::SoundBuffer() const { return TestSound.IsValid() ? TestSound : Available() ? ProgrammeSound() : nullptr; }

double FS47TvProgramme::Wrap(double Seconds) const
{
	const double Len = Length();
	if (Len <= 0 || !FMath::IsFinite(Seconds)) return 0;
	const double W = FMath::Fmod(Seconds, Len);
	return W < 0 ? W + Len : W;
}

int32 FS47TvProgramme::FrameIndex() const
{
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
	if (!P.Fps || Width <= 0 || Height <= 0) return false;
	const int32 Index = FrameIndex();
	if (Index != LastFrame)
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
	return bOk;
}
