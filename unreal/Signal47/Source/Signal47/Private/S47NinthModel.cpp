// world/ninthCow.ts (the exported build result), world/Station01.ts setNinth and ninthAt.
#include "S47NinthModel.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "S47CowMeshes.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47Web.h"

FS47NinthModel::FS47NinthModel(US47Main* InM) : M(InM)
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (!W) return;
	SiteGroup = W->Obj(TEXT("world.site.group"));
	Group = W->Obj(TEXT("world.site.ninth.group"));
	HeadHit = W->Obj(TEXT("world.site.ninth.hit.head")); BodyHit = W->Obj(TEXT("world.site.ninth.hit.body")); FrostHit = W->Obj(TEXT("world.site.ninth.hit.frost"));
	for (const TCHAR* Part : { TEXT("head"), TEXT("body"), TEXT("middle") }) At.Add(Part, W->Vec(FString(TEXT("world.site.ninth.at.")) + Part));
	Box = W->Rect(TEXT("world.site.ninth.box"));
	FS47Obj Hide = Group.Find(TEXT("ninth:hide")), Cuts = Group.Find(TEXT("ninth:cuts"));
	const auto* HideNode = W->Node(Hide.I);
	auto* HideComponent = HideNode ? Cast<UStaticMeshComponent>(HideNode->Comp.Get()) : nullptr;
	const auto Meshes = S47CowMeshes::Load();
	if (Meshes && HideComponent && Cuts)
	{
		// The replacement is group-local in the original fallen pose. Keep the frost, head/
		// flank proxies, footprints and photograph subjects; only retire the old cut decals.
		S47CowMeshes::Replace(HideComponent, Meshes.Abducted);
		Cuts.SetVisible(false);
		UE_LOG(LogTemp, Display, TEXT("S47: cow models abducted ready; original frost and story proxies retained"));
	}
	for (const FString& P : W->PathsUnder(TEXT("world.site.occluders[")))
	{
		const FS47Obj O = W->Obj(P);
		if (O && !Occluders.Contains(O)) Occluders.Add(O);
	}
	SetNinth(false);
}

void FS47NinthModel::SetNinth(bool bOn)
{
	bNinthDown = bOn;
	Group.SetVisible(bOn);
	// three's layer 31 removes hidden proxies from the interaction ray as well as the view.
	for (FS47Obj Hit : { HeadHit, BodyHit, FrostHit })
	{
		Hit.SetVisible(bOn);
		const FS47WebNode* N = Hit.W ? Hit.W->Node(Hit.I) : nullptr;
		if (UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr)
			C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, bOn ? ECR_Block : ECR_Ignore);
	}
	if (!M || !M->Web || !Box.bValid) return;
	const FVector O = SiteGroup.Position();
	TArray<FS47Rect>& Colliders = M->Web->CollidersMutable(TEXT("station01"));
	FS47Rect* C = Colliders.FindByPredicate([](const FS47Rect& R) { return R.Id == TEXT("ninth:body"); });
	if (bOn && !C)
	{
		FS47Rect R; R.MinX = Box.MinX + O.X; R.MaxX = Box.MaxX + O.X; R.MinZ = Box.MinZ + O.Z; R.MaxZ = Box.MaxZ + O.Z; R.Id = TEXT("ninth:body");
		Colliders.Add(R);
	}
	else if (!bOn && C) Colliders.RemoveAll([](const FS47Rect& R) { return R.Id == TEXT("ninth:body"); });
}

FVector FS47NinthModel::NinthAt(const FString& Part) const
{
	const FVector* P = At.Find(Part);
	return (P ? *P : FVector::ZeroVector) + SiteGroup.Position();
}

bool FS47NinthModel::Occluded(const FVector& From, const FVector& To, double Near, double Far) const
{
	const FVector Dir = (To - From).GetSafeNormal();
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47NinthSight), true);
	const FVector Start = S47Three::ToUE(From + Dir * Near), End = S47Three::ToUE(From + Dir * Far);
	for (const FS47Obj& O : Occluders)
	{
		const FS47WebNode* N = O.W ? O.W->Node(O.I) : nullptr;
		UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
		FHitResult Hit;
		// three's intersectObjects(occluders, false): test exactly those meshes, no descendants.
		if (C && C->LineTraceComponent(Hit, Start, End, Q)) return true;
	}
	return false;
}

bool FS47NinthModel::SelfTest(TArray<FString>& Lines)
{
	FS47NinthModel N(nullptr);
	N.At.Add(TEXT("middle"), FVector(-5.6, 0.3, -8.55));
	N.SetNinth(true);
	const bool bOk = N.bNinthDown && N.NinthAt(TEXT("middle")).Equals(FVector(-5.6, 0.3, -8.55)) && FROST_R == 3.3
		&& !N.Occluded(FVector::ZeroVector, FVector(10, 0, 0), 0.05, 9.7);
	Lines.Add(FString::Printf(TEXT("%s ninth model: exported at, 3.3 m frost and visibility state without a scene"), bOk ? TEXT("PASS") : TEXT("FAIL")));
	return bOk;
}
