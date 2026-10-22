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
};
