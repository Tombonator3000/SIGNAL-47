#pragma once

#include "CoreMinimal.h"
#include "S47Glow.h"
#include "S47Obj.h"

class AS47Web;
class FS47DishArray;
class US47Annex;

// web/src/world/saroPower.ts. SARO's own power (story/saroDark.ts: 02:29, the station goes dark for
// 47 seconds). Every light that is SARO's goes with it: the control room's lamps, tubes and screens,
// the annex's tubes, the photo lab's safelights and lamps, the red glass of the DARKROOM lamp, the
// yard's and the array's sodium lamps, the wall packs, the drive cabinets' lamps and the red aircraft
// lamps on all 27 antennas. What is not SARO's stays lit: the highway's street lamps along the road
// west of the site (the county's, on their own line: their orange spills over the grounds by the
// windows side, and a figure standing there is black against it), the motel's sign far down the
// road, the truck's own lights, and the lit places on the horizon.
//
// Two levels: `inner` for what comes straight back (tubes, safelights, incandescent lamps, screens,
// the red lamps) and `sodium` for the sodium lamps outside, which come back dim and take their time
// to warm up again, as sodium does after an outage. The state before the cut is taken whole and put
// back exactly (restore()), so nothing that was dimmed or switched before (the tube that died at the
// impact, the bank's screen with K3 open, the B-12 work lamp) is changed by it. Once the inside has
// struck again (inner back to 1, `settle`), it is left alone; and a lamp the game changes itself while
// the sodium lamps warm up (the B-12 work lamp with its shield) keeps what the game gave it: that
// becomes the level the lamp warms up to.
//
// In the export: the room's lights by their handles, the tubes and screens through AS47Web, the
// glows as their billboards (S47Glow.h), the site's flood pools, the lab's and the annex's flood
// scales, and the materials (the red glass, the paint, the lit glass, M.lampBulb) by their handles.
// (core/ultra.ts, the PC tier's real lamps, is not in the Unreal version: a lamp's strength is its
// pool's width. The antenna field's paint, world.field.paint, has no handle in the export.)
struct FS47PowerParts
{
	TWeakObjectPtr<AS47Web> Web;
	/** The red lamps on the antennas (the array's beacon glow). */
	FS47DishArray* Dishes = nullptr;
	/** The records annex (annex.setPower): chapter two's, once it is made. */
	TFunction<US47Annex*()> Annex;
};

class SIGNAL47_API FS47SaroPower
{
public:
	explicit FS47SaroPower(FS47PowerParts InP);

	double Inner = 1;
	double Sodium = 1;
	/** The power is out, or coming back (anything but the state before the cut). */
	bool Out() const { return Snap.IsSet(); }

	/** The power goes: everything as it is now is kept, and all of it dark. */
	void Cut();
	/** Levels while out or coming back (0..1 each). */
	void Set(double InInner, double InSodium);
	/** The inside back as it was, and left alone from now on (the tubes, the safelights, the screens,
	 *  the red lamps: they come straight back; the sodium lamps go on warming up). */
	void Settle();
	/** The power is back and everything as it was before the cut (or as the game has set it since). */
	void Restore();
	/** Back to the state before the cut at once (a new night, the title, a save opened). */
	void Reset() { Restore(); }

	/** Warm, high sodium heads among the glows (the same test as world/airlight.ts). */
	static bool IsSodium(double R, double G, double B) { return R > 0.85 && G > 0.3 && G < 0.62 && B < 0.26; }
	/** Where SARO is in its own scene: not the highway's lamp line (x -20), the motel down the road or
	 *  the horizon's lights. */
	static bool SaroAt(double X, double Z) { return Z < 300 && FMath::Sqrt(X * X + Z * Z) < 520 && FMath::Abs(X + 20.05) > 1.3; }

private:
	FS47PowerParts P;
	struct FGlowSnap
	{
		FS47GlowView G;
		bool bBeacons = false;   // the antennas' red lamps (their colour is the dish array's aCol)
		TArray<FLinearColor> Col, LastC;
		TArray<int32> Pick;
		TArray<bool> bSodium;
	};
	struct FSlot { int32 I = 0; double W = 0, Last = 0; };
	struct FSnap
	{
		TArray<float> Ceiling, Lamps;
		float Crt = 0;
		TArray<bool> Tubes, Screens;
		TArray<TArray<FLinearColor>> Door;
		float Lab = 0, Annex = 0;
		TArray<FLinearColor> Red;
		FLinearColor Bulb;
		TArray<FLinearColor> Paint, Glass;
		TArray<FGlowSnap> Glows;
		TArray<FSlot> Slots;
		bool bSettled = false;
	};
	TOptional<FSnap> Snap;
	// what is found in the export, once
	TArray<FS47Light> Ceiling, Lamps;
	FS47Light Crt;
	FS47Obj DoorLight;
	int32 LabSet = -1, AnnexSet = -1, SiteSet = -1;
	TArray<FS47Mat> RedGlass, Paint, Screens;
	FS47Mat LampBulb;
	TSet<int32> SiteSkip;
	bool bFound = false;
	void Find();

	void Inside(FSnap& S, double InInner);
	FSnap Take();
	double Strength(int32 I) const;
	FLinearColor GlowCol(const FGlowSnap& G, int32 I) const;
	void SetGlowCol(const FGlowSnap& G, int32 I, const FLinearColor& C) const;
};
