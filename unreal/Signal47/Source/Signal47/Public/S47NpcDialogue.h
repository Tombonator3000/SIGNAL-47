#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Audio;

// core/npcDialogue.ts with the queue of core/recordedDialogue.ts it is built on: the authored
// character lines of the night (N. Vega in room 6, and the line's own voice) played in their
// recorded voices, one foreground queue. The web version loads the clips from the ensemble voice
// bank (core/radioVoice.ts); here they are the character buffers US47Audio decodes
// (Content/Audio/Raw/character, US47Audio::CharacterBuffer, by voice and normalised text).
//
// Roles are selected by the authored interaction, never inferred from arbitrary text.

struct FS47DialogueLine { FString Who, Text; };
enum class ES47DialogueState : uint8 { Idle, Loading, Playing, Error };

class SIGNAL47_API FS47NpcDialogue : public TSharedFromThis<FS47NpcDialogue>
{
public:
	explicit FS47NpcDialogue(US47Audio* InAudio);

	// CHARACTER_SPEAKERS, CHARACTER_LABELS ('stanton' | 'stack' | 'lynch' | 'bacall', and 'price')
	static FString Speaker(const FString& Voice);
	static FString Label(const FString& Voice);
	/** characterLines(voice, lines): the lines this voice speaks (bacall also has THE LINE's). */
	static TArray<FS47DialogueLine> CharacterLines(const FString& Voice, const TArray<FS47DialogueLine>& Lines);

	/** enabled: main.ts sets npcDialogue.enabled and audio.characterVoices together (the setting
	 *  "Character voices"), so it is read from the sound's flag. */
	bool Enabled() const;
	ES47DialogueState State() const { return CurrentState; }
	bool Playing() const { return CurrentState == ES47DialogueState::Playing; }
	bool Available(const FString& Voice, const TArray<FS47DialogueLine>& Lines) const;
	void Play(const FString& Voice, const TArray<FS47DialogueLine>& Lines, TFunction<void(ES47DialogueState)> OnState = nullptr);
	void Stop();

	/** ui.onCharacterStart (main.ts: the other voices and the calls stop, and the CB) */
	TFunction<void()> OnCharacterStart;
	/** ui.speakNora: only Chapter4 calls this, for its authored N. VEGA quote replies and toasts. */
	void SpeakNora(const FString& Text);
	/** The quote in `N. VEGA: "..."`, or false. /^N\. VEGA: "([\s\S]*)"$/ */
	static bool NoraQuote(const FString& Text, FString& Out);

private:
	TWeakObjectPtr<US47Audio> Audio;
	ES47DialogueState CurrentState = ES47DialogueState::Idle;
	uint32 Generation = 0;
	TSharedPtr<FS47BufferSourceNode> Source;
	TArray<TSharedPtr<FS47AudioNode>> ExtraNodes;
	TFunction<void(ES47DialogueState)> Notify;
	struct FClip { FString Voice, Text; TSharedPtr<FS47AudioBuffer> Buffer; };
	TArray<FClip> Resolve(const FString& Voice, const TArray<FS47DialogueLine>& Lines) const;
	void StateTo(ES47DialogueState S) { CurrentState = S; if (Notify) Notify(S); }
	void PlayClip(const TArray<FClip>& Clips, int32 I, uint32 Gen);
	void DisposeNodes();
};
