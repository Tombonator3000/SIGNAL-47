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
 // A message on the set: click, squelch, a voice too broken to make out, and the squelch closing.
 bool Message(US47Audio* Audio);
 // The same broken voice, slowed and from a place: not quite speech, not through any set.
 bool Talk(US47Audio* Audio,const FVector& AtThree);
 // One small burst of crackle on the set, over the hiss. Strength 0 to 1. False if the set is not sounding yet.
 bool Crackle(US47Audio* Audio,double Strength);
 void Hold(bool bHeld);
 void Reset();
 static bool SelfTest(TArray<FString>& Lines);
 static bool RenderTour(const FString& Path);
private:
 struct FVoice { TSharedPtr<FS47BufferSourceNode> Source; TSharedPtr<FS47GainNode> Gain; double End=0; };
 TSharedPtr<FS47AudioContext> Context;
 TArray<TSharedPtr<FS47AudioBuffer>> Cues;
 TArray<FVoice> Voices;
 TSharedPtr<FS47BufferSourceNode> RadioSource, Transmission;
 TSharedPtr<FS47AudioBuffer> TransmissionBuffer,MessageSound;
 TArray<TSharedPtr<FS47AudioBuffer>> Crackles;
 TSharedPtr<FS47BufferSourceNode> MessageSource,TalkSource;
 double NextTransmission=0;
 // T16-12: when the set began to hear one strongly; NotLoud while it does not (a time of its own, which an offline test clock can put below zero).
 static constexpr double NotLoud=-1e9;
 double LoudSince=NotLoud;
 TSharedPtr<FS47GainNode> RadioGain,RadioVolume,TransmissionGain;
 TSharedPtr<FS47PannerNode> RadioPan;
 TWeakObjectPtr<US47Audio> Owner;
 bool bHeld=false,bRadioPlaced=false;
 double RadioLevel=0,NextAny=0,NextKind[3]={0,0,0},CueUntil=0;
 int32 LastKind=-1;
 bool Bind(US47Audio* Audio);
 void RadioMix();
 void StopTransmission();
 void FadeVoices(double Secs);
};
