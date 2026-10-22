#include "S47Ultra.h"

#include "Components/LightComponent.h"
#include "S47Web.h"

FS47Ultra::FS47Ultra(AS47Web* W) : Web(W)
{
	// Every flood lamp now has its own native light. The old nearest-three proxy pool must
	// never take ownership of story slots or switch their widths to the OFF sentinel.
}

float FS47Ultra::Strength(int32 Set, int32 I) const
{
	const AS47Web* W = Web.Get();
	return W ? W->FloodPos(Set, I).A : 0;
}

void FS47Ultra::AddShadowLight(ULightComponent* L)
{
	if (L) L->SetCastShadows(true);
}

void FS47Ultra::Set(bool bInOn)
{
	// Quality scales native renderer settings, never the existence of a lamp or its shadows.
	bOn = bInOn;
}

TArray<TPair<int32, int32>> FS47Ultra::Held() const
{
	return {};
}

void FS47Ultra::Update(double, const FVector&, const TOptional<TArray<FString>>&, bool)
{
}
