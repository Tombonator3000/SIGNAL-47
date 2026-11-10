#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// New Freeform games take over a worn station: the outer antennas are out, in
// branches that each need their own repair. Pure rules, no world access.
// Saves without this state keep the fully working array they were made with.
class FS47ArrayWear
{
public:
 struct FNeed { const TCHAR* Item; int32 Count; };
 struct FBranch { const TCHAR* Id; const TCHAR* Title; const TCHAR* Junction; const TCHAR* Fault; TArray<const TCHAR*> Dishes; TArray<FNeed> Needs; };
 static constexpr int32 MinDishes=6;
 static const TArray<FBranch>& Branches();
 static const FBranch* Branch(const FString& Id);
 static double RateForCount(int32 Count); // joins the existing pair-count rate at 24 antennas
 bool bActive=false;
 TArray<FString> Restored;
 FString Job;double JobSeconds=0;
 // D45-20261110 (Tom: the player picks which branch is repaired next): the operator's order from the array PC. Empty is the
 // table order, as older saves have it; NextBranch, the HUD goal and the OPS summary follow it.
 TArray<FString> Plan;
 void Start(){bActive=true;Restored.Reset();Plan.Reset();Job.Empty();JobSeconds=0;}
 bool IsRestored(const FString& Id) const { return Restored.Contains(Id); }
 const FBranch* BranchOfDish(const FString& Dish) const; // nullptr when that dish is working
 const FBranch* DownJunction(const FString& Dish) const;
 const FBranch* NextBranch() const;
 TArray<const FBranch*> Ordered() const; // D45-20261110: open branches, the plan first, then the rest in table order
 bool Move(const FString& Id,int32 To);  // D45-20261110: puts an open branch at place To of that order; false if nothing changed
 int32 DownCount() const;
 bool Restore(const FString& Id);
 S47J::FObj Save() const;
 void Load(const S47J::FObj& O);
 static bool SelfTest(TArray<FString>& Lines);
};
