#pragma once

#include "CoreMinimal.h"
#include "S47Docs.h"
#include "S47RecordedDialogue.h"
#include "S47UI.h"

class US47Main;
class FS47Town;
class SVerticalBox;
struct FS47TownSeen;
using FS47TalkLine = FS47DialogueLine;
class FS47NpcDialogue;
class FS47EarlDialogue;

// story/caliche.ts: bindTown, the people and their conversations, townPeople, townWitnessDocs.
class SIGNAL47_API FS47Caliche
{
public:
	FS47Caliche(US47Main* InM, FS47Town* InTown);
	struct FReel
	{
		TFunction<bool()> Ready;
		TFunction<void(TFunction<void(const TArray<FS47TalkLine>&)>, TFunction<void(bool)>)> Play;
		TFunction<void()> Stop;
	} Reel;
	TSharedPtr<FS47NpcDialogue> NpcDialogue;
	TSharedPtr<FS47EarlDialogue> EarlDialogue;
	TFunction<void()> OnCharacterStart, OnEarlStart; // main.ts stopNarration/cb.stop hooks
	void StopSpeech();
	void Talk(const FString& W);
	void Choice(const FString& Id);
	void Close();
	static TArray<FS47PersonCard> TownPeople(const FS47TownSeen& Seen);
	static TArray<FS47Doc> TownWitnessDocs(const FS47TownSeen& Seen);
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	FS47Town* Town = nullptr;
	FString Who;
	TArray<FS47TalkLine> Lines, ShownLines;
	bool bReeling = false;
	TSharedPtr<SVerticalBox> Body;
	void BindTown();
	void Refresh();
	void StartVoice(bool bEarl, const FString& Voice, const TArray<FS47TalkLine>& Speech);
	void Once(const FString& Id, const FString& Text);
	void Attendant(bool bFirst);
	bool Available(const FString& W, const FString& Id) const;
	double Day() const;
	bool Here() const;
	void Save() const;
};
