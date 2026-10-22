#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "UObject/Object.h"
#include "S47Inter.generated.h"

class AS47Web;

// core/Interaction.ts: the things the player can use or look at closely. A module adds one with
// its object (a scene path or handle), a label (empty: nothing to do with it right now, no
// prompt) and what using it does; the walker's line of sight picks it.
struct SIGNAL47_API FS47Interactable
{
	FString Id;
	FS47Obj Object;
	TFunction<FString()> Label;        // empty hides the prompt (inert right now)
	TFunction<void()> Use;
	double Range = -1;                 // metres; -1: the default (2.4)
	TFunction<FString()> Examine;      // what a closer look says (the zoom), English
	double ExamineRange = 8;
	bool bLook = false;                // only there to be looked at: never used, never in the way
};

// story/looks.ts: what a closer look says about things that carry no Examine of their own
struct FS47Look
{
	TFunction<FString()> Say;
	double Range = -1; // -1: a metre past where it can be used
};

UCLASS()
class SIGNAL47_API US47Inter : public UObject
{
	GENERATED_BODY()

public:
	void Init(AS47Web* InWeb) { Web = InWeb; }

	// Interaction.add: replaces one with the same id
	FS47Interactable& Add(const FS47Interactable& It);
	FS47Interactable* Get(const FString& Id);
	TMap<FString, FS47Look> Looks;

	// The prologue's things are labelled and used by AS47Story by id (its own switch); anything
	// exported but not added here goes to these.
	TFunction<FString(const FString&)> FallbackLabel;
	TFunction<void(const FString&)> FallbackUse;

	// Interaction.pick / examine / update, from the eye along the view (Unreal space)
	FString Pick(const FVector& EyeUE, const FVector& DirUE, double MaxDist = 2.4) const;
	bool ExamineAt(const FVector& EyeUE, const FVector& DirUE, FString& OutId, FString& OutText) const;
	FString Update(const FVector& EyeUE, const FVector& DirUE)
	{
		for (const TFunction<bool()>& H : Holds) if (H && H()) { Current.Reset(); return Current; }
		Current = Pick(EyeUE, DirUE);
		return Current;
	}
	// main.ts step: nothing is picked or offered while one of these holds (the field camera raised, ...)
	TArray<TFunction<bool()>> Holds;
	FString Current;

	FString LabelOf(const FString& Id) const;
	void UseId(const FString& Id);

private:
	UPROPERTY() TObjectPtr<AS47Web> Web;
	TArray<FS47Interactable> Items;
	TMap<FString, int32> ById;
	double RangeOf(const FString& Id) const;
	bool IsLook(const FString& Id) const;
};
