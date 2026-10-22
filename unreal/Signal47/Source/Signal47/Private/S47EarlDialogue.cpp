// core/earlDialogue.ts
#include "S47EarlDialogue.h"

FS47EarlDialogue::FS47EarlDialogue(US47Audio* Audio) : FS47RecordedDialogue(Audio, TEXT("earl"), [](const FS47DialogueInput& Input, FS47RadioVoice& Bank) -> TOptional<TArray<FS47RadioClip>>
{
	TArray<FS47DialogueLine> Speech;
	for (const auto& Line : Input.Lines) if (Line.Who == TEXT("EARL")) Speech.Add(Line);
	if (Speech.IsEmpty()) return {};
	TArray<FS47RadioClip> Clips;
	for (const auto& Line : Speech)
	{
		const auto Found = Bank.Resolve(Line.Text, 0);
		if (!Found) return {};
		Clips.Append(*Found);
	}
	return Clips;
}) {}

bool FS47EarlDialogue::SelfTest(TArray<FString>& Lines)
{
	FS47EarlDialogue Earl(nullptr);
	const bool bOk = !Earl.Available({ { TEXT("YOU"), TEXT("A question.") }, { TEXT(""), TEXT("An aside.") } });
	Lines.Add(FString(bOk ? TEXT("ok earlDialogue: ") : TEXT("FAIL earlDialogue: ")) + TEXT("only Earl's lines belong to his independent bank"));
	return bOk;
}
