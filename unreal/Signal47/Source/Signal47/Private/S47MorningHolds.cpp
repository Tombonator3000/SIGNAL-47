#include "S47MorningHolds.h"

#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47UI.h"

void FS47DinerCooler::FNodes::Stop()
{
	for (const TSharedPtr<FS47ScheduledSourceNode>& S : Sources) if (S) S->Stop();
	if (Out) Out->Disconnect();
}

// morningHolds.ts DinerCooler.build: hum, rattle and fan, through the same low-pass.
TSharedPtr<FS47DinerCooler::FNodes> FS47DinerCooler::Build()
{
	const TSharedPtr<FS47AudioContext> Ctx = Audio ? Audio->Ctx : nullptr;
	const TSharedPtr<FS47AudioBuffer> Noise = Audio ? Audio->NoiseBuffer() : nullptr;
	if (!Ctx || !Noise) return nullptr;
	FS47AudioContext::FBatch Batch(*Ctx);
	TSharedPtr<FNodes> R = MakeShared<FNodes>();
	R->Out = Ctx->CreateGain(); R->Out->Gain.SetValue(0);
	R->Lp = Ctx->CreateBiquadFilter(); R->Lp->SetType(TEXT("lowpass")); R->Lp->Frequency.SetValue(1100);
	R->Out->Connect(R->Lp); const TSharedPtr<FS47AudioNode> Pan = Audio->Spatial(Pos, 1.6f); if (Pan) R->Lp->Connect(Pan);
	struct FTone { double Mul, G; const TCHAR* Type; };
	for (const FTone& Tone : { FTone{1, 0.55, TEXT("sine")}, FTone{2, 0.3, TEXT("sine")}, FTone{3, 0.08, TEXT("square")}, FTone{5, 0.03, TEXT("sawtooth")} })
	{
		const TSharedPtr<FS47OscillatorNode> O = Ctx->CreateOscillator(); O->SetType(Tone.Type); O->Frequency.SetValue(58 * Tone.Mul);
		const TSharedPtr<FS47GainNode> Og = Ctx->CreateGain(); Og->Gain.SetValue(Tone.G); O->Connect(Og); Og->Connect(R->Out); O->Start(); R->F0.Add(O); R->Sources.Add(O);
	}
	const TSharedPtr<FS47BufferSourceNode> Src = Ctx->CreateBufferSource(); Src->Buffer = Noise; Src->bLoop = true;
	const TSharedPtr<FS47BiquadFilterNode> Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(160); Bp->Q.SetValue(2.5);
	const TSharedPtr<FS47GainNode> Rg = Ctx->CreateGain(); Rg->Gain.SetValue(0.22);
	const TSharedPtr<FS47OscillatorNode> Lfo = Ctx->CreateOscillator(); Lfo->Frequency.SetValue(7.3);
	const TSharedPtr<FS47GainNode> Lg = Ctx->CreateGain(); Lg->Gain.SetValue(0.12);
	Lfo->Connect(Lg); Lg->Connect(Rg->Gain);
	Src->Connect(Bp); Bp->Connect(Rg); Rg->Connect(R->Out); Src->Start(); Lfo->Start(); R->Sources.Add(Src); R->Sources.Add(Lfo);
	const TSharedPtr<FS47BufferSourceNode> Fan = Ctx->CreateBufferSource(); Fan->Buffer = Noise; Fan->bLoop = true; Fan->PlaybackRate.SetValue(0.93);
	const TSharedPtr<FS47BiquadFilterNode> Fb = Ctx->CreateBiquadFilter(); Fb->SetType(TEXT("bandpass")); Fb->Frequency.SetValue(640); Fb->Q.SetValue(0.9);
	const TSharedPtr<FS47GainNode> Fg = Ctx->CreateGain(); Fg->Gain.SetValue(1.0);
	Fan->Connect(Fb); Fb->Connect(Fg); Fg->Connect(R->Out); Fan->Start(); R->Sources.Add(Fan);
	return R;
}

void FS47DinerCooler::Update(TOptional<FVector> At, bool bInside)
{
	const TSharedPtr<FS47AudioContext> Ctx = Audio ? Audio->Ctx : nullptr; if (!Ctx) return;
	FS47AudioContext::FBatch Batch(*Ctx);
	if (!At.IsSet())
	{
		if (N) { const TSharedPtr<FNodes> Old = N; Old->Out->Gain.SetTargetAtTime(0, Ctx->CurrentTime(), 0.2); Ctx->SetTimeout(1.5, [Old] { Old->Stop(); }); N.Reset(); }
		bHeard = false; return;
	}
	Pos = At.GetValue(); if (!N) { N = Build(); if (!N) return; }
	bHeard = true; const double Now = Ctx->CurrentTime();
	const double Want = bRunning ? Level * (bInside ? 1 : 0.3) : 0;
	N->Out->Gain.SetTargetAtTime(Want, Now, bRunning ? 0.25 : 0.02);
	N->Lp->Frequency.SetTargetAtTime(bInside ? 1100 : 170, Now, 0.3);
}

void FS47DinerCooler::Off()
{
	bRunning = false; const TSharedPtr<FS47AudioContext> Ctx = Audio ? Audio->Ctx : nullptr; if (!Ctx || !N) return;
	FS47AudioContext::FBatch Batch(*Ctx); const double Now = Ctx->CurrentTime();
	FS47Sound S; S.Gain = 0.32f; S.Rate = 1.75f; S.At = Pos; Audio->Sfx(TEXT("thudSoft"), S);
	const double Mul[] = {1, 2, 3, 5};
	for (int32 I = 0; I < N->F0.Num(); ++I) { const double F = 58 * Mul[I]; auto& P = N->F0[I]->Frequency; P.CancelScheduledValues(Now); P.SetValueAtTime(F, Now); P.ExponentialRampToValueAtTime(F * 0.55, Now + 0.35); }
	N->Out->Gain.CancelScheduledValues(Now); N->Out->Gain.SetValueAtTime(N->Out->Gain.Value(), Now); N->Out->Gain.SetTargetAtTime(0, Now + 0.06, 0.07);
}

void FS47DinerCooler::On()
{
	bRunning = true; const TSharedPtr<FS47AudioContext> Ctx = Audio ? Audio->Ctx : nullptr; if (!Ctx || !N) return;
	FS47AudioContext::FBatch Batch(*Ctx); const double Now = Ctx->CurrentTime();
	FS47Sound S; S.Gain = 0.3f; S.Rate = 1.55f; S.At = Pos; Audio->Sfx(TEXT("thudSoft"), S);
	const double Mul[] = {1, 2, 3, 5};
	for (int32 I = 0; I < N->F0.Num(); ++I) { const double F = 58 * Mul[I]; auto& P = N->F0[I]->Frequency; P.CancelScheduledValues(Now); P.SetValueAtTime(F * 0.45, Now); P.ExponentialRampToValueAtTime(F, Now + 0.9); }
	N->Out->Gain.CancelScheduledValues(Now); N->Out->Gain.SetValueAtTime(0, Now); N->Out->Gain.LinearRampToValueAtTime(Level * 1.25, Now + 0.5); N->Out->Gain.SetTargetAtTime(Level, Now + 0.9, 0.6);
}

bool FS47MorningHolds::Had() const { return M && M->TownSeen.Contains(S47Morning::KEY); }
void FS47MorningHolds::Save() { if (M && M->Mode == US47Main::EMode::Play) M->RequestAutosave(); }

// morningHolds.ts MorningHolds.update, including the panel's maximum hold.
void FS47MorningHolds::Update(double Dt, const FS47MorningIn& S)
{
	if (Phase == TEXT("idle"))
	{
		if (Had()) { Phase = TEXT("done"); return; }
		const double Day = FMath::Fmod(FMath::Fmod(S.Clock, 86400.0) + 86400, 86400.0);
		Inside = S.bHere ? Inside + Dt : 0;
		if (S.bHere && S.bFree && !S.bPanel && Inside >= S47Morning::SETTLE && Day >= S47Morning::FROM && Day < S47Morning::TO) Begin(S.Clock);
		return;
	}
	if (Phase != TEXT("held")) return;
	if (!S.bArea) { LetGo(true); return; }
	const auto& C = S47Morning::CUES;
	if (S.bPanel && T < C.Line + 0.5 && PanelT < S47Morning::PANEL_MAX) { PanelT += Dt; bByPanel = true; return; }
	if (bByPanel && !S.bPanel) { if (T > C.Line - 1.6) T = Cue = C.Line - 1.6; bByPanel = false; }
	T += Dt;
	auto Step = [&](double Time, TFunctionRef<void()> Fn) { if (T >= Time && Cue < Time) Fn(); };
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	Step(C.Caption, [&] { if (M && M->UI) M->UI->Toast(bRadioWas ? TEXT("The cooler behind the counter cuts out. The radio stops in the middle of a word. Outside, the birds stop.") : TEXT("The cooler behind the counter cuts out. Outside, the birds stop. All of it at once."), 4.2f); });
	Step(C.Line, [&]
	{
		if (M && M->UI) M->UI->Toast(FString::Printf(TEXT("THE WAITRESS: \"%s\""), S47Morning::LINE), 4);
		if (M && M->Story) M->Story->Note(FString::Printf(TEXT("%s. Mesa Diner. Everything stopped at once: the cooler behind the counter, %sthe birds outside, the hum of the room. Ten seconds of nothing. The waitress did not look up: \"%s\""), *S47Text::ClockText(At.IsSet() ? At.GetValue() : S.Clock, false), bRadioWas ? TEXT("the radio in the middle of a word, ") : TEXT(""), S47Morning::LINE));
		Save();
	});
	Step(C.Cooler, [&] { if (Cooler) Cooler->On(); if (A) A->HoldRoomTone(1, 0.5f); });
	Step(C.Radio, [&] { if (Radio) Radio(1); });
	Step(C.Bird, [&] { if (A && A->Night) A->Night->Back(0.3); });
	Cue = T; if (T >= C.End) LetGo(false);
}

bool FS47MorningHolds::Now(const FS47MorningIn& S)
{
	if (Phase == TEXT("held") || !S.bArea) return false;
	if (M) M->TownSeen.Remove(S47Morning::KEY); Begin(S.Clock); return true;
}

void FS47MorningHolds::Begin(double Clock)
{
	Phase = TEXT("held"); T = 0; Cue = 0; At = Clock; PanelT = 0; bByPanel = false;
	bRadioWas = RadioHeard && RadioHeard(); if (M) M->TownSeen.Add(S47Morning::KEY);
	if (Cooler) Cooler->Off();
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	if (A) { A->HoldRoomTone(0, 0.02f); A->HoldThunder(0); if (A->Night) A->Night->Still(); }
	if (Radio) Radio(0); if (Saw) Saw(); Save();
}

void FS47MorningHolds::LetGo(bool bCut)
{
	US47Audio* A = M ? M->Audio.Get() : nullptr;
	if (Cooler && (bCut || !Cooler->bRunning)) Cooler->On();
	if (A) A->HoldRoomTone(1, bCut ? 0.2f : 0.5f); if (Radio) Radio(1);
	if (A) { if (A->Night && (bCut || A->Night->IsStill())) A->Night->Back(bCut ? 4 : 0.3); A->HoldThunder(1); }
	Phase = TEXT("done");
}

void FS47MorningHolds::Reset()
{
	if (Phase == TEXT("held")) LetGo(true);
	Phase = Had() ? TEXT("done") : TEXT("idle"); T = 0; Cue = 0; At.Reset(); Inside = 0; PanelT = 0; bByPanel = false;
}

bool FS47MorningHolds::SelfTest(TArray<FString>& Lines)
{
	US47Main* Main = NewObject<US47Main>(); FS47DinerCooler Cooler0(nullptr); FS47MorningHolds H(Main, &Cooler0);
	FS47MorningIn S; S.bHere = S.bArea = S.bFree = true; S.Clock = S47Morning::FROM;
	H.Update(13.9, S); bool Ok = !H.Busy(); H.Update(0.1, S); Ok &= H.Busy() && H.Had() && !Cooler0.bRunning;
	S.bPanel = true; H.Update(30, S); Ok &= H.T == 0; H.Update(45, S); H.Update(1, S); Ok &= H.T == 1;
	S.bPanel = false; H.Update(14.2, S); Ok &= Cooler0.bRunning; H.Update(14.3, S); Ok &= !H.Busy() && H.Phase == TEXT("done");
	H.Reset(); Ok &= H.Phase == TEXT("done"); Main->TownSeen.Clear(); H.Reset(); S.Clock = S47Morning::TO; H.Update(20, S); Ok &= !H.Busy();
	Ok &= H.Now(S); S.bArea = false; H.Update(0.1, S); Ok &= !H.Busy() && Cooler0.bRunning;
	Lines.Add(FString::Printf(TEXT("%s morningHolds: settle, panel hold/cap, cues, saved key, cutoff and departure"), Ok ? TEXT("PASS") : TEXT("FAIL"))); return Ok;
}
