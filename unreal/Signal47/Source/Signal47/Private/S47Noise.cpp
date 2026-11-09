#include "S47Noise.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Noise, Log, All);

namespace
{
	TArray<S47Noise::FNoise>& NoiseRecent() { static TArray<S47Noise::FNoise> R; return R; }
	uint32& NoiseSerial() { static uint32 S = 0; return S; }
	int32& NoiseMade() { static int32 N = 0; return N; }
}

void S47Noise::Make(const FVector& At, double Loudness, const FString& What)
{
	if (!FMath::IsFinite(Loudness) || Loudness < 0.05 || At.ContainsNaN()) return;
	FNoise N; N.At = At; N.Loudness = FMath::Min(Loudness, 2.0); N.Time = FPlatformTime::Seconds(); N.Serial = ++NoiseSerial(); N.What = What;
	TArray<FNoise>& R = NoiseRecent();
	// A few seconds of them at most: nothing reads an older one.
	R.RemoveAll([&N](const FNoise& Old) { return N.Time - Old.Time > 3.0; });
	if (R.Num() >= 16) R.RemoveAt(0);
	R.Add(N);
	++NoiseMade();
	UE_LOG(LogS47Noise, Display, TEXT("S47: noise: %s, loudness %.2f (heard to %.0f m) at %.1f %.1f %.1f"), *What, N.Loudness, Reach(N.Loudness), At.X, At.Y, At.Z);
}

TOptional<S47Noise::FNoise> S47Noise::Latest(double Within)
{
	const double Now = FPlatformTime::Seconds();
	TOptional<FNoise> Best;
	for (const FNoise& N : NoiseRecent())
		if (Now - N.Time <= Within && (!Best || N.Loudness > Best->Loudness)) Best = N;
	return Best;
}

TOptional<S47Noise::FNoise> S47Noise::Last()
{
	const TArray<FNoise>& R = NoiseRecent();
	if (R.IsEmpty()) return {};
	return R.Last();
}

int32 S47Noise::Count() { return NoiseMade(); }

void S47Noise::Clear() { NoiseRecent().Reset(); }
