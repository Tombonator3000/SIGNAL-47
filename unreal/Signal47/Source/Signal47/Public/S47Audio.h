#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "S47WebAudio.h"
#include "S47Audio.generated.h"

class AActor;
class UAudioComponent;
class USoundBase;
class US47Synth;
class FS47SignalVoice;
class FS47Ambience;

// audio.play's options (core/Audio.ts), positions in the web version's terms (three.js metres)
struct FS47Sound
{
	float Gain = 1.f;
	float Rate = 1.f;
	TOptional<FVector> At;   // three.js world position; unset: not placed (2D)
	float When = 0.f;        // seconds from now
	FString Dest;            // 'radio': through the truck's radio speaker (radioIn); 'music', 'amb', 'radioBus', 'master', 'sfx': that bus
	bool bLoop = false;
	float Lp = 0.f;          // lp: a low-pass at this frequency (0: none)
	float From = 0.f;        // from: seconds into the recording
	TOptional<float> Dur;    // dur: only this long of it
	TSharedPtr<FS47AudioNode> DestNode; // dest: a node of the graph (wins over Dest)
};

// what audio.play returns: the source and its gain (both unset when nothing played)
struct FS47Play
{
	TSharedPtr<FS47BufferSourceNode> Src;
	TSharedPtr<FS47GainNode> Gain;
	TSharedPtr<FS47BiquadFilterNode> Filter;
	bool IsValid() const { return Src.IsValid(); }
	explicit operator bool() const { return Src.IsValid(); }
};

/** Read-only snapshot of a recording loop's actual audio graph (not an asset/component estimate). */
struct FS47LoopState
{
	bool bPlaying = false, bLoop = false;
	double Gain = 0, Rate = 0, LowPass = 0;
	TSharedPtr<const FS47AudioBuffer> Buffer;
};

// a line of a telephone call (core/phoneScripts.ts PhoneCue) and the timeline audio.phoneCall gives
struct FS47PhoneCue { FString Text; FString By; bool bAside = false; TOptional<float> Pause; };
struct FS47CallCue { float At = 0, Duration = 0; FString Text, By; bool bAside = false; };
struct FS47CallTimeline { float Duration = 0; TArray<FS47CallCue> Cues; };

// audio.phoneLine(): the handset's band, its input, its output gain and every node in it
struct FS47PhoneLine { TSharedPtr<FS47AudioNode> Input; TSharedPtr<FS47GainNode> Gain; TArray<TSharedPtr<FS47AudioNode>> Nodes; };
// audio.staticSrc(): the static of an AM set at night, looping and started; Out has the radios' level
struct FS47StaticSrc { TSharedPtr<FS47BufferSourceNode> Src; TSharedPtr<FS47AudioNode> Out; };
// audio.pressure(): the bought low bed, at gain 0 for the caller to raise; Stop(fade), 0 at once
struct FS47Pressure
{
	TSharedPtr<FS47GainNode> Gain;
	TFunction<void(float)> StopFn;
	void Stop(float Fade = 0.f) const { if (StopFn) StopFn(Fade); }
	explicit operator bool() const { return Gain.IsValid(); }
};

// The game's sounds: web/src/core/Audio.ts (AudioSys), ported onto the Web Audio of
// S47WebAudio.h and played through one synth component. Recordings are decoded into the context
// by their web names (phoneRing, thudSoft, stepConcrete0, lic:door_heavy, phone:<id> ...) from the
// WAVE files Scripts/import-audio.sh writes to Content/Audio/Raw and Content/Licensed/Raw (with
// their index.json); the synthesised sounds are the same graphs as the web version's.
//
// Two sets of names: the first block takes Unreal positions (cm) and file names (thud_soft), as
// the prologue was written against; the rest is core/Audio.ts by its own names with positions in
// three.js terms. The first block now goes through the same graphs where the web version has the
// sound, and falls back to the imported sound assets (/Game/Licensed/Sfx, /Game/Audio/Sfx, Synth,
// Music, Radio) for names it does not have.
UCLASS()
class SIGNAL47_API US47Audio : public UObject
{
	GENERATED_BODY()

public:
	void Init(UWorld* InWorld);
	void Tick(float DeltaSeconds);
	virtual void BeginDestroy() override;

	// ---------- the prologue's names (Unreal positions, file names) ----------
	void Play(const FString& Name, float Gain = 1.f, const FVector& At = FVector::ZeroVector, float Rate = 1.f);
	void Loop(const FString& Key, const FString& Name, float Gain = 1.f, const FVector& At = FVector::ZeroVector);
	void Stop(const FString& Key, float Fade = 0.05f); // stop(key, fade)
	void Pour(const FVector& At);
	void StartCarrier();
	void SetCarrier(float Level, float PitchOffset, float StaticLevel);
	void SignalFrame(float Clarity, float Level); // the time is main.ts's t (US47Main::T)
	void SignalOff();
	void StartMotors();
	void StopMotors();
	void Boom(float Strength = 1.f, TSharedPtr<FS47AudioNode> Dest = nullptr, float When = 0.f);
	void HoldRoomTone(float Level, float Tau = 0.1f);
	void StartRoomTone();
	double FutureCall(); // the call from the future: its sounds in order, returns how long it is
	void AmRadio(bool bOn, const FVector& At);
	void AmRadioJam(bool bOn);

	// ---------- core/Audio.ts by its own names, positions in three.js terms ----------
	TSharedPtr<FS47AudioContext> Ctx;
	TSharedPtr<FS47GainNode> Master, SfxBus, Music, Amb, RadioBus; // master, sfx, music, amb, radioBus
	TMap<FString, TSharedPtr<FS47AudioBuffer>> Buf;
	float Volume = 0.8f;
	bool bReady = false;
	bool bKinskiCalls = true, bCharacterVoices = true; // kinskiCalls, characterVoices (main.ts sets them from the settings)
	TFunction<void()> OnPhoneCall;

	/** The bought sounds are in this build (the credits name them only then): BOUGHT_SFX. */
	bool BoughtSfx() const;
	// carrier: the receiver's tone once startCarrier() made it (the prologue's update asks for it)
	struct FCarrier { TSharedPtr<FS47OscillatorNode> Osc; TSharedPtr<FS47GainNode> Gain, Stat; } Carrier;
	TSharedPtr<FS47AudioBuffer> CharacterBuffer(const FString& Voice, const FString& Text) const;
	bool PhonePlaying() const { return PhoneSession.IsValid(); }
	void SetVolume(float V);
	float MusicLevel = 0.7f; // the music's own level (the pause menu), 0..1
	void SetMusic(float V);
	float RadioLevel = 1.f;  // the radios' own level (the pause menu), 0..1
	void SetRadio(float V);
	void Listener(const FVector& Pos, const FVector& Forward, const FVector& Up); // listener(cam)

	FS47Play Sfx(const FString& Name, const FS47Sound& O = FS47Sound());   // play(name, o)
	FString Or(const FString& Name, const FString& Old = FString()) const; // or(name, old)
	TSharedPtr<FS47AudioBuffer> Lic(const FString& Name) const;            // lic(name)
	FS47Play LoopSfx(const FString& Key, const FString& Name, const FS47Sound& O = FS47Sound()); // loop(key, name, o)

	TOptional<FS47StaticSrc> StaticSrc();
	FS47Pressure Pressure();
	void Pressure(bool bOn); // (the framework's stand-in: a bed of its own raised to 1, or let go)
	float RoomToneHeld() const;
	void SignalFrame(float Clarity, float Level, double T);
	TSharedPtr<FS47AudioBuffer> NoiseBuffer() const { return Ctx.IsValid() ? Noise : nullptr; }
	TSharedPtr<FS47AudioNode> Spatial(const FVector& At, float Ref = 1.f, TSharedPtr<FS47AudioNode> Dest = nullptr); // spatial(at, ref, dest)
	void Spatial(const FString& Name, const FVector& At, float Ref = 1.f); // a recording played there

	void AmRadioThree(TOptional<FVector> At); // amRadio(at | null)
	bool RadioOn() const { return Am.Out.IsValid(); }
	void Beep(float Freq = 1200.f, float Dur = 0.08f, float Gain = 0.12f, float When = 0.f, const FString& Type = TEXT("square"));
	void HoldThunder(float Level);
	void Thunder(float Strength = 0.6f);
	void Gasp(float When = 0.f);
	void Gasp(TSharedPtr<FS47AudioNode> Dest, float When = 0.f);
	void PourThree(TOptional<FVector> At = {});
	void TvHiss(TOptional<FVector> At);
	FS47PhoneLine PhoneLine();
	void CallLine(float Secs);
	TOptional<FS47CallTimeline> PhoneCall(const TArray<FS47PhoneCue>& Lines, float Lead = 0.3f, float Gap = 0.45f);
	TSharedPtr<FS47AudioBuffer> LineBuffer(const FString& By, const FString& Text) const;
	void StopPhoneCalls();
	float Dial(const FString& Number);       // seconds the dialling takes
	float Ringback(int32 Times, float Start = 0.f); // seconds
	void Hum(const FString& Key, const FVector& At, float Gain = 0.05f, float Base = 60.f);
	void StopHums();
	void HumLevel(float K, float Tau = 0.08f);
	float HumHeld() const { return HumK; }
	void RelayClack(const FVector& At, bool bThrough);
	void Contactor(const FVector& At, float Gain = 1.f);
	void PowerDown(const FVector& At);
	void PowerUp(const FVector& At);
	void TelexMotor(TOptional<FVector> At);
	void TypeBar(const FVector& At, bool bCr = false);
	void Radio(TOptional<FVector> At, float Level = 0.06f);
	void RadioVoice(float Secs, float When = 0.f);
	void CabRadio(const FString& Mode);      // 'talk' | 'carrier' | 'hiss' | '' (null: off)
	TSharedPtr<FS47AudioNode> RadioIn() const; // the cab speaker's input (null when the radio is off)
	void CabLevel(float K);
	void CabVoice(float Secs, float Formant);
	void CupDown(bool bThroughRadio = false);
	void CupDown(TSharedPtr<FS47AudioNode> Dest);
	void Silence(bool bOn);
	void SetSpace(const FString& InSpace);   // 'room' | 'yard' | 'lab'
	TSharedPtr<FS47Ambience> Night;          // night (core/ambience.ts), after StartNight
	void StartNight();
	struct FStepPick { FString Name; float Gain = 0.f; };
	FStepPick StepPick(const FString& Surface) const;
	void Step(const FString& Surface);       // a footstep on a surface ('tile' | 'concrete' | 'dirt' | 'wood' | 'carpet')
	void NightLife(float Dt, const FVector& At, double Clock = 0.0);
	void Far(const FString& Name, const FVector& At, float Gain, float Rate = 1.f);
	void DuckAll(float Seconds);
	void Shutter(const FString& Dest = FString(), float When = 0.f); // Dest: '' or 'radio'
	void Shutter(TSharedPtr<FS47AudioNode> Dest, float When = 0.f);
	void Servo(float Secs, TOptional<FVector> At = {});
	void Signature(float Gain = 0.5f);
	void Suspend(bool bOn);

	// text as the recordings are keyed (readAloud.ts normalizeReadingText)
	static FString NormalizeReadingText(const FString& Text);
	static bool SelfTest(TArray<FString>& Lines);
	// -S47AudioTour=<file.wav>: every sound in turn, rendered offline into a WAVE file (S47AudioTour.cpp)
	static void Tour(const FString& Path);
	// -S47AudioCompare=<dir>: graphs with known input rendered offline, to hold against a browser's (S47AudioTour.cpp)
	static void Compare(const FString& Dir);

	// ---------- drive/engineSound.ts (FS47EngineSound) ----------
	// A loop's level now (the engine's recordings, mixed by rpm each frame): volume, playback rate,
	// and a low-pass in Hz (0: none).
	void LoopLevel(const FString& Key, float Gain, float Rate, float LowPass = 0.f);
	TOptional<FS47LoopState> InspectLoop(const FString& Key) const;
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
	TWeakObjectPtr<UWorld> World;
	UPROPERTY() TObjectPtr<US47Synth> Synth;
	UPROPERTY() TObjectPtr<AActor> SynthHost;
	UPROPERTY() TMap<FString, TObjectPtr<USoundBase>> Cache;
	UPROPERTY() TMap<FString, TObjectPtr<UAudioComponent>> AssetLoops;
	TSet<FString> Missing;
	USoundBase* Find(const FString& Name);
	UAudioComponent* LoopComponent(const FString& Key, const FString& Name, float Gain, const FVector& At, bool b2D);

	// the recordings' index (Content/Audio/Raw/index.json, Content/Licensed/Raw/index.json)
	TMap<FString, FString> ByFile;           // file name (thud_soft) -> web name (thudSoft), the bought ones first
	TMap<FString, FString> PhoneKeys;        // normalised text -> phone:<id>
	TMap<FString, TMap<FString, FString>> CharacterKeys; // voice -> normalised text -> character:<id>
	void DecodeAll();
	void Unlock(bool bWithSynth, bool bDecode = true); // preload() and unlock(): the context, the buses, the noise, the recordings
	TSharedPtr<FS47AudioBuffer> MakeNoise(bool bBrown) const;
	TSharedRef<FS47PannerNode> Panner(const FVector& Pos, double Ref = 1.2, TSharedPtr<FS47AudioNode> Dest = nullptr);
	TSharedRef<FS47BufferSourceNode> NoiseSrc(bool bBrown = false, bool bLoop = true);
	TSharedPtr<FS47AudioNode> BusNamed(const FString& Name) const;

	TSharedPtr<FS47AudioBuffer> Noise, Brown;
	TMap<FString, FS47Play> Loops;
	TSharedPtr<FS47SignalVoice> Signal;
	TSharedPtr<FS47GainNode> RoomTone, ToneHold, ThunderBus;
	struct FMotor { TSharedPtr<FS47GainNode> Gain; TSharedPtr<FS47OscillatorNode> Osc; } Motor;
	struct FAm { TSharedPtr<FS47GainNode> Out, MusicGain, Hiss; TFunction<void()> Stop; } Am;
	struct FTv { TSharedPtr<FS47BufferSourceNode> Src; TSharedPtr<FS47GainNode> Gain; } Tv;
	struct FPhoneSession { TArray<TSharedPtr<FS47ScheduledSourceNode>> Sources; TArray<TSharedPtr<FS47AudioNode>> Nodes; };
	TSharedPtr<FPhoneSession> PhoneSession;
	TMap<FString, TSharedPtr<FS47GainNode>> Hums;
	TMap<FString, float> HumGain;
	float HumK = 1.f;
	struct FTelex { TSharedPtr<FS47GainNode> G; TFunction<void()> Stop; } TelexRun;
	struct FRadioNodes { TSharedPtr<FS47GainNode> Out, Voice; TFunction<void()> Stop; } RadioNodes;
	struct FCab { TSharedPtr<FS47GainNode> Out; TSharedPtr<FS47AudioNode> Spk; TSharedPtr<FS47GainNode> Talk, Car, Hiss, Swell; TFunction<void()> Stop; } Cab;
	FString Space = TEXT("room");
	FString LastStep;
	FS47Pressure PressureBed;
};
