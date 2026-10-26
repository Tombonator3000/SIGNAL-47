#pragma once

#include "CoreMinimal.h"
#include "S47DishesFollow.h"
#include "S47Json.h"
#include "S47Recording.h"
#include "S47ReelTape.h"
#include "S47Seen.h"
#include "UObject/Object.h"
#include "S47SaroNight.generated.h"

class US47Main;
class FS47DishArray;
class FS47SaroPower;
class FS47SaroDark;
class FS47ArrayBack;
class FS47LineLights;
class FS47Reel047;
class FS47DspScreen;
class UTextureRenderTarget2D;
class UTexture;
struct FS47DarkIn;

// What main.ts reads of the time away (story/abduction.ts) for SARO's own happenings: the dishes that
// follow go with its warning and its waking, the array leaves the schedule for its warning, and the
// recording walks one of its times away. The abduction is its own port: unset, there is no time away
// (the stage is 'calm'), the dishes' save is kept here (AbductSave.dishes, written into the state's
// 'abduct' as the web version has it), and there are no recordings.
struct FS47AbductView
{
	TFunction<FString()> Stage;                         // abduction.stage
	TFunction<TOptional<FS47FollowIn::FWarn>()> Warn;   // abduction.warn: { t, len }
	TFunction<TOptional<FVector>()> BackAt;             // abduction.backAt
	TFunction<double()> Last;                           // abduction.data.last: the clock the player woke at
	TFunction<bool()> Over;                             // abduction.overPhase !== null
	TFunction<TArray<TSharedPtr<FS47Rec>>()> Recordings; // abduction.recordings()
	TFunction<void(int32, bool)> HoldRecord;            // abduction.holdRecord(n, held)
	TFunction<TOptional<FS47DishesSave>()> GetDishes;   // abduction.data.dishes
	TFunction<void(const FS47DishesSave&)> SetDishes;
	TFunction<void()> ClearDishes;                      // delete abduction.data.dishes (the developer's)
};

// main.ts's wiring for SARO's own happenings: SARO going dark at 02:29 and Hondo's telex
// (story/saroDark.ts, world/saroPower.ts), the array put back on the schedule (story/arrayBack.ts),
// the dishes that follow (story/dishesFollow.ts), the lights on B (story/lineLights.ts), reel 047
// (story/reel047.ts) and Earl's reel (core/reelTape.ts), the recording's walk (story/recording.ts,
// recWalk), with the world's parts they move (world/Dish.ts: the red lamps; Exterior.fieldLights).
// Each is built as main.ts builds it (its deps, its *In() context), stepped in main.ts's order from
// US47Night::Step (StepDark after the night's events, StepLights after the glimpses, StepLate where
// main.ts has the array and the time away) and US47Main::Step (Frame: what main.ts does every frame,
// the dishes drawn, the sky light), written in the night's log (SeenStep), kept with the case
// (NightShiftState.array and .hondo through the host, AbductSave.dishes) and put back in startGame's
// order (Restore, Restored).
UCLASS()
class SIGNAL47_API US47SaroNight : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	virtual void BeginDestroy() override;

	TSharedPtr<FS47DishArray> Dishes;
	TSharedPtr<FS47SaroPower> Power;
	TSharedPtr<FS47SaroDark> Dark;
	TSharedPtr<FS47ArrayBack> ArrayBack;
	TSharedPtr<FS47DishesFollow> Follow;
	TSharedPtr<FS47LineLights> LineLights;
	TSharedPtr<FS47Reel047> Reel047;
	TSharedPtr<FS47ReelTape> ReelTape;
	TSharedPtr<FS47RecordingWalk> RecWalk;

	// ---------- what other ports give it (unset: not there) ----------
	FS47AbductView Abduct;
	/** The closer look, 0..1 (main.ts zoomK). */
	TFunction<double()> Zoom;
	/** The dishes tipped by the archive's phone (story/archiveDishes.ts arrayTip): moving, held down. */
	TFunction<bool()> TipMoving, TipHolds;
	/** The CB is talking (core/cb.ts cb.busy). */
	TFunction<bool()> CbBusy;
	/** The radio for Earl's reel (core/radio.ts): radio.boyCallTimes, radio.callerVoice, the caller bank. */
	TFunction<TArray<FS47TapeLine>()> BoyCallTimes;
	TFunction<bool()> CallerVoice;
	TFunction<TArray<TSharedPtr<FS47AudioBuffer>>(const TArray<FString>&)> BoyVoices;
	/** The corner clock's seconds from the others main.ts asks before and after SARO dark
	 *  (abduction.warnClock; archive.clockShow, ch6.clockShow): "hh:mm:ss", or empty. */
	TFunction<FString()> ClockBefore, ClockAfter;

	// ---------- main.ts step() ----------
	/** saroDark.update and the Hondo telex for a night that never went dark (after the night's events). */
	void StepDark(double Dt, bool bModal);
	/** lineLights.update (after the glimpses). */
	void StepLights(double Dt, bool bModal);
	/** arrayBack.update (US47Away steps it before the time away: BeforeAbduction). */
	void StepArray(double Dt, bool bModal);
	/** The time away's part: the recording's walk and the dishes that follow; the corner clock in
	 *  seconds (and arrayBack.update first if nothing stepped it this frame). */
	void StepLate(double Dt, bool bModal);
	/** Every frame (main.ts's last lines: the dishes drawn, the sky light, the reel's own timer). */
	void Frame(double Dt);
	/** seenStep's lines for these, and seenSync. */
	void SeenStep();
	void SeenSync();

	// ---------- the saves ----------
	/** startGame before game.start: recWalk.reset, lineLights.reset, saroDark.halt; the dishes' save. */
	void Restore(const S47J::FObj& State);
	/** startGame after game.start and the doors: saroDark.restore, follow.restore, arrayBack.restore, reelTape.stop. */
	void Restored();
	/** leavePlay (and toTitle: bTitle). */
	void Leave(bool bTitle = false);
	/** writeSave: AbductSave.dishes while the time away is not ported. */
	void Save(const S47J::FObj& State) const;
	/** main.ts saveBlock's lines for these (empty: they do not block). */
	FString SaveBlock() const;

	/** darkIn(ok, modalOpen) and darkWhere() (main.ts). */
	FS47DarkIn DarkIn(bool bOk, bool bModal) const;
	FString DarkWhere() const;
	/** arrayGate(): why the console will not put the array back now (empty: it will). */
	FString ArrayGate() const;

	static bool SelfTest(TArray<FString>& Lines);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> DspTarget;
	TSharedPtr<FS47DspScreen> Dsp;
	int32 DspMat = -1;
	UPROPERTY() TObjectPtr<UTexture> DspWas;
	TOptional<FS47DishesSave> DishesKept;   // AbductSave.dishes without the abduction's port
	TArray<FS47Area> FieldZones;
	// seenStep: was.* and the entries
	struct FWas { FString Dark = TEXT("idle"); bool bDarkFig = false; bool bArray = false; FString Follow = TEXT("idle"); FString Lights = TEXT("none"); bool bReel = false; bool bR47 = false; FString Walk = TEXT("idle"); } Was;
	FS47SeenRef DarkEntry, ArrayEntry, ReelEntry, R47Entry, WalkEntry;
	bool bClockShown = false;
	// the log, for the tests and the screenshot runs
	FString LoggedFigure, LoggedDark = TEXT("idle"), LoggedArray = TEXT("idle"), LoggedFollow = TEXT("idle"), LoggedLights = TEXT("none"), LoggedWalk = TEXT("idle");
	// the developer menu's, for the tests
	bool bReelTest = false;
	bool bDarkLook = false;
	bool bShotLit = false;          // the eyes turned to the figure once it stands (-S47Dark=look)
	TOptional<double> TestWarn;      // a warning of the time away played here for the dishes (no abduction yet)
	TOptional<FVector> TestTruck;    // a truck driven south past the lights (no drive needed)
	TSharedPtr<FS47Rec> TestRec;
	bool bTestWalk = false;          // the walk has the developer's recording (no time away's to hold)
	bool bArrayStepped = false;      // StepArray has run this frame
	bool bWalkNear = false;          // -S47RecWalk=near: the nearest time away walks once there is one
	struct FLineContract;
	TSharedPtr<FLineContract> LineContract;
	void ControlLightsTest(double Dt);

	bool Is(const TFunction<bool()>& F) const { return F && F(); }
	FString Phase() const;
	int32 ChapterNo() const;
	bool AtCh(int32 N) const { return ChapterNo() >= N; }
	double Clock() const;
	bool Cinematic() const;
	bool Driving() const;
	FString Area() const;
	FS47Cam Camera() const;
	FVector Eye() const;
	bool Inside(const TCHAR* Rect, const FVector& P) const;
	double Dawn() const;
	bool Calm() const;
	bool AbductOn() const;
	FString AbStage() const;
	FString SceneId() const;
	double FloorAt(double X, double Z) const;
	double BandShine(const FVector& At) const;
	TOptional<FS47DishesSave> GetDishes() const;
	void SetDishes(const FS47DishesSave& V);
	TOptional<FVector> FollowDish() const;
	void Note(const FString& Text) const;
	void Autosave() const;
	void ShowDsp(bool bOn);
	void TestFlags();
	void LogStates();
};
