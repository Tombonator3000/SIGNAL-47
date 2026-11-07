#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;

// Freeform only (Tom, 7 November: being taken has to cost). What a taking leaves on the ground where it happened. Never
// deleted: it lies there until fetched, and after a few days somebody hands it in at SARO. Film, prints and originals are
// not gear and are not touched. Simple props for now, a box of the right size for each.
class FS47LeftGear
{
public:
 explicit FS47LeftGear(US47Main* Main):M(Main){}
 struct FLeft{FString Item,Area,Title;FVector At=FVector::ZeroVector;double Clock=0;};
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 // Count things go, from what the player has with them: the set first, then the camera. Returns them for the note.
 TArray<FString> Leave(int32 Count,const FString& Area,const FVector& At,const FString& Title,double Clock);
 bool IsLeft(const FString& Item) const{return Left.ContainsByPredicate([&](const FLeft& L){return L.Item==Item;});}
 bool AnyLeft() const{return !Left.IsEmpty();}
 const TArray<FLeft>& Items() const{return Left;}
 FString TaskLine() const;
 static constexpr double HandedInAfter=3*86400.;
 static FString NameOf(const FString& Item);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TArray<FLeft> Left;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 // One prop per kind of thing and exterior root: the ray finds whichever area is shown.
 struct FSlot{FString Item;TWeakObjectPtr<UStaticMeshComponent> Saro,Road;FS47Obj SaroObj,RoadObj;};
 TArray<FSlot> Slots;
 bool bBuilt=false;
 bool Build();
 void Layout();
 bool Here(const FLeft& L) const;
 bool Fetch(const FString& Item);
 void GiveBack(const FLeft& L,bool bToHand);
};
