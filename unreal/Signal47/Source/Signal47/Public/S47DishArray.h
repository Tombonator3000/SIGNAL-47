#pragma once

#include "CoreMinimal.h"
#include "S47Glow.h"

class AS47Web;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

// world/Dish.ts, the parts of it the story moves at run time, beside AS47Web (which has each
// antenna's pointing, curAz/curEl and the targets, slews it and draws the instanced parts):
//
//   Dish       the state a dish has besides its pointing: its red lamp's place in its blink (phase),
//              rogue (driven by nothing on the console: story/dishesFollow.ts), spin (how fast it
//              turns by itself now) and back (turning back to 026 after the schedule:
//              story/arrayBack.ts); snap() through AS47Web.
//   DishArray  the red aircraft lamps on the feeds, one point each: their blink (GlowPoints with a
//              rate of 1.6 and the dish's phase), beaconPhase, holdBeacons with the flare round each
//              lamp (the array's four and seven), and their place on the feed as the dishes turn.
//
// With Exterior.fieldLights (the lamps out among the near dishes: their floods and glows), which
// story/arrayBack.ts dims and turns red. The lamps are the export's billboards (ext.dishArray.beacons
// .points), moved and coloured here; the flare is made here (the export has none: it is made the
// first time it is wanted).
struct SIGNAL47_API FS47Dish
{
	FString Id;
	double X = 0, Z = 0, Scale = 1;
	/** Its red lamp's place in its blink (Math.random() * 6). */
	double Phase = 0;
	/** Driven by nothing on the console (story/dishesFollow.ts): it moves by itself, and how fast it
	 *  turns now (degrees per second, both axes), for the readouts and the closer look. */
	bool bRogue = false;
	double Spin = 0;
	/** Turning back to 026 by itself after the schedule, without a sound (story/arrayBack.ts). */
	bool bBack = false;
};

class SIGNAL47_API FS47DishArray
{
public:
	explicit FS47DishArray(AS47Web* InWeb);

	/** ext.dishes, in the order Exterior.ts builds them (AS47Web's). */
	TArray<FS47Dish> Dishes;

	// ---------- Dish ----------
	double CurAz(int32 I) const;
	double CurEl(int32 I) const;
	void Snap(int32 I, double Az, double El);
	/** Dish.moving: turning by itself, or still slewing to its target. */
	bool Moving(int32 I) const;
	int32 IndexOf(const FString& Id) const;

	// ---------- DishArray ----------
	/** The red lamp's blink, moved along (dishes that follow the player blink together). */
	void BeaconPhase(int32 I, double Phase);
	/** Every red lamp at one brightness at once (the array's four and seven, story/arrayBack.ts),
	 *  with its flare this bright (0: none), or blinking again (unset). */
	void HoldBeacons(TOptional<double> Level, double Flare = 0);
	/** Where the lamps are held now (tests), or unset while they blink. */
	TOptional<double> BeaconsHeld() const { return bHold ? TOptional<double>(Level) : TOptional<double>(); }
	/** How bright the flare is now (tests): 0 while it is not drawn. */
	double Flaring() const { return bFlareOn ? FlareLevel : 0; }
	/** DishArray.update: each lamp where its feed is, and the blink at main.ts's t. */
	void Update(double T, const FVector& Eye);
	/** The lamps' own colour (aCol: world/saroPower.ts takes them down with SARO's power). */
	int32 BeaconCount() const { return BeaconCol.Num(); }
	FLinearColor BeaconBase(int32 I) const { return BeaconCol.IsValidIndex(I) ? BeaconCol[I] : FLinearColor::Black; }
	void SetBeaconBase(int32 I, const FLinearColor& C) { if (BeaconCol.IsValidIndex(I)) BeaconCol[I] = C; }
	/** Where a lamp is (three's world terms). */
	FVector BeaconAt(int32 I) const;

	// ---------- Exterior.fieldLights ----------
	/** The floods out among the near dishes: as built (unset), out (0), or red. */
	void FieldLights(TOptional<double> K, bool bRed = false);
	FString FieldNow = TEXT("on");

	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<AS47Web> Web;
	FS47GlowPoints Beacons;
	TArray<FLinearColor> BeaconCol;
	/** Each lamp's place in its blink (the points' aBlink phase: the dish's own to begin with). */
	TArray<double> BeaconPh;
	TArray<double> Last;
	bool bHold = false;
	double Level = 1;
	// the flare: a wide red haze round each lamp, made the first time it is wanted
	TWeakObjectPtr<UInstancedStaticMeshComponent> Flare;
	TWeakObjectPtr<UMaterialInstanceDynamic> FlareMat;
	bool bFlareOn = false;
	double FlareLevel = 0;
	void MakeFlare();
	// the field's lamps (Exterior.array: LAMPS, the first five with their floods)
	struct FFieldLamp { int32 Flood = -1; double W = 0; int32 Glow = -1; FVector At; };
	TArray<FFieldLamp> FieldLamps;
	FS47GlowPoints ExtGlow;
	int32 SiteSet = -1;
	void FindField();
};
