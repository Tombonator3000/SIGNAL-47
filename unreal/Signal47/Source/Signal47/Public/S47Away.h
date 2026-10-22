#pragma once

#include "CoreMinimal.h"
#include "S47Abduction.h"
#include "S47Json.h"
#include "S47LostNights.h"
#include "S47Seen.h"
#include "S47StarHole.h"
#include "S47TheThird.h"
#include "S47TruckOnB.h"
#include "S47TruckTaken.h"
#include "S47UI.h"
#include "UObject/Object.h"
#include "S47Away.generated.h"

class US47Main;
class SS47AwayOverlay;
class ASceneCapture2D;
class UTextureRenderTarget2D;

// main.ts's wiring of the time away and the trucks: what it builds in boot() for them (the time away,
// story/abduction.ts, with its frame of the scene from a camera of its own and the small probe of what
// would be behind them; taken in the truck, story/truckTaken.ts; the truck on B, story/truckOnB.ts; the
// lost nights, story/lostNights.ts; the third one on FRAME 03, story/theThird.ts; no stars at 026,
// story/starHole.ts; FLASH's frames, story/flashFrames.ts) and what it does with them: abductIn(),
// stationDue(), truckBIn(), truckTakenIn(), holeIn(), bandShine(), thirdNoticed(), their steps in step()'s
// order, their part of seenStep() and seenSync(), the player held on waking (the gaze, the cab's gaze,
// Use in the cab, Q out of it, the footsteps gone), the corner clock in seconds, the music held, the
// hooks into the night (FS47NightHooks), the phones, the album, the chapters, and their part of
// startGame, leavePlay, toTitle, writeSave (abduct, odo, truck, the pose on your back) and saveBlock.
// Held by US47Main (Away); US47Night::Step and US47Night::SeenStep call it in main.ts's order.
//
// What these need of systems ported elsewhere comes in through the hooks below, set by those ports (or
// the integrator); unset, the thing is not there.
UCLASS()
class SIGNAL47_API US47Away : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	virtual void BeginDestroy() override;

	TSharedPtr<FS47Abduction> Abduction;
	TSharedPtr<FS47TruckTaken> TruckTaken;
	TSharedPtr<FS47TruckOnB> TruckB;
	TSharedPtr<FS47LostNights> LostNights;
	TSharedPtr<FS47StarHole> StarHole;
	TSharedPtr<FS47FilmThird> FilmThird;

	// ---------- the hooks into systems ported elsewhere ----------
	/** core/reelTape.ts on Earl's deck (the lost nights' reel). */
	FS47LostTape ReelTape;
	/** Night Shift's threads (NightShiftState.ln): kept here until the host keeps them; the saved case's ns.ln. */
	TArray<FString> Ln;
	/** stationTaken.state === 'idle' and stationTaken.begin(clock) (story/stationTaken.ts, chapter three's port). */
	TFunction<bool()> StationIdle;
	TFunction<void(double)> StationBegin;
	/** What main.ts steps after the time away, in its block (stationTaken.update, the station passed,
	 *  recWalk.update, follow.update: story/stationTaken.ts, recording.ts, dishesFollow.ts); bModal. */
	TFunction<void(double, bool)> AfterAbduction;
	/** recWalk.reset() when Missing time is turned off while it walks (story/recording.ts). */
	TFunction<void()> RecWalkReset;
	/** The town: Town.womanIn and Town.smokerIn (world/Town.ts, the town's port): among people. */
	TFunction<bool()> TownWomanIn, TownSmokerIn;
	/** Chapter six: Chapter6.turnKey (Use in the cab), tryDoor (Q), doorOffered, cbDue, and the old road's
	 *  miles before the line (truckBIn's oldMi). */
	TFunction<bool()> Ch6TurnKey, Ch6TryDoor, Ch6DoorOffered, Ch6CbDue;
	TFunction<TOptional<double>()> Ch6OldMi;
	/** The other voices stop before the reel (stopNarration, cb.stop: their ports). */
	TFunction<void()> HushVoices;

	// ---------- what the chapters and the other night systems ask of these (main.ts) ----------
	/** game.ch4.taken: the time away has come tonight (N. Vega sees the mark it leaves). */
	bool Taken() const { return Abduction && Abduction->Data.Count > 0; }
	/** motel.cb: not in the middle of the time away, in the truck or out of it, nor as the truck comes
	 *  onto B at the turn-off when it could come there (Nora's call waits). */
	bool CbBlocked() const;
	/** ch3.third(cam): the third one on FRAME 03 (unset: none this time). */
	TOptional<FS47FilmShot> ThirdShot(const FS47Cam& Cam);
	/** ch3.onBand: the band on FRAME 03, looked at closely: written down once. */
	void ThirdNoticed();
	/** abductIn(ok) (main.ts). */
	FS47AbductIn AbductIn(bool bOk) const;
	/** The warning's seconds (abduction.warn?.t). */
	TOptional<double> WarnT() const;
	/** The time away holds the player still (the taking and the waking). */
	bool Holds() const { return Abduction && Abduction->Holds(); }
	/** FLASH's frames, shown on the overlay one after another at their own lengths (the developer
	 *  menu, the tests; chapter six shows them with its own timing). */
	void PlayFlash();
	/** How much of the patch with no stars there is now (Sky.hole), and where it is (the sky's own
	 *  direction turned with the area's sky): the album names it. */
	double HoleNow() const { return FS47StarHole::At(Clock(), HoleFrom()); }
	FVector HoleDir() const;
	/** main.ts mapMarks: the times away where they happened, each on the line its place is on. */
	TArray<FS47MapMark> MapMarks() const;
	/** The overlay itself: chapter six may show a frame on it (null: black), and hide it. */
	void ShowFrame(class UTexture2D* T, double Alpha);
	void HideFrame();

	// ---------- step() ----------
	/** No stars where the dishes point (after the glimpses and the town's things on B). */
	void StepHole(double Dt, bool bModal);
	/** The truck on B, taken in the truck, the time away and its block, the corner clock. */
	void StepLate(double Dt, bool bModal);
	/** Every frame (also with a panel open): the outlines over you turn to the eye. */
	void Frame(double Dt);
	/** seenStep: the ones standing, the warning, taken in the truck, the one over you. */
	void SeenStep();
	void SeenSync();

	// ---------- the saves (main.ts startGame, leavePlay, toTitle, writeSave, saveBlock) ----------
	/** startGame, after the curve and townSeen are back: truckTaken.reset, odoRestore, abduction.restore,
	 *  truckB.reset, starHole.reset, the threads. */
	void Restore(const S47J::FObj& State);
	/** startGame, after game.start: abduction.refile, lostNights.restore. */
	void Restored();
	/** leavePlay (bTitle false) and toTitle. */
	void Leave(bool bTitle);
	/** writeSave: abduct, odo, truck and its pose; on your back, Continue stands you up looking ahead. */
	void Save(const S47J::FObj& State) const;
	/** saveBlock: the 47 seconds of the warning are a race; nor while the truck is in its count on B. */
	FString SaveBlock() const;
	/** The save's picture without what stands over you on waking: true when they were there. */
	bool HideOver(bool bHide);

	static bool SelfTest(TArray<FString>& Lines);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	TSharedPtr<SS47AwayOverlay> Overlay;
	bool bOverlayAdded = false;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> ShotTarget;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> ProbeTarget;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> DepthTarget;
	TWeakObjectPtr<ASceneCapture2D> Capture;
	bool bHeldHud = false;
	bool bClockShown = false;
	double HoleClearAt = -1;
	bool bHoleClear = false;
	TArray<TPair<TStrongObjectPtr<class UTexture2D>, double>> Flash;
	double FlashT = 0;
	int32 FlashI = -1;
	// seenStep's was, and the entries it ends
	struct FWas { FString Stage = TEXT("calm"); FString Over; FString Truck = TEXT("idle"); TSet<FString> Records; } Was;
	FS47SeenRef WarnEntry, OverEntry, TruckEntry;
	FString LoggedStage, LoggedTaken, LoggedB;
	TArray<class FS47RuleBook*> Mine;
	TSharedPtr<FS47ThirdFigure> TestThird;

	// ---------- the deps main.ts reads ----------
	bool Modal() const;
	bool Cinematic() const;
	bool Driving() const;
	FString Phase() const;
	bool At(const FString& Chapter) const;
	double Clock() const;
	FString Area() const;
	bool AbductOn() const;
	bool CalmFlash() const;
	FS47Cam Camera() const;
	FVector PlayerAt() const;
	void RequestAutosave() const;
	double Ground(double X, double Z) const;
	bool Standable(double X, double Z) const;
	TOptional<FVector2D> LocalOf(const FString& InArea, const FVector& P) const;
	TOptional<FVector2D> MapOf(const FVector& P) const;
	bool NearArray(double X, double Z) const;
	FString StationDue() const;
	FS47TruckOnBIn TruckBIn(bool bModal) const;
	FS47TruckTakenIn TruckTakenIn(bool bModal) const;
	FS47StarHoleIn HoleIn(bool bModal);
	double BandShine(const FVector& At) const;
	TOptional<double> HoleFrom() const;
	FS47Truck* TruckNow() const;
	bool Shoot(const FS47ShotCam& Cam, int32 W, int32 H, TArray<FColor>& Out, bool bProbe);
	void SyncBooks();
	void BindPlayer();
	void EnsureOverlay();
	void TestFlags();
};
