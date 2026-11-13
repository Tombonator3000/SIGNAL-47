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
 // DREAD-20261110 (Tom): bought Horror Ambience as the ground under a meeting, and a bought hit cut in at the sight. All of it
 // optional: without the private clone's files (lic:dread_*, lic:scare_*, lic:sky_*) the synthesised cues stand alone.
 enum class EScare : uint8 { Soft, Hard, Braam, Swell };
 // Called each frame while not held: how much of the ground (Low) and of the layer over it (High) is wanted, 0 to 1. It eases itself.
 void Dread(US47Audio* Audio,double Low,double High);
 // One bought hit, not placed anywhere. False when its file is missing, it has just been heard, or another cue is sounding.
 // bAudition (F10) plays it whatever was heard last.
 bool Scare(US47Audio* Audio,EScare Kind,double Strength=1.,bool bAudition=false);
 // Heard (D45-20261110): how near anything is as the Freeform set hears it (FS47FieldRadio::Approach, 48 m): the ground comes up to
 // .45 of that before the cues' 18 m, so that it is there under the eerie music while something comes. 0: as before.
 static void DreadFor(double Level,bool bSeen,bool bHunt,double& Low,double& High,double Heard=0);
 // What a Freeform happening sounds like the moment it is first seen, by its distance in metres (under 0: at the player): -1 nothing.
 static int32 ScareFor(const FString& Happening,double DistM);
 int32 BoughtHits() const { return NBought; }
 // 0 to 1: how much of the bought ground is asked for now. The score stands back for it (S47Main): two beds on one bus was mud.
 double DreadNow() const;
 void UpdateRadio(US47Audio* Audio,double Level,TOptional<FVector> AtThree={},bool bHeld=false);
 // A message on the set: click, squelch, the set's static breaking up in speech's rhythm (D45-20261110: no voice babble; the
 // caption carries the words), new each time, and the squelch closing.
 bool Message(US47Audio* Audio);
 // A broken made voice, slowed and from a place: not quite speech, not through any set (the alien at the door; unchanged).
 bool Talk(US47Audio* Audio,const FVector& AtThree);
 // One small burst of crackle on the set, over the hiss. Strength 0 to 1. False if the set is not sounding yet.
 bool Crackle(US47Audio* Audio,double Strength);
 void Hold(bool bHeld);
 void Reset();
 // SIDEBAND-20261113 (one owner per job): this class stays where the ground and the hit are asked for (Dread each frame, Hit at a
 // first sight or a hunt, Scare from a happening), and FLARES (S47Sideband.h) may take either. Ground true: the sink plays the
 // ground, and the bought beds here stay down. Moment true: the sink took the moment, with or without a sound of its own, and
 // neither a bought hit nor the made cue follows. Without a sink, or when it says no (its files are missing, the setting is off,
 // Story), everything here is as it was. An audition (F10) is never offered to it.
 struct FSink { TFunction<bool(double Low,double High,bool bHeld)> Ground; TFunction<bool(int32 Scare,double Strength,bool bAudition)> Moment; };
 void SetSink(FSink In) { Sink=MoveTemp(In); }
 int32 SunkMoments() const { return NSunk; }
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
 FVoice Beds[2]; double BedWant[2]={0,0},NextScare[4]={0,0,0,0},BoughtUntil=0,NextBought=0; int32 ScareTurn[4]={0,0,0,0},NBought=0;
 bool Hit(US47Audio* Audio,EScare Kind,double Strength,double Now);
 FSink Sink; int32 NSunk=0;
 void BedsDown(double TimeConstant);
 bool Bind(US47Audio* Audio);
 void RadioMix();
 void StopTransmission();
 void FadeVoices(double Secs);
};
