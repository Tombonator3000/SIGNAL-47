// world/glow.ts: the exported GlowPoints, one billboard per point
#include "S47Glow.h"

#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Three.h"
#include "S47Web.h"

namespace
{
	UMaterialInstanceDynamic* Mid(const TWeakObjectPtr<UMaterialBillboardComponent>& B)
	{
		return B.IsValid() && B->Elements.Num() > 0 ? Cast<UMaterialInstanceDynamic>(B->Elements[0].Material) : nullptr;
	}
}

FS47GlowPoints::FS47GlowPoints(AS47Web* InWeb, const FString& Path)
{
	if (!InWeb) return;
	const FS47Obj O = InWeb->Obj(Path);
	const FS47WebNode* N = O ? InWeb->Node(O.I) : nullptr;
	if (!N || !N->Comp.IsValid()) return;
	// (in the order the points were added: AS47Web attaches them so)
	for (USceneComponent* C : N->Comp->GetAttachChildren())
	{
		if (UMaterialBillboardComponent* B = Cast<UMaterialBillboardComponent>(C)) Points.Add(B);
	}
}

FLinearColor FS47GlowPoints::Col(int32 I) const
{
	UMaterialInstanceDynamic* D = Points.IsValidIndex(I) ? Mid(Points[I]) : nullptr;
	if (!D) return FLinearColor::Black;
	const FLinearColor C = D->K2_GetVectorParameterValue(TEXT("Color"));
	return FLinearColor(C.R, C.G, C.B, 1.f);
}

void FS47GlowPoints::SetCol(int32 I, const FLinearColor& C) const
{
	if (UMaterialInstanceDynamic* D = Points.IsValidIndex(I) ? Mid(Points[I]) : nullptr) D->SetVectorParameterValue(TEXT("Color"), FLinearColor(C.R, C.G, C.B, 1.f));
}

FVector FS47GlowPoints::Pos(int32 I) const
{
	return Points.IsValidIndex(I) && Points[I].IsValid() ? S47Three::ToThree(Points[I]->GetComponentLocation()) : FVector::ZeroVector;
}

void FS47GlowPoints::SetPos(int32 I, const FVector& P) const
{
	if (Points.IsValidIndex(I) && Points[I].IsValid()) Points[I]->SetWorldLocation(S47Three::ToUE(P));
}

void FS47GlowPoints::SetVisible(int32 I, bool bOn) const
{
	if (Points.IsValidIndex(I) && Points[I].IsValid()) Points[I]->SetVisibility(bOn);
}
