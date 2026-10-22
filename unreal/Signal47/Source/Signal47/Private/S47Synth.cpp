#include "S47Synth.h"

#include "S47WebAudio.h"

bool US47Synth::Init(int32& SampleRate)
{
	// the context's rate (48 kHz, as the browser's usually is); the mixer converts if the device differs
	NumChannels = 2;
	if (Ctx.IsValid()) SampleRate = int32(Ctx->SampleRate);
	return true;
}

int32 US47Synth::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const int32 Channels = FMath::Max(1, NumChannels);
	if (Ctx.IsValid()) Ctx->Render(OutAudio, NumSamples / Channels, Channels);
	else FMemory::Memzero(OutAudio, sizeof(float) * NumSamples);
	return NumSamples;
}
