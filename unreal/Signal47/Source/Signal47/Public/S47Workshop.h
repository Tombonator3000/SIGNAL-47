#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Freeform bench crafting: donor parts with their own identity, an explicit
// test step, exact recipes and reversible builds. Pure rules, no world access.
class FS47Workshop
{
public:
 struct FPart
 {
  int32 Id=0,Donor=0;FString Kind;
  bool bTested=false,bFaulty=false; // bFaulty is hidden from the player until tested
  bool Usable() const { return bTested&&!bFaulty; }
 };
 struct FRecipe { const TCHAR* Id; const TCHAR* Title; const TCHAR* Effect; TArray<const TCHAR*> Parts; int32 Cells; int32 Foil; const TCHAR* Stock=nullptr; }; // Stock: makes a usable item instead of a lasting fitting
 static constexpr int32 TraySize=8,PartsPerDonor=4;
 static const TArray<FRecipe>& Recipes();
 static const FRecipe* Recipe(const FString& Id);
 static FString PartTitle(const FString& Kind);
 TArray<FPart> Tray;
 TArray<FString> Built,Seen; // Seen: part kinds handled, which reveals the recipes that use them
 int32 Donors=0,NextId=1;
 bool CanUnpack() const { return Tray.Num()+PartsPerDonor<=TraySize; }
 bool Unpack();
 const FPart* Part(int32 Id) const { return Tray.FindByPredicate([Id](const FPart& P){return P.Id==Id;}); }
 bool Test(int32 Id);
 bool Scrap(int32 Id);
 bool IsBuilt(const FString& RecipeId) const { return Built.Contains(RecipeId); }
 bool Known(const FRecipe& R) const;
 // Empty when buildable; otherwise the exact shortage in player words.
 FString Missing(const FRecipe& R,int32 Cells,int32 Foil) const;
 bool Build(const FString& RecipeId,int32 Cells,int32 Foil); // caller then spends the recipe's cells and foil
 bool CanStrip(const FString& RecipeId) const;
 bool Strip(const FString& RecipeId);
 S47J::FObj Save() const;
 void Load(const S47J::FObj& O);
 static bool SelfTest(TArray<FString>& Lines);
private:
 void AddPart(const FString& Kind,int32 Donor,bool bTested,bool bFaulty);
};
