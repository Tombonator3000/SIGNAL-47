// core/recordedDialogue.ts
#include "S47RecordedDialogue.h"

#include "S47Audio.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DialogueAudio, Log, All);

// Transparent, opt-in taps measure samples actually pulled by the live destination.
// They run under the context lock, like the rest of S47WebAudio's graph.
class FS47DialogueLevelNode : public FS47AudioNode
{
public:
	explicit FS47DialogueLevelNode(const TSharedRef<FS47AudioContext>& C) : FS47AudioNode(C) {}
	int64 Samples = 0;
	double Peak = 0, SumSquares = 0;
	double Rms() const { return Samples ? FMath::Sqrt(SumSquares / Samples) : 0; }
protected:
	virtual void Process(int64 Quantum) override
	{
		PullInputs(Quantum, Out);
		for (int32 C = 0; C < Out.Channels; ++C) for (const float V : Out.D[C])
		{
			++Samples; Peak = FMath::Max(Peak, FMath::Abs(double(V))); SumSquares += double(V) * V;
		}
	}
};

FS47RecordedDialogue::FS47RecordedDialogue(US47Audio* InAudio, const FString& BankKind, FResolve InResolve, FTelephone InTelephone, FOutputRoute InOutput)
	: Bank(MakeShared<FS47RadioVoice>(BankKind)), Audio(InAudio), ResolveInput(MoveTemp(InResolve)), Telephone(MoveTemp(InTelephone)), OutputRoute(MoveTemp(InOutput)), Life(MakeShared<FLife>())
{
	Life->Owner = this;
	bLevelDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("S47VoiceLevels"));
}
FS47RecordedDialogue::~FS47RecordedDialogue() { Notify = {}; Stop(); Life->Owner = nullptr; Life.Reset(); }

void FS47RecordedDialogue::SetEnabled(bool bValue) { bOn = bValue; if (!bValue) Stop(); }
FS47DialogueStatus FS47RecordedDialogue::Status() const { return { CurrentState, bOn, Text, Clips, Bank->BufferCount() }; }
bool FS47RecordedDialogue::ActiveClip(FString& ClipId, double& Seconds) const
{
	ClipId.Reset(); Seconds = 0;
	const US47Audio* A = Audio.Get();
	if (!Playing() || !Source || !Plan.IsValidIndex(ActiveClipIndex) || !A || !A->Ctx) return false;
	const double Elapsed = A->Ctx->CurrentTime() - SourceStartedAt;
	if (Elapsed < 0) return false; // PlayNext schedules a short silent gap between sources.
	ClipId = Plan[ActiveClipIndex].Id; Seconds = Elapsed;
	return true;
}
bool FS47RecordedDialogue::Available(const FS47DialogueInput& Input) const { return ResolveInput && ResolveInput(Input, *Bank).IsSet(); }
void FS47RecordedDialogue::StateTo(const FString& Value) { CurrentState = Value; const FNotify OnState = Notify; if (OnState) OnState(Value); }

void FS47RecordedDialogue::Stop()
{
	++Generation;
	bPending = false; // The native bank loads synchronously, so the generation replaces AbortController.
	ActiveClipIndex = INDEX_NONE;
	ReportLevels(TEXT("stopped"));
	if (Source)
	{
		Source->OnEnded = nullptr;
		Source->Stop(); Source->Disconnect(); Source.Reset();
	}
	Text.Reset(); Clips.Reset(); Plan.Reset(); Buffers.Reset();
	DisposeNodes();
	StateTo(TEXT("idle")); Notify = {};
}

void FS47RecordedDialogue::DisposeNodes()
{
	for (const auto& Node : ExtraNodes) Node->Disconnect(); ExtraNodes.Reset();
	SourceMeter.Reset(); RoutedMeter.Reset(); OutputBus.Reset(); ClipLevels = {};
}

FS47DialogueAudioLevels FS47RecordedDialogue::AudioLevels() const
{
	if (!SourceMeter || !RoutedMeter) return LastLevels;
	FS47AudioContext::FBatch Batch(*SourceMeter->Ctx);
	FS47DialogueAudioLevels Levels = ClipLevels;
	Levels.SourceSamples = SourceMeter->Samples; Levels.SourcePeak = SourceMeter->Peak; Levels.SourceRms = SourceMeter->Rms();
	Levels.RoutedSamples = RoutedMeter->Samples; Levels.RoutedPeak = RoutedMeter->Peak; Levels.RoutedRms = RoutedMeter->Rms();
	Levels.BusGain = OutputBus ? OutputBus->Gain.Value() : 1;
	return Levels;
}

void FS47RecordedDialogue::ReportLevels(const TCHAR* Reason)
{
	if (!SourceMeter || !RoutedMeter) return;
	LastLevels = AudioLevels();
	UE_LOG(LogS47DialogueAudio, Display, TEXT("S47: dialogue audio clip=%s route=%s receiverFilter=%d reason=%s pcmPeak=%.6f pcmRms=%.6f sourceSamples=%lld sourcePeak=%.6f sourceRms=%.6f routedSamples=%lld routedPeak=%.6f routedRms=%.6f busGain=%.3f"),
		*LastLevels.ClipId, *LastLevels.Channel, LastLevels.bReceiverFilter, Reason, LastLevels.PcmPeak, LastLevels.PcmRms, LastLevels.SourceSamples, LastLevels.SourcePeak, LastLevels.SourceRms,
		LastLevels.RoutedSamples, LastLevels.RoutedPeak, LastLevels.RoutedRms, LastLevels.BusGain);
}

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
		ActiveClipIndex = INDEX_NONE;
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
	const bool bTelephone = Telephone && Telephone(Plan[I]);
	const FOutput Output = OutputRoute ? OutputRoute(Plan[I]) : FOutput();
	OutputBus = Output.bRadioBus ? A->RadioBus : (bTelephone || Output.At.IsSet()) ? A->SfxBus : A->Master;
	TSharedPtr<FS47AudioNode> Tail = Src;
	if (bLevelDiagnostics)
	{
		SourceMeter = MakeShared<FS47DialogueLevelNode>(Ctx.ToSharedRef()); RoutedMeter = MakeShared<FS47DialogueLevelNode>(Ctx.ToSharedRef());
		Src->Connect(SourceMeter); Tail = SourceMeter;
		ExtraNodes.Add(SourceMeter); ExtraNodes.Add(RoutedMeter);
		ClipLevels.ClipId = Plan[I].Id;
		ClipLevels.bReceiverFilter = bTelephone;
		ClipLevels.Channel = !Output.Channel.IsEmpty() ? Output.Channel : Output.bRadioBus ? TEXT("radio") : Output.At.IsSet() ? TEXT("positional") : bTelephone ? TEXT("phone") : TEXT("master");
		double Sum = 0; int64 Samples = 0;
		for (int32 C = 0; C < Src->Buffer->NumberOfChannels(); ++C) for (int32 F = 0; F < Src->Buffer->Length(); ++F)
		{
			const double V = Src->Buffer->GetChannelData(C)[F]; ++Samples; Sum += V * V; ClipLevels.PcmPeak = FMath::Max(ClipLevels.PcmPeak, FMath::Abs(V));
		}
		ClipLevels.PcmRms = Samples ? FMath::Sqrt(Sum / Samples) : 0;
	}
	if (bTelephone)
	{
		const FS47PhoneLine Line = A->PhoneLine(); Tail->Connect(Line.Input); ExtraNodes.Append(Line.Nodes);
		Line.Gain->Disconnect(); Tail = Line.Gain;
	}
	if (Output.At)
	{
		const FVector At = *Output.At;
		const auto Panner = Ctx->CreatePanner(); Panner->RefDistance = 1.8; Panner->RolloffFactor = 1.2; Panner->MaxDistance = 60;
		Panner->SetPosition(At.X, At.Y, At.Z); Tail->Connect(Panner); Tail = Panner; ExtraNodes.Add(Panner);
	}
	if (RoutedMeter) { Tail->Connect(RoutedMeter); Tail = RoutedMeter; }
	Tail->Connect(OutputBus);
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
		Self->ReportLevels(TEXT("ended")); Self->DisposeNodes();
		Self->PlayNext(Gen, I + 1);
	};
	ActiveClipIndex = I;
	SourceStartedAt = Ctx->CurrentTime() + (I ? 0.22 : 0);
	Src->Start(SourceStartedAt);
	StateTo(TEXT("playing"));
}

void FS47RecordedDialogue::Failed(int32 Gen)
{
	if (Gen != Generation || !bPending) return;
	bPending = false;
	ActiveClipIndex = INDEX_NONE;
	ReportLevels(TEXT("error"));
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
	FString ActiveId; double ActiveSeconds = 0;
	Check(R.ActiveClip(ActiveId, ActiveSeconds) && ActiveId == C0.Id && FMath::IsNearlyZero(ActiveSeconds), TEXT("active clip reports the first audible source and its clock"));
	TArray<float> Pcm; Pcm.SetNumZeroed(1024 * 2); A->Ctx->Render(Pcm.GetData(), 1024); A->Ctx->Tick(0.03);
	Check(R.Playing() && R.Status().Text == TOptional<FString>(C1.Text), TEXT("onended advances to the next source"));
	Check(!R.ActiveClip(ActiveId, ActiveSeconds) && ActiveId.IsEmpty(), TEXT("the scheduled inter-clip gap has no audible face take"));
	Pcm.SetNumZeroed(4800 * 2); A->Ctx->Render(Pcm.GetData(), 4800); A->Ctx->Tick(0.1);
	Check(R.Playing(), TEXT("the next source waits the authored 0.22 seconds"));
	Pcm.SetNumZeroed(6000 * 2); A->Ctx->Render(Pcm.GetData(), 6000); A->Ctx->Tick(0.125);
	Check(R.ActiveClip(ActiveId, ActiveSeconds) && ActiveId == C1.Id && ActiveSeconds >= 0 && ActiveSeconds < 0.01, TEXT("the second audible source reports its own id and source-relative time"));
	const double BeforePause = ActiveSeconds;
	A->Ctx->Suspend(true); Pcm.SetNumZeroed(480 * 2); A->Ctx->Render(Pcm.GetData(), 480); A->Ctx->Tick(0.01);
	Check(R.ActiveClip(ActiveId, ActiveSeconds) && ActiveId == C1.Id && FMath::IsNearlyEqual(ActiveSeconds, BeforePause), TEXT("suspending audio freezes the active source clock"));
	A->Ctx->Suspend(false);
	R.Stop(); Pcm.SetNumZeroed(16000 * 2); A->Ctx->Render(Pcm.GetData(), 16000); A->Ctx->Tick(0.34);
	Check(R.State() == TEXT("idle") && !R.Status().Text && R.Status().Clips.IsEmpty(), TEXT("stop invalidates the pending generation and clears its queue"));
	Check(!R.ActiveClip(ActiveId, ActiveSeconds) && ActiveId.IsEmpty(), TEXT("stop clears the active face take"));
	// Render the real routing graph, not just decoded buffers or source state.
	struct FRendered { FS47DialogueAudioLevels Levels; double Rms = 0; };
	auto RenderRoute = [](bool bPhone, bool bRadio, TOptional<FVector> At, double RadioGain, double SfxGain) -> FRendered
	{
		US47Audio* A2 = NewObject<US47Audio>(); A2->Ctx = FS47AudioContext::Make();
		A2->Master = A2->Ctx->CreateGain(); A2->Master->Connect(A2->Ctx->Destination);
		A2->SfxBus = A2->Ctx->CreateGain(); A2->SfxBus->Gain.SetValue(SfxGain); A2->SfxBus->Connect(A2->Master);
		A2->RadioBus = A2->Ctx->CreateGain(); A2->RadioBus->Gain.SetValue(RadioGain); A2->RadioBus->Connect(A2->Master);
		FS47RecordedDialogue D(A2, TEXT("ensemble"), {}, [bPhone](const FS47RadioClip&) { return bPhone; },
			[bRadio, At](const FS47RadioClip&) { FOutput Out; Out.At = At; Out.bRadioBus = bRadio; return Out; });
		D.SetLevelDiagnostics(true);
		FS47RadioClip Clip; Clip.Id = TEXT("selftest-route"); Clip.Text = TEXT("Synthetic routing test.");
		const auto B = A2->Ctx->CreateBuffer(1, 9600, 48000);
		for (int32 F = 0; F < B->Length(); ++F) B->GetChannelData(0)[F] = 0.25f * FMath::Sin(2.0 * PI * 1000 * F / 48000);
		D.Plan = { Clip }; D.Buffers.Add(Clip.Id, B); D.bPending = true; D.PlayNext(D.Generation, 0);
		TArray<float> Rendered; Rendered.SetNumZeroed(2048 * 2); A2->Ctx->Render(Rendered.GetData(), 2048);
		FRendered Result; Result.Levels = D.AudioLevels();
		double Sum = 0; for (const float V : Rendered) Sum += V * V;
		Result.Rms = FMath::Sqrt(Sum / Rendered.Num());
		const int64 BeforePause = Result.Levels.SourceSamples;
		A2->Ctx->Suspend(true); A2->Ctx->Render(Rendered.GetData(), 2048);
		const bool bPaused = D.AudioLevels().SourceSamples == BeforePause;
		A2->Ctx->Suspend(false); A2->Ctx->Render(Rendered.GetData(), 2048);
		if (!bPaused || D.AudioLevels().SourceSamples <= BeforePause) Result.Rms = -1;
		D.Stop(); return Result;
	};
	const auto Direct = RenderRoute(false, false, {}, 1, 1);
	Check(Direct.Levels.SourceSamples > 0 && Direct.Levels.SourceRms > 0.15 && Direct.Rms > 0.15, TEXT("diagnostics measure PCM pulled by the live destination; pause holds and resume advances it"));
	const auto Receiver = RenderRoute(true, true, {}, 1, 0);
	Check(Receiver.Levels.Channel == TEXT("radio") && Receiver.Levels.bReceiverFilter && Receiver.Levels.RoutedRms > 0.05 && Receiver.Rms > 0.05, TEXT("a filtered receiver reaches RadioBus even while SfxBus is muted"));
	const auto MutedRadio = RenderRoute(true, true, {}, 0, 1);
	Check(MutedRadio.Levels.SourceRms > 0.15 && MutedRadio.Levels.RoutedRms > 0.05 && MutedRadio.Levels.BusGain == 0 && MutedRadio.Rms == 0, TEXT("the radio volume bus controls a filtered recording without losing its source metrics"));
	const auto Near = RenderRoute(false, false, FVector(0, 0, -1), 1, 1);
	const auto Far = RenderRoute(false, false, FVector(0, 0, -20), 1, 1);
	Check(Near.Levels.Channel == TEXT("positional") && Near.Rms > 0.1 && Far.Rms > 0 && Far.Rms < Near.Rms * 0.2, TEXT("an in-room recording pans and attenuates at its authored three.js position"));
	return bOk;
}
