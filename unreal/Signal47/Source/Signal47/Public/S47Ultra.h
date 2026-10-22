#pragma once

#include "CoreMinimal.h"

class AS47Web;
class USpotLightComponent;
class ULightComponent;

// core/ultra.ts, the PC tier's lamps (Settings: Ultra): the game lights its places with fake
// floodlights (kit.ts), pools without shadows; here the three lamps nearest the player become real
// spotlights that cast soft shadows (the dishes, the fence, the truck throw long shadows across the
// sodium pools). The fake lamp is switched off while its real one burns (its slot holds OFF), and
// comes back when the player walks away; its brightness is matched to the fake pool right under the
// lamp. The desk lamp in the control room casts shadows too. (The post effects and the round 9 maps
// of the tier are core/vhs.ts's and art.ts's, not here.)
class SIGNAL47_API FS47Ultra
{
public:
	static constexpr int32 POOL = 3;
	static constexpr float OFF = -1.2345e-5f;   // what a lamp handed to a real light holds (the shader skips w <= 0)

	bool bOn = false;
	explicit FS47Ultra(AS47Web* W);
	/** A fake lamp's strength as the game wants it, also while it is handed to a real light. */
	float Strength(int32 Set, int32 I) const;
	/** Lights that should cast shadows in this tier (the desk lamp in the control room). */
	void AddShadowLight(ULightComponent* L);
	/** set(on) */
	void Set(bool bInOn);
	/** update(dt, at, keys, extras): the flood sets (by key) that may hand over lamps where the player
	 *  is (empty: none; unset: any), at in three's terms. */
	void Update(double Dt, const FVector& At, const TOptional<TArray<FString>>& Keys, bool bExtras = true);
	/** The lamps held now, as set and slot (tests). */
	TArray<TPair<int32, int32>> Held() const;

private:
	struct FLamp { int32 Set = -1, I = -1; float Want = 0; };
	TWeakObjectPtr<AS47Web> Web;
	TArray<TWeakObjectPtr<USpotLightComponent>> Spots;
	TArray<TOptional<FLamp>> HeldLamps;
	TArray<TWeakObjectPtr<ULightComponent>> Extra;
	double T = 0;
	void Release(int32 K);
	void Park(USpotLightComponent* S);
};
