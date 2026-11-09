#pragma once

#include "CoreMinimal.h"
#include "Containers/BitArray.h"
#include "S47SkyEventSpec.h"
#include "UObject/StrongObjectPtr.h"

class AS47Web;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

// world/Sky.ts: what the night sky does over the night. The dome, the stars, the comet, the shooting
// star and the painted horizon (backdrop.ts) are the export's sky group, drawn here with their
// shaders ported (import_web.py: M_WebSkyDome, M_WebStars, M_WebMeteor, M_WebSkyBand) and their
// uniforms set each frame: the clock (the dawn, the sun, the star hole after 02:15), the comet's
// hours, the shooting stars (one every 18 to 40 s at night, and the developer menu's), the
// lightning in a storm on the horizon with its thunder. The group follows the eye and turns with
// the area (World.skyYaw); here it is drawn ten times as far off as three has it (SKY_SCALE), past
// the farthest land, as three draws it at the far plane. Without the new materials (an older
// import) the export's picture of the sky stays (AS47Web's sky sphere) and only the numbers run.

// No stars where the antennas point: from 02:15 every dish stands at az 026, el 32 (canon).
// The field's place, rims and fade are the shared STAR_HOLE (S47SkyEventSpec.h).
/** skyDir: a direction in the sky's own terms (azimuth clockwise from north, -Z; east +X; elevation). */
SIGNAL47_API FVector S47SkyDir(double Az, double El);
/** starHoleAt: how much of the hole there is at a clock, when it begins at `from` (Infinity: not tonight yet). */
SIGNAL47_API double S47StarHoleAt(double Clock, double From);
/** Render-only art calibration around Caliche; zero at SARO and the story's other locations. */
SIGNAL47_API double S47CalicheVisualWeight(const FVector& EyeThree);

class SIGNAL47_API FS47Sky
{
public:
	static constexpr double SKY_SCALE = 10;

	// ---------- the uniforms (Sky.uniforms) ----------
	double Time = 0;
	double Flash = 0;
	FVector FlashDir = FVector(1, 0.05, -0.3).GetSafeNormal();
	double Dawn = 0;                      // Morning-only curve; story gates keep their old meaning.
	double Evening = 0;                   // Late afternoon -> sunset -> blue hour, independent of Dawn.
	double Midday = 0;                    // Rendering only: full daylight 06:00-17:00; never a story Dawn gate.
	/** Pure Sky is visual only: 06:00-07:00 in, 18:00-18:53 out; original night and dawn stay exact. */
	double PureSkyWeight = 0;
	static double PureSkyWeightAt(double Clock);
	/** Rendering only: how much the land is still lit from the open sky with the sun down, 0 to 1. Never a story gate, never the dome's Daylight(). */
	double Twilight = 0;
	static double TwilightAt(double Clock);
	/** Rendering only (SKY.md): the volumetric layer's cover and the cirrus above it, from the clock.
	 *  Day and dusk a broken 40-70 % layer; 18:50-21:00 it breaks up to a few night clouds (never
	 *  overcast); 04:30-06:30 it builds back. Story beats keep open sky locally (Draw: clear directions). */
	double CloudCover = 0.55, Cirrus = 0.45;
	static double CloudCoverAt(double Clock);
	static double CirrusAt(double Clock);
	/** The shared day/night response of the existing dome, stars, horizon and native lights. */
	double Daylight() const { const double Base = FMath::Max(Dawn, Evening); return Base + (1.0 - Base) * Midday; }
	/** Stars stay absent until civil twilight; the old morning star fade remains unchanged. */
	double StarDimming() const { const double Base = Evening > 0 ? FMath::Min(1.0, Evening / 0.05) : Dawn; return Base + (1.0 - Base) * Midday; }
	/** Rendering only: diameter multiplier on the private StarsMid's existing PR uniform. */
	double FinaleStarScale = 1;
	/** Local display-only dimming floor on StarsMid; never changes Dawn or StarDimming(). */
	double FinaleStarDim = 0;
	/** Rendering only: 1 keeps ordinary weather; ready finale can fade it without changing its clock curves. */
	double FinaleWeatherWeight = 1;
	FVector SunDir = FVector(0.98, -0.02, -0.19).GetSafeNormal();
	double Sun = 0;
	FVector4 Glow[3] = { FVector4(1, 0, 0, 1), FVector4(1, 0, 0, 1), FVector4(1, 0, 0, 1) }; // uGlow (backdrop.ts sets them)
	FVector HoleDir = S47SkyDir(STAR_HOLE.Az, STAR_HOLE.El);
	FVector2D HoleR;
	double HoleK = 0;                     // uHole
	/** When the hole begins tonight (seconds since midnight; main.ts sets it each frame). */
	double HoleFrom = 2 * 3600 + 15 * 60 + 12 + STAR_HOLE.After;
	/** The haze at the horizon now: the scene's fog follows it (main.ts). */
	FLinearColor FogColor = FLinearColor(0.043f, 0.059f, 0.098f);
	bool bComet = true;
	int32 Meteors = 0;                    // how many have crossed the sky (tests)
	/** onThunder(delay, strength): main.ts plays it after the delay. */
	TFunction<void(double, double)> OnThunder;
	/** WEATHER-20261108: thunder far off by day under a shower's cloud, 0 to 1 (FS47WeatherLive): heard now and then, no flash. */
	double DayThunder = 0;

	FS47Sky();
	/** Dawn remains 04:40-05:30, unchanged through 05:40. Midday blends 05:40-06:00 and 17:00-18:00.
	 *  Evening retains westward sun, sunset 18:27 and blue hour to 19:07.
	 *  The optional elevation remains the finale's override; it never changes the evening clock. */
	void SetClock(double Clock, TOptional<double> SunElevDeg = TOptional<double>());
	/** setDawnLift(k): how much of the dawn an area shows. */
	void SetDawnLift(double K) { DawnLift = K; }
	/** setCometClock: up from the evening until a little after three. */
	void SetCometClock(double Clock);
	/** MOON-20261108: the moon where S47Weather::Moon has it (degrees), the share of it lit, and the sun's place for its lit side. */
	void SetMoon(bool bShow, double Az, double El, double Lit, double SunAz, double SunEl);
	/** PT-MOON-20261109, F10 "World fixes (T)": the moon put where a line wants it (degrees, share lit), the sun its elongation away to the
	 *  west; CloudCover >= 0 holds the sky's cover; Old draws the painted disc of 8 November for a before picture. Off: as the date has it. */
	struct FDevMoon { bool bOn = false; double Az = 180, El = 40, Lit = .5, CloudCover = -1; bool bOld = false; };
	static FDevMoon DevMoon;
	/** The moon's plane uses M_S47_Moon (NASA's near side, its phase shaded, hidden by clouds); false: the painted fallback. */
	bool MoonTextured() const { return bMoonTex; }
	/** meteorNow(): a shooting star now, unless one is crossing already. */
	void MeteorNow();
	/** Tests (-S47Meteor with -S47MeteorAim): the shooting star now starts where Dir points (the sky's terms). */
	void MeteorFrom(const FVector& Dir);
	/** update(dt, t, follow): the shooting stars and the lightning (the group's place: Draw). */
	void Update(double Dt, double T);

	// ---------- Unreal ----------
	/** The sky group's parts from the export (AS47Web, its life data): false when there is no live sky. */
	bool Build(AS47Web* W);
	bool IsLive() const { return bLive; }
	/** Where the eye is (three's terms) and how the sky is turned (World.skyYaw): the group's place and
	 *  every part's parameters. */
	void Draw(const FVector& Eye, double Yaw);
	/** The shooting star's state (tests): visible, its brightness. */
	bool MeteorVisible() const { return bMetVisible; }
	double MeteorI() const { return MetI; }

	// ---------- FX-01: the meteor passage by the star hole (Unreal only) ----------
	// PLAY-31-23: once the signal has turned the dishes and the player steps out of the SARO building,
	// a dense 36 s passage of streaks round 026 / 32; several cross BEHIND the empty field (its mask
	// hides head and tail alike, then they come out beyond it). Then the rare shooting stars return.
	// An extension of the material-drawn shooting star (M_WebMeteor, same strip, shader and hole mask,
	// a bounded pool), not a Niagara system. The renderer owns no lifecycle: US47WorldLife decides
	// pending/running/done, saves it, and hands the presentation time here (SetPassageTime).
	struct FPassMet
	{
		double At = 0, Dur = .6, Len = .15, Bright = 1;
		/** How much of the way the drawn trail spans in flight (meteorStep: .65). Crossers draw a shorter one. */
		double Trail = .65;
		FVector Start = FVector::ZeroVector, Tan = FVector::ZeroVector;
		bool bLow = true;
		/** Crosses behind the field: enters and is wholly hidden inside the inner rim, then emerges beyond the outer one. */
		bool bCross = false;
		/** Its strip in the pool, assigned once so no two streaks drawn at the same time share one. */
		int32 Slot = 0;
	};
	/** Seconds: the passage's length (each streak inside it) and every streak's lingering tail. */
	static constexpr double PASSAGE_LEN = S47SkyEvent::PassageLength, PASSAGE_TAIL = S47SkyEvent::PassageTail, PASSAGE_QUIET = 60;
	/** The pool of strips (High's concurrency cap) and Low's own cap on streaks drawn at once. */
	static constexpr int32 PASSAGE_SLOTS = 12, PASSAGE_LOW_HEADS = 5;
	static const TArray<FPassMet>& PassageSchedule();
	/** One streak's strip at T seconds into it (as meteorStep lays the shooting star out): false outside it. */
	static bool PassageLayout(const FPassMet& P, double T, double& OutHead, double& OutBack, double& OutFade);
	static int32 PassageCount(bool bLow);
	/** The most streaks drawn at once on that tier (sampled every 10 ms). */
	static int32 PassageMaxHeads(bool bLow);
	/** How many streaks on that tier cross behind the field. */
	static int32 PassageCrossers(bool bLow);
	/** A streak against STAR_HOLE (seconds into it): its head first inside the inner rim, its whole drawn
	 *  segment first inside it, its head first out beyond the outer rim again while still in flight. */
	static bool PassageMaskProfile(const FPassMet& P, double& OutEnter, double& OutHidden, double& OutEmerge);
	/** Tests: passage times at which a crossing streak of that tier is wholly hidden behind the field. */
	static TArray<double> PassageHiddenMoments(bool bLow);
	/** Low (graphics menu Effects Low): fewer streaks, at most PASSAGE_LOW_HEADS at once. */
	bool bPassageLow = false;
	/** Life's terminal state, mirrored for drawing and tests; the renderer never decides it. */
	bool bPassageDone = false;
	/** Presentation time now (-1: hidden), the streaks drawn by it, those begun since the last reset;
	 *  of those drawn, how many reach into the field's outer rim and how many lie wholly in its inner one. */
	double PassageT = -1;
	int32 PassageHeads = 0, PassageStarted = 0, PassageMasked = 0, PassageHidden = 0;
	bool PassageActive() const { return !bPassageDone && PassageT >= 0 && PassageT <= PASSAGE_LEN + PASSAGE_TAIL; }
	/** Life's presentation time. Negative (or done) hides the strips only: counters and progress stay.
	 *  The same value holds the same image; time never runs back, so nothing replays before a reset. */
	void SetPassageTime(double Elapsed, bool bDone = false);
	/** A new night or a load: spent or not, and nothing of a previous playback counted. */
	void PassageReset(bool bDone);
	int32 PassageSlotsBuilt() const { return PassComps.Num(); }
	int32 PassageSlotsVisible() const;
	/** Tests: the first drawn strip's bounds and whether the renderer drew it lately. */
	FString PassageDebug() const;
	/** Tests: how far along the most advanced crossing streak is (0..1, its head; -1 none). */
	double PassageLongest() const;
	/** Tests: draw the passage's strips this frame or not (an A/B pair of the same frozen moment). */
	bool bPassageHiddenForTest = false;
	/** The passage's streaks against the rare shooting star (same strip and shader; brighter, being several at once in one sector). */
	static constexpr double PASSAGE_GAIN = 2.0;
	static bool PassageSelfTest(TArray<FString>& Lines);

	// ---------- the developer menu's meteor shower (S47DevEvents) ----------
	/** Its own bounded pool: several streaks drawn at once, each on a free strip of its own. */
	static constexpr int32 SHOWER_SLOTS = 10;
	/** Seconds between its streaks (each lasts .65 to 1.15 s, so three to five are usually crossing at once). */
	static constexpr double SHOWER_GAP_MIN = .12, SHOWER_GAP_MAX = .42;
	/** A new shower: a fresh radiant high in the sky, and none of the last one left. */
	void ShowerBegin();
	/** A streak from the radiant now (Ahead: seconds already flown); false when every strip is in use. */
	bool ShowerMeteor(double Ahead = 0);
	void ShowerClear();
	/** Held by the developer events' free-play guard (menu, modal, story pause): the image freezes. */
	bool bShowerHeld = false;
	int32 ShowerStarted = 0;
	int32 ShowerLive() const;
	int32 ShowerSlotsBuilt() const { return ShowerComps.Num(); }
	int32 ShowerSlotsVisible() const;
	/** SHOWER-LOOK-20261109: the shower's cloud is this many cards (DrawCell). */
	static constexpr int32 CellCards = 3;

private:
	double DawnLift = 1;
	struct FMet { double T = -1, Dur = 0.6, Len = 0.15, Bright = 1, Next = 10; FVector Start = FVector::ZeroVector, Tan = FVector::ZeroVector; };
	FMet Met;
	double FlashT = 0, NextFlash = 9, DayThunderIn = 8;
	// the streak as meteorStep lays it out (its corners are the material's: M_WebMeteor)
	bool bMetVisible = false;
	double MetI = 0, Head = 0, Back = 0;
	void MeteorStep(double Dt);
	/** Which scheduled streaks have begun since the last reset (any number of them, not a 64-bit word). */
	TBitArray<> PassageSeen;
	/** The furthest presentation time since the last reset: a smaller one cannot replay. */
	double PassageHigh = -1;
	/** The rare shooting stars wait this long after a passage that was seen. */
	double QuietLeft = 0;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> PassComps;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> PassMids;
	struct FShowerMet { FPassMet P; double T = -1; bool bFresh = false; };
	FShowerMet Shower[SHOWER_SLOTS];
	FVector ShowerRadiant = FVector(0, 1, 0);
	void ShowerStep(double Dt);
	TArray<TWeakObjectPtr<UStaticMeshComponent>> ShowerComps;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> ShowerMids;

	bool bLive = false;
	TWeakObjectPtr<AS47Web> Web;
	int32 GroupNode = -1;
	int32 ConceptBackdropNode = -1;
	TArray<TPair<int32, bool>> ConceptLegacyMesas;
	bool bConceptLegacyMesasHidden = false;
	TWeakObjectPtr<UStaticMeshComponent> DomeComp, CometComp, MeteorComp, BandComp, MoonComp;
	// M5 sweep 2: the day's weather over the sky (S47Weather::Live, Freeform only). By day the sky is a photograph that neither the
	// cloud cover nor the sun's disc reaches, so a sand day stayed blue and a shower clear: a sphere inside the dome, unlit and
	// translucent, brown in the sand and grey under a shower's cloud, hidden when there is no such weather.
	TWeakObjectPtr<UStaticMeshComponent> VeilComp;
	TStrongObjectPtr<UMaterialInstanceDynamic> VeilMid;
	bool bVeilTried = false;
	void DrawVeil(AS47Web* W, const FVector& Eye);
	// M5 sweep 3: the shower itself. The veil alone only greyed the blue (sweep-003: still a blue sky with white cloud): a painted card on
	// the side the shower comes from (upwind), CellDistance out and turned to the eye, a dark, heavy cloud with grey virga hanging under
	// it that dries before it reaches the ground. Drawn over the veil; hidden by night and without a shower.
	// SHOWER-LOOK-20261109: one flat card with a hard, straight base read as a painted plate. Now three, each its own picture (a soft,
	// torn base, rain shafts that sag it, virga that curls back with the wind and dries at its own height): the cell, a paler mass
	// behind it and a smaller, darker piece in front, on a little different distances and to either side; and they walk past with the
	// wind on the shower's own clock (CellPlace): upwind before, close beside the land while it rains, downwind after, gone in the end.
	TWeakObjectPtr<UStaticMeshComponent> CellComp[CellCards];
	TStrongObjectPtr<UMaterialInstanceDynamic> CellMid[CellCards];
	bool bCellTried = false;
	void DrawCell(AS47Web* W, const FVector& Eye, double Rain, double Day);
public:
	/** The veil's colour and opacity for the sand (0 to 1), a shower's cloud (0 to 1) and the daylight (tested). */
	static void VeilOf(double Sand, double Shower, double Day, FLinearColor& OutColor, double& OutOpacity);
	/** The shower cloud's picture, Wd by Ht, its top row first (tested): the cloud, the virga under its base, nothing at the foot.
	 *  Variant: 0 the cell, 1 the mass behind it, 2 the piece in front (each its own lumps, shafts and torn base). */
	static void CellPixels(int32 Wd, int32 Ht, TArray<FColor>& Out, int32 Variant = 0);
	/** Where the shower stands from the eye and how big its card is (metres). */
	static constexpr double CellDistance = 7000, CellWidth = 14000, CellHeight = 4000;
	/** How fast the shower walks with the wind (metres an hour), and how far to the side of the land it passes (metres). */
	static constexpr double CellSpeed = 9000, CellAside = 3200, CellFar = 16000;
	/** Where the shower's cell stands from the eye (three-space metres, x east, z south) ShowerH hours after it set in, with the wind
	 *  blowing toward (DirX, DirZ), and how much of it shows for its distance (0 gone over the horizon). Without a shower's clock
	 *  (S47Weather::NoShower) it stands CellDistance upwind, as it did. */
	static void CellPlace(double DirX, double DirZ, double ShowerH, FVector2D& OutOffset, double& OutShow);
private:
	TStrongObjectPtr<UMaterialInstanceDynamic> MoonMid, MoonTexMid;
	bool bMoon = false, bMoonTex = false;
	double MoonAz = 0, MoonEl = -90, MoonLit = 0, MoonSunAz = 0, MoonSunEl = -90, MoonPainted = -1, StarRadius = 400;
	double MoonStarDim = 0; // a bright moon high washes out the faint stars and the Milky Way
	void PaintMoon();
	TWeakObjectPtr<UInstancedStaticMeshComponent> StarsComp;
	// The inactive dome must survive GC while the component holds the other day/night material.
	TStrongObjectPtr<UMaterialInstanceDynamic> DomeMid, PureSkyMid;
	TWeakObjectPtr<UMaterialInstanceDynamic> BandMid, StarsMid, MeteorMid;
	FLinearColor HoleSky = FLinearColor(0.006f, 0.008f, 0.02f);

public:
	// what backdrop.ts sets on its own uniforms each frame (FS47Backdrop::Update): the band's MID
	UMaterialInstanceDynamic* Band() const { return BandMid.Get(); }
	bool BandVisible = true;
};
