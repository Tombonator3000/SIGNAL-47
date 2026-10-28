#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

class US47Main;

// Explicit, isolated live controller: -S47HorrorContract=SC01 -UserDir=/tmp/...
// Root invokes Step after the production Horror coordinator's update.
class FS47HorrorTest
{
public:
	explicit FS47HorrorTest(US47Main* InM);
	~FS47HorrorTest();
	void Step(double Dt);

private:
	struct FState;
	TUniquePtr<FState> State;
	US47Main* M = nullptr;
	void Fail(const FString& Why);
	bool Check(bool Good, const FString& Why);
	void Stage(int32 Next, double Hold = .2);
	FS47Obj Object(const FString& Id) const;
	FVector Target(const FString& Id) const;
	void Aim(const FVector& Point);
	bool Pick(const FString& Id, const FString& Alternate = FString(), bool bUse = true);
	void Path(const TArray<FVector>& Points);
	bool Walk();
	bool StoreRestore(const FString& Name, const S47J::FObj& Snapshot, bool bRestore = true);
	void Shot(const FString& Name);
};
