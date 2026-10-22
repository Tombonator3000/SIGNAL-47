// story/dishesFollow.ts
#include "S47DishesFollow.h"

#include "S47Audio.h"
#include "S47DishArray.h"
#include "S47Docs.h"
#include "S47WebAudio.h"

using namespace S47Follow;

namespace
{
	/** The elevation axle's height at scale 1 (Dish.matrices): where the boresight turns about. */
	const double AXLE = 12.4;
	const double DEG = 180 / UE_DOUBLE_PI;
	// how they move: the nearest unmistakable, further out later and slower, far out late
	const FS47DishesFollow::FRate RATE[3] = { { 13, 7, 7 }, { 6, 3.5, 3 }, { 3.2, 2, 1.4 } };
	const double AWAY = 1.5;                                  // when the light comes, all at once and faster
	const FS47DishesFollow::FRate BACK = { 2.2, 1.3, 0.6 };   // home again at the pace of 02:15 (Prologue.turnDishes)
	const double NEAR = 55, MID = 120, FAR = 260;             // metres from the player
	const int32 NEAR_MAX = 3;                                 // the unmistakable ones, at most (one more as the player goes among them)
	const double HOLD = 5.5, WATCH = 7;                       // seconds they stay on the place before they go back
	const double FENCE_Z = -20;                               // world/arrayLayout.ts

	double Wrap180(double A) { return FMath::Fmod(A + 540, 360.0) - 180; }
	double Norm(double A) { return FMath::Fmod(FMath::Fmod(A, 360.0) + 360, 360.0); }
	double WrapPi(double A) { return FMath::Atan2(FMath::Sin(A), FMath::Cos(A)); }
	double Rnd(double A, double B) { return A + FMath::FRand() * (B - A); }
	double JsRound(double V) { return FMath::FloorToDouble(V + 0.5); }
	const TCHAR* WORDS[] = { TEXT("no"), TEXT("one"), TEXT("two"), TEXT("three"), TEXT("four"), TEXT("five"), TEXT("six"), TEXT("seven"), TEXT("eight"), TEXT("nine"), TEXT("ten"), TEXT("eleven"), TEXT("twelve") };
	FString Word(int32 N) { return N >= 0 && N < 13 ? FString(WORDS[N]) : FString::FromInt(N); }
	FString List(const TArray<FString>& Ids)
	{
		if (Ids.Num() <= 1) return FString::Join(Ids, TEXT(""));
		TArray<FString> Head = Ids; Head.Pop();
		return FString::Printf(TEXT("%s and %s"), *FString::Join(Head, TEXT(", ")), *Ids.Last());
	}
	/** One axis of a drive: up to vmax, accelerating and braking at acc, stopping on the target. */
	double Axis(double V, double Err, double VMax, double Acc, double Dt)
	{
		const double Want = FMath::Sign(Err) * FMath::Min(VMax, FMath::Sqrt(2 * Acc * FMath::Abs(Err)) * 0.92);
		// it gathers speed at acc, a heavy thing; it brakes harder than that
		const bool bFaster = FMath::Abs(Want) > FMath::Abs(V) && Want * V >= 0;
		const double Lim = (bFaster ? 1 : 1.8) * Acc * Dt;
		return V + FMath::Clamp(Want - V, -Lim, Lim);
	}
	/** Where a dish has to point to look at p (degrees), and how far it is on the ground. */
	struct FAim { double Az, El, H; };
	FAim AimAt(const FS47Dish& D, const FVector& P)
	{
		const double Dx = P.X - D.X, Dz = P.Z - D.Z, Dy = P.Y - AXLE * D.Scale, H = FMath::Sqrt(Dx * Dx + Dz * Dz);
		return { Norm(FMath::Atan2(Dx, -Dz) * DEG), FMath::Atan2(Dy, FMath::Max(0.5, H)) * DEG, H };
	}
	/** Where a dish is from the player's view, in words. */
	FString Where(const FVector& Eye, const FVector& View, const FS47Dish& D)
	{
		const double Fx = View.X, Fz = View.Z, Dx = D.X - Eye.X, Dz = D.Z - Eye.Z;
		double F = FMath::Sqrt(Fx * Fx + Fz * Fz), H = FMath::Sqrt(Dx * Dx + Dz * Dz);
		if (F == 0) F = 1;
		if (H == 0) H = 1;
		const double Cos = (Fx * Dx + Fz * Dz) / (F * H), Side = Fx * Dz - Fz * Dx;
		return Cos > 0.64 ? TEXT("Ahead of me") : Cos < -0.5 ? TEXT("Behind me") : Side > 0 ? TEXT("To my right") : TEXT("To my left");
	}
}

// ---------- the sound of the drives ----------
// The motor of 02:15 (AudioSys.startMotors: a sawtooth low down, through a low-pass), placed at the
// dish and sped up and slowed with it, with the whine of its gears over it; a contactor and a
// brake each way (the bought switch and heavy door from the private repo, pitched down, or the
// game's own when they are not there). Three voices for the nearest that move; the rest of the
// array that moves is one bed, not placed, as it was heard from the control room at 02:15.
class FS47DriveVoice
{
public:
	TSharedPtr<FS47DishesFollow::FRig> Rig;
	double Level = 0;

	FS47DriveVoice(const TSharedRef<FS47AudioContext>& InCtx, const TSharedPtr<FS47AudioNode>& Out) : Ctx(InCtx)
	{
		FS47AudioContext::FBatch B(*Ctx);
		Saw = Ctx->CreateOscillator(); Saw->SetType(TEXT("sawtooth")); Saw->Frequency.SetValue(34);
		Lp = Ctx->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(300); Lp->Q.SetValue(0.8);
		Gear = Ctx->CreateOscillator(); Gear->SetType(TEXT("square")); Gear->Frequency.SetValue(156);
		Bp = Ctx->CreateBiquadFilter(); Bp->SetType(TEXT("bandpass")); Bp->Frequency.SetValue(156); Bp->Q.SetValue(7);
		const TSharedRef<FS47GainNode> Gg = Ctx->CreateGain(); Gg->Gain.SetValue(0.2);
		Lfo = Ctx->CreateOscillator(); Lfo->Frequency.SetValue(5.3);
		const TSharedRef<FS47GainNode> Depth = Ctx->CreateGain(); Depth->Gain.SetValue(3);
		G = Ctx->CreateGain(); G->Gain.SetValue(0);
		const TSharedRef<FS47PannerNode> P = Ctx->CreatePanner();
		P->PanningModel = TEXT("equalpower"); P->SetDistanceModel(TEXT("inverse")); P->RefDistance = 7; P->RolloffFactor = 1; P->MaxDistance = 400;
		Pan = P;
		Saw->Connect(Lp); Lp->Connect(G);
		Gear->Connect(Bp); Bp->Connect(Gg); Gg->Connect(G);
		Lfo->Connect(Depth); Depth->Connect(Gear->Frequency);
		G->Connect(P); P->Connect(Out);
		Saw->Start(); Gear->Start(); Lfo->Start();
	}
	void Set(const TSharedPtr<FS47DishesFollow::FRig>& R, const FS47Dish* D, double T)
	{
		Rig = R;
		FS47AudioContext::FBatch B(*Ctx);
		const double Now = Ctx->CurrentTime();
		if (!R.IsValid() || !D) { Level = 0; G->Gain.SetTargetAtTime(0, Now, 0.25); return; }
		const double Y = 8 * D->Scale;
		Pan->PositionX.SetValue(D->X); Pan->PositionY.SetValue(Y); Pan->PositionZ.SetValue(D->Z);
		const double K = FMath::Clamp((FMath::Abs(R->VAz) + FMath::Abs(R->VEl)) / (R->Rate.Az * 0.75), 0.0, 1.0);
		const bool bStrain = R->Push > 0;
		const double F = bStrain ? 24 : 30 + 30 * K;
		Saw->Frequency.SetTargetAtTime(F, Now, 0.18);
		Gear->Frequency.SetTargetAtTime(F * 4.6, Now, 0.18); Bp->Frequency.SetTargetAtTime(F * 4.6, Now, 0.18);
		Lp->Frequency.SetTargetAtTime(bStrain ? 520 : 220 + 700 * K, Now, 0.2);
		// pushing against the limit: low, louder, juddering
		// a drive that runs is heard even when it barely turns (the hum of it; the pitch goes with the speed)
		Level = bStrain ? 0.34 * (0.55 + 0.45 * FMath::Sign(FMath::Sin(T * UE_DOUBLE_PI * 2 * 7))) : 0.46 * (0.45 + 0.55 * FMath::Pow(K, 0.7)) * (R->Tier == 0 ? 1 : 0.7);
		G->Gain.SetTargetAtTime(Level, Now, bStrain ? 0.012 : 0.12);
	}
	void Mute(double T) { Rig.Reset(); Level = 0; FS47AudioContext::FBatch B(*Ctx); G->Gain.CancelScheduledValues(T); G->Gain.SetValueAtTime(0, T); }
	void Stop()
	{
		FS47AudioContext::FBatch B(*Ctx);
		const double Now = Ctx->CurrentTime();
		G->Gain.CancelScheduledValues(Now); G->Gain.SetTargetAtTime(0, Now, 0.15);
		for (const TSharedPtr<FS47OscillatorNode>& O : { Saw, Gear, Lfo }) O->Stop(Now + 1);
		TSharedPtr<FS47GainNode> Gk = G;
		Ctx->SetTimeout(1.2, [Gk] { Gk->Disconnect(); });
	}

private:
	TSharedRef<FS47AudioContext> Ctx;
	TSharedPtr<FS47GainNode> G;
	TSharedPtr<FS47PannerNode> Pan;
	TSharedPtr<FS47OscillatorNode> Saw, Gear, Lfo;
	TSharedPtr<FS47BiquadFilterNode> Lp, Bp;
};

class FS47Drives
{
public:
	explicit FS47Drives(TWeakObjectPtr<US47Audio> InAudio, FS47DishArray* InDishes) : Audio(InAudio), Dishes(InDishes) {}
	TArray<TSharedPtr<FS47DriveVoice>> VoicesL;
	struct FBed { TSharedPtr<FS47OscillatorNode> Osc; TSharedPtr<FS47GainNode> G; };
	TOptional<FBed> Bed;
	double Pick = 0, Tick = 0, Time = 0;

	TSharedPtr<FS47AudioContext> Ctx() const { return Audio.IsValid() ? Audio->Ctx : nullptr; }
	const FS47Dish* DishOf(const TSharedPtr<FS47DishesFollow::FRig>& R) const { return R.IsValid() && Dishes && Dishes->Dishes.IsValidIndex(R->I) ? &Dishes->Dishes[R->I] : nullptr; }

	void Open()
	{
		const TSharedPtr<FS47AudioContext> C = Ctx();
		if (!C.IsValid() || VoicesL.Num()) return;
		for (int32 I = 0; I < 3; ++I) VoicesL.Add(MakeShared<FS47DriveVoice>(C.ToSharedRef(), Audio->SfxBus));
		// the rest of the array, as at 02:15
		FS47AudioContext::FBatch B(*C);
		const TSharedRef<FS47OscillatorNode> Osc = C->CreateOscillator(); Osc->SetType(TEXT("sawtooth")); Osc->Frequency.SetValue(58);
		const TSharedRef<FS47BiquadFilterNode> Lp = C->CreateBiquadFilter(); Lp->SetType(TEXT("lowpass")); Lp->Frequency.SetValue(240);
		const TSharedRef<FS47GainNode> G = C->CreateGain(); G->Gain.SetValue(0);
		Osc->Connect(Lp); Lp->Connect(G); G->Connect(Audio->Amb); Osc->Start();
		Bed = FBed{ Osc, G };
	}
	/** Quiet for now, fading (the night held still under the notebook); the next update brings them back. */
	void Rest()
	{
		const TSharedPtr<FS47AudioContext> C = Ctx();
		const double T = C.IsValid() ? C->CurrentTime() : 0;
		for (const TSharedPtr<FS47DriveVoice>& V : VoicesL) V->Set(nullptr, nullptr, Time);
		if (Bed && C.IsValid()) { FS47AudioContext::FBatch B(*C); Bed->G->Gain.SetTargetAtTime(0, T, 0.3); }
		Tick = 0;
	}
	/** Silent at once (the sound back after the time away). */
	void Hush()
	{
		const TSharedPtr<FS47AudioContext> C = Ctx();
		const double T = C.IsValid() ? C->CurrentTime() : 0;
		for (const TSharedPtr<FS47DriveVoice>& V : VoicesL) V->Mute(T);
		if (Bed && C.IsValid()) { FS47AudioContext::FBatch B(*C); Bed->G->Gain.CancelScheduledValues(T); Bed->G->Gain.SetValueAtTime(0, T); }
	}
	void Close()
	{
		for (const TSharedPtr<FS47DriveVoice>& V : VoicesL) V->Stop();
		VoicesL.Reset();
		const TSharedPtr<FS47AudioContext> C = Ctx();
		if (Bed && C.IsValid())
		{
			FS47AudioContext::FBatch B(*C);
			const double T = C->CurrentTime();
			Bed->G->Gain.CancelScheduledValues(T); Bed->G->Gain.SetTargetAtTime(0, T, 0.4); Bed->Osc->Frequency.SetTargetAtTime(30, T, 0.8);
			Bed->Osc->Stop(T + 3);
		}
		Bed.Reset();
	}
	void Update(double Dt, const TArray<TSharedPtr<FS47DishesFollow::FRig>>& Rigs, const FVector& Eye, bool bNearOnly)
	{
		Time += Dt;
		if (!VoicesL.Num()) { Open(); if (!VoicesL.Num()) return; }
		TArray<TSharedPtr<FS47DishesFollow::FRig>> Moving;
		for (const TSharedPtr<FS47DishesFollow::FRig>& R : Rigs) if (R->bOn && (R->bRun || R->Push > 0)) Moving.Add(R);
		if ((Pick -= Dt) <= 0)
		{
			Pick = 0.3;
			// while they follow, the voices are the near ones' only (standing still, quiet near by); going
			// home, the nearest that move; the rest is the bed
			TArray<TSharedPtr<FS47DishesFollow::FRig>> Near;
			for (const TSharedPtr<FS47DishesFollow::FRig>& R : Moving) if (!bNearOnly || R->Tier == 0) Near.Add(R);
			auto H = [this, &Eye](const TSharedPtr<FS47DishesFollow::FRig>& R) { const FS47Dish* Ds = DishOf(R); return Ds ? FMath::Sqrt(FMath::Square(Ds->X - Eye.X) + FMath::Square(Ds->Z - Eye.Z)) : 1e9; };
			Near.StableSort([&](const TSharedPtr<FS47DishesFollow::FRig>& A, const TSharedPtr<FS47DishesFollow::FRig>& B) { return H(A) < H(B); });
			if (Near.Num() > VoicesL.Num()) Near.SetNum(VoicesL.Num());
			// keep a voice on its dish while that one is still among the nearest
			TArray<TSharedPtr<FS47DriveVoice>> Free;
			for (const TSharedPtr<FS47DriveVoice>& V : VoicesL) if (!V->Rig.IsValid() || !Near.Contains(V->Rig)) Free.Add(V);
			for (const TSharedPtr<FS47DishesFollow::FRig>& R : Near)
			{
				if (VoicesL.ContainsByPredicate([&](const TSharedPtr<FS47DriveVoice>& V) { return V->Rig == R; })) continue;
				if (Free.Num()) { Free[0]->Rig = R; Free.RemoveAt(0); }
			}
			for (const TSharedPtr<FS47DriveVoice>& V : Free) V->Rig.Reset();
		}
		// the parameters twenty times a second is enough (and keeps the automation short)
		if ((Tick -= Dt) > 0) return;
		Tick = 0.05;
		for (const TSharedPtr<FS47DriveVoice>& V : VoicesL)
		{
			const TSharedPtr<FS47DishesFollow::FRig> R = V->Rig.IsValid() && Moving.Contains(V->Rig) ? V->Rig : nullptr;
			V->Set(R, DishOf(R), Time);
		}
		const TSharedPtr<FS47AudioContext> C = Ctx();
		if (Bed && C.IsValid())
		{
			double Rest = 0;
			for (const TSharedPtr<FS47DishesFollow::FRig>& R : Moving) if (!VoicesL.ContainsByPredicate([&](const TSharedPtr<FS47DriveVoice>& V) { return V->Rig == R; })) Rest += FMath::Abs(R->VAz) + FMath::Abs(R->VEl);
			FS47AudioContext::FBatch B(*C);
			const double Now = C->CurrentTime();
			// further off than the near ones: lower than the whole array was from the control room at 02:15
			Bed->G->Gain.SetTargetAtTime(0.07 * FMath::Min(1.0, Rest / 12), Now, 0.6);
			Bed->Osc->Frequency.SetTargetAtTime(46 + 12 * FMath::Min(1.0, Rest / 12), Now, 1.2);
		}
	}
	/** A contactor and the brake: letting go (start) or setting (stop), at the dish's foot. */
	void Clunk(const FS47Dish& D, bool bStart, bool bCalm, double When = 0)
	{
		US47Audio* A = Audio.Get();
		if (!A) return;
		const TSharedPtr<FS47AudioNode> Sp = A->Spatial(FVector(D.X, 6 * D.Scale, D.Z), 6);
		if (!Sp.IsValid()) return;
		const double K = bCalm ? 0.6 : 1;
		const FString Sw = A->Or(TEXT("rack_switch"), TEXT("switch")), Brake = A->Or(TEXT("door_heavy"), TEXT("doorMetal"));
		auto Play = [&](const FString& Name, double Gain, double Rate, double Lp, double W)
		{
			FS47Sound O; O.DestNode = Sp; O.Gain = float(Gain); O.Rate = float(Rate); O.Lp = float(Lp); O.When = float(W);
			A->Sfx(Name, O);
		};
		if (bStart) { Play(Sw, 0.5 * K, 0.62, 0, When); Play(Brake, 0.4 * K, 0.5, 1300, When + 0.09); }
		else { Play(Brake, 0.34 * K, 0.45, 900, When); Play(Sw, 0.36 * K, 0.55, 0, When + 0.06); }
	}

private:
	TWeakObjectPtr<US47Audio> Audio;
	FS47DishArray* Dishes = nullptr;
};

// ---------- the case's JSON ----------

S47J::FObj FS47DishesSave::ToJson() const
{
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("clock"), Clock);
	S47J::Set(O, TEXT("ids"), Ids);
	S47J::Set(O, TEXT("back"), bBack);
	return O;
}

TOptional<FS47DishesSave> FS47DishesSave::Copy(const S47J::FObj& O)
{
	if (!O.IsValid() || !S47J::Has(O, TEXT("clock"))) return {};
	FS47DishesSave S;
	S.Clock = S47J::Num(O, TEXT("clock")); S.Ids = S47J::Strs(O, TEXT("ids")); S.bBack = S47J::Bool(O, TEXT("back"));
	return S;
}

// ---------- DishesFollow ----------

FS47DishesFollow::FS47DishesFollow(FS47FollowDeps InD) : D(MoveTemp(InD))
{
	Sound = MakeUnique<FS47Drives>(D.Audio, D.Dishes);
}

FS47DishesFollow::~FS47DishesFollow() = default;

bool FS47DishesFollow::NearArray(const TArray<FS47Area>& FieldZones, const FS47DishArray* Dishes, double X, double Z)
{
	// onField(x, z, 3): z < FENCE_Z && FIELD_ZONES.some(inRect(r, x, z, 3))
	if (Z < FENCE_Z) for (const FS47Area& R : FieldZones) if (R.Contains(X, Z, 3)) return true;
	// nearestAntenna(x, z).d < 45 (ANTENNAS: the dishes, in the same order)
	double Best = 1e18;
	if (Dishes) for (const FS47Dish& Dd : Dishes->Dishes) Best = FMath::Min(Best, FMath::Sqrt(FMath::Square(Dd.X - X) + FMath::Square(Dd.Z - Z)));
	return Best < 45;
}

const FS47Dish& FS47DishesFollow::DishOf(const FRig& R) const { return D.Dishes->Dishes[R.I]; }
FString FS47DishesFollow::IdOf(const FRig& R) const { return D.Dishes ? D.Dishes->Dishes[R.I].Id : FString(); }
TSharedPtr<FS47DishesFollow::FRig> FS47DishesFollow::RigOf(const FString& Id) const
{
	for (const TSharedPtr<FRig>& R : Rigs) if (IdOf(*R) == Id) return R;
	return nullptr;
}

TArray<FS47DishesFollow::FState> FS47DishesFollow::State() const
{
	TArray<FState> Out;
	for (const TSharedPtr<FRig>& R : Rigs) Out.Add({ IdOf(*R), R->Tier, R->bOn, R->OnAt, R->bLocked, R->bEver, R->bRun, R->Az, R->El, R->Aim, R->Push > 0 });
	return Out;
}

TArray<TPair<FString, double>> FS47DishesFollow::Voices() const
{
	TArray<TPair<FString, double>> Out;
	for (const TSharedPtr<FS47DriveVoice>& V : Sound->VoicesL) if (V->Rig.IsValid() && V->Level > 0.01) Out.Add({ IdOf(*V->Rig), V->Level });
	return Out;
}

void FS47DishesFollow::Wait() { if (Mode != TEXT("idle")) Sound->Rest(); }

FS47DishesFollow::FRate FS47DishesFollow::RateFor(int32 I, int32 Tier, const FVector& Eye, TOptional<double> FromAz) const
{
	// how fast a dish may go: the near ones the faster the further round they have to come to the
	// player (on them within about eight seconds of starting), the others at their tier's pace
	const FS47Dish& Dd = D.Dishes->Dishes[I];
	const double Swing = FMath::Abs(Wrap180(AimAt(Dd, Eye).Az - FromAz.Get(D.Dishes->CurAz(I))));
	const FRate& K = RATE[Tier];
	// the near ones on the player within about eight seconds of starting; further out within about
	// twelve, far out about eighteen: the field goes quiet again when the player stands still
	const double Az = Tier == 0 ? FMath::Clamp(Swing / 6, K.Az - 2, 24.0) : Tier == 1 ? FMath::Clamp(Swing / 10, 4.0, 11.0) : FMath::Clamp(Swing / 16, 2.2, 6.0);
	return { Az, K.El, FMath::Max(K.Acc, Az * (Tier == 0 ? 0.7 : 0.5)) };
}

void FS47DishesFollow::Update(double Dt, const FS47FollowIn& S)
{
	if (!D.Dishes) return;
	if (Mode == TEXT("idle"))
	{
		const TOptional<FS47FollowIn::FWarn>& W = S.Warning;
		if (S.Stage == TEXT("warning") && W && !Kept() && S.bSaro && S.bOnFoot && D.NearArray && D.NearArray(S.Eye.X, S.Eye.Z)
			&& W->T >= FirstAt(W->Len) && W->T < 0.8 * W->Len) Begin(S);
		return;
	}
	// out of SARO's area: nobody there to see them go back
	if (!S.bSaro) { End(S.Clock, false); return; }
	T += Dt;
	if (Mode == TEXT("follow"))
	{
		if (S.Stage == TEXT("light")) Away(S);
		else if (S.Stage != TEXT("warning")) HoldPlace(S);
		else Follow(Dt, S);
	}
	else if (Mode == TEXT("hold")) { if ((HoldT -= Dt) <= 0) GoHome(S); }
	else if (Mode == TEXT("away")) { if (S.Stage == TEXT("back")) Woke(S); else if (S.Stage == TEXT("calm")) GoHome(S); }
	else if (Mode == TEXT("watch")) { if (!S.bOver && (HoldT -= Dt) <= 0) GoHome(S); }
	if (Mode == TEXT("idle")) return;   // ended above
	Step(Dt, S);
	if (Mode == TEXT("home"))
	{
		bool bAll = true;
		for (const TSharedPtr<FRig>& R : Rigs) if (R->bOn && !(FMath::Abs(Wrap180(R->Az - HOME_AZ)) < 0.05 && FMath::Abs(R->El - HOME_EL) < 0.05 && FMath::Abs(R->VAz) + FMath::Abs(R->VEl) < 0.05)) { bAll = false; break; }
		if (bAll) End(S.Clock);
	}
}

// ---------- the event ----------

void FS47DishesFollow::Begin(const FS47FollowIn& S)
{
	Mode = TEXT("follow"); T = 0; Clock0 = S.Clock; Len = S.Warning ? S.Warning->Len : 30;
	Rigs.Reset(); Skip.Reset(); Common.Reset(); FirstFar = -1; ChooseT = 1;
	Said = FSaid(); bNoted = false;
	From = S.Eye; bMoved = false;
	Choose(S.Eye, S.Warning ? S.Warning->T : 0);
	if (D.Set) D.Set({ JsRound(S.Clock), {}, false });
	Sound->Open();
	if (D.Save) D.Save();
}

void FS47DishesFollow::Choose(const FVector& Eye, double W0)
{
	// which dishes take part, and when each starts: by how far each is from the player now. w0: how
	// far into the warning it is (the far ones start with the pressure at :31, or soon after if
	// it began later than that)
	struct FBy { int32 I; double H; };
	TArray<FBy> By;
	for (int32 I = 0; I < D.Dishes->Dishes.Num(); ++I) By.Add({ I, FMath::Sqrt(FMath::Square(D.Dishes->Dishes[I].X - Eye.X) + FMath::Square(D.Dishes->Dishes[I].Z - Eye.Z)) });
	By.StableSort([](const FBy& A, const FBy& B) { return A.H < B.H; });
	const double DELAY[3] = { 0, 1.8, 3.2 }, K = FMath::Min(1.0, Len / WARN);
	int32 NearN = 0, MidN = 0;
	for (int32 Rank = 0; Rank < By.Num(); ++Rank)
	{
		const int32 I = By[Rank].I; const double H = By[Rank].H;
		// the nearest is always one of the near ones, even off in a corner of the field
		int32 Tier = (H < NEAR || (Rank == 0 && H < 110)) && NearN < NEAR_MAX ? 0 : H < MID ? 1 : H < FAR ? 2 : -1;
		if (Tier == 2 && FMath::FRand() > 0.45) Tier = -1;   // far out, most never do
		if (Tier == -1) { Skip.Add(D.Dishes->Dishes[I].Id); continue; }
		const double At = Tier == 0 ? DELAY[NearN] : Tier == 1 ? (4 + MidN * 0.8 + Rnd(0, 1.2)) * K : FMath::Max(8 * K, BEAT.Tone * K - W0) + Rnd(0, 6 * K);
		if (Tier == 0) NearN++; else if (Tier == 1) MidN++;
		Rigs.Add(MakeRig(I, Tier, T + At, Eye));
	}
}

void FS47DishesFollow::Promote(const FVector& Eye)
{
	// as the player goes among them: a dish come near is a near one, and soon; one that never was
	// starts when the player comes within reach of it
	auto NearN = [this] { int32 N = 0; for (const TSharedPtr<FRig>& R : Rigs) if (R->Tier == 0) N++; return N; };
	for (int32 I = 0; I < D.Dishes->Dishes.Num(); ++I)
	{
		const FS47Dish& Dd = D.Dishes->Dishes[I];
		const double H = FMath::Sqrt(FMath::Square(Dd.X - Eye.X) + FMath::Square(Dd.Z - Eye.Z));
		const TSharedPtr<FRig> R = RigOf(Dd.Id);
		if (R.IsValid())
		{
			if (R->Tier > 0 && H < NEAR - 15 && NearN() < NEAR_MAX + 1) { R->Tier = 0; R->Rate = RateFor(I, 0, Eye, R->Az); if (!R->bOn) R->At = FMath::Min(R->At, T + 0.6); }
			else if (R->Tier == 2 && H < MID - 10) { R->Tier = 1; R->Rate = RateFor(I, 1, Eye); if (!R->bOn) R->At = FMath::Min(R->At, T + 1.5); }
		}
		else if (H < MID - 10)
		{
			Skip.Remove(Dd.Id);
			const bool bN = H < NEAR - 15 && NearN() < NEAR_MAX + 1;
			Rigs.Add(MakeRig(I, bN ? 0 : 1, T + (bN ? 0.6 : 1 + Rnd(0, 1.5)), Eye));
		}
	}
}

TSharedPtr<FS47DishesFollow::FRig> FS47DishesFollow::MakeRig(int32 I, int32 Tier, double At, const FVector& Eye) const
{
	const TSharedPtr<FRig> R = MakeShared<FRig>();
	R->I = I; R->Tier = Tier; R->At = At; R->Az = Norm(D.Dishes->CurAz(I)); R->El = D.Dishes->CurEl(I); R->Aim = Eye;
	R->Strain = Rnd(1.5, 3); R->Ph = D.Dishes->Dishes[I].Phase; R->Rate = RateFor(I, Tier, Eye);
	return R;
}

void FS47DishesFollow::Follow(double Dt, const FS47FollowIn& S)
{
	if ((ChooseT -= Dt) <= 0) { ChooseT = 1; Promote(S.Eye); }
	if (!bMoved && FMath::Sqrt(FMath::Square(S.Eye.X - From.X) + FMath::Square(S.Eye.Z - From.Z)) > 3) bMoved = true;
	for (const TSharedPtr<FRig>& R : Rigs)
	{
		// (one not started yet starts from where its dish is then: the array may have been moving,
		// story/arrayBack.ts; standing at 026 as it usually is, nothing changes)
		if (!R->bOn && (R->Az != Norm(D.Dishes->CurAz(R->I)) || R->El != D.Dishes->CurEl(R->I))) { R->Az = Norm(D.Dishes->CurAz(R->I)); R->El = D.Dishes->CurEl(R->I); R->Rate = RateFor(R->I, R->Tier, S.Eye, R->Az); }
		if (!R->bOn && T >= R->At) Start(*R, S);
		if (R->bOn) R->Aim = S.Eye;
	}
	// the third thing: once one is on the player, a step away and it comes after
	if (Said.bLock && !Said.bMove && FMath::Sqrt(FMath::Square(S.Eye.X - LockAt.X) + FMath::Square(S.Eye.Z - LockAt.Z)) > 2.5
		&& Rigs.ContainsByPredicate([](const TSharedPtr<FRig>& R) { return R->Tier == 0 && R->bRun; }))
	{
		Said.bMove = true;
		Toast(TEXT("When I move, they move."), 3.2);
	}
}

void FS47DishesFollow::Start(FRig& R, const FS47FollowIn& S)
{
	R.bOn = true; R.OnAt = T;
	if (R.Tier == 0) Log(FString::Printf(TEXT("DRIVE %s  MOTION  NO COMMAND"), *IdOf(R)), S.Clock);
	else if (FirstFar < 0) FirstFar = S.Clock;
	if (R.Tier == 0 && !Said.bStart)
	{
		Said.bStart = true;
		Toast(FString::Printf(TEXT("%s, out in the array, a drive motor starts up. Nobody is at the console."), *Where(S.Eye, S.View, DishOf(R))), 4.2);
	}
}

void FS47DishesFollow::HoldPlace(const FS47FollowIn& S)
{
	// got away: they stay on the place a while
	Mode = TEXT("hold"); HoldT = HOLD;
	// what was seen, when it was seen: that they went back is written once they are back (end)
	Record(TEXT(" When I got away from them they stayed on the place where I had been."));
}

void FS47DishesFollow::Away(const FS47FollowIn& S)
{
	// the light: away from the player all at once, to the grass where the player will wake
	Mode = TEXT("away");
	const FVector At = S.BackAt ? FVector(S.BackAt->X, (D.Ground ? D.Ground(S.BackAt->X, S.BackAt->Z) : 0) + 0.35, S.BackAt->Z) : S.Eye;
	int32 N = 0;
	for (const TSharedPtr<FRig>& R : Rigs)
	{
		if (!R->bOn && R->Tier == 2) continue;
		if (!R->bOn) { R->bOn = true; R->OnAt = T; }
		R->Aim = At; R->bLocked = false; N++;
		// every brake at once, near enough to hear
		if (R->Tier == 0) { R->bRun = true; R->Still = 0; R->ClunkT = T; Sound->Clunk(DishOf(*R), true, S.bCalm, Rnd(0, 0.18)); }
	}
	Record(TEXT(" When the light came they all turned away from me at once, out along the line, to the dark."));
	Log(FString::Printf(TEXT("ARRAY  %d DRIVES  MOTION  NO COMMAND"), N), S.Clock);
}

void FS47DishesFollow::Woke(const FS47FollowIn& S)
{
	// woken: they are pointing down at the place, and have been since the light
	Mode = TEXT("watch"); HoldT = WATCH;
	Sound->Hush();   // the sound comes back with the waking: not their motors from before
	TArray<FString> Ids;
	TSharedPtr<FRig> Watcher; double WatcherH = 1e18;
	for (const TSharedPtr<FRig>& R : Rigs)
	{
		if (!R->bOn) continue;
		R->Aim = S.Eye;
		const FAim A = AimAt(DishOf(*R), S.Eye);
		R->Az = A.Az; R->El = FMath::Max(EL_MIN, A.El); R->VAz = R->VEl = 0; R->bRun = false; R->Still = 0; R->bLocked = true;
		Write(*R, 0);
		if (A.H < 140) Ids.Add(IdOf(*R));
		if (A.H < 80 && (!Watcher.IsValid() || A.H < WatcherH)) { Watcher = R; WatcherH = A.H; }
	}
	// the view lifts from the stars towards the nearest of them, a little to one side (not with one
	// of them over the player: the abduction has turned the player for that, and they are over it)
	if (Watcher.IsValid() && !S.bOver && D.Face) D.Face(FMath::Atan2(-(DishOf(*Watcher).X - S.Eye.X), -(DishOf(*Watcher).Z - S.Eye.Z)) + Rnd(-0.22, 0.22));
	TArray<FString> Three = Ids; if (Three.Num() > 3) Three.SetNum(3);
	const FString Who = Ids.Num() > 3 ? FString::Printf(TEXT("%s and the others"), *FString::Join(Three, TEXT(", "))) : List(Ids);
	if (Ids.Num()) Note(FString::Printf(TEXT("%s. When I woke, %s %s pointing down at the grass where I lay. %s had turned to it when the light came, before I was there."),
		*S47Text::ClockText(S.Woke, false), *Who, Ids.Num() > 1 ? TEXT("were") : TEXT("was"), Ids.Num() > 1 ? TEXT("They") : TEXT("It")));
}

void FS47DishesFollow::GoHome(const FS47FollowIn& S)
{
	Mode = TEXT("home");
	for (const TSharedPtr<FRig>& R : Rigs)
	{
		R->bLocked = false;
		if (R->bOn && R->Tier == 0 && !R->bRun && T - R->ClunkT > 1) { R->bRun = true; R->Still = 0; R->ClunkT = T; Sound->Clunk(DishOf(*R), true, S.bCalm, Rnd(0.1, 0.9)); }
	}
}

void FS47DishesFollow::End(double Clock, bool bSeen)
{
	// (home(): gone from SARO's area, home at once, unseen: end puts each one there)
	TArray<TSharedPtr<FRig>> On;
	for (const TSharedPtr<FRig>& R : Rigs) if (R->bOn) On.Add(R);
	On.StableSort([](const TSharedPtr<FRig>& A, const TSharedPtr<FRig>& B) { return A->OnAt < B->OnAt; });
	for (const TSharedPtr<FRig>& R : Rigs) Release(*R);
	Log(FString::Printf(TEXT("ARRAY  %d DRIVES  AZ 026 EL 32  NO COMMAND"), On.Num()), Clock);
	if (bSeen && bNoted && On.Num()) Note(FString::Printf(TEXT("%s. %s. Nobody sent a command."), *S47Text::ClockText(Clock, false),
		On.Num() == 1 ? *FString::Printf(TEXT("%s is back on 026 by itself"), *IdOf(*On[0])) : TEXT("They are back on 026, every one of them, by themselves")));
	bNoted = false;
	TArray<FString> Ids;
	for (const TSharedPtr<FRig>& R : On) Ids.Add(IdOf(*R));
	if (D.Set) D.Set({ JsRound(Clock0), Ids, true });
	Sound->Close();
	Rigs.Reset(); Mode = TEXT("idle");
	if (D.Save) D.Save();
}

void FS47DishesFollow::Release(FRig& R)
{
	if (!D.Dishes) return;
	D.Dishes->Snap(R.I, HOME_AZ, HOME_EL);
	FS47Dish& Dd = D.Dishes->Dishes[R.I];
	Dd.bRogue = false; Dd.Spin = 0;
	D.Dishes->BeaconPhase(R.I, Dd.Phase);
}

void FS47DishesFollow::Record(const FString& After)
{
	// what the notebook keeps, once the warning is over (and the log's line for the ones further out)
	TArray<TSharedPtr<FRig>> On;
	for (const TSharedPtr<FRig>& R : Rigs) if (R->bOn) On.Add(R);
	On.StableSort([](const TSharedPtr<FRig>& A, const TSharedPtr<FRig>& B) { return A->OnAt < B->OnAt; });
	TArray<FString> Near, Far;
	for (const TSharedPtr<FRig>& R : On) (R->Tier == 0 ? Near : Far).Add(IdOf(*R));
	const int32 More = On.Num() - Near.Num();
	if (Far.Num() && FirstFar >= 0)
	{
		TArray<FString> Five = Far; if (Five.Num() > 5) Five.SetNum(5);
		Log(FString::Printf(TEXT("ARRAY  %s%s  MOTION  NO COMMAND"), *FString::Join(Five, TEXT(" ")), Far.Num() > 5 ? *FString::Printf(TEXT(" +%d"), Far.Num() - 5) : TEXT("")), FirstFar);
	}
	if (!Near.Num()) return;
	bNoted = true;
	const bool bOne = Near.Num() == 1;
	const bool bSaw = On.ContainsByPredicate([](const TSharedPtr<FRig>& R) { return R->bEver; });
	const FString After1 = !More ? FString() : More > 12 ? FString::Printf(TEXT(", and most of the array further out after %s"), bOne ? TEXT("it") : TEXT("them"))
		: FString::Printf(TEXT(", and %s more further out after %s"), *Word(More), bOne ? TEXT("it") : TEXT("them"));
	const TCHAR* They = bOne && !More ? TEXT("it") : TEXT("they");
	Note(FString::Printf(TEXT("%s. Out in the array, with the crickets gone. %s came down out of 026 by %s and %s%s. %s Nobody was at the console.%s"),
		*S47Text::ClockText(Clock0, false), *List(Near), bOne ? TEXT("itself") : TEXT("themselves"), bSaw ? TEXT("turned to me") : TEXT("began to turn towards me"), *After1,
		bMoved ? *FString::Printf(TEXT("When I moved, %s followed."), They) : *FString::Printf(TEXT("I stood still, and %s stayed on me."), They), *After));
	if (TOptional<FS47DishesSave> V = Kept())
	{
		TArray<FString> Ids;
		for (const TSharedPtr<FRig>& R : On) Ids.Add(IdOf(*R));
		V->Ids = Ids;
		if (D.Set) D.Set(*V);
	}
}

// ---------- the drives ----------

void FS47DishesFollow::Step(double Dt, const FS47FollowIn& S)
{
	const bool bHome = Mode == TEXT("home"), bAway = Mode == TEXT("away");
	// the one over the player: the only one that pushes against its limit
	FRig* Over = nullptr; double Oh = 1e18;
	for (const TSharedPtr<FRig>& R : Rigs) { const double H = FMath::Sqrt(FMath::Square(DishOf(*R).X - S.Eye.X) + FMath::Square(DishOf(*R).Z - S.Eye.Z)); if (R->bOn && H < Oh) { Oh = H; Over = R.Get(); } }
	for (const TSharedPtr<FRig>& Rp : Rigs)
	{
		FRig& R = *Rp;
		if (!R.bOn) continue;
		const FRate K = bHome ? BACK : R.Rate;
		const double Mul = bAway ? AWAY : 1;
		double TAz = HOME_AZ, Want = HOME_EL;
		if (!bHome) { const FAim A = AimAt(DishOf(R), R.Aim); TAz = A.Az; Want = A.El; }
		const double TEl = FMath::Clamp(Want, EL_MIN, 88.0);
		const double EAz = Wrap180(TAz - R.Az), EEl = TEl - R.El;
		R.VAz = Axis(R.VAz, EAz, K.Az * Mul, K.Acc * Mul, Dt);
		R.VEl = Axis(R.VEl, EEl, K.El * Mul, K.Acc * Mul * 0.7, Dt);
		// the last step onto the target lands on it and stops there (exactly home at the end), so it
		// does not hunt to and fro about it
		if (FMath::Abs(EAz) <= FMath::Abs(R.VAz) * Dt + 0.02 && FMath::Abs(R.VAz) < 4) { R.Az = TAz; R.VAz = 0; } else R.Az = Norm(R.Az + R.VAz * Dt);
		if (FMath::Abs(EEl) <= FMath::Abs(R.VEl) * Dt + 0.02 && FMath::Abs(R.VEl) < 4) { R.El = TEl; R.VEl = 0; } else R.El += R.VEl * Dt;
		const bool bWas = R.bLocked;
		R.bLocked = !bHome && FMath::Abs(Wrap180(TAz - R.Az)) < 3 && FMath::Abs(TEl - R.El) < 3;
		if (R.bLocked) R.bEver = true;
		if (R.bLocked && !bWas && R.Tier == 0 && Mode == TEXT("follow") && !Said.bLock)
		{
			Said.bLock = true; LockAt = S.Eye;
			Toast(FString::Printf(TEXT("%s has come down out of 026. It is pointing at me."), *IdOf(R)), 4);
		}
		// down on its final limit with the player lower still: the console has it; and the one right
		// over the player pushes against it now and then, a groan in the quiet
		double Off = 0;
		const bool bAtLimit = Mode == TEXT("follow") && Want < EL_MIN - 0.5 && R.El <= EL_MIN + 0.3 && FMath::Abs(EAz) < 5;
		if (bAtLimit && R.Tier == 0 && !R.bLimit) { R.bLimit = true; Log(FString::Printf(TEXT("DRIVE %s  EL FINAL LIMIT  NO COMMAND"), *IdOf(R)), S.Clock); }
		if (bAtLimit && &R == Over && Want < EL_MIN - 5 && !R.bRun)
		{
			if (R.Push > 0) { R.Push -= Dt; Off = -0.5 * FMath::Sin(UE_DOUBLE_PI * FMath::Max(0.0, 1 - R.Push / 0.8)); }
			else if ((R.Strain -= Dt) <= 0) { R.Push = 0.8; R.Strain = Rnd(3.5, 6); }
		}
		else R.Push = 0;
		// running: its motor, and a brake each way (the near ones)
		const double Sp = FMath::Abs(R.VAz) + FMath::Abs(R.VEl);
		if (!R.bRun && Sp > 0.7) { R.bRun = true; R.Still = 0; if (R.Tier == 0 && T - R.ClunkT > 1.2) { R.ClunkT = T; Sound->Clunk(DishOf(R), true, S.bCalm); } }
		else if (R.bRun && Sp < 0.1) { if ((R.Still += Dt) > 0.45) { R.bRun = false; if (R.Tier == 0 && T - R.ClunkT > 1.2) { R.ClunkT = T; Sound->Clunk(DishOf(R), false, S.bCalm); } } }
		else if (R.bRun) R.Still = 0;
		// the lamps of those on the player blink as one; afterwards each its own way again
		if (Mode == TEXT("follow") && R.bLocked) { if (!Common) Common = R.Ph; }
		const double Tgt = !bHome && R.bEver && Common ? *Common : DishOf(R).Phase;
		R.Ph += FMath::Clamp(WrapPi(Tgt - R.Ph), -0.9 * Dt, 0.9 * Dt);
		D.Dishes->BeaconPhase(R.I, R.Ph);
		Write(R, Off);
	}
	Sound->Update(Dt, Rigs, S.Eye, Mode == TEXT("follow"));
}

void FS47DishesFollow::Write(FRig& R, double Off)
{
	// d.curAz = d.tgtAz = r.az; d.curEl = d.tgtEl = r.el + off
	D.Dishes->Snap(R.I, R.Az, R.El + Off);
	FS47Dish& Dd = D.Dishes->Dishes[R.I];
	Dd.bRogue = true; Dd.Spin = FMath::Abs(R.VAz) + FMath::Abs(R.VEl) + (Off ? 0.5 : 0);
}

// ---------- the case ----------

void FS47DishesFollow::Reset()
{
	Sound->Close();
	for (const TSharedPtr<FRig>& R : Rigs) Release(*R);
	Rigs.Reset(); Mode = TEXT("idle"); T = 0; bNoted = false;
}

void FS47DishesFollow::Restore()
{
	Reset();
	TOptional<FS47DishesSave> V = Kept();
	if (V && !V->bBack)
	{
		// saved as it began (begin), before any drive was in the log: the log has them move, too
		if (!V->Ids.Num()) Log(TEXT("ARRAY  MOTION  NO COMMAND"), V->Clock + 3);
		Log(FString::Printf(TEXT("ARRAY  %sAZ 026 EL 32  NO COMMAND"), V->Ids.Num() ? *FString::Printf(TEXT("%d DRIVES  "), V->Ids.Num()) : TEXT("")), V->Clock + 90);
		V->bBack = true;
		if (D.Set) D.Set(*V);
	}
}

bool FS47DishesFollow::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s dishes follow: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	Check(FMath::IsNearlyEqual(FirstAt(47), 17.5) && FMath::IsNearlyEqual(FirstAt(23.5), 8.75), TEXT("the first a breath after the beep at :16"));
	// a few dishes without the world, the player among them in the warning
	FS47DishArray Arr(nullptr);
	const double At[4][2] = { { -48, -66 }, { -16, -52 }, { 9.5, -42 }, { 120, -350 } };
	const TCHAR* Ids[4] = { TEXT("S-01"), TEXT("S-02"), TEXT("S-03"), TEXT("S-23") };
	for (int32 I = 0; I < 4; ++I) { FS47Dish Dd; Dd.Id = Ids[I]; Dd.X = At[I][0]; Dd.Z = At[I][1]; Dd.Scale = 0.85; Dd.Phase = I; Arr.Dishes.Add(Dd); }
	Check(NearArray({}, &Arr, -10, -50) && !NearArray({}, &Arr, 0, 40), TEXT("near the array: within 45 m of a dish"));
	TOptional<FS47DishesSave> Store;
	TArray<FString> Logs, Notes, Toasts;
	FS47FollowDeps Dp;
	Dp.Dishes = &Arr;
	Dp.Get = [&Store] { return Store; };
	Dp.Set = [&Store](const FS47DishesSave& V) { Store = V; };
	Dp.Log = [&Logs](const FString& S, double) { Logs.Add(S); };
	Dp.Note = [&Notes](const FString& S) { Notes.Add(S); };
	Dp.Toast = [&Toasts](const FString& S, double) { Toasts.Add(S); };
	Dp.NearArray = [&Arr](double X, double Z) { return NearArray({}, &Arr, X, Z); };
	FS47DishesFollow F(Dp);
	FS47FollowIn S;
	S.Stage = TEXT("warning"); S.Warning = FS47FollowIn::FWarn{ 10, 47 }; S.bSaro = true; S.bOnFoot = true; S.Eye = FVector(-20, 1.0, -48); S.Clock = 3 * 3600;
	F.Update(0.1, S);
	Check(F.Mode == TEXT("idle"), TEXT("not before :17.5 into the warning"));
	S.Warning->T = 18;
	F.Update(0.1, S);
	Check(F.Mode == TEXT("follow") && Store && !Store->bBack && F.State().Num() == 3, TEXT("it begins: the three near ones (the far one, if at all, later)"));
	for (int32 I = 0; I < 60; ++I) F.Update(0.1, S);
	Check(Logs.ContainsByPredicate([](const FString& L) { return L.StartsWith(TEXT("DRIVE S-02  MOTION  NO COMMAND")); }) && Toasts.ContainsByPredicate([](const FString& T) { return T.Contains(TEXT("a drive motor starts up")); }),
		TEXT("the nearest starts: the log has it, and the motor is heard"));
	for (int32 I = 0; I < 100; ++I) F.Update(0.1, S);
	Check(F.State().ContainsByPredicate([](const FState& St) { return St.bLocked && St.Id == TEXT("S-02"); }), TEXT("S-02 is on the player within some eight seconds"));
	// got away (the warning passed): they stay on the place, then home, and the notebook has both
	S.Stage = TEXT("calm"); S.Warning.Reset();
	F.Update(0.1, S);
	Check(F.Mode == TEXT("hold") && Notes.Num() == 1 && Notes[0].Contains(TEXT("came down out of 026")) && Notes[0].EndsWith(TEXT("stayed on the place where I had been.")), TEXT("away from them: they hold the place"));
	for (int32 I = 0; I < 1500 && F.Mode != TEXT("idle"); ++I) F.Update(0.1, S);
	Check(F.Mode == TEXT("idle") && Store && Store->bBack && Notes.Num() == 2 && Logs.Last().Contains(TEXT("AZ 026 EL 32  NO COMMAND")), TEXT("home at 026 at the pace of 02:15, the log and the notebook have it"));
	// kept: not again
	S.Stage = TEXT("warning"); S.Warning = FS47FollowIn::FWarn{ 20, 47 };
	F.Update(0.1, S);
	Check(F.Mode == TEXT("idle"), TEXT("once in a night"));
	// a save made while they were out comes back with them home and the log written
	Store = FS47DishesSave{ 3 * 3600, { TEXT("S-02") }, false };
	Logs.Reset();
	F.Restore();
	Check(Store->bBack && Logs.Num() == 1 && Logs[0] == TEXT("ARRAY  1 DRIVES  AZ 026 EL 32  NO COMMAND"), TEXT("restore: home, and the line that says so"));
	return bOk;
}
