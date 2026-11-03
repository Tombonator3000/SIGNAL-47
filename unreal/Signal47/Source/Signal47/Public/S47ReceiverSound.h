#pragma once
#include "CoreMinimal.h"
#include "S47SoundMixer.h"

class US47Audio;

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
 static bool Allows(const FString& Recipe, ES47SoundUse Use);
 static bool SelfTest(TArray<FString>& Lines);
private:
 TWeakObjectPtr<US47Audio> Audio;
 TSharedPtr<FS47AudioNode> Destination;
 int32 Selected = 0;
 double EndAt = 0;
 bool bPlaying = false;
 FString Feedback;
};
