#include "S47Inter.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "S47Web.h"

FS47Interactable& US47Inter::Add(const FS47Interactable& It)
{
	if (const int32* Have = ById.Find(It.Id))
	{
		Items[*Have] = It;
	}
	else
	{
		ById.Add(It.Id, Items.Add(It));
	}
	if (Web && It.Object.IsValid()) Web->MarkUsable(It.Object.I, It.Id);
	return Items[ById[It.Id]];
}

FS47Interactable* US47Inter::Get(const FString& Id)
{
	const int32* I = ById.Find(Id);
	return I ? &Items[*I] : nullptr;
}

double US47Inter::RangeOf(const FString& Id) const
{
	if (const int32* I = ById.Find(Id); I && Items[*I].Range > 0) return Items[*I].Range;
	if (const FS47WebItem* E = Web ? Web->Item(Id) : nullptr) return E->Range;
	return -1;
}

bool US47Inter::IsLook(const FString& Id) const
{
	if (const int32* I = ById.Find(Id)) return Items[*I].bLook;
	const FS47WebItem* E = Web ? Web->Item(Id) : nullptr;
	return E && E->bLook;
}

FString US47Inter::LabelOf(const FString& Id) const
{
	if (const int32* I = ById.Find(Id)) return Items[*I].Label ? Items[*I].Label() : FString();
	return FallbackLabel ? FallbackLabel(Id) : FString();
}

void US47Inter::UseId(const FString& Id)
{
	if (const int32* I = ById.Find(Id))
	{
		if (Items[*I].Use) Items[*I].Use();
		return;
	}
	if (FallbackUse) FallbackUse(Id);
}

FString US47Inter::Pick(const FVector& EyeUE, const FVector& DirUE, double MaxDist) const
{
	// Interaction.pick: the first thing that can be used on the ray (6 m), if it is in its range
	// and has something to offer now. Only usable things answer the trace, as only they are in
	// three's list of meshes.
	if (!Web || !Web->GetWorld()) return FString();
	TArray<FHitResult> Hits;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47Pick), true);
	Web->GetWorld()->LineTraceMultiByChannel(Hits, EyeUE, EyeUE + DirUE.GetSafeNormal() * 600.0, ECC_GameTraceChannel1, Q);
	for (const FHitResult& H : Hits)
	{
		const FString Id = Web->UsableAt(H.GetComponent());
		if (Id.IsEmpty() || IsLook(Id)) continue;
		const double R = RangeOf(Id);
		if (H.Distance / 100.0 > (R > 0 ? R : MaxDist)) return FString();
		return LabelOf(Id).IsEmpty() ? FString() : Id;
	}
	return FString();
}

bool US47Inter::ExamineAt(const FVector& EyeUE, const FVector& DirUE, FString& OutId, FString& OutText) const
{
	// Interaction.examine: the first thing on the line of sight (60 m) that has something to say
	if (!Web || !Web->GetWorld()) return false;
	TArray<FHitResult> Hits;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(S47Examine), true);
	Web->GetWorld()->LineTraceMultiByChannel(Hits, EyeUE, EyeUE + DirUE.GetSafeNormal() * 6000.0, ECC_GameTraceChannel1, Q);
	for (const FHitResult& H : Hits)
	{
		const FString Id = Web->UsableAt(H.GetComponent());
		if (Id.IsEmpty()) continue;
		const int32 Node = Web->NodeOf(H.GetComponent());
		if (Node >= 0 && !FS47Obj(Web, Node).Shown()) continue; // a hidden group: another area, a room not drawn
		const int32* I = ById.Find(Id);
		const FS47Interactable* It = I ? &Items[*I] : nullptr;
		const FS47Look* Look = (It && It->Examine) ? nullptr : Looks.Find(Id);
		TFunction<FString()> Say = (It && It->Examine) ? It->Examine : (Look ? Look->Say : TFunction<FString()>());
		if (!Say) continue;
		const double R = RangeOf(Id);
		const double Reach = Look ? (Look->Range > 0 ? Look->Range : (R > 0 ? R : 2.4) + 1) : (It ? It->ExamineRange : 8);
		if (H.Distance / 100.0 > Reach) continue;
		const FString Text = Say();
		if (!Text.IsEmpty()) { OutId = Id; OutText = Text; return true; }
	}
	return false;
}
