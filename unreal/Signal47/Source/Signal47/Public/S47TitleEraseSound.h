#pragma once
#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Audio;

// Starts only from the title's post-FirstTouch visual callback. This object
// never unlocks audio or listens to input. One successful Start per instance.
class FS47TitleEraseSound
{
public:
 static constexpr double Duration=1.65;
 FS47TitleEraseSound()=default;
 ~FS47TitleEraseSound();
 FS47TitleEraseSound(const FS47TitleEraseSound&)=delete;
 FS47TitleEraseSound& operator=(const FS47TitleEraseSound&)=delete;
 bool Start(US47Audio* Audio,bool bCalm=false);
 // Stops this cue only. Does not re-arm a successfully started instance.
 void Stop();
 static bool SelfTest(TArray<FString>& Lines);
 // Normal cue, a short gap, then the softer calm variant, on a private context.
 static bool RenderTour(const FString& AbsoluteWavPath);
private:
 bool bStarted=false;
 TSharedPtr<FS47BufferSourceNode> Source;
 TSharedPtr<FS47GainNode> Gain;
};
