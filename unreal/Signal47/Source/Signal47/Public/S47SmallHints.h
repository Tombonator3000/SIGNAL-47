#pragma once
#include "CoreMinimal.h"

class US47Main;

// Freeform (Tom, 7 November: "more must happen"): the small deniable things of a night. A phone rings once, a rack clicks, the crickets
// stop. Each is one of the Director's own minor events, with its budget, cooldown and rest, and writes no finding: nothing is
// established by them, and the evening stays a normal job. Sounds and Foley only, from what the game already has.
namespace S47SmallHints
{
 SIGNAL47_API bool Is(const FString& Id);
 SIGNAL47_API TArray<FString> Ids();
 SIGNAL47_API const TCHAR* TitleOf(const FString& Id);
 SIGNAL47_API const TCHAR* NoteOf(const FString& Id);
 // Not tied to SARO: outdoors at night anywhere, or a set the player carries.
 SIGNAL47_API bool Anywhere(const FString& Id);
 SIGNAL47_API FVector AnchorOf(US47Main* M,const FString& Id);
 // Near enough for it to be noticed at all, and the world in the state it needs. Sight is the Director's own test (InView there).
 SIGNAL47_API bool Fits(US47Main* M,const FString& Id,const FVector& PlayerAt,TFunctionRef<bool(const FVector&)> InView);
 // Once, when the event starts its effects.
 SIGNAL47_API void Play(US47Main* M,const FString& Id,const FVector& Anchor);
 // How far through a night's budget of small things it may be by now: they should not all fall in the first hours. Hour of day; Jitter 0 to 1.
 SIGNAL47_API bool PaceAllows(int32 Spent,int32 Budget,double Hour,double Jitter);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
