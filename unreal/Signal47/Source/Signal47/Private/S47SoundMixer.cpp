#include "S47SoundMixer.h"

#include "Dom/JsonObject.h"
#include "S47SignalVoice.h"

namespace
{
	bool EnvelopeKnown(const FString& E)
	{
		return E == TEXT("constant") || E == TEXT("static") || E == TEXT("signal") || E == TEXT("pulse") || E == TEXT("activity");
	}
	double Unit(double V) { return FMath::IsFinite(V) ? FMath::Clamp(V, 0.0, 1.0) : 0; }
	bool ValidUse(ES47SoundUse Use) { return Use == ES47SoundUse::Receiver || Use == ES47SoundUse::Vhs || Use == ES47SoundUse::PcTransfer; }
	bool CheckFields(const FJsonObject& J, const TSet<FString>& Allowed, TArray<FString>& Errors)
	{
		bool bOk = true;
		for (const auto& Field : J.Values) if (!Allowed.Contains(FString(Field.Key.ToView())))
		{
			Errors.Add(FString::Printf(TEXT("sound mixer: unknown field %s"), *FString(Field.Key.ToView()))); bOk = false;
		}
		return bOk;
	}
	// Crop without changing the original recording. Loop overlap joins tail to head over 5 ms.
	TSharedRef<FS47AudioBuffer> Slice(const FS47AudioContext& Ctx, const FS47AudioBuffer& B, const FS47MixLayer& L)
	{
		const int32 Start = int32(L.Start * B.SampleRate());
		const int32 N = L.Length > 0 ? int32(L.Length * B.SampleRate()) : B.Length() - Start;
		const int32 K = FMath::Min(int32(B.SampleRate() * 0.005), N / 8);
		const int32 Size = L.bLoop ? N - K : N;
		auto Copy = Ctx.CreateBuffer(B.NumberOfChannels(), Size, B.SampleRate());
		for (int32 Ch = 0; Ch < B.NumberOfChannels(); ++Ch)
		{
			const float* Src = B.GetChannelData(Ch) + Start; float* D = Copy->GetChannelData(Ch);
			for (int32 I = 0; I < Size; ++I)
			{
				if (L.bLoop && K > 0 && I >= N - 2 * K)
				{
					const int32 J = I - (N - 2 * K); const double A = (J + 1.0) / K;
					D[I] = float(Src[N - K + J] * (1 - A) + Src[J] * A);
				}
				else if (L.bLoop) D[I] = Src[I + K];
				else
				{
					const double Edge = K > 0 ? FMath::Min(1.0, double(FMath::Min(I, N - 1 - I)) / K) : 1;
					D[I] = float(Src[I] * Edge);
				}
			}
		}
		return Copy;
	}
}

bool FS47SoundMixer::ParseRecipe(const TSharedPtr<FJsonObject>& Json, FS47MixRecipe& Recipe, TArray<FString>& Errors)
{
	const int32 Before = Errors.Num();
	if (!Json) { Errors.Add(TEXT("sound mixer: recipe is not an object")); return false; }
	CheckFields(*Json, { TEXT("name"), TEXT("use"), TEXT("layers") }, Errors);
	FS47MixRecipe Parsed; FString Use;
	if (!Json->TryGetStringField(TEXT("name"), Parsed.Name) || Parsed.Name.IsEmpty()) Errors.Add(TEXT("sound mixer: recipe needs a name"));
	if (!Json->TryGetStringField(TEXT("use"), Use)) Errors.Add(TEXT("sound mixer: recipe needs a use"));
	else if (Use == TEXT("receiver")) Parsed.Use = ES47SoundUse::Receiver;
	else if (Use == TEXT("vhs")) Parsed.Use = ES47SoundUse::Vhs;
	else if (Use == TEXT("pc_transfer")) Parsed.Use = ES47SoundUse::PcTransfer;
	else Errors.Add(TEXT("sound mixer: use must be receiver, vhs or pc_transfer"));
	const TArray<TSharedPtr<FJsonValue>>* JsLayers = nullptr;
	if (!Json->TryGetArrayField(TEXT("layers"), JsLayers) || JsLayers->Num() < 1 || JsLayers->Num() > 16) Errors.Add(TEXT("sound mixer: recipe needs 1..16 layers"));
	else for (const auto& Value : *JsLayers)
	{
		const TSharedPtr<FJsonObject>* Obj = nullptr;
		if (!Value.IsValid() || !Value->TryGetObject(Obj) || !Obj->IsValid()) { Errors.Add(TEXT("sound mixer: layer must be an object")); continue; }
		const FJsonObject& J = **Obj;
		CheckFields(J, { TEXT("source"), TEXT("gain_db"), TEXT("rate"), TEXT("highpass_hz"), TEXT("lowpass_hz"), TEXT("start_s"), TEXT("length_s"), TEXT("loop"), TEXT("envelope") }, Errors);
		FS47MixLayer L;
		if (!J.TryGetStringField(TEXT("source"), L.Source) || L.Source.IsEmpty()) Errors.Add(TEXT("sound mixer: layer needs a source"));
		auto Number = [&](const TCHAR* Key, double& Dest) { if (J.HasField(Key) && !J.TryGetNumberField(Key, Dest)) Errors.Add(FString::Printf(TEXT("sound mixer: %s must be numeric"), Key)); };
		Number(TEXT("gain_db"), L.GainDb); Number(TEXT("rate"), L.Rate); Number(TEXT("highpass_hz"), L.HighPass);
		Number(TEXT("lowpass_hz"), L.LowPass); Number(TEXT("start_s"), L.Start); Number(TEXT("length_s"), L.Length);
		if (J.HasField(TEXT("loop")) && !J.TryGetBoolField(TEXT("loop"), L.bLoop)) Errors.Add(TEXT("sound mixer: loop must be boolean"));
		if (J.HasField(TEXT("envelope")) && !J.TryGetStringField(TEXT("envelope"), L.Envelope)) Errors.Add(TEXT("sound mixer: envelope must be a string"));
		if (!EnvelopeKnown(L.Envelope)) Errors.Add(TEXT("sound mixer: unknown envelope"));
		Parsed.Layers.Add(MoveTemp(L));
	}
	if (Errors.Num() != Before) return false;
	Recipe = MoveTemp(Parsed); return true;
}

TSharedPtr<FS47SoundMixer> FS47SoundMixer::Make(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Dest,
	const FS47MixRecipe& Recipe, const TMap<FString, FS47MixAsset>& Bank, TArray<FString>& Errors)
{
	const int32 Before = Errors.Num();
	if (!Dest || &Dest->Ctx.Get() != &InCtx.Get() || InCtx->State() == TEXT("closed")) Errors.Add(TEXT("sound mixer: destination/context unavailable"));
	if (Recipe.Name.IsEmpty() || !ValidUse(Recipe.Use) || Recipe.Layers.Num() < 1 || Recipe.Layers.Num() > 16) Errors.Add(TEXT("sound mixer: invalid recipe name, use or layer count"));
	for (const FS47MixLayer& L : Recipe.Layers)
	{
		const FS47MixAsset* A = Bank.Find(L.Source);
		if (!A || !A->Buffer) { Errors.Add(FString::Printf(TEXT("sound mixer: missing source %s"), *L.Source)); continue; }
		if (A->bModemOnly && Recipe.Use != ES47SoundUse::PcTransfer) Errors.Add(TEXT("sound mixer: modem recordings are reserved for PC transfers"));
		const FS47AudioBuffer& B = *A->Buffer;
		bool bNumbers = true;
		for (double V : { L.GainDb, L.Rate, L.HighPass, L.LowPass, L.Start, L.Length }) bNumbers &= FMath::IsFinite(V);
		if (!bNumbers || L.GainDb < -96 || L.GainDb > 0 || L.Rate < 0.1 || L.Rate > 4 || L.HighPass < 0 || L.HighPass > 20000 || L.LowPass < 0 || L.LowPass > 20000
			|| (L.LowPass > 0 && L.HighPass >= L.LowPass) || !EnvelopeKnown(L.Envelope)
			|| !FMath::IsFinite(B.SampleRate()) || B.SampleRate() < 8000 || B.SampleRate() > 192000 || B.NumberOfChannels() < 1 || B.NumberOfChannels() > 2
			|| L.Start < 0 || L.Length < 0 || L.Start >= B.Duration() || L.Start + L.Length > B.Duration() + 1e-7
			|| (L.Length > 0 && L.Length * B.SampleRate() < 16) || B.Length() - L.Start * B.SampleRate() < 16
			|| L.HighPass >= B.SampleRate() * 0.5 || L.LowPass >= B.SampleRate() * 0.5)
		{
			Errors.Add(FString::Printf(TEXT("sound mixer: invalid values in source %s"), *L.Source)); continue;
		}
		const int32 Start = int32(L.Start * B.SampleRate());
		const int32 N = L.Length > 0 ? int32(L.Length * B.SampleRate()) : B.Length() - Start;
		bool bFinite = true;
		for (int32 Ch = 0; Ch < B.NumberOfChannels() && bFinite; ++Ch)
			for (int32 I = Start; I < Start + N; ++I) if (!FMath::IsFinite(B.GetChannelData(Ch)[I])) { bFinite = false; break; }
		if (!bFinite) Errors.Add(FString::Printf(TEXT("sound mixer: nonfinite samples in %s"), *L.Source));
	}
	if (Errors.Num() != Before) return nullptr; // No node or source is created before complete validation.
	FS47AudioContext::FBatch Batch(*InCtx);
	TSharedPtr<FS47SoundMixer> Mix = MakeShareable(new FS47SoundMixer(InCtx));
	Mix->Out = InCtx->CreateGain(); Mix->Out->Gain.SetValue(0); Mix->Out->Connect(Dest);
	auto Limiter = InCtx->CreateWaveShaper(); Limiter->Curve.SetNumUninitialized(2048);
	for (int32 I = 0; I < 2048; ++I) Limiter->Curve[I] = float(0.95 * FMath::Tanh(2.0 * (2.0 * I / 2047 - 1)) / FMath::Tanh(2.0));
	Limiter->Connect(Mix->Out);
	auto Comp = InCtx->CreateDynamicsCompressor(); Comp->Threshold.SetValue(-12); Comp->Ratio.SetValue(8); Comp->Attack.SetValue(0.003); Comp->Release.SetValue(0.15); Comp->Connect(Limiter);
	for (const FS47MixLayer& L : Recipe.Layers)
	{
		auto Source = InCtx->CreateBufferSource(); Source->Buffer = Slice(*InCtx, *Bank[L.Source].Buffer, L); Source->bLoop = L.bLoop; Source->PlaybackRate.SetValue(L.Rate);
		TSharedPtr<FS47AudioNode> Tail = Source;
		if (L.HighPass > 0) { auto F = InCtx->CreateBiquadFilter(); F->SetType(TEXT("highpass")); F->Frequency.SetValue(L.HighPass); Tail->Connect(F); Tail = F; }
		if (L.LowPass > 0) { auto F = InCtx->CreateBiquadFilter(); F->SetType(TEXT("lowpass")); F->Frequency.SetValue(L.LowPass); Tail->Connect(F); Tail = F; }
		auto Gain = InCtx->CreateGain(); Gain->Gain.SetValue(0); Tail->Connect(Gain); Gain->Connect(Comp);
		Mix->Layers.Add({ Gain, FMath::Pow(10.0, L.GainDb / 20), L.Envelope });
		Source->Start(InCtx->CurrentTime()); Mix->Sources.Add(Source);
	}
	Mix->SetLevel(1); return Mix;
}

FS47SoundMixer::~FS47SoundMixer() { Stop(0); }

void FS47SoundMixer::Frame(double Clarity, double Interference, double Activity, double T)
{
	if (bStopped) return;
	FS47AudioContext::FBatch Batch(*Ctx);
	Clarity = Unit(Clarity); Interference = Unit(Interference); Activity = Unit(Activity);
	const bool bPulse = S47SignalVoice::PulseAt(FMath::IsFinite(T) ? T : 0).bOn;
	for (const FLayer& L : Layers)
	{
		double K = 1;
		if (L.Envelope == TEXT("static")) K = (1 - 0.85 * Clarity) * Interference;
		else if (L.Envelope == TEXT("signal")) K = Clarity * Clarity;
		else if (L.Envelope == TEXT("pulse")) K = bPulse ? Clarity * Clarity : 0;
		else if (L.Envelope == TEXT("activity")) K = Activity;
		L.Gain->Gain.SetTargetAtTime(L.LinearGain * K, Ctx->CurrentTime(), L.Envelope == TEXT("pulse") ? 0.008 : 0.04);
	}
}

void FS47SoundMixer::SetLevel(double Level, double Tau)
{
	if (bStopped || !Out) return;
	Out->Gain.SetTargetAtTime(Unit(Level), Ctx->CurrentTime(), FMath::IsFinite(Tau) ? FMath::Clamp(Tau, 0.001, 5.0) : 0.05);
}

void FS47SoundMixer::Stop(double Fade)
{
	if (bStopped) return;
	bStopped = true;
	FS47AudioContext::FBatch Batch(*Ctx);
	const double Seconds = FMath::IsFinite(Fade) ? FMath::Clamp(Fade, 0.0, 5.0) : 0;
	const auto Old = MoveTemp(Sources); const auto Output = Out;
	if (Output) { Output->Gain.CancelAndHoldAtTime(Ctx->CurrentTime()); Output->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), FMath::Max(0.001, Seconds)); }
	for (const auto& Source : Old) Source->Stop(Ctx->CurrentTime() + Seconds * 6);
	auto Release = [Old, Output] { for (const auto& Source : Old) Source->Disconnect(); if (Output) Output->Disconnect(); };
	if (Seconds > 0) Ctx->SetTimeout(Seconds * 6, MoveTemp(Release)); else Release();
	Layers.Reset(); Out.Reset();
}

bool FS47SoundMixer::SelfTest(TArray<FString>& Lines)
{
	bool bAll = true;
	auto Check = [&](bool bOk, const TCHAR* Name) { bAll &= bOk; Lines.Add(FString::Printf(TEXT("%s sound mixer: %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), Name)); };
	auto C = FS47AudioContext::Make(48000);
	auto Buffer = C->CreateBuffer(1, 4800, 48000);
	for (int32 I = 0; I < Buffer->Length(); ++I) Buffer->GetChannelData(0)[I] = float(0.2 * FMath::Sin(2 * PI * 440 * I / 48000));
	TMap<FString, FS47MixAsset> Bank; Bank.Add(TEXT("hiss"), { Buffer, false }); Bank.Add(TEXT("modem"), { Buffer, true });
	FS47MixRecipe Recipe; Recipe.Name = TEXT("test"); FS47MixLayer L; L.Source = TEXT("hiss"); Recipe.Layers.Add(L);
	TArray<FString> Errors;
	auto Mix = Make(C, C->Destination, Recipe, Bank, Errors);
	Check(Mix.IsValid() && Errors.IsEmpty(), TEXT("valid recording layer creates a graph"));
	TArray<float> Samples; Samples.SetNumZeroed(4800 * 2);
	if (Mix) Mix->Frame(1, 1, 1, 0.05);
	C->Render(Samples.GetData(), 4800, 2); C->Tick(0.1);
	double Energy = 0; bool bFinite = true;
	for (float V : Samples) { bFinite &= FMath::IsFinite(V) && FMath::Abs(V) <= 0.951; Energy += V * V; }
	Check(bFinite && Energy > 0.001, TEXT("recording renders finite nonzero bounded samples"));
	if (Mix) { Mix->Stop(0.01); Mix->Stop(0); }
	C->Render(Samples.GetData(), 4800, 2); C->Tick(0.1);
	Samples.Init(0, 9600); C->Render(Samples.GetData(), 4800, 2);
	double After = 0; for (float V : Samples) After += FMath::Abs(V);
	Check(After == 0, TEXT("fade and repeated stop leave silence"));
	Recipe.Layers[0].Source = TEXT("modem"); Errors.Reset();
	Check(!Make(C, C->Destination, Recipe, Bank, Errors) && !Errors.IsEmpty(), TEXT("receiver cannot use modem"));
	Recipe.Use = ES47SoundUse::Vhs; Errors.Reset();
	Check(!Make(C, C->Destination, Recipe, Bank, Errors), TEXT("VHS cannot use modem"));
	Recipe.Use = ES47SoundUse::PcTransfer; Errors.Reset();
	auto Pc = Make(C, C->Destination, Recipe, Bank, Errors);
	Check(Pc.IsValid(), TEXT("PC transfer accepts modem")); if (Pc) Pc->Stop(0);
	Recipe.Use = ES47SoundUse::Receiver; Recipe.Layers[0].Source = TEXT("hiss"); L.Source = TEXT("missing"); Recipe.Layers.Add(L); Errors.Reset();
	Check(!Make(C, C->Destination, Recipe, Bank, Errors), TEXT("missing second layer rejects whole recipe"));
	Samples.Init(0, 9600); C->Render(Samples.GetData(), 4800, 2); double Rejected = 0; for (float V : Samples) Rejected += FMath::Abs(V);
	Check(Rejected == 0, TEXT("rejected recipe leaves no partially audible graph"));
	Recipe.Layers.Pop(); Recipe.Layers[0].Rate = 0; Errors.Reset();
	Check(!Make(C, C->Destination, Recipe, Bank, Errors), TEXT("zero playback rate rejected"));
	auto J = MakeShared<FJsonObject>(); J->SetStringField(TEXT("name"), TEXT("bad")); J->SetStringField(TEXT("use"), TEXT("alien")); Errors.Reset();
	FS47MixRecipe Parsed; Check(!ParseRecipe(J, Parsed, Errors), TEXT("invalid JSON use and empty layers rejected"));
	C->Close(); return bAll;
}
