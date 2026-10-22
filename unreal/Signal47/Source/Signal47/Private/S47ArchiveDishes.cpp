// story/archiveDishes.ts
#include "S47ArchiveDishes.h"

#include "S47Audio.h"
#include "S47DishArray.h"
#include "S47Web.h"
#include "S47WebAudio.h"

bool FS47ArrayTip::Tip()
{
	if (Holds() || Taken()) return false;
	// for (const x of dishes) { x.rogue = true; x.point(TIP.az, TIP.el, SLEW); }
	if (AS47Web* W = D.Web.Get()) W->PointDishes(TIP_AZ, TIP_EL, SLEW);
	// this.common = this.d.dishes[0]?.phase ?? 0
	const FS47DishArray* A = D.Array ? D.Array() : nullptr;
	Common = A && A->Dishes.Num() ? A->Dishes[0].Phase : 0;
	Mode = TEXT("tipping");
	Start();
	return true;
}

bool FS47ArrayTip::Raise()
{
	if (Mode != TEXT("down")) return false;
	if (AS47Web* W = D.Web.Get()) W->PointDishes(HOME_AZ, HOME_EL, SLEW);
	Mode = TEXT("raising");
	Start();
	return true;
}

void FS47ArrayTip::Home()
{
	if (!Holds()) return;
	Release();
	Quiet(0.05);
}

void FS47ArrayTip::Restore(bool bTipped)
{
	Quiet(0.01);
	if (Holds()) Release();
	if (!bTipped) return;
	// for (const x of dishes) { x.rogue = true; x.snap(TIP.az, TIP.el); }
	if (AS47Web* W = D.Web.Get()) W->SnapDishes(TIP_AZ, TIP_EL);
	const FS47DishArray* A = D.Array ? D.Array() : nullptr;
	Common = A && A->Dishes.Num() ? A->Dishes[0].Phase : 0;
	Mode = TEXT("down");
	Beacons(true);
}

void FS47ArrayTip::Reset()
{
	Quiet(0.01);
	if (Holds()) Release();
}

FString FS47ArrayTip::Update(double Dt)
{
	(void)Dt;
	US47Audio* A = D.Audio.Get();
	if (Bed && A && A->Ctx.IsValid())
	{
		FS47AudioContext::FBatch B(*A->Ctx);
		const double Now = A->Ctx->CurrentTime();
		Bed->Lp->Frequency.SetTargetAtTime(bMuffled ? 260 : 820, Now, 0.4);
		Bed->Whine->Gain.SetTargetAtTime(bMuffled ? 0.0 : 0.05, Now, 0.4);
	}
	if (!Moving()) return FString();
	if (Mode == TEXT("tipping")) Beacons(true);
	const AS47Web* W = D.Web.Get();
	if (W && W->DishesMoving()) return FString();
	// all there: every brake at once, and the motors stop
	Brakes(false);
	Quiet(0.5);
	if (Mode == TEXT("tipping")) { Mode = TEXT("down"); return TEXT("down"); }
	Release();
	return TEXT("home");
}

// ---------- the dishes ----------

void FS47ArrayTip::Release()
{
	// dishes.forEach((x, i) => { x.snap(HOME.az, HOME.el); x.rogue = false; x.spin = 0; x.slew = 6; array.beaconPhase(i, x.phase); })
	if (AS47Web* W = D.Web.Get())
	{
		W->SnapDishes(HOME_AZ, HOME_EL);
		W->PointDishes(HOME_AZ, HOME_EL, 6); // (x.slew = 6)
	}
	if (FS47DishArray* A = D.Array ? D.Array() : nullptr) for (int32 I = 0; I < A->Dishes.Num(); ++I) A->BeaconPhase(I, A->Dishes[I].Phase);
	Mode = TEXT("idle");
}

void FS47ArrayTip::Beacons(bool bOn)
{
	// the red lamps of the tipped array blink as one
	// this.d.dishes.forEach((x, i) => this.d.array.beaconPhase(i, on ? this.common : x.phase))
	if (FS47DishArray* A = D.Array ? D.Array() : nullptr) for (int32 I = 0; I < A->Dishes.Num(); ++I) A->BeaconPhase(I, bOn ? Common : A->Dishes[I].Phase);
}

// ---------- the sound ----------

TArray<int32> FS47ArrayTip::Near(int32 N) const
{
	// the nearest dishes to SARO's buildings, where the brakes are heard from (the first rows)
	TArray<int32> Out;
	const AS47Web* W = D.Web.Get();
	if (!W) return Out;
	for (int32 I = 0; I < W->DishCount(); ++I) Out.Add(I);
	Out.StableSort([W](int32 A, int32 B) { return FMath::Sqrt(FMath::Square(W->DishX(A)) + FMath::Square(W->DishZ(A))) < FMath::Sqrt(FMath::Square(W->DishX(B)) + FMath::Square(W->DishZ(B))); });
	if (Out.Num() > N) Out.SetNum(N);
	return Out;
}

void FS47ArrayTip::Brakes(bool bStart)
{
	// every brake letting go (or setting) within a few hundredths of a second: one ragged clunk
	US47Audio* A = D.Audio.Get();
	const AS47Web* W = D.Web.Get();
	if (!A || !A->Ctx.IsValid() || !W) return;
	FS47AudioContext::FBatch B(*A->Ctx);
	const FString Sw = A->Or(TEXT("rack_switch"), TEXT("switch")), Brake = A->Or(TEXT("door_heavy"), TEXT("doorMetal"));
	const TArray<int32> Near4 = Near();
	for (int32 i = 0; i < Near4.Num(); ++i)
	{
		const int32 X = Near4[i];
		const TSharedPtr<FS47AudioNode> Sp = A->Spatial(FVector(W->DishX(X), 6 * W->DishScale(X), W->DishZ(X)), 9);
		if (!Sp.IsValid()) continue;
		const float Wn = i * 0.013f + FMath::FRand() * 0.02f, K = bMuffled ? 0.7f : 1.f;
		FS47Sound S1; S1.DestNode = Sp; S1.Gain = (bStart ? 0.55f : 0.45f) * K; S1.Rate = bStart ? 0.48f : 0.42f; S1.Lp = bMuffled ? 700.f : 1300.f; S1.When = Wn;
		A->Sfx(Brake, S1);
		FS47Sound S2; S2.DestNode = Sp; S2.Gain = 0.4f * K; S2.Rate = 0.55f; S2.Lp = bMuffled ? 900.f : 0.f; S2.When = Wn + 0.06f;
		A->Sfx(Sw, S2);
	}
	// the rest of the array, further off, as one dull knock
	FS47Sound T; T.Gain = bMuffled ? 0.5f : 0.35f; T.Rate = 0.4f; T.Lp = 600.f; T.When = 0.05f;
	A->Sfx(TEXT("thudSoft"), T);
}

void FS47ArrayTip::Start()
{
	Brakes(true);
	US47Audio* A = D.Audio.Get();
	if (!A || !A->Ctx.IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch B(*Ctx);
	if (!Bed)
	{
		// 27 drives on one bus, a little apart in speed: the beat between them is the array
		auto Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(bMuffled ? 260 : 820); Lp->Q.SetValue(0.7);
		auto G = Ctx->CreateGain(); G->Gain.SetValue(0);
		auto Whine = Ctx->CreateGain(); Whine->Gain.SetValue(0);
		Lp->Connect(G); G->Connect(A->Amb); Whine->Connect(G);
		FBed Nb;
		const double F[4] = { 46, 51.5, 57, 61.8 }, Lv[4] = { 0.5, 0.35, 0.3, 0.22 };
		for (int32 i = 0; i < 4; ++i)
		{
			auto O = Ctx->CreateOscillator(); O->SetType(TEXT("sawtooth")); O->Frequency.SetValue(F[i] * 0.6);
			auto Og = Ctx->CreateGain(); Og->Gain.SetValue(Lv[i]); O->Connect(Og); Og->Connect(Lp); O->Start();
			O->Frequency.SetTargetAtTime(F[i], Ctx->CurrentTime(), 1.4);
			Nb.Osc.Add(O);
		}
		// the gears, heard outdoors only
		auto Gear = Ctx->CreateOscillator(); Gear->SetType(TEXT("square")); Gear->Frequency.SetValue(236);
		auto Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(236); Bp->Q.SetValue(6);
		Gear->Connect(Bp); Bp->Connect(Whine); Gear->Start(); Nb.Osc.Add(Gear);
		Nb.Lp = Lp; Nb.G = G; Nb.Whine = Whine;
		Bed = Nb;
	}
	Bed->G->Gain.SetTargetAtTime(0.16, Ctx->CurrentTime(), 0.9);
}

void FS47ArrayTip::Quiet(double Tau)
{
	TOptional<FBed> Bd = MoveTemp(Bed);
	Bed.Reset();
	US47Audio* A = D.Audio.Get();
	if (!Bd || !A || !A->Ctx.IsValid()) return;
	const TSharedRef<FS47AudioContext> Ctx = A->Ctx.ToSharedRef();
	FS47AudioContext::FBatch B(*Ctx);
	const double T = Ctx->CurrentTime();
	Bd->G->Gain.CancelScheduledValues(T); Bd->G->Gain.SetTargetAtTime(0, T, Tau);
	for (const TSharedPtr<FS47OscillatorNode>& O : Bd->Osc) { O->Frequency.SetTargetAtTime(O->Frequency.Value() * 0.5, T, Tau * 1.6); O->Stop(T + Tau * 8 + 0.1); }
	TSharedPtr<FS47GainNode> G = Bd->G;
	Ctx->SetTimeout(Tau * 8 + 0.5, [G] { if (G.IsValid()) G->Disconnect(); });
}

bool FS47ArrayTip::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s archive dishes: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// without the world: it takes the array, tips it, and gives it back
	bool bRogue = false;
	FS47ArrayTipDeps Dp;
	Dp.Rogue = [&bRogue] { return bRogue; };
	FS47ArrayTip T(Dp);
	bRogue = true;
	Check(T.Taken() && !T.Tip() && T.Mode == TEXT("idle"), TEXT("not while something else drives a dish"));
	bRogue = false;
	Check(T.Tip() && T.Holds() && T.Moving() && !T.Down() && !T.Taken(), TEXT("tipping holds the dishes"));
	Check(T.Update(0.1) == TEXT("down") && T.Down() && !T.Moving(), TEXT("down when they have stopped"));
	Check(!T.Tip() && T.Raise() && T.Mode == TEXT("raising"), TEXT("raised from down, not tipped twice"));
	Check(T.Update(0.1) == TEXT("home") && !T.Holds(), TEXT("given back at home"));
	T.Restore(true);
	Check(T.Down(), TEXT("a night put back tipped"));
	T.Home();
	Check(!T.Holds(), TEXT("home at once"));
	return bOk;
}
