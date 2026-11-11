#pragma once
#include "CoreMinimal.h"
#include "S47SoundMixer.h"

class US47Audio;
class FS47BufferSourceNode;
class FS47GainNode;

// Reference transport only. It never owns an observing run, evidence cursor or saved fact.
class SIGNAL47_API FS47ReceiverSound
{
public:
 explicit FS47ReceiverSound(US47Audio* Audio, TSharedPtr<FS47AudioNode> Destination = nullptr);
 ~FS47ReceiverSound();
 void Next();
 bool Toggle();
 void Stop();
 void Step(bool bMayListen, double T);
 bool Playing() const { return bPlaying; }
 FString Label() const;
 // D48-20261110 (D46-16): the references are known sources a 1986 station meets. Kind 0 natural, 1 man-made; FeederCause is the first
 // case's explanation a man-made one stands for (0 the feeder, 1 a local transmitter), -1 for the natural ones.
 static int32 Count();
 static const TCHAR* NameOf(int32 Index);
 static const TCHAR* BlurbOf(int32 Index);
 static int32 KindOf(int32 Index);
 static int32 FeederCause(int32 Index);
 // The look of a source on a spectrogram, 0..1 at a time and frequency in 0..1 (the take is drawn beside it on the DATA page).
 static float Signature(int32 Index,float T,float F);
 int32 Index() const { return Selected; }
 static bool Allows(const FString& Recipe, ES47SoundUse Use);
 static bool SelfTest(TArray<FString>& Lines);
private:
 TWeakObjectPtr<US47Audio> Audio;
 TSharedPtr<FS47AudioNode> Destination;
 int32 Selected = 0;
 double EndAt = 0;
 bool bPlaying = false;
 TSharedPtr<FS47BufferSourceNode> SynthSource;
 TSharedPtr<FS47GainNode> SynthGain;
 FString Feedback;
};
