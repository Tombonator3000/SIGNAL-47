#pragma once

#include "S47RecordedDialogue.h"

// core/earlDialogue.ts: Earl's public API and bank remain independent of other characters.
class SIGNAL47_API FS47EarlDialogue : public FS47RecordedDialogue
{
public:
	explicit FS47EarlDialogue(US47Audio* Audio);
	bool Available(const TArray<FS47DialogueLine>& Lines) const { return FS47RecordedDialogue::Available({ FString(), Lines }); }
	void Play(const TArray<FS47DialogueLine>& Lines, FNotify OnState = {}) { FS47RecordedDialogue::Play({ FString(), Lines }, MoveTemp(OnState)); }
	static bool SelfTest(TArray<FString>& Lines);
};
