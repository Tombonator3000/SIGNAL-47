// core/npcDialogue.ts
#include "S47NpcDialogue.h"

const TMap<FString, FString>& FS47NpcDialogue::CharacterSpeakers()
{
	static const TMap<FString, FString> Speakers = { { TEXT("stanton"), TEXT("THE ATTENDANT") }, { TEXT("stack"), TEXT("THE DEPUTY") }, { TEXT("lynch"), TEXT("THE SOUVENIR MAN") }, { TEXT("bacall"), TEXT("N. VEGA") } };
	return Speakers;
}
const TMap<FString, FString>& FS47NpcDialogue::CharacterLabels()
{
	static const TMap<FString, FString> Labels = { { TEXT("stanton"), TEXT("Harry Dean Stanton") }, { TEXT("stack"), TEXT("Robert Stack") }, { TEXT("lynch"), TEXT("David Lynch") }, { TEXT("bacall"), TEXT("Lauren Bacall") }, { TEXT("price"), TEXT("Vincent Price") } };
	return Labels;
}
TArray<FS47DialogueLine> FS47NpcDialogue::CharacterLines(const FString& Voice, const TArray<FS47DialogueLine>& Lines)
{
	const FString Speaker = Voice == TEXT("price") ? TEXT("THE LINE") : CharacterSpeakers().FindRef(Voice);
	TArray<FS47DialogueLine> Out;
	for (const auto& Line : Lines) if ((!Speaker.IsEmpty() && Line.Who == Speaker) || (Voice == TEXT("bacall") && Line.Who == TEXT("THE LINE"))) Out.Add(Line);
	return Out;
}
FS47NpcDialogue::FS47NpcDialogue(US47Audio* Audio) : FS47RecordedDialogue(Audio, TEXT("ensemble"), [](const FS47DialogueInput& Input, FS47RadioVoice& Bank) -> TOptional<TArray<FS47RadioClip>>
{
	const auto Speech = CharacterLines(Input.Voice, Input.Lines);
	if (Speech.IsEmpty()) return {};
	TArray<FS47RadioClip> Clips;
	for (const auto& Line : Speech)
	{
		const auto Found = Bank.Resolve(Line.Text, 0, Line.Who == TEXT("THE LINE") ? TEXT("price") : Input.Voice);
		if (!Found) return {};
		Clips.Append(*Found);
	}
	return Clips;
}, [](const FS47RadioClip& Clip) { return Clip.Voice == TEXT("price"); }) {}

bool FS47NpcDialogue::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString(bCond ? TEXT("ok npcDialogue: ") : TEXT("FAIL npcDialogue: ")) + What); bOk &= bCond; };
	const TArray<FS47DialogueLine> Speech = { {TEXT("YOU"), TEXT("You.")}, {TEXT("THE ATTENDANT"), TEXT("Cold one tonight.")}, {TEXT("N. VEGA"), TEXT("Nora.")}, {TEXT("THE LINE"), TEXT("The line.")}, {TEXT(""), TEXT("An aside.")} };
	const auto A = CharacterLines(TEXT("stanton"), Speech), B = CharacterLines(TEXT("bacall"), Speech), P = CharacterLines(TEXT("price"), Speech);
	Check(A.Num() == 1 && A[0].Who == TEXT("THE ATTENDANT"), TEXT("Stanton speaks only the authored attendant lines"));
	Check(B.Num() == 2 && B[0].Who == TEXT("N. VEGA") && B[1].Who == TEXT("THE LINE") && P.Num() == 1, TEXT("Bacall includes the line, whose authored voice is Price"));
	Check(CharacterLines(TEXT("other"), Speech).IsEmpty() && CharacterSpeakers().Num() == 4 && CharacterLabels().FindRef(TEXT("stack")) == TEXT("Robert Stack"), TEXT("roles are explicit, with the original names"));
	return bOk;
}
