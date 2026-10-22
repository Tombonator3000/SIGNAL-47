#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "S47Synth.generated.h"

class FS47AudioContext;

// The speaker of the Web Audio context (S47WebAudio.h): a 2D synth whose audio is the context's
// destination, rendered on the audio thread. Nothing here is placed in Unreal's space; the
// context pans its sounds itself against its listener (the player's eye, US47Audio::Tick).
UCLASS()
class US47Synth : public USynthComponent
{
	GENERATED_BODY()

public:
	TSharedPtr<FS47AudioContext> Ctx;

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
};
