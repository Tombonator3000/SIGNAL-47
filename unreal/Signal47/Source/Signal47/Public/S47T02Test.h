#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

class US47Main;

// Opt-in live API/disk contract. Root calls Step after Office/Substation production steps.
// It requires developer case zero and an explicit isolated /tmp UserDir.
class FS47T02Test
{
public:
 explicit FS47T02Test(US47Main* InM);
 ~FS47T02Test();
 void Step(double Dt);
private:
 struct FState;
 TUniquePtr<FState> State;
 US47Main* M = nullptr;
 void Fail(const FString& Why);
 bool Check(bool Good, const FString& What);
 void Stage(int32 Next, double Hold = .2);
 FS47Obj Object(const FString& Id) const;
 FVector Target(const FString& Id) const;
 void Aim(const FVector& Point);
 bool Pick(const FString& Id, const FString& Alternate = FString(), bool bUse = true);
 void Path(const TArray<FVector>& Points);
 bool Walk();
 bool AuditNative(double Dt);
 bool SaveLoad(const FString& Name, const S47J::FObj& Source, bool WantVerified, bool RemovePhotoFile = false);
 bool PlaceAtSaroFolder();
 bool NoTransformerContext() const;
 bool CheckFiledPaper(const S47J::FObj& OriginalCard, const FString& Context);
 bool AdvanceDialogue(const FString& Choice = FString());
 void Shot(const FString& Name);
};
