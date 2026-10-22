#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "S47Audio.generated.h"

class UAudioComponent;
class USoundBase;

// audio.play's options (core/Audio.ts), positions in the web version's terms (three.js metres)
struct FS47Sound
{
	float Gain = 1.f;
	float Rate = 1.f;
	TOptional<FVector> At;   // three.js world position; unset: not placed (2D)
	float When = 0.f;        // seconds from now
	FString Dest;            // 'radio': through the truck's radio speaker (radioIn)
	bool bLoop = false;
};

// a line of a telephone call (core/phoneScripts.ts PhoneCue) and the timeline audio.phoneCall gives
struct FS47PhoneCue { FString Text; FString By; bool bAside = false; TOptional<float> Pause; };
struct FS47CallCue { float At = 0, Duration = 0; FString Text, By; bool bAside = false; };
struct FS47CallTimeline { float Duration = 0; TArray<FS47CallCue> Cues; };

// The game's sounds, after web/src/core/Audio.ts: one-shots by name, loops by key, and the
// receiver's carrier, the anomaly's pulses and the dish motors as looping components whose
// volume follows the story. Sounds are looked up in /Game/Licensed/Sfx (the bought pack, if
// imported), /Game/Audio/Sfx (the free sounds) and /Game/Audio/Synth (tones made by
// Scripts/make-synth.py). A missing sound is logged once and skipped; the game goes on.
UCLASS()
class SIGNAL47_API US47Audio : public UObject
{
	GENERATED_BODY()

public:
	void Init(UWorld* InWorld);
	void Tick(float DeltaSeconds);

	void Play(const FString& Name, float Gain = 1.f, const FVector& At = FVector::ZeroVector, float Rate = 1.f);
	void Loop(const FString& Key, const FString& Name, float Gain = 1.f, const FVector& At = FVector::ZeroVector);
	void Stop(const FString& Key, float Fade = 0.05f);
	void Pour(const FVector& At);
	void StartCarrier();
	void SetCarrier(float Level, float PitchOffset, float StaticLevel);
	void SignalFrame(float Clarity, float Level);
	void SignalOff();
	void StartMotors();
	void StopMotors();
	void Boom(float Strength);
	void HoldRoomTone(float Level);
	void StartRoomTone();
	double FutureCall(); // the call from the future: its sounds in order, returns how long it is
	void AmRadio(bool bOn, const FVector& At);
	void AmRadioJam(bool bOn);

	// ---------- core/Audio.ts by its own names (S47AudioWeb.cpp), positions in three.js terms ----------
	// A call that is not ported yet does what the web version does when a recording is missing:
	// nothing (PhoneCall returns unset, so the caller shows its captions).
	void Sfx(const FString& Name, const FS47Sound& O = FS47Sound());       // play(name, o)
	FString Or(const FString& Name, const FString& Old = FString()) const;   // the bought recording if there is one
	void LoopSfx(const FString& Key, const FString& Name, const FS47Sound& O = FS47Sound()); // loop(key, name, o)
	void Beep(float Freq = 1200.f, float Dur = 0.08f, float Gain = 0.12f, float When = 0.f, const FString& Type = TEXT("square"));
	void Hum(const FString& Key, const FVector& At, float Gain = 0.05f, float Base = 60.f);
	void StopHums();
	void HumLevel(float K, float Tau = 0.08f);
	void Servo(float Secs, TOptional<FVector> At = {});
	void Signature(float Gain = 0.5f);
	void Shutter(const FString& Dest = FString(), float When = 0.f);
	void PourThree(TOptional<FVector> At = {});
	void Spatial(const FString& Name, const FVector& At, float Ref = 1.f);
	TOptional<FS47CallTimeline> PhoneCall(const TArray<FS47PhoneCue>& Lines, float Lead = 0.3f, float Gap = 0.45f);
	void StopPhoneCalls();
	bool PhonePlaying() const { return bPhonePlaying; }
	void CallLine(float Secs);
	float Dial(const FString& Number);       // seconds the dialling takes
	float Ringback(int32 Times, float Start = 0.f); // seconds
	void Radio(TOptional<FVector> At, float Level = 0.06f);
	void RadioVoice(float Secs, float When = 0.f);
	void TvHiss(TOptional<FVector> At);
	void RelayClack(const FVector& At, bool bThrough);
	void Contactor(const FVector& At, float Gain = 1.f);
	void PowerDown(const FVector& At);
	void PowerUp(const FVector& At);
	void TelexMotor(TOptional<FVector> At);
	void TypeBar(const FVector& At, bool bCr = false);
	void CabRadio(const FString& Mode);      // 'talk' | 'carrier' | 'hiss' | '' (off)
	void CabLevel(float K);
	void CabVoice(float Secs, float Formant);
	void CupDown(bool bThroughRadio = false);
	void Pressure(bool bOn);
	void Silence(bool bOn);
	void SetSpace(const FString& Space);     // 'room' | 'yard' | 'lab'
	void Step(const FString& Surface);       // a footstep on a surface
	void Gasp(float When = 0.f);
	void Thunder(float Strength = 0.6f);
	void HoldThunder(float Level);
	void DuckAll(float Seconds);
	bool bKinskiCalls = true, bCharacterVoices = true;

	// ---------- drive/engineSound.ts (FS47EngineSound) ----------
	// A loop's level now (the engine's recordings, mixed by rpm each frame): volume, playback rate,
	// and a low-pass in Hz (0: none).
	void LoopLevel(const FString& Key, float Gain, float Rate, float LowPass = 0.f);
	float Duration(const FString& Name);     // a recording's length in seconds (0 when it is missing)
	// The truck's sounds made with Web Audio nodes in the web version, by EngineSound's own names
	// (silent stand-ins until the procedural sound is in, S47AudioWeb.cpp):
	void EngineBus(float Level, float Rumble, float RumbleFreq); // each frame: the truck's level, the band-passed rumble's gain and centre
	void EngineTyres(const FString& Surface, float Speed);       // each frame: rolling noise, gravel crunch, dirt rumble, wind (tyres)
	void EngineCough(float K);                                   // a misfire's dull pop in the exhaust (cough)
	void EngineCrank(float Secs);                                // the starter turning the dead engine over (crank)
	void EngineSolenoid(const TArray<float>& Secs, float K);     // the key on a dead truck: solenoid clicks (solenoid)
	void EngineRattle(float Speed);                              // a cattle guard under both axles (rattle)
	void EngineThump(float Speed);                               // a knock against a post or a fence (thump)
	void EngineStop(float After);                                // the truck's own layers fade and stop (stop)

private:
	bool bPhonePlaying = false;
	void NotYet(const TCHAR* What) const;
	TWeakObjectPtr<UWorld> World;
	UPROPERTY() TMap<FString, TObjectPtr<USoundBase>> Cache;
	UPROPERTY() TMap<FString, TObjectPtr<UAudioComponent>> Loops;
	TSet<FString> Missing;
	float RoomToneTarget = 1.f, RoomToneLevel = 1.f;
	USoundBase* Find(const FString& Name);
	UAudioComponent* LoopComponent(const FString& Key, const FString& Name, float Gain, const FVector& At, bool b2D);
};
