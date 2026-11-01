#pragma once
#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Audio;

// One mixer per player, shared by nearby small Roswells and the SC04 threat.
// All positions are gameplay Three metres (Y up). Unset means carried/2D.
class FS47EncounterSound
{
public:
 enum class EKind : uint8 { Presence, Discovery, Hunt };
 FS47EncounterSound()=default;
 ~FS47EncounterSound();
 FS47EncounterSound(const FS47EncounterSound&)=delete;
 FS47EncounterSound& operator=(const FS47EncounterSound&)=delete;
 bool Cue(US47Audio* Audio,EKind Kind,TOptional<FVector> AtThree={},double Strength=1.);
 void UpdateRadio(US47Audio* Audio,double Level,TOptional<FVector> AtThree={},bool bHeld=false);
 void Hold(bool bHeld);
 void Reset();
 static bool SelfTest(TArray<FString>& Lines);
 static bool RenderTour(const FString& Path);
private:
 struct FVoice { TSharedPtr<FS47BufferSourceNode> Source; TSharedPtr<FS47GainNode> Gain; double End=0; };
 TSharedPtr<FS47AudioContext> Context;
 TArray<TSharedPtr<FS47AudioBuffer>> Cues;
 TArray<FVoice> Voices;
 TSharedPtr<FS47BufferSourceNode> RadioSource;
 TSharedPtr<FS47GainNode> RadioGain,RadioVolume;
 TSharedPtr<FS47PannerNode> RadioPan;
 TWeakObjectPtr<US47Audio> Owner;
 bool bHeld=false,bRadioPlaced=false;
 double RadioLevel=0,NextAny=0,NextKind[3]={0,0,0},CueUntil=0;
 int32 LastKind=-1;
 bool Bind(US47Audio* Audio);
 void RadioMix();
 void FadeVoices(double Secs);
};
