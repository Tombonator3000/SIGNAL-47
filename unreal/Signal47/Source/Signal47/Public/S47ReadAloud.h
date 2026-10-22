#pragma once

#include "CoreMinimal.h"

class US47Audio;
class FS47AudioBuffer;
class FS47BufferSourceNode;

// web/src/core/readAloud.ts and core/narrationBanks.ts. Optional prerecorded narration: Kinski reading
// papers and conversations (the 'kinskiReadAloud' setting), and the same player with the Serling bank
// as the narrator (descriptions, the chapter cards, the opening card). No running voice service is
// needed: a text is read only when every word of it is in the bank's recordings, whole or pieced
// together from recorded fragments, numbers, clocks and identifiers.
//
// (The recordings are the banks' catalogs, decoded into WAVE files by Scripts/audio-raw.py under
// Content/Audio/Raw/voice/<bank>/ with the catalogs in voices.json, as the radio's banks are. Only the
// recording being read is loaded and decoded, never the whole library, as in the web version: loading
// one is a file read here, done at once when its turn comes, so the web version's load timeout and
// abort signals have nothing to wait on.)

/** readAloud.ts ReadingState: 'loading' | 'playing' | 'idle' | 'error' */
struct FS47ReadingClip { FString Id, Text; double Duration = 0; FString File; };
/** ReadingBank: the clips, each file's recording (urls: the raw WAVE file; a clip without one is not
 *  read), and the label under the Read buttons. */
struct FS47ReadingBank { TArray<FS47ReadingClip> Clips; TMap<FString, FString> Urls; FString Label; };

namespace S47Reading
{
	/** normalizeReadingText: text.replace(/\s+/g, ' ').trim() */
	SIGNAL47_API FString NormalizeReadingText(const FString& Text);
	/** documentReadingParagraphs: UI.document draws these source markers as pencil and paper
	 *  separators: no '~ ' before a pencil line, no line of dashes; the paragraphs, normalised. */
	SIGNAL47_API TArray<FString> DocumentReadingParagraphs(const FString& Text);
	/** readAloud.ts kinskiBank: 'Synthetic Kinski parody'. */
	SIGNAL47_API const FS47ReadingBank& KinskiBank();
	/** core/narrationBanks.ts serlingBank: the Serling bank and the opening's, 'Synthetic Rod Serling parody'. */
	SIGNAL47_API const FS47ReadingBank& SerlingBank();
}

class SIGNAL47_API FS47ReadAloud
{
public:
	explicit FS47ReadAloud(US47Audio* InAudio, const FS47ReadingBank& InBank = S47Reading::KinskiBank());
	~FS47ReadAloud();

	const FString& Label() const { return Bank.Label; }
	int32 RunId() const { return Generation; }

	bool Enabled() const { return bOn; }
	void SetEnabled(bool bValue);
	const FString& State() const { return CurrentState; }
	bool Playing() const { return CurrentState == TEXT("playing"); }
	bool Loading() const { return CurrentState == TEXT("loading"); }

	bool Available(const TArray<FString>& Texts) const { return Resolve(Texts).IsSet(); }
	/** The actual recorded fragments, useful when checking a composed description. */
	TOptional<TArray<FString>> Segments(const TArray<FString>& Texts) const;

	void Stop();
	/** play(texts, onState, onText): reads the texts in turn; OnText(i) as text i begins, and OnDone once
	 *  this play() has settled (read to the end, stopped, replaced, or failed), as its promise does. */
	void Play(const TArray<FString>& Texts, TFunction<void(const FString&)> OnState = {}, TFunction<void(int32)> OnText = {}, TFunction<void()> OnDone = {});

	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<US47Audio> Audio;
	FS47ReadingBank Bank;
	bool bOn = false;
	FString CurrentState = TEXT("idle");
	TFunction<void(const FString&)> Notify;
	int32 Generation = 0;
	bool bPending = false;                       // pending: the run's AbortController, not aborted
	TSharedPtr<FS47BufferSourceNode> Source;
	TFunction<void()> FinishSource;              // settles the play() in progress
	TMap<FString, TSharedPtr<FS47AudioBuffer>> Buffers;
	TArray<FString> BufferOrder;
	int64 BufferBytes = 0;
	mutable TMap<FString, TOptional<TArray<int32>>> Resolved;
	mutable TArray<FString> ResolvedOrder;

	// clips: normalised text -> clip (in the order first seen, the last clip with that text), and the index
	TMap<FString, int32> Clips;
	struct FIndexed { FString Text; int32 Clip = -1; };
	TMap<TCHAR, TArray<FIndexed>> Indexed;

	// the run in progress
	TArray<int32> Queue;
	TMap<int32, int32> TextStarts;
	TFunction<void(int32)> OnTextFn;
	struct FLife { FS47ReadAloud* Owner = nullptr; };
	TSharedPtr<FLife> Life;

	bool HasUrl(int32 Clip) const;
	const int32* ClipOf(const FString& Text) const;
	TOptional<TArray<int32>> Resolve(const TArray<FString>& Texts) const;
	TOptional<TArray<int32>> ResolveText(const FString& Text) const;
	void StateTo(const FString& S);
	bool Active(int32 Gen) const { return Gen == Generation && bPending; }
	void PlayNext(int32 Gen, int32 I);
	void Fail(int32 Gen, const FString& Why);
	TSharedPtr<FS47AudioBuffer> Load(const FS47ReadingClip& Clip);
};
