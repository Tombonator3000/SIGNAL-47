#include "S47AnomalyDread.h"

#include "S47Audio.h"
#include "S47EarlDialogue.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NpcDialogue.h"
#include "S47Phones.h"
#include "S47ReadAloud.h"
#include "S47Story.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr double LENGTH = 1.65;
	bool ReaderBusy(const TSharedPtr<FS47ReadAloud>& Reader) { return Reader && (Reader->Playing() || Reader->Loading()); }
}

FS47AnomalyDread::FS47AnomalyDread(US47Main* InM) : M(InM) {}
FS47AnomalyDread::~FS47AnomalyDread() { Reset(); }

bool FS47AnomalyDread::MayBegin(double Clock, bool bProtected, bool bHere)
{
	return S47Timeline::Night(Clock) >= 0 && !bProtected && bHere;
}

bool FS47AnomalyDread::Protected(bool bIncludeOneAtATime) const
{
	const US47Main* Main = M.Get();
	if (!Main || Main->Mode != US47Main::EMode::Play || !Main->UI || !Main->Story || !Main->Audio) return true;
	if (Main->UI->Modal() || Main->Story->IsCinematic() || Main->Audio->PhonePlaying()) return true;
	if (bIncludeOneAtATime && Main->OneAtATime()) return true;
	// NightGoing omits this helper's own Busy flag. A newly started major scene wins;
	// cancel this beat instead of muting, stopping or rescheduling its dialogue.
	if (Main->Night && (Main->Night->NightGoing() || (Main->Night->Hooks.NinthQuiet && Main->Night->Hooks.NinthQuiet()))) return true;
	if (Main->Phones && Main->Phones->CbBusy && Main->Phones->CbBusy()) return true;
	if (ReaderBusy(Main->UI->ReadAloud) || ReaderBusy(Main->UI->Narrator)) return true;
	if (Main->UI->NpcDialogue && (Main->UI->NpcDialogue->Playing() || Main->UI->NpcDialogue->Loading())) return true;
	if (Main->UI->EarlDialogue && (Main->UI->EarlDialogue->Playing() || Main->UI->EarlDialogue->Loading())) return true;
	return false;
}

void FS47AnomalyDread::CaptureColor(UMaterialInstanceDynamic* Mid, TOptional<FLinearColor> On)
{
	if (!Mid || Paint.ContainsByPredicate([Mid](const FPaint& P) { return P.Mid.Get() == Mid; })) return;
	FLinearColor Original;
	if (!Mid->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Original)) return;
	Paint.Add({ Mid, Original, On.IsSet() ? On.GetValue() : Original, Original });
}

bool FS47AnomalyDread::SelectLight(const FVector& At, int32 Kind)
{
	AS47Web* W = Web.Get(); if (!W) return false;
	if (Kind == 3)
	{
		// Deliberate rule bend: Earl still leaves at four, but the empty studio's sign
		// flashes once. Only this existing sign's paint changes, never the studio clock.
		const FVector Runtime = W->Vec(TEXT("world.town.runtime.colors.onAir"), FVector(1, 1, 1));
		const FLinearColor On(Runtime.X, Runtime.Y, Runtime.Z, 1);
		for (const FString& Path : W->PathsUnder(TEXT("world.town.runtime.panels.onAir.")))
			for (UMaterialInstanceDynamic* Mid : W->NodeMids(W->Obj(Path).I)) CaptureColor(Mid, On);
		return Paint.Num() > 0;
	}
	FloodSet = W->FloodSet(TEXT("town"));
	if (FloodSet < 0 || W->FloodScaleOf(FloodSet) <= 0) return false;
	double Nearest = 50 * 50;
	for (int32 Slot = 1; Slot <= 7; ++Slot)
	{
		FVector P; float Width = 0; FLinearColor Color;
		if (!W->FloodPool(FloodSet, Slot, P, Width, Color) || Width <= .01f || W->FloodSkip(FloodSet, Slot)) continue;
		const double D = FMath::Square(P.X - At.X) + FMath::Square(P.Z - At.Z);
		if (D >= Nearest) continue;
		Nearest = D; FloodSlot = Slot; OriginalWidth = AppliedWidth = Width;
	}
	if (FloodSlot < 0) return false;
	const int32 LampIndex = FloodSlot - 1, Quad = (LampIndex + 4) * 4;
	const FString PanelPath = TEXT("world.town.runtime.panels.lamps.") + FString::FromInt(Quad);
	for (UMaterialInstanceDynamic* Mid : W->NodeMids(W->Obj(PanelPath).I)) CaptureColor(Mid);
	for (int32 Glow : { 2 + LampIndex * 2, 3 + LampIndex * 2 })
		for (UMaterialInstanceDynamic* Mid : W->NodeMids(W->Obj(FString::Printf(TEXT("world.town.runtime.glows[%d]"), Glow)).I)) CaptureColor(Mid);
	return true; // the existing native point light follows this live town flood slot
}

double FS47AnomalyDread::LightLevel(int32 Kind, double T)
{
	if (Kind == 3) return T < .26 ? 1.0 : 0.0; // one short, wrong ON AIR blink
	if (T < .08) return .12;
	if (T < .22) return 1.0;
	if (T < .42) return .07;
	return 1.0;
}

void FS47AnomalyDread::ApplyLight(double T)
{
	const double K = LightLevel(ActiveKind, T);
	if (AS47Web* W = Web.Get(); W && FloodSet >= 0 && FloodSlot >= 0)
	{
		// Another owner (for example scheduled dawn or the existing light wave) wins.
		FVector P; float Width = 0; FLinearColor C;
		if (!W->FloodPool(FloodSet, FloodSlot, P, Width, C) || (!FMath::IsNearlyEqual(Width, AppliedWidth, .0001f) && !FMath::IsNearlyEqual(Width, OriginalWidth, .0001f))) { Reset(); return; }
		AppliedWidth = OriginalWidth * float(K); W->SetFlood(FloodSet, FloodSlot, AppliedWidth);
	}
	for (FPaint& P : Paint) if (UMaterialInstanceDynamic* Mid = P.Mid.Get())
	{
		P.Applied = P.On * float(K); P.Applied.A = P.Original.A;
		Mid->SetVectorParameterValue(TEXT("Color"), P.Applied);
	}
}

void FS47AnomalyDread::StartAudio(const FVector& At)
{
	US47Main* Main = M.Get(); US47Audio* A = Main ? Main->Audio.Get() : nullptr;
	if (!A || !A->Ctx || !A->Amb || !A->Master || !A->SfxBus) return;
	Context = A->Ctx; AmbientBus = A->Amb; MasterBus = A->Master;
	FS47AudioContext::FBatch Batch(*Context);
	const double Now = Context->CurrentTime();
	// A private multiplier is spliced into only the ambience bus. Its original gain,
	// automation, radio, character voices and master settings are never changed.
	Duck = Context->CreateGain(); Duck->Gain.SetValueAtTime(1, Now);
	Duck->Gain.LinearRampToValueAtTime(.24, Now + .055);
	Duck->Gain.SetValueAtTime(.24, Now + .72);
	Duck->Gain.LinearRampToValueAtTime(1, Now + 1.45);
	AmbientBus->Disconnect(MasterBus); AmbientBus->Connect(Duck); Duck->Connect(MasterBus);
	Cue = Context->CreateGain(); Cue->Gain.SetValue(1);
	const auto Spatial = A->Spatial(At, 2.f, A->SfxBus);
	Cue->Connect(Spatial ? Spatial : A->SfxBus);
	if (Spatial) CueNodes.Add(Spatial);
	// Thin beating tones and a dry filtered-noise scrape, from the authored place.
	// No speech, borrowed recording, generated sample or global stop is involved.
	for (int32 I = 0; I < 2; ++I)
	{
		auto Osc = Context->CreateOscillator(); Osc->Type = ES47OscType::Sine;
		Osc->Frequency.SetValueAtTime(I == 0 ? 122.0 : 124.7, Now);
		Osc->Frequency.LinearRampToValueAtTime(I == 0 ? 118.0 : 120.2, Now + 1.3);
		auto Env = Context->CreateGain(); Env->Gain.SetValueAtTime(0, Now);
		Env->Gain.LinearRampToValueAtTime(I == 0 ? .035 : .014, Now + .11);
		Env->Gain.LinearRampToValueAtTime(I == 0 ? .022 : .009, Now + .68);
		Env->Gain.LinearRampToValueAtTime(0, Now + 1.45);
		Osc->Connect(Env); Env->Connect(Cue); Osc->Start(Now); Osc->Stop(Now + 1.5);
		Sources.Add(Osc); CueNodes.Add(Env);
	}
	if (const auto Noise = A->NoiseBuffer())
	{
		auto Source = Context->CreateBufferSource(); Source->Buffer = Noise; Source->bLoop = true;
		auto Band = Context->CreateBiquadFilter(); Band->Type = ES47BiquadType::Bandpass;
		Band->Frequency.SetValueAtTime(1460, Now); Band->Frequency.LinearRampToValueAtTime(720, Now + .65); Band->Q.SetValue(1.3);
		auto Paper = Context->CreateGain(); Paper->Gain.SetValueAtTime(0, Now);
		Paper->Gain.LinearRampToValueAtTime(.085, Now + .025); Paper->Gain.LinearRampToValueAtTime(.006, Now + .19);
		Paper->Gain.SetValueAtTime(.006, Now + .29); Paper->Gain.LinearRampToValueAtTime(.045, Now + .315); Paper->Gain.LinearRampToValueAtTime(0, Now + .52);
		auto Low = Context->CreateBiquadFilter(); Low->Type = ES47BiquadType::Lowpass; Low->Frequency.SetValue(320);
		auto Wind = Context->CreateGain(); Wind->Gain.SetValueAtTime(0, Now);
		Wind->Gain.LinearRampToValueAtTime(.07, Now + .28); Wind->Gain.LinearRampToValueAtTime(0, Now + 1.35);
		Source->Connect(Band); Band->Connect(Paper); Paper->Connect(Cue);
		Source->Connect(Low); Low->Connect(Wind); Wind->Connect(Cue);
		Source->Start(Now); Source->Stop(Now + 1.5);
		Sources.Add(Source); CueNodes.Append({ Band, Paper, Low, Wind });
	}
}

bool FS47AnomalyDread::Pulse(FVector At, int32 Kind)
{
	US47Main* Main = M.Get();
	if (bActive || !Main || !Main->Story || !Main->World || !Main->Web) return false;
	const bool bHere = (Main->World->Area == TEXT("diner") || Main->World->Area == TEXT("road")) && !Main->World->bDriving;
	if (!MayBegin(Main->Story->Clock(), Protected(true), bHere)) return false;
	const double Day = S47Timeline::Day(Main->Story->Clock());
	if (Kind == 3 && (Day < 4 * 3600 || Day >= 12 * 3600)) return false;
	Web = Main->Web; ActiveKind = Kind;
	if (!SelectLight(At, Kind)) { Reset(); return false; }
	Area = Main->World->Area; Started = Main->Story->Clock(); Elapsed = 0; bActive = bFirstFrame = true;
	ApplyLight(0); StartAudio(At);
	return true;
}

void FS47AnomalyDread::Step(double Dt)
{
	if (!bActive) return;
	US47Main* Main = M.Get();
	if (!Main || !Main->Story || !Main->World || Main->World->Area != Area || Main->World->bDriving
		|| S47Timeline::Night(Main->Story->Clock()) < 0 || FMath::Abs(Main->Story->Clock() - Started) > 15 || Protected(false))
	{
		Reset(); return;
	}
	// Pulse is called before this frame's world overlay. Keep one real render of
	// the initial light state even when that first frame has a long Dt.
	if (bFirstFrame) { bFirstFrame = false; ApplyLight(0); return; }
	// Like the synth graph, this short cue expires even if the caller opens a panel.
	Elapsed += FMath::Max(0., Dt);
	if (Elapsed >= LENGTH) { Reset(); return; }
	ApplyLight(Elapsed);
}

void FS47AnomalyDread::Reset()
{
	if (Context)
	{
		FS47AudioContext::FBatch Batch(*Context);
		for (const auto& Source : Sources) if (Source) { Source->Stop(); Source->Disconnect(); }
		for (const auto& Node : CueNodes) if (Node) Node->Disconnect();
		if (Cue) Cue->Disconnect();
		if (AmbientBus && Duck && MasterBus)
		{
			AmbientBus->Disconnect(Duck); Duck->Disconnect(MasterBus); AmbientBus->Connect(MasterBus);
		}
	}
	// Restore only values still owned by this beat. A world update that has already
	// supplied a new scene's value is left alone.
	AS47Web* W = Web.Get();
	if (W && FloodSet >= 0 && FloodSlot >= 0)
	{
		FVector P; float Width = 0; FLinearColor C;
		if (W->FloodPool(FloodSet, FloodSlot, P, Width, C) && FMath::IsNearlyEqual(Width, AppliedWidth, .0001f)) W->SetFlood(FloodSet, FloodSlot, OriginalWidth);
	}
	const US47Main* Main = M.Get();
	const bool bClosedStudio = Main && Main->Story && S47Timeline::Day(Main->Story->Clock()) >= 4 * 3600 && S47Timeline::Day(Main->Story->Clock()) < 12 * 3600;
	for (const FPaint& P : Paint) if (UMaterialInstanceDynamic* Mid = P.Mid.Get())
	{
		FLinearColor Now;
		if (ActiveKind == 3 && !bClosedStudio) continue;
		if (Mid->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Now) && Now.Equals(P.Applied, .0001f)) Mid->SetVectorParameterValue(TEXT("Color"), P.Original);
	}
	Sources.Reset(); CueNodes.Reset(); Cue.Reset(); Duck.Reset(); AmbientBus.Reset(); MasterBus.Reset(); Context.Reset();
	Paint.Reset(); Web.Reset(); Area.Empty();
	FloodSet = FloodSlot = -1; OriginalWidth = AppliedWidth = 0; ActiveKind = 0; Started = Elapsed = 0; bActive = bFirstFrame = false;
}

bool FS47AnomalyDread::SelfTest(TArray<FString>& Lines)
{
	bool Ok = !MayBegin(-6 * 3600, false, true) && !MayBegin(-1, false, true) && MayBegin(1, false, true);
	Ok &= !MayBegin(4 * 3600, true, true) && !MayBegin(4 * 3600, false, false);
	Ok &= LightLevel(3, 0) == 1 && LightLevel(3, .27) == 0 && LightLevel(1, .1) == 1 && LightLevel(1, 1.6) == 1;
	Lines.Add(FString::Printf(TEXT("%s AnomalyDread: no evening/modal/major beat, bounded street dip and one closed-studio blink"), Ok ? TEXT("PASS") : TEXT("FAIL")));
	return Ok;
}
