// core/npcDialogue.ts and core/recordedDialogue.ts (the queue), ui/UI.ts speakNora
#include "S47NpcDialogue.h"

#include "S47Audio.h"

FS47NpcDialogue::FS47NpcDialogue(US47Audio* InAudio) : Audio(InAudio) {}

FString FS47NpcDialogue::Speaker(const FString& Voice)
{
	// CHARACTER_SPEAKERS
	if (Voice == TEXT("stanton")) return TEXT("THE ATTENDANT");
	if (Voice == TEXT("stack")) return TEXT("THE DEPUTY");
	if (Voice == TEXT("lynch")) return TEXT("THE SOUVENIR MAN");
	if (Voice == TEXT("bacall")) return TEXT("N. VEGA");
	return FString();
}

FString FS47NpcDialogue::Label(const FString& Voice)
{
	// CHARACTER_LABELS
	if (Voice == TEXT("stanton")) return TEXT("Harry Dean Stanton");
	if (Voice == TEXT("stack")) return TEXT("Robert Stack");
	if (Voice == TEXT("lynch")) return TEXT("David Lynch");
	if (Voice == TEXT("bacall")) return TEXT("Lauren Bacall");
	if (Voice == TEXT("price")) return TEXT("Vincent Price");
	return FString();
}

TArray<FS47DialogueLine> FS47NpcDialogue::CharacterLines(const FString& Voice, const TArray<FS47DialogueLine>& Lines)
{
	TArray<FS47DialogueLine> Out;
	for (const FS47DialogueLine& L : Lines)
	{
		if (L.Who == (Voice == TEXT("price") ? FString(TEXT("THE LINE")) : Speaker(Voice)) || (Voice == TEXT("bacall") && L.Who == TEXT("THE LINE"))) Out.Add(L);
	}
	return Out;
}

bool FS47NpcDialogue::Enabled() const { return Audio.IsValid() && Audio->bCharacterVoices; }

TArray<FS47NpcDialogue::FClip> FS47NpcDialogue::Resolve(const FString& Voice, const TArray<FS47DialogueLine>& Lines) const
{
	// NPCDialogue's resolveInput: every spoken line has its clip, or there is none
	const TArray<FS47DialogueLine> Speech = CharacterLines(Voice, Lines);
	if (!Speech.Num() || !Audio.IsValid()) return {};
	TArray<FClip> Clips;
	for (const FS47DialogueLine& L : Speech)
	{
		const FString V = L.Who == TEXT("THE LINE") ? FString(TEXT("price")) : Voice;
		const TSharedPtr<FS47AudioBuffer> B = Audio->CharacterBuffer(V, L.Text);
		if (!B.IsValid()) return {};
		Clips.Add({ V, L.Text, B });
	}
	return Clips;
}

bool FS47NpcDialogue::Available(const FString& Voice, const TArray<FS47DialogueLine>& Lines) const { return Resolve(Voice, Lines).Num() > 0; }

void FS47NpcDialogue::DisposeNodes()
{
	for (const TSharedPtr<FS47AudioNode>& N : ExtraNodes) N->Disconnect();
	ExtraNodes.Reset();
}

void FS47NpcDialogue::Stop()
{
	++Generation;
	if (Source.IsValid())
	{
		Source->OnEnded = nullptr;
		Source->Stop(); // (already ended: nothing)
		Source->Disconnect();
		Source.Reset();
	}
	DisposeNodes();
	StateTo(ES47DialogueState::Idle);
	Notify = nullptr;
}

void FS47NpcDialogue::Play(const FString& Voice, const TArray<FS47DialogueLine>& Lines, TFunction<void(ES47DialogueState)> OnState)
{
	Stop();
	Notify = MoveTemp(OnState);
	if (!Enabled()) { StateTo(ES47DialogueState::Idle); Notify = nullptr; return; }
	const TArray<FClip> Clips = Resolve(Voice, Lines);
	if (!Clips.Num()) { StateTo(ES47DialogueState::Error); Notify = nullptr; return; }
	StateTo(ES47DialogueState::Loading);
	// (the clips are decoded already: nothing to wait for, and the context runs)
	US47Audio* A = Audio.Get();
	if (!A || !A->Ctx.IsValid()) { StateTo(ES47DialogueState::Error); Notify = nullptr; return; }
	PlayClip(Clips, 0, Generation);
}

void FS47NpcDialogue::PlayClip(const TArray<FClip>& Clips, int32 I, uint32 Gen)
{
	US47Audio* A = Audio.Get();
	if (Gen != Generation || !A || !A->Ctx.IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch B(*Ctx);
	auto S = Ctx->CreateBufferSource(); S->Buffer = Clips[I].Buffer;
	// A person speaking nearby uses the ordinary volume, without the AM receiver filter or the
	// radio-volume slider; the line's own voice comes down the telephone.
	if (Clips[I].Voice == TEXT("price"))
	{
		const FS47PhoneLine Line = A->PhoneLine();
		S->Connect(Line.Input);
		ExtraNodes.Append(Line.Nodes);
	}
	else S->Connect(A->Master);
	Source = S;
	const TWeakPtr<FS47NpcDialogue> Self = AsShared();
	const TWeakPtr<FS47BufferSourceNode> Ws = S;
	S->OnEnded = [Self, Ws, Clips, I, Gen]
	{
		const TSharedPtr<FS47NpcDialogue> P = Self.Pin();
		const TSharedPtr<FS47BufferSourceNode> Src = Ws.Pin();
		if (Src.IsValid()) Src->Disconnect();
		if (!P.IsValid()) return;
		if (P->Source == Src) P->Source.Reset();
		if (Gen != P->Generation) return;
		if (I + 1 < Clips.Num()) { P->PlayClip(Clips, I + 1, Gen); return; }
		P->DisposeNodes();
		P->StateTo(ES47DialogueState::Idle);
		P->Notify = nullptr;
	};
	S->Start(Ctx->CurrentTime() + (I ? 0.22 : 0));
	StateTo(ES47DialogueState::Playing);
}

bool FS47NpcDialogue::NoraQuote(const FString& Text, FString& Out)
{
	static const FString Head = TEXT("N. VEGA: \"");
	if (!Text.StartsWith(Head, ESearchCase::CaseSensitive) || !Text.EndsWith(TEXT("\"")) || Text.Len() < Head.Len() + 1) return false;
	Out = Text.Mid(Head.Len(), Text.Len() - Head.Len() - 1);
	return true;
}

void FS47NpcDialogue::SpeakNora(const FString& Text)
{
	if (!Enabled()) return;
	FString Quote;
	if (!NoraQuote(Text, Quote)) return;
	if (OnCharacterStart) OnCharacterStart();
	Play(TEXT("bacall"), { { TEXT("N. VEGA"), Quote } });
}
