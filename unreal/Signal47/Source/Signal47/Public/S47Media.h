#pragma once

#include "S47WebAudio.h"

struct FS47MediaStream;

// HTMLAudioElement + MediaElementAudioSourceNode in core/radio.ts and core/score.ts.
// Unreal difference: import-audio writes seekable PCM WAVE; only a few decoded chunks are
// held, with the next chunks read on the thread pool. The output is an ordinary graph node.
class SIGNAL47_API FS47MediaSource : public FS47AudioNode
{
public:
	explicit FS47MediaSource(const TSharedRef<FS47AudioContext>& InCtx);
	virtual ~FS47MediaSource() override;
	// Web names: music/<file>.mp3 or radio/<file>.mp3. An absolute WAVE path also works (tests).
	static TSharedPtr<FS47MediaSource> Open(const TSharedRef<FS47AudioContext>& Ctx, const FString& File);
	bool Play();
	void Pause(bool bOn = true);
	void Seek(double Seconds);
	void Stop();
	double Duration() const;
	double CurrentTime() const;
	bool Paused() const;
	FString File() const { return Name; }
	TFunction<void()> OnEnded, OnError; // both run on the game thread, from AudioContext::Tick
	// SIDEBAND-20261113: decks that play as one (the stems of a piece): the same length and rate, started together. Play, Pause,
	// Stop and Seek on the first reach all of them under one hold of the context's lock, so the audio thread sees them start in
	// the same quantum or not at all. While they play, the first one pulled in a quantum gives its position to the others, and
	// if any of them is still waiting for its samples from disk they all wait: they cannot drift apart, by construction.
	// False (and nothing is grouped) when the lengths or rates differ.
	static bool Group(const TArray<TSharedPtr<FS47MediaSource>>& Decks);
	bool Grouped() const { return Sync.IsValid(); }
	static bool SelfTest(TArray<FString>& Lines);

protected:
	virtual void Process(int64 Quantum) override;

private:
	TSharedPtr<FS47MediaStream, ESPMode::ThreadSafe> Stream;
	FString Name;
	double Pos = 0;
	bool bPlaying = false, bFailed = false;
	uint64 Serial = 0;
	void Notify(bool bError);
	TSharedPtr<struct FS47MediaSync> Sync;
	bool Leads() const;
	bool PlaySelf();
	void SeekSelf(double Seconds);
	bool ReadyAt(double At, double Step) const;
};
