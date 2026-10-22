#include "S47Ultra.h"

#include "Components/LightComponent.h"
#include "Components/SpotLightComponent.h"
#include "S47Three.h"
#include "S47Web.h"

FS47Ultra::FS47Ultra(AS47Web* W) : Web(W)
{
	if (!W) return;
	for (int32 K = 0; K < POOL; ++K)
	{
		// new THREE.SpotLight(0xffa24a, 0, 24, 1.2, 0.85, 2): castShadow, its map 1024
		USpotLightComponent* S = NewObject<USpotLightComponent>(W);
		S->SetMobility(EComponentMobility::Movable);
		S->SetupAttachment(W->GetRootComponent());
		S->SetIntensityUnits(ELightUnits::Candelas);
		S->SetIntensity(0);
		S->SetLightColor(S47Three::Hex(0xffa24a));
		S->SetAttenuationRadius(2400);
		S->SetOuterConeAngle(FMath::RadiansToDegrees(1.2f));
		S->SetInnerConeAngle(FMath::RadiansToDegrees(1.2f * (1 - 0.85f)));
		S->SetCastShadows(true);
		S->SetVisibility(false);
		S->RegisterComponent();
		Park(S);
		Spots.Add(S);
		HeldLamps.Add(TOptional<FLamp>());
	}
}

float FS47Ultra::Strength(int32 Set, int32 I) const
{
	const AS47Web* W = Web.Get();
	if (!W) return 0;
	const float P = W->FloodPos(Set, I).A;
	if (P != OFF) return P;
	for (const TOptional<FLamp>& H : HeldLamps) if (H && H->Set == Set && H->I == I) return H->Want;
	return 0;
}

void FS47Ultra::AddShadowLight(ULightComponent* L)
{
	if (!L) return;
	Extra.Add(L);
	L->SetCastShadows(bOn);
}

void FS47Ultra::Set(bool bInOn)
{
	if (bInOn == bOn) return;
	bOn = bInOn;
	for (const TWeakObjectPtr<USpotLightComponent>& S : Spots) if (S.IsValid()) S->SetVisibility(bOn);
	for (const TWeakObjectPtr<ULightComponent>& L : Extra) if (L.IsValid()) L->SetCastShadows(bOn);
	if (!bOn) for (int32 K = 0; K < POOL; ++K) Release(K);
}

void FS47Ultra::Release(int32 K)
{
	// hand the lamp back to its fake light
	AS47Web* W = Web.Get();
	if (HeldLamps[K].IsSet() && W)
	{
		const FLamp& H = *HeldLamps[K];
		if (W->FloodPos(H.Set, H.I).A == OFF) W->SetFlood(H.Set, H.I, H.Want);
	}
	HeldLamps[K].Reset();
	if (Spots[K].IsValid()) Park(Spots[K].Get());
}

void FS47Ultra::Park(USpotLightComponent* S)
{
	// an unused light stays where it is, dark (three parks it far under the ground)
	S->SetIntensity(0);
	S->SetWorldLocation(S47Three::ToUE(0, -500, 0));
}

TArray<TPair<int32, int32>> FS47Ultra::Held() const
{
	TArray<TPair<int32, int32>> O;
	for (const TOptional<FLamp>& H : HeldLamps) if (H) O.Add({ H->Set, H->I });
	return O;
}

void FS47Ultra::Update(double Dt, const FVector& At, const TOptional<TArray<FString>>& Keys, bool bExtras)
{
	(void)bExtras; // (the shadow maps' redraw rate: Unreal draws them as it needs)
	AS47Web* W = Web.Get();
	if (!bOn || !W) return;
	// what the game wants from each held lamp: it may have dimmed, switched or flickered it meanwhile;
	// the fake one goes off again, or both would burn
	for (TOptional<FLamp>& H : HeldLamps)
	{
		if (!H) continue;
		const float P = W->FloodPos(H->Set, H->I).A;
		if (P != OFF) { H->Want = P; W->SetFlood(H->Set, H->I, OFF); }
	}
	T -= Dt;
	if (T <= 0)
	{
		T = 0.25;
		// the three nearest overhead lamps within reach
		struct FNear { int32 Set, I; double D; };
		TArray<FNear> Near;
		for (int32 S = 0; S < W->FloodSetCount(); ++S)
		{
			const FString Key = W->FloodKey(S);
			if (Key == TEXT("cab") || (Keys.IsSet() && !Keys->Contains(Key))) continue;
			for (int32 I = 0; I < W->FloodCount(S); ++I)
			{
				if (W->FloodSkip(S, I)) continue;
				const FLinearColor P = W->FloodPos(S, I);
				const double Wd = P.A == OFF ? Strength(S, I) : P.A;
				// overhead lamps only; the wide area floods over the array stay fake
				if (Wd < 2.5 || Wd > 20 || P.G < 1.8) continue;
				const double D = FMath::Sqrt(FMath::Square(P.R - At.X) + FMath::Square(P.B - At.Z));
				if (D < 40) Near.Add({ S, I, D });
			}
		}
		Near.StableSort([](const FNear& A, const FNear& B) { return A.D < B.D; });
		if (Near.Num() > POOL) Near.SetNum(POOL);
		// keep what is still picked, release the rest, then fill the free slots
		for (int32 K = 0; K < POOL; ++K)
		{
			const TOptional<FLamp>& H = HeldLamps[K];
			if (H && !Near.ContainsByPredicate([&H](const FNear& Q) { return Q.Set == H->Set && Q.I == H->I; })) Release(K);
		}
		for (const FNear& Q : Near)
		{
			if (HeldLamps.ContainsByPredicate([&Q](const TOptional<FLamp>& H) { return H && H->Set == Q.Set && H->I == Q.I; })) continue;
			const int32 K = HeldLamps.IndexOfByPredicate([](const TOptional<FLamp>& H) { return !H.IsSet(); });
			if (K < 0) break;
			const FLinearColor P = W->FloodPos(Q.Set, Q.I);
			FLamp L; L.Set = Q.Set; L.I = Q.I; L.Want = P.A;
			HeldLamps[K] = L;
			W->SetFlood(Q.Set, Q.I, OFF);
			if (USpotLightComponent* S = Spots[K].Get())
			{
				// at the lamp, straight down (its target at the lamp's foot)
				S->SetWorldLocation(S47Three::ToUE(P.R, P.G, P.B));
				S->SetWorldRotation(S47Three::DirToUE(FVector(0, -1, 0.0001)).Rotation());
				S->SetLightColor(W->FloodCol(Q.Set, Q.I));
			}
		}
	}
	// brightness: match the fake pool right under the lamp (kit.ts: w * win^2 / (1 + d^2 * 0.012) * scale)
	for (int32 K = 0; K < POOL; ++K)
	{
		const TOptional<FLamp>& H = HeldLamps[K];
		USpotLightComponent* S = Spots[K].Get();
		if (!H || !S) continue;
		const FLinearColor P = W->FloodPos(H->Set, H->I);
		const double Y = FMath::Max(1.0, double(P.G)), Reach = H->Want * 1.6 + 8;
		const double Win = FMath::Max(0.0, 1 - Y / Reach);
		S->SetIntensity(float(UE_DOUBLE_PI * Y * Y * H->Want * Win * Win / (1 + Y * Y * 0.012) * W->FloodScaleOf(H->Set) * 1.6));
		S->SetAttenuationRadius(float(Reach * 100));
	}
}
