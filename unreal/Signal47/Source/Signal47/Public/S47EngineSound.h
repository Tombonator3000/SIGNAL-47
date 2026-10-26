#pragma once

#include "CoreMinimal.h"
#include "S47Drive.h"
#include "S47WebAudio.h"

class US47Audio;

namespace S47EngineSound
{
	/** engineSound.ts solenoidClicks: absolute AudioContext times; returns the last click's time. */
	SIGNAL47_API double SolenoidClicks(const TSharedRef<FS47AudioContext>& Ctx, const TSharedPtr<FS47AudioNode>& Dest, const TSharedPtr<FS47AudioBuffer>& Noise, const TArray<double>& At, double K = 1);
}

// drive/engineSound.ts: the truck's V8, heard from the cab. Two recordings (a carburetted V8 at
// idle, and an old truck at a steady cruise from inside), each looped, pitched up and down with the
// rpm, crossing over between 900 and 1500 rpm, under a low-pass that opens with load and a fixed
// "cab" low-pass. Getting in, the engine starts with its own recording; arriving, it shuts off.
//
// The recordings (engineIdle, engineDrive, engineStart, engineOff) play through US47Audio by
// name; their gains, rates and the filter follow the web version's Web Audio automation exactly
// (FS47EngineParam below: setValueAtTime, linear and exponential ramps, setTargetAtTime,
// cancelScheduledValues), evaluated each frame. The parts made of Web Audio nodes (the band-passed
// rumble, tyres and wind, the misfire's pop, the starter, the cattle guard, the knock) go to
// US47Audio's Engine* calls (silent stand-ins until the procedural sound is in). The solenoid
// builds engineSound.ts's own node graph through the truck's persistent out level.
// The old oscillator engine the web version plays until the recordings are decoded is not needed:
// the recordings are imported with the game.

/** A Web Audio AudioParam: its value over time from the events scheduled on it (the spec's rules). */
struct SIGNAL47_API FS47EngineParam
{
	explicit FS47EngineParam(double InValue = 0) : BaseV(InValue) {}
	double Value(double T) const;                         // param.value at time T
	void SetValueAtTime(double V, double T) { Add({ EKind::Set, T, V, 0 }); }
	void LinearRampToValueAtTime(double V, double T) { Add({ EKind::Linear, T, V, 0 }); }
	void ExponentialRampToValueAtTime(double V, double T) { Add({ EKind::Exp, T, V, 0 }); }
	void SetTargetAtTime(double V, double T, double Tau) { Add({ EKind::Target, T, V, Tau }); }
	void CancelScheduledValues(double T);
	/** Forget what lies wholly before T (the value from then on is unchanged). */
	void Prune(double T);

private:
	enum class EKind : uint8 { Set, Linear, Exp, Target };
	struct FEvent { EKind Kind; double Time, V, Tau; };
	TArray<FEvent> Events;
	double BaseV = 0, BaseT = 0;
	void Add(const FEvent& E);
	static bool IsRamp(const FEvent& E) { return E.Kind == EKind::Linear || E.Kind == EKind::Exp; }
};

class SIGNAL47_API FS47EngineSound
{
public:
	explicit FS47EngineSound(US47Audio* InAudio);
	~FS47EngineSound();

	/** Whether the recordings are playing (the tests). */
	bool Recorded() const { return bEng; }
	/** Running or stalled; false when Start() would turn the key (the key's own sound). */
	bool On() const { return bEng; }
	bool LoopsReady() const { return bLoops; }
	bool Stopping() const { return StopAt >= 0; }

	/** Engine on (idle) and the tyre layers ready. */
	void Start();
	/** Fade everything out and free the nodes. */
	void Stop();
	/** Every frame: engine speed in rpm, load 0..1 (the throttle). */
	void Set(double InRpm, double InLoad);
	/** The engine dies: two or three chugs and a wind-down, then silence. Start() runs it again. */
	void Stall();
	/** All of the truck's sound goes, at once and together, and stays gone, or comes back. */
	void Hush(bool bOn);
	/** The engine is off and quiet, found that way (no wind-down): the key starts it (Restart). */
	void Still();
	/** A misfire: the note drops out for a moment, with a dull pop in the exhaust. */
	void Cough(double K = 1);
	/** The starter turning the engine over for secs (it is dead). */
	void Crank(double Secs = 1.2);
	/** The key with the engine dead and the battery gone with it: only the solenoid's clicks. */
	void Solenoid(const TArray<double>& Secs, double K = 1);
	/** It catches after a stall: the engine note comes back with a little surge. */
	void Restart();
	/** Every frame: rolling noise for the surface under the truck and the speed in m/s. */
	void Tyres(ES47SurfaceKind Surface, double Speed);
	/** A cattle guard: both axles over a dozen steel pipes. Speed in m/s. */
	void Rattle(double Speed);
	/** A knock against a post or a fence, strength from the impact speed in m/s. */
	void Thump(double Speed);
	/** Every frame (also after Stop(), while it fades): the levels to the sounds. */
	void Tick();
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Audio* Audio = nullptr;
	// the Web Audio graph's params (out, the engine bus, the two recordings' gains and rates, the
	// engine's low-pass, the rumble's band and gain)
	FS47EngineParam Out, Bus, Gi, Gd, IdleRate, DriveRate, Lp, Band, Noise;
	TSharedPtr<FS47GainNode> SolenoidOut; // native counterpart of out for solenoidClicks, also after the engine stops
	bool bEng = false;        // eng !== null
	bool bLoops = false;      // the loops are playing (until they are stopped after a Stop's fade)
	double StopAt = -1;
	double Rpm = 650, Load = 0;
	bool bRunning = false, bStalled = false, bHushed = false, bTyres = false;
	double Level = 0.1;
	double Master() const;
	double Now() const;
	void Build();
	void EngineNodes();
	double Once(const TCHAR* Clip, double Gain);
	void TickAt(double T); // shared envelope evaluation, also for deterministic lifecycle checks
};
