#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"
#include <atomic>

// A small Web Audio for the port: the parts of the API (https://webaudio.github.io/web-audio-api/)
// that web/src uses, so the graphs core/Audio.ts and the modules after it build port line by
// line (ctx.createGain() is Ctx->CreateGain(), g.gain.setTargetAtTime(0, t, 0.3) is
// G->Gain.SetTargetAtTime(0, T, 0.3)). An AudioContext pulls its graph 128 frames at a time
// from the destination, as the browser does: AudioParams with the same automation (values,
// linear and exponential ramps, setTargetAtTime, value curves, cancelling, signals connected
// into them), the biquad filters with the specification's formulas, the equal-power panner with
// the three distance models, oscillators, buffer sources, delays (also inside feedback loops),
// wave shapers, a stereo panner and the dynamics compressor. US47Audio renders the context
// through one USynthComponent (S47Synth.h).
//
// Threads: graphs are built on the game thread and rendered on the audio thread. Every call
// takes the context's lock; hold FS47AudioContext::FBatch over a whole graph (one JavaScript
// task) so the audio thread never renders half of one. Callbacks (onended, setTimeout) run on
// the game thread from FS47AudioContext::Tick.

class FS47AudioContext;
class FS47AudioNode;

constexpr int32 S47Quantum = 128; // the render quantum, frames

// One render quantum of a node's output: one or two channels.
struct FS47AudioBlock
{
	int32 Channels = 1;
	float D[2][S47Quantum];
	void Zero(int32 InChannels);
};

// AudioBuffer
class SIGNAL47_API FS47AudioBuffer
{
public:
	FS47AudioBuffer(int32 InChannels, int32 InLength, float InSampleRate);
	int32 NumberOfChannels() const { return Data.Num(); }
	int32 Length() const { return Frames; }
	float SampleRate() const { return Rate; }
	double Duration() const { return Frames / double(Rate); }
	float* GetChannelData(int32 C) { return Data[C].GetData(); }
	const float* GetChannelData(int32 C) const { return Data[C].GetData(); }

private:
	TArray<TArray<float>> Data;
	int32 Frames = 0;
	float Rate = 48000.f;
};

// AudioParam: an intrinsic value with a timeline of automation events, plus the signals
// connected into it. a-rate parameters are computed per frame, k-rate ones per quantum.
class SIGNAL47_API FS47AudioParam
{
public:
	FS47AudioParam(FS47AudioNode& InOwner, double InDefault, double InMin, double InMax, bool bInARate = true);

	double Value() const;     // .value: the intrinsic value as last rendered (as the browser's getter)
	void SetValue(double V);  // .value = v (setValueAtTime(v, currentTime) once automation exists)
	FS47AudioParam& SetValueAtTime(double V, double T);
	FS47AudioParam& LinearRampToValueAtTime(double V, double T);
	FS47AudioParam& ExponentialRampToValueAtTime(double V, double T);
	FS47AudioParam& SetTargetAtTime(double Target, double T, double TimeConstant);
	FS47AudioParam& SetValueCurveAtTime(const TArray<float>& Values, double T, double Duration);
	FS47AudioParam& CancelScheduledValues(double T);
	FS47AudioParam& CancelAndHoldAtTime(double T);

	const double DefaultValue, MinValue, MaxValue;

	// rendering (audio thread, under the lock): the values for this quantum, bConst if all equal
	const float* Render(int64 Quantum, bool& bConst);
	double ValueAt(double T) const; // the intrinsic value at a time (tests)

private:
	friend class FS47AudioNode;
	enum class EEvent : uint8 { SetValue, Linear, Exponential, Target, Curve };
	struct FEvent
	{
		EEvent Type = EEvent::SetValue;
		double Time = 0, V = 0, Tau = 0, Duration = 0;
		TArray<float> Curve;
		double InsertT = 0, InsertV = 0;          // a ramp with no event before it starts here
		mutable bool bStart = false;              // lazily: a target's value before it, a ramp's start after a target
		mutable double StartT = 0, StartV = 0;
	};
	FS47AudioNode& Owner;
	const bool bARate;
	double Intrinsic;
	TArray<FEvent> Events;
	TArray<TWeakPtr<FS47AudioNode>> Inputs;
	int64 RenderedQ = -1;
	bool bLastConst = true;
	double LastIntrinsic = 0;            // the timeline's value at the end of the last quantum rendered
	double OverrideV = 0;                // set or cancelled since then: what the getter says until the next quantum
	int64 OverrideQ = -1;
	float Values[S47Quantum];

	void Insert(FEvent E);
	int32 CountAtOrBefore(double T) const;
	double Eval(int32 K, double T) const;
	double TargetStart(int32 I) const;
	void RampStart(int32 K, double T, double& T0, double& V0) const;
	double EndValue(int32 I) const;
	void Prune(double T);
	double Clamp(double V) const { return FMath::Clamp(V, MinValue, MaxValue); }
};

// AudioNode: connect, disconnect, and the pull of one quantum. A node holds the nodes it feeds
// (so a playing source keeps its chain alive) and only weak links to its inputs.
class SIGNAL47_API FS47AudioNode : public TSharedFromThis<FS47AudioNode>
{
public:
	explicit FS47AudioNode(const TSharedRef<FS47AudioContext>& InCtx);
	virtual ~FS47AudioNode() = default;

	void Connect(const TSharedPtr<FS47AudioNode>& To);
	void Connect(FS47AudioParam& To);
	void Disconnect(); // every outgoing connection
	void Disconnect(const TSharedPtr<FS47AudioNode>& To); // disconnect(destination): just this receiver

	const TSharedRef<FS47AudioContext> Ctx;

	// rendering (audio thread, under the lock)
	virtual const FS47AudioBlock& Pull(int64 Quantum);

protected:
	virtual void Process(int64 Quantum) = 0;
	// sums the inputs into In (channelCountMode 'max' up to MaxChannels, or exactly Explicit)
	void PullInputs(int64 Quantum, FS47AudioBlock& In, int32 MaxChannels = 2, int32 Explicit = 0);
	FS47AudioBlock Out;
	int64 Done = -1;
	TArray<TWeakPtr<FS47AudioNode>> Inputs;

private:
	friend class FS47AudioParam;
	friend class FS47AudioContext;
	struct FParamLink { TSharedPtr<FS47AudioNode> Node; FS47AudioParam* Param = nullptr; };
	TArray<TSharedPtr<FS47AudioNode>> OutNodes;
	TArray<FParamLink> OutParams;
};

// GainNode
class SIGNAL47_API FS47GainNode : public FS47AudioNode
{
public:
	explicit FS47GainNode(const TSharedRef<FS47AudioContext>& InCtx);
	FS47AudioParam Gain;
protected:
	virtual void Process(int64 Quantum) override;
private:
	FS47AudioBlock In;
};

// AudioScheduledSourceNode: start, stop, onended
class SIGNAL47_API FS47ScheduledSourceNode : public FS47AudioNode
{
public:
	explicit FS47ScheduledSourceNode(const TSharedRef<FS47AudioContext>& InCtx) : FS47AudioNode(InCtx) {}
	void Start(double When = 0.0);
	void Stop(double When = 0.0);  // before start: nothing (the browser throws; the game catches it)
	TFunction<void()> OnEnded;     // onended, on the game thread
	bool Started() const { return bStarted; }
	bool Ended() const { return bEnded; }

protected:
	friend class FS47AudioContext;
	int64 StartFrame = 0, StopFrame = MAX_int64;
	bool bStarted = false, bEnded = false;
	void Finish(); // ended: the context drops it and calls OnEnded
};

enum class ES47OscType : uint8 { Sine, Square, Sawtooth, Triangle };

// OscillatorNode (the four built-in shapes; edges band-limited with PolyBLEP where the browser
// uses band-limited wavetables)
class SIGNAL47_API FS47OscillatorNode : public FS47ScheduledSourceNode
{
public:
	explicit FS47OscillatorNode(const TSharedRef<FS47AudioContext>& InCtx);
	ES47OscType Type = ES47OscType::Sine;
	void SetType(const FString& Name); // 'sine' | 'square' | 'sawtooth' | 'triangle'
	FS47AudioParam Frequency, Detune;
	static double Shape(ES47OscType InType, double P, double Dt); // one sample at a phase 0..1
protected:
	virtual void Process(int64 Quantum) override;
private:
	double Phase = 0.0;
};

// AudioBufferSourceNode
class SIGNAL47_API FS47BufferSourceNode : public FS47ScheduledSourceNode
{
public:
	explicit FS47BufferSourceNode(const TSharedRef<FS47AudioContext>& InCtx);
	TSharedPtr<FS47AudioBuffer> Buffer;
	bool bLoop = false;
	double LoopStart = 0.0, LoopEnd = 0.0;
	FS47AudioParam PlaybackRate, Detune; // k-rate
	void Start(double When = 0.0, double Offset = 0.0, double Duration = -1.0); // Duration < 0: to the end
protected:
	virtual void Process(int64 Quantum) override;
private:
	double Pos = 0.0, Played = 0.0, Limit = -1.0;
};

enum class ES47BiquadType : uint8 { Lowpass, Highpass, Bandpass, Lowshelf, Highshelf, Peaking, Notch, Allpass };

// BiquadFilterNode, the specification's coefficients (Q in dB for the low- and high-pass)
class SIGNAL47_API FS47BiquadFilterNode : public FS47AudioNode
{
public:
	explicit FS47BiquadFilterNode(const TSharedRef<FS47AudioContext>& InCtx);
	ES47BiquadType Type = ES47BiquadType::Lowpass;
	void SetType(const FString& Name);
	FS47AudioParam Frequency, Detune, Q, Gain;
	// b0, b1, b2, a1, a2 divided by a0
	static void Coefficients(ES47BiquadType InType, double SampleRate, double Freq, double InDetune, double InQ, double GainDb, double Out[5]);
protected:
	virtual void Process(int64 Quantum) override;
private:
	FS47AudioBlock In;
	double X1[2] = { 0, 0 }, X2[2] = { 0, 0 }, Y1[2] = { 0, 0 }, Y2[2] = { 0, 0 };
	int32 LastChannels = 1;
};

enum class ES47DistanceModel : uint8 { Linear, Inverse, Exponential };

// PannerNode ('equalpower'; 'HRTF' is not made and pans as equal power). Its position and the
// listener's are taken once per quantum, and the gains glide across the quantum.
class SIGNAL47_API FS47PannerNode : public FS47AudioNode
{
public:
	explicit FS47PannerNode(const TSharedRef<FS47AudioContext>& InCtx);
	FString PanningModel = TEXT("equalpower");
	ES47DistanceModel DistanceModel = ES47DistanceModel::Inverse;
	double RefDistance = 1.0, MaxDistance = 10000.0, RolloffFactor = 1.0;
	FS47AudioParam PositionX, PositionY, PositionZ;
	void SetPosition(double X, double Y, double Z);
	void SetDistanceModel(const FString& Name);
	static double DistanceGain(ES47DistanceModel Model, double Distance, double Ref, double Max, double Rolloff);
	// azimuth in degrees of a source seen from a listener (the specification's algorithm)
	static double Azimuth(const FVector& Source, const FVector& Listener, const FVector& Forward, const FVector& Up);
	static void EqualPower(double Azimuth, int32 InChannels, double& GainL, double& GainR);
protected:
	virtual void Process(int64 Quantum) override;
private:
	FS47AudioBlock In;
	double LastL = -1.0, LastR = -1.0;
};

// StereoPannerNode
class SIGNAL47_API FS47StereoPannerNode : public FS47AudioNode
{
public:
	explicit FS47StereoPannerNode(const TSharedRef<FS47AudioContext>& InCtx);
	FS47AudioParam Pan;
protected:
	virtual void Process(int64 Quantum) override;
private:
	FS47AudioBlock In;
};

// DelayNode. In a feedback loop its delay is at least one quantum, as the specification says, and
// what comes back round the loop is the last quantum's output, as in Chromium (so a loop's period
// is the delay plus one quantum, as the web version sounds in the browser).
class SIGNAL47_API FS47DelayNode : public FS47AudioNode
{
public:
	FS47DelayNode(const TSharedRef<FS47AudioContext>& InCtx, double MaxDelayTime = 1.0);
	FS47AudioParam DelayTime;
	virtual const FS47AudioBlock& Pull(int64 Quantum) override;
protected:
	virtual void Process(int64 Quantum) override {}
private:
	FS47AudioBlock In;
	TArray<float> Ring[2];
	int32 Write = 0;
	bool bBusy = false, bInCycle = false;
	int32 RingChannels = 1;
	void Read(int64 Quantum);
	void WriteIn(int64 Quantum);
};

// WaveShaperNode (oversample is taken as 'none': Chromium's 2x gives the same level, only less
// aliasing and a few frames of latency)
class SIGNAL47_API FS47WaveShaperNode : public FS47AudioNode
{
public:
	explicit FS47WaveShaperNode(const TSharedRef<FS47AudioContext>& InCtx) : FS47AudioNode(InCtx) {}
	TArray<float> Curve;
	FString Oversample = TEXT("none");
	static float Shape(const TArray<float>& InCurve, float X);
protected:
	virtual void Process(int64 Quantum) override;
private:
	FS47AudioBlock In;
};

// DynamicsCompressorNode: Chromium's DynamicsCompressorKernel (the static curve with its knee, the
// make-up gain, the detector, the attack and the adaptive release, the sine warp, the 6 ms pre-delay)
class SIGNAL47_API FS47DynamicsCompressorNode : public FS47AudioNode
{
public:
	explicit FS47DynamicsCompressorNode(const TSharedRef<FS47AudioContext>& InCtx);
	FS47AudioParam Threshold, Knee, Ratio, Attack, Release; // k-rate
	float Reduction = 0.f; // dB, read-only
	// the static curve: the output level for an input level (linear), and the make-up gain
	static double Curve(double X, double ThresholdDb, double KneeDb, double InRatio);
	static double Makeup(double ThresholdDb, double KneeDb, double InRatio);
protected:
	virtual void Process(int64 Quantum) override;
private:
	FS47AudioBlock In;
	TArray<float> Pre[2];
	int32 PreRead = 0, PreWrite = 256, LastPreDelayFrames = 256;
	float DetectorAverage = 0.f, CompressorGain = 1.f, MeteringGain = 1.f, MaxAttackDiffDb = -1.f;
};

// AudioDestinationNode: what the context renders (stereo)
class SIGNAL47_API FS47AudioDestinationNode : public FS47AudioNode
{
public:
	explicit FS47AudioDestinationNode(const TSharedRef<FS47AudioContext>& InCtx) : FS47AudioNode(InCtx) {}
protected:
	virtual void Process(int64 Quantum) override { PullInputs(Quantum, Out, 2, 2); }
};

// AudioListener (a node only so that its parameters have an owner; it is never pulled)
class SIGNAL47_API FS47AudioListener : public FS47AudioNode
{
public:
	explicit FS47AudioListener(const TSharedRef<FS47AudioContext>& InCtx);
	FS47AudioParam PositionX, PositionY, PositionZ, ForwardX, ForwardY, ForwardZ, UpX, UpY, UpZ;
protected:
	virtual void Process(int64 Quantum) override {}
};

// AudioContext
class SIGNAL47_API FS47AudioContext : public TSharedFromThis<FS47AudioContext>
{
public:
	static TSharedRef<FS47AudioContext> Make(float InSampleRate = 48000.f);
	explicit FS47AudioContext(float InSampleRate) : SampleRate(InSampleRate) {}

	const float SampleRate;
	double CurrentTime() const { return double(Frame.load()) / SampleRate; }
	FString State() const { return bClosed ? TEXT("closed") : bSuspended ? TEXT("suspended") : TEXT("running"); }
	TSharedPtr<FS47AudioDestinationNode> Destination;
	TSharedPtr<FS47AudioListener> Listener;

	TSharedRef<FS47GainNode> CreateGain();
	TSharedRef<FS47OscillatorNode> CreateOscillator();
	TSharedRef<FS47BufferSourceNode> CreateBufferSource();
	TSharedRef<FS47BiquadFilterNode> CreateBiquadFilter();
	TSharedRef<FS47PannerNode> CreatePanner();
	TSharedRef<FS47StereoPannerNode> CreateStereoPanner();
	TSharedRef<FS47DelayNode> CreateDelay(double MaxDelayTime = 1.0);
	TSharedRef<FS47WaveShaperNode> CreateWaveShaper();
	TSharedRef<FS47DynamicsCompressorNode> CreateDynamicsCompressor();
	TSharedRef<FS47AudioBuffer> CreateBuffer(int32 Channels, int32 Length, float Rate) const;

	void Suspend(bool bOn) { FScopeLock L(&Lock); bSuspended = bOn; } // suspend() / resume()
	void Close();                                                      // close(): everything stops

	// window.setTimeout, and the onended events, run on the game thread by Tick (real seconds)
	void SetTimeout(double Seconds, TFunction<void()> Fn);
	void Tick(double DeltaSeconds);
	void Post(TFunction<void()> Fn); // an audio-thread notification, run on the game thread by Tick

	// the audio thread: interleaved frames (one or two channels)
	void Render(float* Interleaved, int32 Frames, int32 NumChannels = 2);

	mutable FCriticalSection Lock;
	struct FBatch
	{
		explicit FBatch(const FS47AudioContext& C) : L(&C.Lock) {}
		FScopeLock L;
	};

	// render state (under the lock)
	int64 QuantumFrame = 0; // the first frame of the quantum being rendered
	int64 Quantum = 0;

private:
	friend class FS47ScheduledSourceNode;
	std::atomic<int64> Frame { 0 }; // frames rendered
	bool bSuspended = false, bClosed = false;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Active, Ended;
	float Q[2][S47Quantum];
	int32 QPos = S47Quantum;
	struct FTimeout { double At; TFunction<void()> Fn; };
	TArray<FTimeout> Timeouts;
	TArray<TFunction<void()>> Posted;
	double Clock = 0.0;
	void RenderQuantum();
};

// Web Audio's names for types ('square', 'bandpass'), the self test, and WAVE files
namespace S47Wa
{
	SIGNAL47_API ES47OscType OscType(const FString& Name);
	SIGNAL47_API ES47BiquadType BiquadType(const FString& Name);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
	// a RIFF WAVE file (PCM 8/16/24/32 bit or float) as a buffer, or null
	SIGNAL47_API TSharedPtr<FS47AudioBuffer> DecodeWav(const TArray<uint8>& Bytes);
}
