#pragma once

#include "CoreMinimal.h"
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

/** No stars where the antennas point: from 02:15 every dish stands at az 026, el 32 (canon). */
struct FS47StarHole { double Az, El, Inner, Outer, After, Fade; };
inline constexpr FS47StarHole STAR_HOLE = { 26, 32, 3.4, 4.6, 10, 100 };
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

	FS47Sky();
	/** Dawn remains 04:40-05:30, unchanged through 05:40. Midday blends 05:40-06:00 and 17:00-18:00.
	 *  Evening retains westward sun, sunset 18:27 and blue hour to 19:07.
	 *  The optional elevation remains the finale's override; it never changes the evening clock. */
	void SetClock(double Clock, TOptional<double> SunElevDeg = TOptional<double>());
	/** setDawnLift(k): how much of the dawn an area shows. */
	void SetDawnLift(double K) { DawnLift = K; }
	/** setCometClock: up from the evening until a little after three. */
	void SetCometClock(double Clock);
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

	// ---------- FX-01: the short meteor passage by the star hole (Unreal only) ----------
	// NIAGARA_VFX_2026-10-28.md: a few streaks become a short, denser passage in the sector next to
	// 026 / 32 and die away as the hole begins; then the sky goes back to its rare shooting stars.
	// An extension of the material-drawn shooting star (M_WebMeteor, same strip and shader, a small
	// fixed pool), not a Niagara system: the sky is drawn in its own terms round the eye. The phase is
	// the night's clock against the real event (HoleFrom = EventClock + STAR_HOLE.After), so pause
	// and open panels freeze it, a load restores it, and a spent passage (saved) never plays again.
	struct FPassMet { double At = 0, Dur = .6, Len = .15, Bright = 1; FVector Start = FVector::ZeroVector, Tan = FVector::ZeroVector; bool bLow = true; };
	/** Seconds: from HoleFrom - LEAD (the event + 4 s, one second after the dishes begin to turn) for LEN, each streak's tail TAIL. */
	static constexpr double PASSAGE_LEAD = 6, PASSAGE_LEN = 11, PASSAGE_TAIL = .2, PASSAGE_QUIET = 60;
	static constexpr int32 PASSAGE_SLOTS = 6;
	static const TArray<FPassMet>& PassageSchedule();
	/** Seconds since the passage began at this clock (night hours only); -1e9 when there is none tonight. */
	static double PassagePhaseAt(double Clock, double HoleFrom);
	/** One streak's strip at T seconds into it (as meteorStep lays the shooting star out): false outside it. */
	static bool PassageLayout(const FPassMet& P, double T, double& OutHead, double& OutBack, double& OutFade);
	static int32 PassageCount(bool bLow);
	/** The most streaks crossing at once on that tier (sampled every 10 ms). */
	static int32 PassageMaxHeads(bool bLow);
	/** Low (graphics menu Effects Low): fewer streaks, at most three at once. */
	bool bPassageLow = false;
	/** Spent: saved with the case (Flow: meteorPassage). Set once the clock is past its end. */
	bool bPassageDone = false;
	/** Phase now (seconds, negative before), the streaks crossing now and those begun this playback. */
	double PassageT = -1e9;
	int32 PassageHeads = 0, PassageStarted = 0;
	bool PassageActive() const { return !bPassageDone && PassageT >= 0 && PassageT <= PASSAGE_LEN + PASSAGE_TAIL; }
	/** A new night or a load: spent or not, and nothing of a previous playback counted. */
	void PassageReset(bool bDone) { bPassageDone = bDone; PassageT = -1e9; PassageHeads = PassageStarted = 0; PassageBits = 0; }
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

private:
	double DawnLift = 1;
	struct FMet { double T = -1, Dur = 0.6, Len = 0.15, Bright = 1, Next = 10; FVector Start = FVector::ZeroVector, Tan = FVector::ZeroVector; };
	FMet Met;
	double FlashT = 0, NextFlash = 9;
	// the streak as meteorStep lays it out (its corners are the material's: M_WebMeteor)
	bool bMetVisible = false;
	double MetI = 0, Head = 0, Back = 0;
	void MeteorStep(double Dt);
	uint64 PassageBits = 0;
	void PassageStep(double Clock);
	TArray<TWeakObjectPtr<UStaticMeshComponent>> PassComps;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> PassMids;

	bool bLive = false;
	TWeakObjectPtr<AS47Web> Web;
	int32 GroupNode = -1;
	int32 ConceptBackdropNode = -1;
	TArray<TPair<int32, bool>> ConceptLegacyMesas;
	bool bConceptLegacyMesasHidden = false;
	TWeakObjectPtr<UStaticMeshComponent> DomeComp, CometComp, MeteorComp, BandComp;
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
