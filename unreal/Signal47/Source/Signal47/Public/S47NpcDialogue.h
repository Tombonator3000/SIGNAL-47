#pragma once

#include "S47RecordedDialogue.h"

using FS47NpcInput = FS47DialogueInput;

// core/npcDialogue.ts: roles are selected by the authored interaction, never arbitrary text.
class SIGNAL47_API FS47NpcDialogue : public FS47RecordedDialogue
{
public:
	explicit FS47NpcDialogue(US47Audio* Audio);
	static const TMap<FString, FString>& CharacterSpeakers();
	static const TMap<FString, FString>& CharacterLabels();
	static TArray<FS47DialogueLine> CharacterLines(const FString& Voice, const TArray<FS47DialogueLine>& Lines);
	static bool SelfTest(TArray<FString>& Lines);
};
