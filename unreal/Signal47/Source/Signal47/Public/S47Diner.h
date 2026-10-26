#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"

class US47Main;
class UTexture;
class UMaterialInstanceDynamic;

// world/Diner.ts: the exported place, its people, proxies and changing light.
class SIGNAL47_API FS47Diner
{
public:
	explicit FS47Diner(US47Main* InM);
	FS47Obj Group, Interior;
	FS47Obj Obj(const FString& Id) const;
	FS47Obj Proxy(const FString& Id) const;
	bool bSignLit = true;
	double Dawn = 0;
	void SetSurfaceArt(UTexture* Booth, UTexture* Wall);
	void SetSignLit(bool bOn);
	void SetDawn(double K);
	void SetNear(double W);
	// Diner.ts shine/neon: copies of the driven headlights and sign lamps in another flood set.
	void Shine(int32 Src, const TArray<int32>& Slots, double K);
	void Neon(int32 Out, const TArray<int32>& Slots, double Reach, double K);
	void Update(double Dt, double T);
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	int32 Flood = -1;
	double Near = 1;
	struct FAuthoredGlow { TWeakObjectPtr<UMaterialInstanceDynamic> Mid; FLinearColor Base; int32 Kind = 0; };
	TArray<FAuthoredGlow> AuthoredGlows;
	void BindAuthoredLighting();
	void ApplyAuthoredLighting();
};
