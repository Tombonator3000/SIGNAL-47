#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"

class US47Main;

// world/ninthCow.ts: buildNinthCow's meshes, pictures and hit areas are exported intact. The
// native module binds that result; Station01.ts's setNinth/ninthAt keep its visibility, proxies
// and footprint together. No hide, cuts, frost picture or geometry is rebuilt by hand.
class SIGNAL47_API FS47NinthModel
{
public:
	explicit FS47NinthModel(US47Main* InM);
	static constexpr double FROST_R = 3.3;
	FS47Obj Group;
	FS47Obj HeadHit, BodyHit, FrostHit;
	TArray<FS47Obj> Occluders;
	bool bNinthDown = false;
	bool IsValid() const { return Group.IsValid(); }
	void SetNinth(bool bOn);
	FVector NinthAt(const FString& Part) const;
	bool Occluded(const FVector& From, const FVector& To, double Near, double Far) const;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	FS47Obj SiteGroup;
	TMap<FString, FVector> At;
	FS47Area Box;
};
