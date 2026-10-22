#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47NightEvents.h"
#include "S47Seen.h"
#include "UObject/Object.h"
#include "S47Night.generated.h"

class US47Main;
class FS47NightCurve;
class FS47NightCalls;
class FS47Glimpses;
class FS47Flashlight;
class FS47Album;
struct FS47GlimpsesIn;

// What main.ts asks of the night's other systems, by the names main.ts reads them, for the systems
// not ported yet (story/abduction.ts, core/roadside.ts, story/townEvents.ts, story/roofSteps.ts,
// story/motelTv.ts, story/lineLights.ts, core/reelTape.ts, story/saroDark.ts, story/officePhone.ts,
// story/archivePhone.ts, story/arrayBack.ts, core/cutouts.ts, story/truckOnB.ts, story/truckTaken.ts,
// story/recording.ts, story/morningHolds.ts, story/ninthCow.ts, story/starHole.ts) and the car's
// radio (core/radio.ts, its own port). Each is unset until its port sets it; unset, that system is
// never busy, never waiting, and has nothing to give.
struct FS47NightHooks
{
	TFunction<bool()> AbductionBusy;                 // abduction.busy
	TFunction<int32()> AbductionExtra;               // abduction.data.spots with a doc: exposures on the roll that were not the player's
	TFunction<bool()> RoadsideSilent;                // roadside.silent: the 47 s of silence
	TFunction<void()> RoadsideSilence;               // roadside.silenceNow (the night event 'silence')
	TFunction<FString()> TownEventsNow;              // townEvents.now ('none' when nothing)
	TFunction<bool()> TownEventsWaiting;             // townEvents.waiting: the payphone's first call waits for the player
	TFunction<bool()> RoofStepsBusy;                 // roofSteps.busy
	TFunction<bool()> MotelTvBusy, MotelTvWaiting;   // motelTv.busy, motelTv.waiting
	TFunction<bool()> LineLightsOn;                  // lineLights.state !== 'none'
	TFunction<bool()> ReelTapeBusy;                  // reelTape.busy
	TFunction<bool()> SaroDarkBusy;                  // saroDark.busy
	TFunction<bool()> SaroDarkKeep;                  // chapter one's 47 kept for SARO dark (hondo, phase idle, SaroDark.pending)
	TFunction<bool()> OfficePhoneBusy, OfficePhoneRinging; // officePhone.busy, officePhone.state !== 'idle'
	TFunction<bool()> ArchiveBusy, ArchivePeaking, ArchiveKeep; // archive.busy, .peaking; chapter two's 47 kept for it (archive.pending)
	TFunction<bool()> ArrayBackBusy;                 // arrayBack.busy
	TFunction<bool()> CutoutPost;                    // cutouts.up === 'post'
	TFunction<bool()> TruckBBusy, TruckBPeaking;     // truckB.busy, truckB.peaking
	TFunction<bool()> TruckTakenBusy;                // truckTaken.busy
	TFunction<bool()> RecWalkBusy, RecWalkWaiting;   // recWalk.busy, recWalk.waiting
	TFunction<bool()> MorningBusy;                   // morning.busy
	TFunction<bool()> NinthQuiet;                    // ninth.quiet > 0
	TFunction<void(const FString&, bool)> StarHolePhotographed; // starHole.photographed(shot.id, free)
	// the car's radio
	TFunction<bool()> CarRadioOn;                    // carRadio.on
	TFunction<bool()> Radio1420;                     // carRadio.live && carRadio.station === 'caliche' && radio.stations.caliche.free
	TFunction<bool(double)> CarRadioDrift;           // carRadio.live && carRadio.drift(secs)
	TFunction<void(const FString&)> SetCarRadio;     // carRadio.set(true, station)
	TFunction<double()> Bulletin;                    // radio.stations.caliche.cutIn(BULLETIN, game.clock)
	TFunction<TOptional<FS47ArchiveProgress>()> BulletinProgress; // radio.stations.caliche.archiveStatus
	TFunction<bool()> BulletinHeard;                 // in the cab, on the caliche station, live, before the sign-off
};

/** main.ts saw()'s options: { at, map, line, looking, watch }. Line set (empty for null) is the
 *  thing's own; Looking set (with or without a value) is as given; bNoWatch is watch: null; Watch
 *  is a partial watch (its Pos unset: the thing's place). */
struct FS47SawOpts
{
	TOptional<FVector> At;
	TOptional<FVector2D> Map;
	TOptional<FString> Line;
	bool bLookingSet = false;
	TOptional<bool> Looking;
	bool bNoWatch = false;
	TOptional<FS47SeenWatch> Watch;
};

// main.ts's night layer: the systems it builds in boot() for the night (the seen log, the curve, the
// night's events, the calls out in the dark, the glimpses, the flashlight, the album) and what it
// does with them: saw(), seenStep(), sawKey(), curveStep(), curveSign(), nightIn(), glimpseIn(),
// oneAtATime(), nightGoing(), the steps of step() that are theirs, the F key, and their part of the
// saves (GameState: flashlight, town, seen, curve, album) and of startGame / leavePlay / toTitle.
// Held by US47Main (Night); US47Main::Step calls it in main.ts's order.
UCLASS()
class SIGNAL47_API US47Night : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);

	TSharedPtr<FS47SeenLog> SeenLog;
	TSharedPtr<FS47NightCurve> Curve;
	TSharedPtr<FS47NightEvents> NightEvents;
	TSharedPtr<FS47NightCalls> Calls;
	TSharedPtr<FS47Glimpses> Glimpses;
	TSharedPtr<FS47Flashlight> Flashlight;
	TSharedPtr<FS47Album> Album;
	FS47NightHooks Hooks;

	/** Nothing is written down while no night runs, or while a save puts one back (startGame). */
	bool bSeenQuiet = true;
	/** Seconds the night and the music keep quiet from the first close look at the face. */
	static constexpr double FACE_QUIET = 34;
	double FaceQuiet = 0;
	/** The small signs the curve has given (the tests): which, and at what clock. */
	struct FSign { FString What; double Clock = 0; };
	TArray<FSign> CurveSigns;

	// ---------- main.ts ----------
	FS47SeenRef Saw(const FString& Kind, const FString& What, const FS47SawOpts& O = FS47SawOpts());
	void SawKey(const FString& K);
	/** eventAt(what): where an event's thing is in the area shown (unset: only heard, or not here), and
	 *  EVENT_RANGE: how far off it can be looked at (metres; Else when it has none). */
	TOptional<FVector> EventAt(const FString& What) const;
	static double EventRange(const FString& What, double Else);
	void SeenSync();
	void SeenStep(double Dt);
	void CurveStep(double Dt, bool bModal);
	void CurveSign();
	FS47NightIn NightIn(bool bOk);
	FS47GlimpsesIn GlimpseIn(bool bOk) const;
	bool OneAtATime() const;
	bool NightGoing() const;
	/** The ground under the player, for footsteps (main.ts surfaceAt). */
	FString SurfaceAt() const;
	/** sky.uniforms.uDawn: 0 until 04:40, 1 at sunrise (the world's sky, US47WorldLife's Sky; Sky.setClock
	 *  with main.ts's dawn lift). */
	double Dawn() const;
	/** world.grounds.floorAt: the ground's height anywhere in SARO's scene (world/Grounds.ts). */
	static double SaroFloorAt(const class AS47Web* Web, double X, double Z);

	// ---------- step() ----------
	/** flashlight.update, early in the frame (before the radio and the night outside). */
	void FlashlightStep(double Dt);
	/** The print with the face, seenStep, curveStep, then the night's systems (play only). */
	void Step(double Dt);

	// ---------- the saves ----------
	/** startGame: what the save had (or a new night: null), in main.ts's order. */
	void Restore(const S47J::FObj& State);
	/** startGame's end: where things stand now, without writing anything down; the log listens again. */
	void Restored();
	/** leavePlay and toTitle (bTitle: only the curve). */
	void Leave(bool bTitle = false);
	/** writeSave: flashlight, town, seen, curve, album into the GameState. */
	void Save(const S47J::FObj& State) const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	FVector LastFoot = FVector::ZeroVector;
	double SignFlick = 0, PostFlick = 0;
	struct FWas { FString Glimpse; int32 Calls = 0; bool bRegular = false; } Was;
	/** ch5.s.regular: the waitress has poured for the regular (story/Chapter5.ts, event 11). */
	bool Regular() const;
	int32 CallsLogged = 0;
	FString GlimpseLogged;

	bool Is(const TFunction<bool()>& F) const { return F && F(); }
	FString Phase() const;
	bool At(const FString& Chapter) const;
	double Clock() const;
	bool Modal() const;
	bool Cinematic() const;
	bool Driving() const;
	FString Area() const;
	FS47Cam Camera() const;
	FVector PlayerAt() const;
	FString PlaceName(const FVector* P = nullptr) const;
	TOptional<FVector2D> LocalOf(const FString& InArea, const FVector& P) const;
	TOptional<FVector2D> MapOf(const FVector& P) const;
	FString LineHere(const FVector& P) const;
	void RequestAutosave() const;
	void ToggleLight();
	void TestFlags();
};
