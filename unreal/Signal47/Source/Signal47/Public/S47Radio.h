#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"
#include "S47RadioVoice.h"
#include <limits>

class US47Audio;
class FS47Radio;
class FS47MediaSource;

// core/radio.ts: Line, Seg, Slotted. Shared segment identity is queue.indexOf/includes in TS.
struct FS47RadioLine
{
	FString Text, Who = TEXT("host"), By;
	TFunction<FString(double)> TextAt;
	FS47RadioLine() = default;
	FS47RadioLine(const FString& InText, const FString& InWho = TEXT("host"), const FString& InBy = FString()) : Text(InText), Who(InWho), By(InBy) {}
	FS47RadioLine(TFunction<FString(double)> InText, const FString& InWho = TEXT("host"), const FString& InBy = FString()) : Who(InWho), By(InBy), TextAt(MoveTemp(InText)) {}
	FString At(double Clock) const { return TextAt ? TextAt(Clock) : Text; }
};
struct FS47RadioSeg
{
	FString Kind = TEXT("dead"), Tune;
	TArray<FS47RadioLine> Lines;
	double Len = 0;
	TOptional<double> Duration;
	bool bSlotted = false;
};
struct FS47ArchiveProgress { int32 Id = 0; FString State; double Elapsed = 0, Duration = 0; };
struct FS47RadioSyllable { double At, D, F; };
struct FS47BoyCallTime { FString Text; bool bBoy; double Secs, Gap; };

namespace S47Radio
{
	inline constexpr double SIGN_OFF = 4 * 3600, SIGN_ON = 5 * 3600, ANTHEM_SECS = 74.8, ARCHIVE_SECS = 47, BOY_AT = 2 * 3600 + 26 * 60;
	extern SIGNAL47_API const TArray<FString> DIAL;
	SIGNAL47_API int32 Freq(const FString& Id);
	SIGNAL47_API FString SayTime(double Clock);
	SIGNAL47_API double SecsOf(const FString& Text);
	SIGNAL47_API TArray<FS47RadioSyllable> Syllables(const FString& Text, double Secs);
	SIGNAL47_API double DialX(double Freq);
	SIGNAL47_API TArray<FS47BoyCallTime> BoyCallTimes(FS47Radio& Radio);
}

class SIGNAL47_API FS47Station : public TSharedFromThis<FS47Station>
{
public:
	FS47Station(const FString& InId, FS47Radio& InRadio);
	~FS47Station();
	const FString Id;
	TArray<TSharedPtr<FS47RadioSeg>> Queue;
	double T = 0;
	TSharedPtr<FS47GainNode> Bus;
	bool bTrackFailed = false, bAnthemFailed = false;
	int32 Listeners = 0;
	TFunction<void(const FString&, const FString&, TOptional<double>)> OnLine;
	const FS47RadioSeg& Seg() const { return *Queue[0]; }
	bool OnAir() const { return Seg().Kind != TEXT("dead"); }
	double Strength(double Clock, double Time) const;
	void Reset(double Clock);
	bool Free() const;
	TSharedPtr<FS47RadioSeg> Slot(const TArray<FS47RadioLine>& Lines, double Clock);
	FString Where(const TSharedPtr<FS47RadioSeg>& X) const;
	bool Unslot(const TSharedPtr<FS47RadioSeg>& X);
	double CutIn(const TArray<FS47RadioLine>& Lines, double Clock);
	void Revoice(const FString& Kind = TEXT("host"));
	void Update(double Dt, double Clock);
	TSharedPtr<FS47GainNode> EnsureBus();
	void Listen(bool bOn);
	TOptional<FS47ArchiveProgress> ArchiveStatus() const { return Archive; }
	bool Awake() const { return N.IsValid(); }
	struct FVoiceStatus
	{
		bool bLoading = false, bPlaying = false;
		FString Text;
		TOptional<double> Duration;
		TArray<FS47RadioClip> Clips;
		int32 BufferCount = 0;
	};
	FVoiceStatus HostStatus() const { return VoiceStatus(TEXT("host")); }
	FVoiceStatus CallerStatus() const { return VoiceStatus(TEXT("caller")); }
	TSharedPtr<FS47MediaSource> TrackEl() const;
	TSharedPtr<FS47MediaSource> AnthemEl() const;

private:
	friend class FS47Radio;
	FS47Radio& Radio;
	struct FCfg
	{
		TArray<TSharedPtr<FS47RadioSeg>> Loop, FixedSeq;
		TOptional<double> FixedAt;
		FString Then = TEXT("loop");
		bool bOffBefore = false;
	} Cfg;
	double Len = std::numeric_limits<double>::infinity(), Sc = 0, Calm = 10;
	int32 LoopAt = 0, Said = -1, SegNo = 0, TrackSkip = -1;
	FString After = TEXT("loop");
	TOptional<double> Prev, AudioAt;
	bool bFading = false, bHostReady = false, bHostPending = false;
	int32 HostGeneration = 0, ArchiveId = 0;
	struct FSpokenLine
	{
		FS47RadioLine Line;
		FString Text, Bank;
		double Secs = 0, At = 0;
		TOptional<TArray<FS47RadioClip>> Clips;
		TOptional<TArray<TSharedPtr<FS47AudioBuffer>>> Buffers;
	};
	TArray<FSpokenLine> Spoken;
	struct FInterrupted
	{
		TArray<TSharedPtr<FS47RadioSeg>> Queue;
		double T, Len, Sc;
		int32 Said, SegNo, TrackSkip;
		bool bFading, bHostReady, bHostVoice, bCallerVoice;
		TArray<FSpokenLine> Spoken;
	};
	TOptional<FInterrupted> Interrupted;
	TOptional<FS47ArchiveProgress> Archive;
	TArray<TSharedPtr<FS47BufferSourceNode>> HostSources;
	struct FNodes
	{
		TSharedPtr<FS47GainNode> Voice, Car, Mus;
		TSharedPtr<FS47OscillatorNode> Horn, Vib, Osc;
		TSharedPtr<FS47BiquadFilterNode> Mute;
		TSharedPtr<FS47BufferSourceNode> Music;
		TSharedPtr<FS47MediaSource> Anthem, Track;
		TSharedPtr<FS47GainNode> AnthemGain, TrackGain;
	};
	TSharedPtr<FNodes> N;
	TArray<FSpokenLine> LinesFor(const FS47RadioSeg& S, double Clock);
	void Layout();
	double LengthOf(const FS47RadioSeg& S, double Clock);
	void Plan(double Clock);
	double MapPosition(const TArray<double>& Before, const TArray<double>& New) const;
	void Start(const TArray<TSharedPtr<FS47RadioSeg>>& Segs, double Clock, double Skip = 0);
	TArray<TSharedPtr<FS47RadioSeg>> Refill(double Clock);
	void Fixed(double Clock);
	void Cross(double Since, double Clock);
	bool Forward(double Dt, double Clock, bool bQuiet = false);
	void Wake();
	void Sleep();
	void Sync();
	void StopHostSources();
	void CancelHost();
	void CancelArchive();
	void AnnounceRecordedLine();
	void PrepareHost();
	FVoiceStatus VoiceStatus(const FString& Bank) const;
	void PlayAnthem(FNodes& Nodes);
	void PlayTrack(FNodes& Nodes, const FString& Tune, double Length);
	void StopTrack(FNodes& Nodes);
	void StopAnthem(FNodes& Nodes);
};

struct FS47ReceiverOpts { double Vol; double Ref = 1.2; };
class SIGNAL47_API FS47Receiver
{
public:
	FS47Receiver(FS47Radio& InRadio, const FS47ReceiverOpts& InO);
	~FS47Receiver();
	bool bOn = false;
	FString Station = TEXT("caliche");
	TOptional<FVector> Pos;
	double Power = 1, Muffle = 0, Interference = 0;
	int32 Cracks = 0;
	void Set(bool bNewOn, const FString& NewStation = FString());
	FString Next();
	bool Live() const { return bOn && Pos.IsSet(); }
	bool Carrying() const { return Carry > 0 && Pos.IsSet(); }
	void Carrier(double Secs) { Carry = Secs > 0 ? FMath::Max(Carry, Secs) : 0; }
	void Update(double Dt);
	bool Drift(double Secs = 3.5, double Depth = 1);
	void Reset();
private:
	FS47Radio& Radio;
	FS47ReceiverOpts O;
	double Sweep = 0, Carry = 0;
	bool bDial = false;
	struct FFuzz { double Next = 12 + FMath::FRand() * 20, Left = 0, Len = 1, Depth = 0; } Fuzz;
	FString Listening;
	struct FNodes
	{
		TSharedPtr<FS47GainNode> Sig, Stat, Mix, Car, Out, Bus;
		TSharedPtr<FS47BiquadFilterNode> Cab;
		TSharedPtr<FS47PannerNode> Pan;
		TSharedPtr<FS47BufferSourceNode> Static;
		TSharedPtr<FS47OscillatorNode> Osc;
	};
	TSharedPtr<FNodes> N;
	TSharedPtr<FNodes> Build();
	void Crack(double Len, double K);
};

class SIGNAL47_API FS47Radio
{
public:
	explicit FS47Radio(US47Audio* InAudio);
	~FS47Radio();
	US47Audio* Audio;
	TMap<FString, TSharedPtr<FS47Station>> Stations;
	TArray<TSharedPtr<FS47Receiver>> Receivers;
	double Clock = 0, Time = 0;
	TFunction<void(const FString&, const FString&, const FString&, TOptional<double>)> OnLine;
	bool bBoyCalled = false;
	FS47RadioVoice HostBank, CallerBank;
	bool HostVoice() const { return bRecordedHost; }
	bool CallerVoice() const { return bRecordedCallers; }
	void SetHostVoice(bool bOn);
	void SetCallerVoice(bool bOn);
	TSharedPtr<FS47Receiver> Receiver(const FS47ReceiverOpts& O);
	void Reset(double InClock);
	void Update(double Dt, double InClock);
	struct FMediaState { double Time = 0; bool bPaused = false, bFailed = false; };
	struct FStationState
	{
		FString Seg;
		double T = 0, Strength = 0;
		bool bFree = false, bSlotted = false, bAwake = false, bAnthemFailed = false;
		int32 Listeners = 0;
		TOptional<FMediaState> Anthem;
		FS47Station::FVoiceStatus Host, Callers;
		TOptional<FS47ArchiveProgress> Archive;
	};
	TMap<FString, FStationState> State() const;
	static bool SelfTest(TArray<FString>& Lines);
private:
	bool bRecordedHost = false, bRecordedCallers = false;
};
