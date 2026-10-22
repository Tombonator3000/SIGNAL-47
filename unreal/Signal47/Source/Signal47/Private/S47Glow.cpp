#include "S47Glow.h"

#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Three.h"

float FS47GlowPoints::Factor(double T, const FVector2D& B, float InHold, float InLevel)
{
	// float b = aBlink.x > 0.0 ? 0.55 + 0.45 * max(0.0, sin(uTime * aBlink.x + aBlink.y)) : 1.0;
	// vCol = aCol * mix(b, uLevel, uHold);
	const double Bl = B.X > 0 ? 0.55 + 0.45 * FMath::Max(0.0, FMath::Sin(T * B.X + B.Y)) : 1.0;
	return float(Bl + (InLevel - Bl) * InHold);
}

void FS47GlowPoints::Apply(int32 I)
{
	if (!Mid.IsValidIndex(I) || !Mid[I].IsValid()) return;
	if (Brightness.Num() != Num()) Brightness.Init(-1.f, Num());
	const float K = Factor(Time, Blink.IsValidIndex(I) ? Blink[I] : FVector2D::ZeroVector, Hold, Level);
	if (FMath::Abs(Brightness[I] - K) < 1e-4f) return;
	Brightness[I] = K;
	Mid[I]->SetScalarParameterValue(TEXT("Brightness"), K);
}

void FS47GlowPoints::SetPosition(int32 I, const FVector& P)
{
	if (!Pos.IsValidIndex(I)) return;
	Pos[I] = P;
	if (Bill.IsValidIndex(I) && Bill[I].IsValid()) Bill[I]->SetRelativeLocation(S47Three::ToUE(P));
}

void FS47GlowPoints::SetPhase(int32 I, double Phase)
{
	if (!Blink.IsValidIndex(I) || Blink[I].Y == Phase) return;
	Blink[I].Y = Phase;
	Apply(I);
}

void FS47GlowPoints::SetColor(int32 I, const FLinearColor& C)
{
	if (!Col.IsValidIndex(I) || Col[I] == C) return;
	Col[I] = C;
	if (Mid.IsValidIndex(I) && Mid[I].IsValid()) Mid[I]->SetVectorParameterValue(TEXT("Color"), FLinearColor(C.R, C.G, C.B, 1.f));
}

void FS47GlowPoints::SetSize(int32 I, float S)
{
	if (!Size.IsValidIndex(I) || Size[I] == S) return;
	Size[I] = S;
	UMaterialBillboardComponent* B = Bill.IsValidIndex(I) ? Bill[I].Get() : nullptr;
	if (!B || B->Elements.Num() == 0) return;
	// (aSize is the sprite's width in metres; the element's size is half of it each way, as AS47Web made it)
	B->Elements[0].BaseSizeX = B->Elements[0].BaseSizeY = S * 50.f;
	B->MarkRenderStateDirty();
}

void FS47GlowPoints::HoldAt(TOptional<float> InLevel)
{
	// hold(level) { this.uHold.value = level === null ? 0 : 1; if (level !== null) this.uLevel.value = level; }
	Hold = InLevel.IsSet() ? 1.f : 0.f;
	if (InLevel.IsSet()) Level = *InLevel;
	for (int32 I = 0; I < Num(); ++I) Apply(I);
}

void FS47GlowPoints::Update(double T)
{
	Time = T;
	// (the steady points keep their brightness, 1 or the held level: only the blinking ones change with uTime)
	for (int32 I = 0; I < Num(); ++I) if (Blink.IsValidIndex(I) && Blink[I].X > 0) Apply(I);
}
