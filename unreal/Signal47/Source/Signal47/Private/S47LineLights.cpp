// story/lineLights.ts
#include "S47LineLights.h"

#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Docs.h"
#include "S47Lines.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	/** Along B towards STATION 01 (the transit), in SARO's terms (the same as the map's). */
	FVector TO_STATION() { const S47Lines::FStraight& B = S47Lines::LINES('B'); return FVector(-B.Dir.X, 0, -B.Dir.Y); }
	/** How far ahead along B they make for when they turn off (metres). */
	const double AHEAD = 250;
	/** PointsMaterial({ color: 0xfff4e0, size: 4, sizeAttenuation: false, ... }) */
	const uint32 COLOR = 0xfff4e0;
	const double SIZE_PX = 4;
}

FS47LineLights::FS47LineLights(AS47Web* InWeb, TFunction<void(const FString&)> InNote, TFunction<void(double)> InOnDone)
	: Note(MoveTemp(InNote)), OnDone(MoveTemp(InOnDone)), Web(InWeb), Off(TO_STATION())
{
	Base.Init(FVector::ZeroVector, 3); Shown = Base;
	UMaterialInterface* Glow = InWeb ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow")) : nullptr;
	if (!Glow) return;
	for (int32 I = 0; I < 3; ++I)
	{
		UMaterialBillboardComponent* B = NewObject<UMaterialBillboardComponent>(InWeb);
		B->SetupAttachment(InWeb->GetRootComponent());
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Glow, InWeb);
		D->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
		B->AddElement(D, nullptr, false, 50, 50, nullptr);
		B->SetCastShadow(false);
		B->RegisterComponent();
		B->SetVisibility(false);
		Pts.Add(B);
	}
}

void FS47LineLights::SetVisible(bool bOn)
{
	bVisible = bOn;
	for (const TWeakObjectPtr<UMaterialBillboardComponent>& B : Pts) if (B.IsValid()) B->SetVisibility(bOn);
}

void FS47LineLights::Apply()
{
	// mat.opacity on an additive point: its colour times the opacity
	const FLinearColor C = S47Three::Hex(COLOR) * float(Op);
	for (int32 I = 0; I < Pts.Num(); ++I)
	{
		UMaterialBillboardComponent* B = Pts[I].Get();
		if (!B) continue;
		B->SetWorldLocation(S47Three::ToUE(Shown[I]));
		if (B->Elements.Num()) if (UMaterialInstanceDynamic* D = Cast<UMaterialInstanceDynamic>(B->Elements[0].Material)) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(C.R, C.G, C.B, 1.f));
	}
}

void FS47LineLights::Draw(const FS47Cam& Cam, double ViewportH)
{
	if (!bVisible || ViewportH <= 0) return;
	// four pixels at the point's distance: 2 tan(fov / 2) * d / height (the element's size is half each way)
	for (int32 I = 0; I < Pts.Num(); ++I)
	{
		UMaterialBillboardComponent* B = Pts[I].Get();
		if (!B || !B->Elements.Num()) continue;
		const double Dist = FVector::Dist(Cam.Pos, Shown[I]);
		const float Half = float(SIZE_PX * 2 * FMath::Tan(FMath::DegreesToRadians(Cam.Fov) / 2) * Dist / ViewportH * 100 / 2);
		B->Elements[0].BaseSizeX = B->Elements[0].BaseSizeY = Half;
		B->MarkRenderStateDirty();
	}
}

void FS47LineLights::Update(double Dt, const FS47LineLightsIn& S)
{
	const TOptional<FVector>& Tr = S.Truck;
	if (State == TEXT("none"))
	{
		if (bDone || !S.bOk || !Tr) { if (Tr) LastZ = Tr->Z; return; }
		const double Dz = Tr->Z - LastZ; LastZ = Tr->Z;
		if ((Dz > 0.05 || (bAnyWay && Dz < -0.05)) && Tr->Z > 120 && Tr->Z < 240 && S.May != TOptional<bool>(false)) Begin(*Tr, S, Dz > 0 ? 1 : -1);
		return;
	}
	T += Dt;
	if (State == TEXT("pace"))
	{
		// abreast of the truck, 200 m off to the west, about twenty metres up
		const double Dz = Tr ? Tr->Z - LastZ : 0; if (Tr) LastZ = Tr->Z;
		for (FVector& B : Base) B.Z += Dz;
		Op = FMath::Min(0.95, T / 2.5);
		if (T > 16 || !Tr || !S.bOk)
		{
			State = TEXT("off"); T = 0; TurnOff();
			if (Note) Note(FString::Printf(TEXT("%s. Three lights, low, west of the highway, keeping pace with the truck. Then they turned off along B and went out."), *S47Text::ClockText(S.Clock, false)));
			if (OnDone) OnDone(S.Clock);
		}
	}
	else
	{
		for (FVector& B : Base) B += Off * (38 * Dt);
		Op = FMath::Max(0.0, 0.95 - T / 7);
		if (T > 7.5) { State = TEXT("none"); SetVisible(false); return; }
	}
	for (int32 I = 0; I < Base.Num(); ++I) Shown[I] = FVector(Base[I].X, Base[I].Y + FMath::Sin(T * 0.7 + I) * 0.4, Base[I].Z);
	Apply();
}

void FS47LineLights::TurnOff()
{
	// which way they go when they leave the truck: onto B and along it towards STATION 01 (from the
	// middle one: a point of B AHEAD metres on from the nearest, in SARO's terms)
	const FVector M = Base[1];
	const TOptional<FVector2D> P = S47Lines::ToMap(TEXT("saro"), M.X, M.Z);
	if (!P) { Off = TO_STATION(); return; }
	// foot('B', p): pointAt('B', alongFrom('B', p)); then AHEAD metres back towards the transit
	const FVector2D Ft = S47Lines::PointAt('B', S47Lines::AlongFrom('B', P->X, P->Y));
	const FVector2D Q = S47Lines::PointAt('B', S47Lines::AlongFrom('B', Ft.X, Ft.Y) - AHEAD);
	const TOptional<FVector2D> L = S47Lines::FromMap(TEXT("saro"), Q.X, Q.Y);
	if (!L) { Off = TO_STATION(); return; }
	Off = FVector(L->X - M.X, 0, L->Y - M.Z).GetSafeNormal();
}

TOptional<FVector> FS47LineLights::At() const
{
	if (State == TEXT("none") || !Base.Num()) return {};
	return Base[1];
}

void FS47LineLights::Begin(const FVector& Tr, const FS47LineLightsIn& S, int32 Ahead)
{
	// `ahead`: +1 driving south, -1 north; the three stand a little ahead of abreast either way
	bDone = true; State = TEXT("pace"); T = 0; LastZ = Tr.Z;
	for (int32 I = 0; I < 3; ++I)
	{
		const double X = Tr.X - 210 - I * 6, Z = Tr.Z + Ahead * (30 + I * 16);
		Base[I] = FVector(X, (S.Ground ? S.Ground(X, Z) : 0) + 19 + I * 1.5, Z);
	}
	Shown = Base;
	SetVisible(true); Op = 0;
	Apply();
}

bool FS47LineLights::Now(const FS47LineLightsIn& S, int32 Ahead)
{
	if (State != TEXT("none") || !S.Truck) return false;
	Begin(*S.Truck, S, Ahead);
	return true;
}

void FS47LineLights::Reset(bool bInDone)
{
	bDone = bInDone; State = TEXT("none"); SetVisible(false); bAnyWay = false;
}

bool FS47LineLights::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s line lights: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	TArray<FString> Notes; double Done = -1;
	FS47LineLights L(nullptr, [&Notes](const FString& S) { Notes.Add(S); }, [&Done](double C) { Done = C; });
	FS47LineLightsIn S; S.bOk = true; S.Clock = 3 * 3600; S.Ground = [](double, double) { return 0.0; };
	// driving south down the highway from SARO
	S.Truck = FVector(-24, 0, 100);
	L.Update(0.1, S);
	S.Truck = FVector(-24, 0, 130);
	L.Update(0.1, S);
	Check(L.State == TEXT("pace") && L.At() && FMath::IsNearlyEqual(L.At()->X, -24 - 216.0) && FMath::IsNearlyEqual(L.At()->Y, 20.5), TEXT("south of SARO they come up abreast, 210 m west, about twenty up"));
	for (int32 I = 0; I < 170; ++I) { S.Truck->Z += 1; L.Update(0.1, S); }
	Check(L.State == TEXT("off") && Notes.Num() == 1 && Done == 3 * 3600, TEXT("after sixteen seconds they turn off, and the notebook has them"));
	const FVector A = *L.At();
	L.Update(1, S);
	const FVector B = *L.At();
	Check(FVector::DotProduct((B - A).GetSafeNormal(), TO_STATION()) > 0.5 && FMath::IsNearlyEqual((B - A).Size(), 38.0, 0.01), TEXT("away along B towards STATION 01, 38 m a second"));
	for (int32 I = 0; I < 10; ++I) L.Update(1, S);
	Check(L.State == TEXT("none") && L.bDone, TEXT("and out; once in a night"));
	return bOk;
}
