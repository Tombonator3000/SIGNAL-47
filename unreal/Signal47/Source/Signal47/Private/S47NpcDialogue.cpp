// core/npcDialogue.ts
#include "S47NpcDialogue.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// VOICE-OUT-3 (Tom 9 November 2026): the web version's bacall (N. Vega), price (THE LINE) and lynch (the
// souvenir man) are clones of real people's voices and are gone. N. Vega, the line and the souvenir man
// have no voice role here; their lines are text, as every unrecorded line is (FS47RadioVoice::RetiredVoices).
const TMap<FString, FString>& FS47NpcDialogue::CharacterSpeakers()
{
	static const TMap<FString, FString> Speakers = { { TEXT("stanton"), TEXT("THE ATTENDANT") }, { TEXT("stack"), TEXT("THE DEPUTY") } };
	return Speakers;
}
const TMap<FString, FString>& FS47NpcDialogue::CharacterLabels()
{
	static const TMap<FString, FString> Labels = { { TEXT("stanton"), TEXT("The gas attendant") }, { TEXT("stack"), TEXT("The deputy") } };
	return Labels;
}
TArray<FS47DialogueLine> FS47NpcDialogue::CharacterLines(const FString& Voice, const TArray<FS47DialogueLine>& Lines)
{
	const FString Speaker = FS47RadioVoice::RetiredVoice(Voice) ? FString() : CharacterSpeakers().FindRef(Voice);
	TArray<FS47DialogueLine> Out;
	for (const auto& Line : Lines) if (!Speaker.IsEmpty() && Line.Who == Speaker) Out.Add(Line);
	return Out;
}
FS47NpcDialogue::FS47NpcDialogue(US47Audio* Audio) : FS47RecordedDialogue(Audio, TEXT("ensemble"), [](const FS47DialogueInput& Input, FS47RadioVoice& Bank) -> TOptional<TArray<FS47RadioClip>>
{
	const auto Speech = CharacterLines(Input.Voice, Input.Lines);
	if (Speech.IsEmpty()) return {};
	TArray<FS47RadioClip> Clips;
	for (const auto& Line : Speech)
	{
		const auto Found = Bank.Resolve(Line.Text, 0, Input.Voice);
		if (!Found) return {};
		Clips.Append(*Found);
	}
	return Clips;
}) {}

ES47DialogueState FS47NpcDialogue::StateOf(const FString& S)
{
	return S == TEXT("loading") ? ES47DialogueState::Loading : S == TEXT("playing") ? ES47DialogueState::Playing : S == TEXT("error") ? ES47DialogueState::Error : ES47DialogueState::Idle;
}

void FS47NpcDialogue::Play(const FString& Voice, const TArray<FS47DialogueLine>& Lines, TFunction<void(ES47DialogueState)> OnState)
{
	Play(FS47DialogueInput{ Voice, Lines }, OnState ? FNotify([OnState](const FString& S) { OnState(StateOf(S)); }) : FNotify());
}

bool FS47NpcDialogue::NoraQuote(const FString& Text, FString& Out)
{
	static const FString Head = TEXT("N. VEGA: \"");
	if (!Text.StartsWith(Head, ESearchCase::CaseSensitive) || !Text.EndsWith(TEXT("\"")) || Text.Len() < Head.Len() + 1) return false;
	Out = Text.Mid(Head.Len(), Text.Len() - Head.Len() - 1);
	return true;
}

bool FS47NpcDialogue::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString(bCond ? TEXT("ok npcDialogue: ") : TEXT("FAIL npcDialogue: ")) + What); bOk &= bCond; };
	const TArray<FS47DialogueLine> Speech = { {TEXT("YOU"), TEXT("You.")}, {TEXT("THE ATTENDANT"), TEXT("Cold one tonight.")}, {TEXT("N. VEGA"), TEXT("Nora.")}, {TEXT("THE LINE"), TEXT("The line.")}, {TEXT(""), TEXT("An aside.")} };
	const auto A = CharacterLines(TEXT("stanton"), Speech);
	Check(A.Num() == 1 && A[0].Who == TEXT("THE ATTENDANT"), TEXT("Stanton speaks only the authored attendant lines"));
	Check(CharacterLines(TEXT("other"), Speech).IsEmpty() && CharacterSpeakers().Num() == 2 && CharacterLabels().FindRef(TEXT("stack")) == TEXT("The deputy"), TEXT("roles are explicit, with the original names"));
	// VOICE-OUT-3: no role, bank or catalog clip carries a retired voice, and N. Vega, the line and the
	// souvenir man are text only
	bool bRoles = true;
	for (const auto& Pair : CharacterSpeakers()) bRoles &= !FS47RadioVoice::RetiredVoice(Pair.Key) && Pair.Value != TEXT("N. VEGA") && Pair.Value != TEXT("THE LINE") && Pair.Value != TEXT("THE SOUVENIR MAN");
	for (const auto& Pair : CharacterLabels()) bRoles &= !FS47RadioVoice::RetiredVoice(Pair.Key);
	for (const FString& Retired : FS47RadioVoice::RetiredVoices()) bRoles &= CharacterLines(Retired, Speech).IsEmpty() && Label(Retired).IsEmpty();
	Check(bRoles && FS47RadioVoice::RetiredVoices().Num() == 4 && FS47RadioVoice::RetiredVoice(TEXT("Bacall")) && FS47RadioVoice::RetiredVoice(TEXT("price")) && FS47RadioVoice::RetiredVoice(TEXT("LYNCH")) && !FS47RadioVoice::RetiredVoice(TEXT("stack")),
		TEXT("no voice role is a retired bank (bacall, price, lynch, kinski); N. Vega, the line and the souvenir man are captions"));
	{
		// the build's own tables (Scripts/audio-raw.py): no bank, role or character index with a retired name
		const FString Raw = FPaths::ProjectContentDir() / TEXT("Audio/Raw");
		FString Bad, Text;
		TSharedPtr<FJsonObject> J;
		const bool bVoices = FFileHelper::LoadFileToString(Text, *(Raw / TEXT("voices.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) && J.IsValid();
		if (bVoices) for (const auto& Bank : J->Values)
		{
			if (FS47RadioVoice::RetiredVoice(FString(Bank.Key.ToView()))) Bad += FString(Bank.Key.ToView()) + TEXT(" ");
			const TSharedPtr<FJsonObject>* B = nullptr; const TSharedPtr<FJsonObject>* Cat = nullptr; const TArray<TSharedPtr<FJsonValue>>* Clips = nullptr;
			if (!Bank.Value->TryGetObject(B) || !B || !(*B)->TryGetObjectField(TEXT("catalog"), Cat) || !Cat || !(*Cat)->TryGetArrayField(TEXT("clips"), Clips) || !Clips) continue;
			for (const auto& V : *Clips)
			{
				FString Who; const TSharedPtr<FJsonObject> O = V->AsObject();
				if (O && O->TryGetStringField(TEXT("voice"), Who) && FS47RadioVoice::RetiredVoice(Who)) { Bad += FString(Bank.Key.ToView()) + TEXT("/") + Who + TEXT(" "); break; }
			}
		}
		const bool bIndex = FFileHelper::LoadFileToString(Text, *(Raw / TEXT("index.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), J) && J.IsValid();
		const TSharedPtr<FJsonObject>* Character = nullptr;
		if (bIndex && J->TryGetObjectField(TEXT("character"), Character) && Character)
			for (const auto& V : (*Character)->Values) if (FS47RadioVoice::RetiredVoice(FString(V.Key.ToView()))) Bad += TEXT("index/") + FString(V.Key.ToView()) + TEXT(" ");
		if (bVoices || bIndex) Check(Bad.IsEmpty(), *(TEXT("Audio/Raw has no retired voice bank (bacall, price, lynch, kinski)") + (Bad.IsEmpty() ? FString() : TEXT(": ") + Bad)));
		else Lines.Add(TEXT("INFO npcDialogue: no Audio/Raw/voices.json or index.json in this build; the retired-voice check of the tables is skipped"));
	}
	return bOk;
}
