#pragma once

#include "CoreMinimal.h"

class AS47Web;
class ULightComponent;

// Compatibility with WorldLife's quality callbacks. Native lighting owns every lamp on every
// tier; the former nearest-three spotlight pool and its fake-flood ownership are retired.
class SIGNAL47_API FS47Ultra
{
public:
	bool bOn = false;
	explicit FS47Ultra(AS47Web* W);
	float Strength(int32 Set, int32 I) const;
	void AddShadowLight(ULightComponent* L);
	void Set(bool bInOn);
	void Update(double Dt, const FVector& At, const TOptional<TArray<FString>>& Keys, bool bExtras = true);
	TArray<TPair<int32, int32>> Held() const;

private:
	TWeakObjectPtr<AS47Web> Web;
};
