#pragma once

#include "CoreMinimal.h"
#include "S47RadioVoice.h"

class US47Audio;
class FS47DialogueLevelNode;

struct FS47DialogueAudioLevels
{
	FString ClipId, Channel;
	bool bReceiverFilter = false;
	int64 SourceSamples = 0, RoutedSamples = 0;
	double PcmPeak = 0, PcmRms = 0, SourcePeak = 0, SourceRms = 0, RoutedPeak = 0, RoutedRms = 0, BusGain = 1;
};

struct FS47DialogueLine
{
	FString Who, Text;
	bool operator==(const FS47DialogueLine& Other) const { return Who == Other.Who && Text == Other.Text; }
};
// C++ uses one input shape for RecordedDialogue<Input>'s authored wrappers; Earl leaves Voice empty.
struct FS47DialogueInput { FString Voice; TArray<FS47DialogueLine> Lines; };
struct FS47DialogueStatus
{
	FString State;
	bool bEnabled = false;
	TOptional<FString> Text;
	struct FClip { FString Id, Text; double Duration = 0; FString Voice; };
	TArray<FClip> Clips;
	int32 BufferCount = 0;
};

// core/recordedDialogue.ts: one foreground queue shared by authored character-dialogue wrappers.
class SIGNAL47_API FS47RecordedDialogue
{
public:
	using FResolve = TFunction<TOptional<TArray<FS47RadioClip>>(const FS47DialogueInput&, FS47RadioVoice&)>;
	using FTelephone = TFunction<bool(const FS47RadioClip&)>;
	using FNotify = TFunction<void(const FString&)>;
	// Native evening voices can share the receiver filter and still use the car radio's bus,
	// or speak at an actor's three.js position (metres, y up).
	struct FOutput { TOptional<FVector> At; bool bRadioBus = false; FString Channel; };
	using FOutputRoute = TFunction<FOutput(const FS47RadioClip&)>;
	FS47RecordedDialogue(US47Audio* InAudio, const FString& BankKind, FResolve InResolve, FTelephone InTelephone = {}, FOutputRoute InOutput = {});
	virtual ~FS47RecordedDialogue();
	TSharedPtr<FS47RadioVoice> Bank;
	bool Enabled() const { return bOn; }
	void SetEnabled(bool bValue);
	FString State() const { return CurrentState; }
	bool Playing() const { return CurrentState == TEXT("playing"); }
	bool Loading() const { return CurrentState == TEXT("loading"); }
	double StartedAt() const { return SourceStartedAt; }
	FS47DialogueStatus Status() const;
	// Exact source now audible, rather than the first entry in the complete plan.
	// False during loading, inter-clip scheduling gaps, and after stop/end/error.
	bool ActiveClip(FString& ClipId, double& Seconds) const;
	bool Available(const FS47DialogueInput& Input) const;
	void Stop();
	void Play(const FS47DialogueInput& Input, FNotify OnState = {});
	void SetLevelDiagnostics(bool bValue) { bLevelDiagnostics = bValue; }
	FS47DialogueAudioLevels AudioLevels() const;
	static bool SelfTest(TArray<FString>& Lines);
private:
	bool bOn = false;
	double SourceStartedAt = 0;
	int32 ActiveClipIndex = INDEX_NONE;
	FString CurrentState = TEXT("idle");
	int32 Generation = 0;
	bool bPending = false;
	TSharedPtr<FS47BufferSourceNode> Source;
	FNotify Notify;
	TArray<FS47DialogueStatus::FClip> Clips;
	TOptional<FString> Text;
	TArray<TSharedPtr<FS47AudioNode>> ExtraNodes;
	TWeakObjectPtr<US47Audio> Audio;
	FResolve ResolveInput;
	FTelephone Telephone;
	FOutputRoute OutputRoute;
	bool bLevelDiagnostics = false;
	TSharedPtr<FS47DialogueLevelNode> SourceMeter, RoutedMeter;
	TSharedPtr<FS47GainNode> OutputBus;
	FS47DialogueAudioLevels ClipLevels, LastLevels;
	TArray<FS47RadioClip> Plan;
	TMap<FString, TSharedPtr<FS47AudioBuffer>> Buffers;
	struct FLife { FS47RecordedDialogue* Owner = nullptr; };
	TSharedPtr<FLife> Life;
	void StateTo(const FString& Value);
	void DisposeNodes();
	void PlayNext(int32 Gen, int32 I);
	void Failed(int32 Gen);
	void ReportLevels(const TCHAR* Reason);
};
