#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class FJsonObject;

// Unreal-only sound workshop: recordings and the existing synth share one audio graph.
enum class ES47SoundUse : uint8 { Receiver, Vhs, PcTransfer, Equipment };

struct FS47MixAsset
{
	TSharedPtr<FS47AudioBuffer> Buffer;
	bool bModemOnly = false;
};

struct FS47MixLayer
{
	FString Source;
	double GainDb = -18, Rate = 1, HighPass = 0, LowPass = 0, Start = 0, Length = 0;
	bool bLoop = true;
	FString Envelope = TEXT("constant"); // constant, static, signal, pulse, activity
};

struct FS47MixRecipe
{
	FString Name;
	ES47SoundUse Use = ES47SoundUse::Receiver;
	TArray<FS47MixLayer> Layers;
};

class SIGNAL47_API FS47SoundMixer
{
public:
	static TSharedPtr<FS47SoundMixer> Make(const TSharedRef<FS47AudioContext>& Ctx,
		const TSharedPtr<FS47AudioNode>& Dest, const FS47MixRecipe& Recipe,
		const TMap<FString, FS47MixAsset>& Bank, TArray<FString>& Errors);
	static bool ParseRecipe(const TSharedPtr<FJsonObject>& Json, FS47MixRecipe& Recipe, TArray<FString>& Errors);
	static bool SelfTest(TArray<FString>& Out);
	~FS47SoundMixer();
	void Frame(double Clarity, double Interference, double Activity, double T);
	void SetLevel(double Level, double Tau = 0.05);
	void Stop(double Fade = 0.05);
	bool IsStopped() const { return bStopped; }

private:
	explicit FS47SoundMixer(const TSharedRef<FS47AudioContext>& InCtx) : Ctx(InCtx) {}
	struct FLayer { TSharedPtr<FS47GainNode> Gain; double LinearGain; FString Envelope; };
	TSharedRef<FS47AudioContext> Ctx;
	TSharedPtr<FS47GainNode> Out;
	TArray<TSharedPtr<FS47BufferSourceNode>> Sources;
	TArray<FLayer> Layers;
	bool bStopped = false;
};
