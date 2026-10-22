// world/saroPower.ts
#include "S47SaroPower.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "S47Annex.h"
#include "S47DishArray.h"
#include "S47Json.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	/** What is left of the unlit paint in the dark (PowerParts.paint). */
	const double PAINT_DARK = 0.36;
	/** OFF_GLASS: new MeshBasicMaterial({ color: 0x1a1a1a }) */
	const uint32 OFF_GLASS = 0x1a1a1a;
	// Exterior.ts: the array's five wide floods (flood.skip: never a real light, and not SARO's to dim here)
	const double ARRAY_FLOODS[5][2] = { { 30, -30 }, { -10, -42 }, { -40, -56 }, { 12, -78 }, { 48, -86 } };

	FLinearColor MatColor(const FS47Mat& M)
	{
		UMaterialInstanceDynamic* D = M ? M.W->SharedMid(M.Id) : nullptr;
		if (!D) return FLinearColor::White;
		const FLinearColor C = D->K2_GetVectorParameterValue(TEXT("Color"));
		return FLinearColor(C.R, C.G, C.B, 1.f);
	}
	FLinearColor Times(const FLinearColor& C, double K) { return FLinearColor(float(C.R * K), float(C.G * K), float(C.B * K), 1.f); }
}

FS47SaroPower::FS47SaroPower(FS47PowerParts InP) : P(MoveTemp(InP)) {}

void FS47SaroPower::Find()
{
	if (bFound || !P.Web.IsValid()) return;
	bFound = true;
	AS47Web* W = P.Web.Get();
	for (int32 I = 0; I < 8; ++I) { const FS47Light L = W->Light(FString::Printf(TEXT("room.lights.ceiling[%d]"), I)); if (!L) break; Ceiling.Add(L); }
	for (int32 I = 0; I < 8; ++I) { const FS47Light L = W->Light(FString::Printf(TEXT("room.lights.lamps[%d]"), I)); if (!L) break; Lamps.Add(L); }
	Crt = W->Light(TEXT("room.lights.crt"));
	DoorLight = W->Obj(TEXT("room.doorLight"));
	LabSet = W->FloodSet(TEXT("lab"));
	AnnexSet = W->FloodSet(TEXT("annex"));
	SiteSet = W->FloodSet(TEXT("site"));
	// (the materials' handles, one by one: name[0], name[1] ...)
	auto Mats = [W](const TCHAR* Name, TArray<FS47Mat>& Out)
	{
		for (int32 I = 0; I < 64; ++I) { const FS47Mat Mt = W->Material(FString::Printf(TEXT("%s[%d]"), Name, I)); if (!Mt) break; Out.Add(Mt); }
	};
	Mats(TEXT("yard.redGlass"), RedGlass);
	// paint: [...yard.paint, ...ext.paint, ...world.field.paint] (the field's has no handle)
	for (const TCHAR* Name : { TEXT("yard.paint"), TEXT("ext.paint"), TEXT("world.field.paint") }) Mats(Name, Paint);
	// screens: [...yard.screens, ...annex.screens]
	for (const TCHAR* Name : { TEXT("yard.screens"), TEXT("annex.screens") }) Mats(Name, Screens);
	LampBulb = W->Material(TEXT("world.site.m.lampBulb"));
	// site.skip: the array's wide floods, and the SARO truck's headlight slots (kit.headSlots)
	for (int32 K = 0; K < W->FloodCount(SiteSet); ++K)
	{
		const FLinearColor Pl = W->FloodPool(SiteSet, K);
		for (const auto& F : ARRAY_FLOODS) if (FMath::Abs(Pl.R - F[0]) < 0.01 && FMath::Abs(Pl.B - F[1]) < 0.01 && FMath::Abs(Pl.G - 3) < 0.01) SiteSkip.Add(K);
	}
	if (const TSharedPtr<FJsonObject>& D = W->DriveData())
	{
		const S47J::FObj Head = S47J::Obj(S47J::Obj(S47J::Obj(D, TEXT("legs")), TEXT("saro")), TEXT("head"));
		if (Head.IsValid() && S47J::Int(Head, TEXT("set"), -1) == SiteSet) for (const double V : S47J::Nums(Head, TEXT("slots"))) SiteSkip.Add(int32(V));
	}
}

double FS47SaroPower::Strength(int32 I) const
{
	// (core/ultra.ts Ultra.strength: without the PC tier's real lamps, the pool's own width)
	return P.Web.IsValid() ? P.Web->FloodPool(SiteSet, I).A : 0;
}

FLinearColor FS47SaroPower::GlowCol(const FGlowSnap& G, int32 I) const
{
	return G.bBeacons ? (P.Dishes ? P.Dishes->BeaconBase(I) : FLinearColor::Black) : G.G.Col(I);
}

void FS47SaroPower::SetGlowCol(const FGlowSnap& G, int32 I, const FLinearColor& C) const
{
	if (G.bBeacons) { if (P.Dishes) P.Dishes->SetBeaconBase(I, C); }
	else G.G.SetCol(I, C);
}

void FS47SaroPower::Cut()
{
	Find();
	if (!Snap) Snap = Take();
	Set(0, 0);
}

void FS47SaroPower::Set(double InInner, double InSodium)
{
	if (!Snap) return;
	FSnap& S = *Snap;
	Sodium = InSodium;
	if (!S.bSettled) Inside(S, InInner);
	// the lamps' own glass (the yard's lamp posts and the desk lamps' bulbs: M.lampBulb is SARO's only)
	LampBulb.SetColor(Times(S.Bulb, 0.012 + 0.988 * InSodium));
	for (int32 I = 0; I < Paint.Num() && I < S.Paint.Num(); ++I) Paint[I].SetColor(Times(S.Paint[I], PAINT_DARK + (1 - PAINT_DARK) * InSodium));
	for (FGlowSnap& Gs : S.Glows)
	{
		for (int32 K = 0; K < Gs.Pick.Num(); ++K)
		{
			const int32 I = Gs.Pick[K];
			if (!Gs.bSodium[K] && S.bSettled) continue;
			// (changed by the game since: that is what it is to be)
			const FLinearColor C = GlowCol(Gs, I);
			if (FMath::Abs(C.R - Gs.LastC[I].R) + FMath::Abs(C.G - Gs.LastC[I].G) + FMath::Abs(C.B - Gs.LastC[I].B) > 1e-5) Gs.Col[I] = C;
			const double F = Gs.bSodium[K] ? InSodium : InInner;
			const FLinearColor Now = Times(Gs.Col[I], F);
			SetGlowCol(Gs, I, Now);
			Gs.LastC[I] = Now;
		}
	}
	AS47Web* W = P.Web.Get();
	if (!W) return;
	for (FSlot& Sl : S.Slots)
	{
		const double Now = Strength(Sl.I);
		if (FMath::Abs(Now - Sl.Last) > 1e-6) Sl.W = Now;
		Sl.Last = Sl.W * InSodium;
		const FLinearColor Pl = W->FloodPool(SiteSet, Sl.I);
		W->SetFloodPool(SiteSet, Sl.I, FVector(Pl.R, Pl.G, Pl.B), float(Sl.Last), W->FloodColOf(SiteSet, Sl.I), false);
	}
	W->FlushFlood(SiteSet);
}

void FS47SaroPower::Settle()
{
	if (!Snap || Snap->bSettled) return;
	Inside(*Snap, 1);
	Snap->bSettled = true;
	Set(1, Sodium);
}

void FS47SaroPower::Inside(FSnap& S, double InInner)
{
	Inner = InInner;
	AS47Web* W = P.Web.Get();
	if (!W) return;
	const bool bOn = InInner > 0.5;
	for (int32 I = 0; I < Ceiling.Num() && I < S.Ceiling.Num(); ++I) Ceiling[I].SetIntensity(float(S.Ceiling[I] * InInner));
	for (int32 I = 0; I < Lamps.Num() && I < S.Lamps.Num(); ++I) Lamps[I].SetIntensity(float(S.Lamps[I] * InInner));
	if (Crt) Crt.SetIntensity(float(S.Crt * InInner));
	for (int32 I = 0; I < S.Tubes.Num(); ++I) W->SetTube(I, bOn && S.Tubes[I]);
	// [room.crtLeft, room.crtCenter, room.crtRight, room.crtOps].setPowered(on && s.screens[i])
	for (int32 I = 0; I < S.Screens.Num(); ++I) W->SetCrtPowered(I, bOn && S.Screens[I]);
	// room.doorLight.material = on ? s.door : OFF_GLASS
	if (DoorLight)
	{
		const TArray<UMaterialInstanceDynamic*> Mids = W->NodeMids(DoorLight.I);
		for (int32 I = 0; I < Mids.Num() && I < S.Door.Num(); ++I)
		{
			if (!Mids[I]) continue;
			for (const FLinearColor& C : S.Door[I]) Mids[I]->SetVectorParameterValue(TEXT("Color"), bOn ? C : S47Three::Hex(OFF_GLASS));
		}
	}
	if (LabSet >= 0) W->SetFloodScale(LabSet, float(S.Lab * InInner));
	if (AnnexSet >= 0) W->SetFloodScale(AnnexSet, float(S.Annex * InInner));
	if (US47Annex* A = P.Annex ? P.Annex() : nullptr) A->SetPower(InInner);
	for (int32 I = 0; I < RedGlass.Num() && I < S.Red.Num(); ++I) RedGlass[I].SetColor(Times(S.Red[I], 0.008 + 0.992 * InInner));
	for (int32 I = 0; I < Screens.Num() && I < S.Glass.Num(); ++I) Screens[I].SetColor(Times(S.Glass[I], 0.01 + 0.99 * InInner));
}

void FS47SaroPower::Restore()
{
	if (!Snap) return;
	Settle();
	Set(1, 1);
	Snap.Reset();
	Inner = Sodium = 1;
}

FS47SaroPower::FSnap FS47SaroPower::Take()
{
	FSnap S;
	AS47Web* W = P.Web.Get();
	if (!W) return S;
	auto Glow = [&](const FS47GlowPoints& G, bool bBeacons, TFunctionRef<bool(const FVector&)> Keep)
	{
		FGlowSnap Gs;
		Gs.G = G; Gs.bBeacons = bBeacons;
		const int32 N = bBeacons ? (P.Dishes ? P.Dishes->BeaconCount() : 0) : G.Num();
		if (!N) return;
		for (int32 I = 0; I < N; ++I)
		{
			const FLinearColor C = GlowCol(Gs, I);
			Gs.Col.Add(C); Gs.LastC.Add(C);
			const FVector At = bBeacons ? P.Dishes->BeaconAt(I) : G.Pos(I);
			if (!Keep(At)) continue;
			Gs.Pick.Add(I); Gs.bSodium.Add(IsSodium(C.R, C.G, C.B));
		}
		S.Glows.Add(MoveTemp(Gs));
	};
	auto All = [](const FVector&) { return true; };
	Glow(FS47GlowPoints(W, TEXT("yard.glow.points")), false, All);
	Glow(FS47GlowPoints(W, TEXT("annex.glow.points")), false, All);
	Glow(FS47GlowPoints(W, TEXT("world.field.glow.points")), false, All);
	Glow(FS47GlowPoints(), true, All);
	// the site's own lamps among the exterior's: not the highway's, the motel's sign or the horizon's
	Glow(FS47GlowPoints(W, TEXT("ext.glow.points")), false, [](const FVector& At) { return SaroAt(At.X, At.Z); });
	for (int32 I = 0; I < W->FloodCount(SiteSet); ++I)
	{
		if (SiteSkip.Contains(I)) continue;
		const FLinearColor Pl = W->FloodPool(SiteSet, I);
		if (Pl.G < -100 || !SaroAt(Pl.R, Pl.B)) continue;
		const double Wd = Strength(I);
		S.Slots.Add({ I, Wd, Wd });
	}
	for (const FS47Light& L : Ceiling) S.Ceiling.Add(L.Intensity());
	for (const FS47Light& L : Lamps) S.Lamps.Add(L.Intensity());
	S.Crt = Crt ? Crt.Intensity() : 0;
	for (int32 I = 0; I < W->TubeCount(); ++I) S.Tubes.Add(W->TubeLit(I));
	for (int32 I = 0; I < 4; ++I) S.Screens.Add(W->IsCrtPowered(I));
	if (DoorLight)
	{
		for (UMaterialInstanceDynamic* D : W->NodeMids(DoorLight.I))
		{
			TArray<FLinearColor> C;
			if (D) C.Add(D->K2_GetVectorParameterValue(TEXT("Color")));
			S.Door.Add(C);
		}
	}
	S.Lab = W->FloodScaleOf(LabSet);
	S.Annex = W->FloodScaleOf(AnnexSet);
	for (const FS47Mat& M : RedGlass) S.Red.Add(MatColor(M));
	S.Bulb = MatColor(LampBulb);
	for (const FS47Mat& M : Paint) S.Paint.Add(MatColor(M));
	for (const FS47Mat& M : Screens) S.Glass.Add(MatColor(M));
	return S;
}
