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
 void Start(){bActive=true;Restored.Reset();Job.Empty();JobSeconds=0;}
 bool IsRestored(const FString& Id) const { return Restored.Contains(Id); }
 const FBranch* BranchOfDish(const FString& Dish) const; // nullptr when that dish is working
 const FBranch* DownJunction(const FString& Dish) const;
 const FBranch* NextBranch() const;
 int32 DownCount() const;
 bool Restore(const FString& Id);
 S47J::FObj Save() const;
 void Load(const S47J::FObj& O);
 static bool SelfTest(TArray<FString>& Lines);
};
