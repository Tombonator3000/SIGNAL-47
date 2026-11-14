#pragma once

#include "S47WebAudio.h"

struct FS47MediaStream;

// HTMLAudioElement + MediaElementAudioSourceNode in core/radio.ts and core/score.ts.
// Unreal difference: import-audio writes seekable PCM WAVE; only a few decoded chunks are
// held, with the next chunks read on the thread pool. The output is an ordinary graph node.
// SIDEBAND-STEMS-ADPCM: IMA ADPCM WAVE (0x11) plays too, behind the same interface; its blocks are decoded where a chunk is read.
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
	// The shared position is in the files' own frames, so every deck of a group has one rate and one length in frames (any rate:
	// it need not be the context's or another piece's); the decks may differ in channels (a one-channel stem beside two-channel
	// ones). False (and nothing is grouped) when the lengths or rates differ.
	// SIDEBAND-STEMS-ADPCM: the decks may also differ in form (16-bit PCM beside IMA ADPCM): every deck finds its own chunk for the
	// shared frame and an ADPCM sample depends on its block alone, so a group stays on one sample whatever its decks are made of
	// (the stems path still opens one form for all of a piece: S47Stems::Folder).
	static bool Group(const TArray<TSharedPtr<FS47MediaSource>>& Decks);
	bool Grouped() const { return Sync.IsValid(); }
	bool Adpcm() const; // the file is IMA ADPCM (S47Adpcm.h), not 16-bit PCM
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
