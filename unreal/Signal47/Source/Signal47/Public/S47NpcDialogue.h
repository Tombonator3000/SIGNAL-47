#pragma once

#include "S47RecordedDialogue.h"

using FS47NpcInput = FS47DialogueInput;

/** The queue's state as the talk panel shows it (recordedDialogue.ts DialogueState). */
enum class ES47DialogueState : uint8 { Idle, Loading, Playing, Error };

// core/npcDialogue.ts: roles are selected by the authored interaction, never arbitrary text. One
// instance for the night (main.ts npcDialogue): Caliche's people and N. Vega in room 6 share it
// (ui.npcDialogue), and ui.speakNora speaks through it.
class SIGNAL47_API FS47NpcDialogue : public FS47RecordedDialogue
{
public:
	explicit FS47NpcDialogue(US47Audio* Audio);
	static const TMap<FString, FString>& CharacterSpeakers();
	static const TMap<FString, FString>& CharacterLabels();
	/** CHARACTER_SPEAKERS[voice], CHARACTER_LABELS[voice] ('' for none). */
	static FString Speaker(const FString& Voice) { return CharacterSpeakers().FindRef(Voice); }
	static FString Label(const FString& Voice) { return CharacterLabels().FindRef(Voice); }
	static TArray<FS47DialogueLine> CharacterLines(const FString& Voice, const TArray<FS47DialogueLine>& Lines);

	using FS47RecordedDialogue::Available;
	using FS47RecordedDialogue::Play;
	bool Available(const FString& Voice, const TArray<FS47DialogueLine>& Lines) const { return Available(FS47DialogueInput{ Voice, Lines }); }
	void Play(const FString& Voice, const TArray<FS47DialogueLine>& Lines, TFunction<void(ES47DialogueState)> OnState = nullptr);
	static ES47DialogueState StateOf(const FString& S);

	/** ui.onCharacterStart (main.ts: the other voices and the calls stop, and the CB). */
	TFunction<void()> OnCharacterStart;
	/** ui.speakNora: only Chapter4 calls this, for its authored N. VEGA quote replies and toasts. */
	void SpeakNora(const FString& Text);
	/** The quote in `N. VEGA: "..."`, or false. /^N\. VEGA: "([\s\S]*)"$/ */
	static bool NoraQuote(const FString& Text, FString& Out);

	static bool SelfTest(TArray<FString>& Lines);
};
