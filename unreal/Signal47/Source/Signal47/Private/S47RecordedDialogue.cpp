// core/recordedDialogue.ts
#include "S47RecordedDialogue.h"

#include "S47Audio.h"

FS47RecordedDialogue::FS47RecordedDialogue(US47Audio* InAudio, const FString& BankKind, FResolve InResolve, FTelephone InTelephone)
	: Bank(MakeShared<FS47RadioVoice>(BankKind)), Audio(InAudio), ResolveInput(MoveTemp(InResolve)), Telephone(MoveTemp(InTelephone)), Life(MakeShared<FLife>())
{
	Life->Owner = this;
}
FS47RecordedDialogue::~FS47RecordedDialogue() { Notify = {}; Stop(); Life->Owner = nullptr; Life.Reset(); }

void FS47RecordedDialogue::SetEnabled(bool bValue) { bOn = bValue; if (!bValue) Stop(); }
FS47DialogueStatus FS47RecordedDialogue::Status() const { return { CurrentState, bOn, Text, Clips, Bank->BufferCount() }; }
bool FS47RecordedDialogue::Available(const FS47DialogueInput& Input) const { return ResolveInput && ResolveInput(Input, *Bank).IsSet(); }
void FS47RecordedDialogue::StateTo(const FString& Value) { CurrentState = Value; const FNotify OnState = Notify; if (OnState) OnState(Value); }

void FS47RecordedDialogue::Stop()
{
	++Generation;
	bPending = false; // The native bank loads synchronously, so the generation replaces AbortController.
	if (Source)
	{
		Source->OnEnded = nullptr;
		Source->Stop(); Source->Disconnect(); Source.Reset();
	}
	Text.Reset(); Clips.Reset(); Plan.Reset(); Buffers.Reset();
	DisposeNodes();
	StateTo(TEXT("idle")); Notify = {};
}

void FS47RecordedDialogue::DisposeNodes() { for (const auto& Node : ExtraNodes) Node->Disconnect(); ExtraNodes.Reset(); }

void FS47RecordedDialogue::Play(const FS47DialogueInput& Input, FNotify OnState)
{
	Stop();
	Notify = MoveTemp(OnState);
	if (!Enabled()) { StateTo(TEXT("idle")); Notify = {}; return; }
	const TOptional<TArray<FS47RadioClip>> Found = ResolveInput ? ResolveInput(Input, *Bank) : TOptional<TArray<FS47RadioClip>>();
	if (!Found) { StateTo(TEXT("error")); Notify = {}; return; }
	const int32 Gen = Generation; bPending = true;
	StateTo(TEXT("loading"));
	if (Gen != Generation || !bPending) return;
	US47Audio* A = Audio.Get();
	// The game's audio is already unlocked by its first gesture; a queue must not resume a paused night.
	if (!A || !A->Ctx || A->Ctx->State() != TEXT("running")) { Failed(Gen); return; }
	Plan = *Found;
	TArray<FS47RadioClip> ToLoad;
	for (const FS47RadioClip& Clip : Plan)
	{
		// Reuse Audio.ts's exact authored character recording when it is already decoded.
		const auto Buffer = Clip.Voice.IsEmpty() ? nullptr : A->CharacterBuffer(Clip.Voice, Clip.Text);
		if (Buffer) Buffers.Add(Clip.Id, Buffer); else ToLoad.Add(Clip);
	}
	TMap<FString, TSharedPtr<FS47AudioBuffer>> Loaded;
	// Native difference: RadioVoice's selected short PCM clips load synchronously, before any sound.
	if (!Bank->Load(ToLoad, Loaded, [this, Gen] { return Gen != Generation || !bPending; })) { Failed(Gen); return; }
	if (Gen != Generation || !bPending) return;
	for (const auto& Pair : Loaded) Buffers.Add(Pair.Key, Pair.Value);
	for (const FS47RadioClip& Clip : Plan) if (!Buffers.FindRef(Clip.Id)) { Failed(Gen); return; }
	for (const FS47RadioClip& Clip : Plan) Clips.Add({ Clip.Id, Clip.Text, Buffers.FindRef(Clip.Id)->Duration(), Clip.Voice });
	PlayNext(Gen, 0);
}

void FS47RecordedDialogue::PlayNext(int32 Gen, int32 I)
{
	if (Gen != Generation || !bPending) return;
	if (I >= Plan.Num())
	{
		bPending = false; Text.Reset(); DisposeNodes();
		Plan.Reset(); Buffers.Reset();
		StateTo(TEXT("idle")); Notify = {}; return;
	}
	US47Audio* A = Audio.Get();
	if (!A || !A->Ctx) { Failed(Gen); return; }
	const auto Ctx = A->Ctx;
	FS47AudioContext::FBatch Batch(*Ctx);
	const auto Src = Ctx->CreateBufferSource(); Src->Buffer = Buffers.FindRef(Plan[I].Id);
	if (!Src->Buffer) { Failed(Gen); return; }
	if (Telephone && Telephone(Plan[I]))
	{
		const FS47PhoneLine Line = A->PhoneLine(); Src->Connect(Line.Input); ExtraNodes.Append(Line.Nodes);
	}
	else Src->Connect(A->Master);
	Source = Src; Text = Plan[I].Text;
	const TWeakPtr<FLife> WeakLife = Life;
	const TWeakPtr<FS47BufferSourceNode> WeakSource = Src;
	Src->OnEnded = [WeakLife, WeakSource, Gen, I]
	{
		const auto L = WeakLife.Pin(); const auto Ended = WeakSource.Pin();
		if (Ended) Ended->Disconnect();
		if (!L || !L->Owner || L->Owner->Generation != Gen) return;
		FS47RecordedDialogue* Self = L->Owner;
		if (Self->Source == Ended) Self->Source.Reset();
		Self->PlayNext(Gen, I + 1);
	};
	Src->Start(Ctx->CurrentTime() + (I ? 0.22 : 0));
	StateTo(TEXT("playing"));
}

void FS47RecordedDialogue::Failed(int32 Gen)
{
	if (Gen != Generation || !bPending) return;
	bPending = false;
	if (Source) { Source->OnEnded = nullptr; Source->Stop(); Source->Disconnect(); Source.Reset(); }
	Text.Reset(); Plan.Reset(); Buffers.Reset(); DisposeNodes();
	StateTo(TEXT("error")); Notify = {};
}

bool FS47RecordedDialogue::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const TCHAR* What) { Lines.Add(FString(bCond ? TEXT("ok recordedDialogue: ") : TEXT("FAIL recordedDialogue: ")) + What); bOk &= bCond; };
	US47Audio* A = NewObject<US47Audio>(); A->Ctx = FS47AudioContext::Make(); A->Master = A->Ctx->CreateGain(); A->Master->Connect(A->Ctx->Destination);
	FS47RecordedDialogue R(A, TEXT("ensemble"), [](const FS47DialogueInput&, FS47RadioVoice&) -> TOptional<TArray<FS47RadioClip>> { return {}; });
	TArray<FString> States;
	R.Play({}, [&](const FString& S) { States.Add(S); }); Check(R.State() == TEXT("idle") && States == TArray<FString>{TEXT("idle")}, TEXT("disabled queues stay idle"));
	R.SetEnabled(true); R.Play({}, [&](const FString& S) { States.Add(S); }); Check(R.State() == TEXT("error") && !R.Available({}), TEXT("an unavailable complete exchange never begins a partial reply"));
	R.Stop();
	FS47RadioClip C0; C0.Id = TEXT("first"); C0.Text = TEXT("First line.");
	FS47RadioClip C1; C1.Id = TEXT("second"); C1.Text = TEXT("Second line.");
	R.Plan = { C0, C1 }; R.bPending = true;
	const auto B0 = A->Ctx->CreateBuffer(1, 480, 48000), B1 = A->Ctx->CreateBuffer(1, 480, 48000);
	R.Buffers.Add(C0.Id, B0); R.Buffers.Add(C1.Id, B1);
	R.PlayNext(R.Generation, 0); Check(R.Playing() && R.Status().Text == TOptional<FString>(C0.Text), TEXT("the first source starts with its exact text"));
	TArray<float> Pcm; Pcm.SetNumZeroed(1024 * 2); A->Ctx->Render(Pcm.GetData(), 1024); A->Ctx->Tick(0.03);
	Check(R.Playing() && R.Status().Text == TOptional<FString>(C1.Text), TEXT("onended advances to the next source"));
	Pcm.SetNumZeroed(4800 * 2); A->Ctx->Render(Pcm.GetData(), 4800); A->Ctx->Tick(0.1);
	Check(R.Playing(), TEXT("the next source waits the authored 0.22 seconds"));
	R.Stop(); Pcm.SetNumZeroed(16000 * 2); A->Ctx->Render(Pcm.GetData(), 16000); A->Ctx->Tick(0.34);
	Check(R.State() == TEXT("idle") && !R.Status().Text && R.Status().Clips.IsEmpty(), TEXT("stop invalidates the pending generation and clears its queue"));
	return bOk;
}
