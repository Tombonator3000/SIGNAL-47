#pragma once

#include "CoreMinimal.h"
#include "S47Audio.h"
#include "S47Docs.h"
#include "S47Json.h"
#include "S47Seen.h"
#include "UObject/StrongObjectPtr.h"

class AS47Web;
class AS47Walker;
class FS47Flashlight;
class FS47Group;
class FS47JacketWalker;
class FS47CardSilhouette;
class FS47Pix;
class UTexture2D;
struct FS47Truck;

// web/src/story/abduction.ts. The time away (HENDELSER.md, section 8; Tom, 16 October: yes to all
// four questions). On foot, alone, outdoors on one of the lines, at night: the pressure rises, and at
// the top of it they take a recording of you, the way you take a picture of an antenna. Nobody in
// the game says so.
//
//   the warning  always 47 s, timed as the 47 s after the line went dead at 02:13 (the low beep at
//                16, the printer at 31, K3 at 40, the event at 47). The corner clock goes over to
//                seconds and counts the warning's own: HH:MM:00 to :47 (WarnClock). At once the
//                crickets stop. At :16 a small beep, and the player's own footsteps are gone. At :31
//                the pressure comes in, and the flashlight starts to stutter. At :40 it dies with the
//                click K3 made. Get off the line or in among people before the light comes, and it
//                passes.
//   the light    white, from above, from nothing to be seen.
//   the taking   the picture freezes with a tracking fault, the sound is cut, a few single frames
//                (the light; the grass from above with your shadow in it; a hand with fingers too long
//                at the edge; yourself lying in the grass from above), black.
//   lost time    17 to 47 minutes, never past the next fixed point of the story.
//   over you     on your back, the stars, and no sound at all; you blink and lift your head to look
//                down along yourself, and at the top of the view, a few metres off beyond your feet,
//                one of them stands with its head bent down to you, black against the sky or something
//                lit, nearer than they ever come, and still. A second and a half, or until your eyes go
//                up to it, and it is not there; the crickets start again and the flashlight comes on.
//                The first time a little aside and some way off; the second nearer, just past your
//                boots; the third time there are no stars first, and there are two.
//   back         on your back in the grass, the stars; the view lifts. 100 to 400 m further along, in a
//                ring of flattened grass with frost on it; the flashlight beside you, on. The truck's
//                engine has died where it stood and wants the key.
//   traces       the lost time, one exposure too many on the field camera (yourself from above), a line
//                in your own hand you do not remember writing, a nosebleed that stopped by itself. And
//                afterwards, where you were taken, someone in a SARO jacket stands quite still for the
//                rest of the night: the recording (R5). Gone if you walk up to it. The last 47 seconds
//                are kept with it, and once, later, it walks them (story/recording.ts).
//
// At STATION 01, after N. Vega's call in chapter three, everyone goes (story/stationTaken.ts): Force.
// In the truck over B at SARO's turn-off, southbound, at the night's peak (story/truckTaken.ts):
// BeginCab, the truck has the beats, and only the truck letting go lets it pass (CabFree). Taken, the
// player wakes at the wheel 300 m further on facing the wrong way, and the one over them is on the
// bonnet, bent to the glass.
//
// At least 40 minutes of the night between two, at most three in a night. main.ts decides when it may
// run (US47Away), the night's curve when (its peak).
//
// What the web version draws on the page is drawn here by the overlay US47Away puts over the game
// (the white light from above, the single frames on black, the HUD put away while one stands over
// you): FS47AbductScreen. The single frames are drawn on a canvas on the CPU (S47Raster) as the web
// version draws them, the painted ones from the art shipped with the build; you from above is the scene
// itself, captured from a camera over the place you lay. What is built in code at run time (the rings
// of frost, the one standing, the one lying in the grass, you along the bottom of the view, the outlines
// over you) is built from the engine's basic shapes (S47Figures).

/** abduction.ts AbductIn: the time away's context, as main.ts abductIn() gives it. */
struct FS47AbductIn
{
	bool bOk = false;
	/** The setting (on by default). */
	bool bEnabled = true;
	/** On foot, outdoors, alone, on a line: which ('A' | 'B' | 'C'); empty when not. */
	FString Line;
	/** How the line runs here (a unit vector on the ground), to come back further along it. */
	FVector Along = FVector::ZeroVector;
	double Clock = 0;
	/** The clock by which the story's next fixed point comes; the lost time stops short of it. */
	double NextFixed = -1e9;
	/** Chapter three or four: the one before chapter five is due. */
	bool bLate = false;
	bool bCalm = false;
	/** How much faster the pressure rises here (the antenna field before its dishes have turned). */
	double Boost = 1;
	/** The night's curve is at its peak: the warning may begin (unset: it may). */
	TOptional<bool> May;
	/** The warning in the truck has let go of the player (story/truckTaken.ts): it passes. */
	bool bCabFree = false;
};

/** The head's look kept within a box in the cab: yaw and pitch ranges (radians). */
struct FS47LookBox { double Yaw0 = 0, Yaw1 = 0, Pitch0 = 0, Pitch1 = 0; };

/** abduction.ts AbductCab: the truck, as the time away in it needs it (main.ts, story/truckTaken.ts). */
struct FS47AbductCab
{
	/** The player is at the wheel now (else out of the truck, on foot). */
	TFunction<bool()> InCab;
	/** The truck in the world (its own metres, -z forward), or null. */
	TFunction<FS47Truck*()> Group;
	/** The head's look in the cab (from straight ahead: yaw left, pitch up), set at once, and kept within a box. */
	TFunction<void(double, double)> Look;
	TFunction<TOptional<FVector2D>()> LookNow;
	TFunction<void(TOptional<FS47LookBox>)> LookBox;
	/** Where the truck will be put down (world, on the ground), once the light has come. */
	TFunction<TOptional<FVector>()> Plan;
	/** The waking: the truck put down further on (bInCab: the player at the wheel), and where it stands now. */
	TFunction<TOptional<FVector>(double, bool)> Wake;
	/** The notes of it, after the time away's own (a: the clock it was taken, b: the clock now). */
	TFunction<void(const FString&, const FString&, bool)> Woke;
	/** What the waking says once the view has come up, at the wheel. */
	TFunction<FString()> BackText;
	/** The light from above, into the cab through the glass (0..1). */
	TFunction<void(double)> Light;
};

/** A camera of the time away's own (THREE.PerspectiveCamera): where it is, its rotation (three's, its
 *  -Z the view), its vertical field of view and aspect. */
struct FS47ShotCam
{
	FVector Pos = FVector::ZeroVector;
	FQuat Q = FQuat::Identity;
	double Fov = 50, Aspect = 1, Near = 0.1, Far = 2000;
	/** camera.lookAt(target): -Z towards it, y up. */
	void LookAt(const FVector& Target);
	FS47Cam Cam() const;
};

/** What the web version does on the page: the white light from above (.abduct-light, --abduct), the
 *  frame canvas over everything (a picture at an alpha on black; null: black), and html.abduct-held. */
struct FS47AbductScreen
{
	TFunction<void(TOptional<double>)> Light;
	TFunction<void(UTexture2D*, double)> Show;
	TFunction<void()> Hide;
	TFunction<void(bool)> Held;
};

/** abduction.ts AbductDeps. */
struct FS47AbductDeps
{
	US47Audio* Audio = nullptr;
	AS47Walker* Player = nullptr;
	AS47Web* Web = nullptr;
	FS47AbductScreen Screen;
	/** A frame of the scene seen by this camera, drawn at once: 640 x 400, the top row first. */
	TFunction<bool(const FS47ShotCam&, TArray<FColor>&)> Shoot;
	/** The scene from this camera, small (w by h, RGBA, the bottom row first), as it is before the
	 *  tape, with the open sky pure magenta: what would be behind them. */
	TFunction<bool(const FS47ShotCam&, int32, int32, TArray<uint8>&)> Probe;
	TFunction<double(double, double)> Ground;
	/** Ground a player can stand on (the area's zones, minus the boxes in the way). */
	TFunction<bool(double, double)> Standable;
	/** The truck, for the time away in it (story/truckTaken.ts; main.ts). */
	TOptional<FS47AbductCab> Cab;
	/** Off the asphalt, onto the verge, where the place has one. */
	TFunction<void(FVector&)> Settle;
	FS47Flashlight* Flashlight = nullptr;
	/** The night's sounds to level (in about tau seconds), and back to all of them after secs: Ambience.hush. */
	TFunction<void(double, double, TOptional<double>, TOptional<double>)> Hush;
	TFunction<void(double)> Glitch;
	TFunction<void(double)> SkipClock;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, float)> Toast;
	/** The field camera: whether the player has it. */
	TFunction<bool()> Camera;
	TFunction<void(const FS47Doc&)> File;
	TFunction<void()> KeyStall;
	TFunction<void()> Save;
	/** How much a light from near the eye falls on a point, 0 to 1 (main.ts bandShine). */
	TFunction<double(const FVector&)> Shine;
	/** The camera the player sees with now (main.ts camera). */
	TFunction<FS47Cam()> Eye;
};

/** abduction.ts AbductSave: what is kept with the case. */
struct SIGNAL47_API FS47AbductSpot
{
	FString Scene;
	FVector Take = FVector::ZeroVector, Back = FVector::ZeroVector;
	double Clock = 0;
	TOptional<FString> Pic;
	TOptional<FS47Doc> Doc;
	/** Which time it was that one of them stood over you on waking (1 to 3). */
	TOptional<int32> Over;
	/** The last 47 seconds before the light: x, z and the view's yaw every quarter second. */
	TOptional<TArray<double>> Path;
	/** When the recording walked them (the night's clock): once, never again. */
	TOptional<double> Walked;
	/** STATION 01's, after N. Vega's call. */
	bool bStation = false;
	/** In the truck, over B at SARO's turn-off. */
	bool bCab = false;
};
struct SIGNAL47_API FS47AbductSave
{
	int32 Count = 0;
	double Last = -1e9;
	TArray<FS47AbductSpot> Spots;
	/** The one night the array came down out of 026 and followed the player through a warning
	 *  (story/dishesFollow.ts): { clock, ids, back }, as that port keeps it. */
	S47J::FObj Dishes;
	/** STATION 01 after N. Vega's call: 'taken' | 'passed' | empty. */
	FString Station;
	S47J::FObj ToJson() const;
	static FS47AbductSave FromJson(const S47J::FObj& O);
};
SIGNAL47_API S47J::FObj S47DocToJson(const FS47Doc& D);
SIGNAL47_API FS47Doc S47DocFromJson(const S47J::FObj& O);

class SIGNAL47_API FS47Abduction
{
public:
	explicit FS47Abduction(FS47AbductDeps InD);
	~FS47Abduction();

	/** The warning's length, and its beats: the 47 seconds after the line went dead in the prologue. */
	static constexpr double WARN = 47;
	struct FBeat { double Steps, Tone, Dark; };
	static constexpr FBeat BEAT = { 16, 31, 40 };
	/** recording.ts PATH_DT: the path is sampled this often (seconds). */
	static constexpr double PATH_DT = 0.25;

	/** 'calm' | 'warning' | 'light' | 'taken' | 'black' | 'back' */
	FString Stage = TEXT("calm");
	/** The footsteps and the night's sounds are gone (main.ts mutes the steps). */
	bool bSilent = false;
	/** 0..1, how near the top (the developer menu, the tests). */
	double Pressure = 0;
	FS47AbductSave Data;
	/** The scene the player is in, as main.ts names it (rings and recordings belong to one). */
	FString SceneId = TEXT("saro");
	/** This warning cannot be got away from (STATION 01). */
	bool bForced = false;
	/** This warning is the truck's. */
	bool bCab = false;
	/** How each way you could have lain looked for them to stand in (the tests). */
	struct FFacing { double Yaw = 0; bool bReach = false; double Against = 0; };
	TArray<FFacing> Facings;
	/** Where the player will wake, once the light has come. */
	TOptional<FVector> BackAt;
	/** The waking is at the wheel (the time away in the truck), until it is over. */
	bool bAwakeCab = false;
	/** The last time one of them was over you, it went because the eyes went up to it. */
	bool bOverLooked = false;
	/** The warning's length (47 but for the tests). */
	double WarnLen = WARN;

	bool Busy() const { return Stage != TEXT("calm"); }
	struct FWarn { double T = 0, Len = WARN; };
	/** How far into the warning, and how long it runs; unset outside it. */
	TOptional<FWarn> Warn() const;
	/** Holds the player still (the taking and the waking). */
	bool Holds() const;
	/** The corner clock while the warning runs: the minute it began, and the warning's own seconds. */
	struct FWarnClock { FString Hm, Ss; };
	TOptional<FWarnClock> WarnClock() const;
	/** The warning or the taking is under way (no save then). */
	bool Racing() const;

	void Update(double Dt, const FS47AbductIn& S);
	/** Start the warning now (the developer menu, the tests). */
	void Begin(const FS47AbductIn& S);
	/** Start the warning that cannot be got away from (STATION 01 after N. Vega's call). */
	void Force(const FS47AbductIn& S);
	/** Start the warning in the truck (story/truckTaken.ts): the player at the wheel. */
	void BeginCab(const FS47AbductIn& S);
	/** Whether the time away could begin now as far as it goes itself. */
	bool MayBegin(const FS47AbductIn& S) const;

	/** The look while held at the wheel: only while one of them is there, and stiffly. */
	FVector2D CabGaze(double Dx, double Dy) const;
	/** The look while you are held: only while one of them is there, and stiffly. */
	void Gaze(double Dx, double Dy);
	/** Where waking is up to with one of them over you ('stars' | 'blink' | 'there'), or empty. */
	FString OverPhase() const;
	/** The middle of each one's head while they are there (the tests). */
	TArray<FVector> OverHeads() const;
	/** A save's picture is taken without them; true when they were there. */
	bool HideOver(bool bHide);
	/** The outlines over you turn to the eye (their onBeforeRender), each frame. */
	void FaceOver(const FVector& Eye);

	/** The recordings standing where the player was taken that are shown from here. */
	struct FShown { int32 N = 0; FVector At; };
	TArray<FShown> RecordsShown() const;
	/** The rings of frost where the player woke (the album names them). */
	TArray<FVector> RingsShown() const;
	/** The times away as the recording's walk sees them (story/recording.ts Rec). */
	struct FRec
	{
		int32 N = 0;
		FString Scene;
		FVector Take;
		TOptional<TArray<double>> Path;
		TOptional<double> Walked;
		double Clock = 0;
		FS47JacketWalker* Fig = nullptr;
		TFunction<void(TOptional<double>)> MarkWalked;
	};
	TArray<FRec> Recordings();
	/** The walk has the one standing for time away n (held), or gives it back. */
	void HoldRecord(int32 N, bool bHeld);

	// ---------- the case ----------
	void Restore(const S47J::FObj& V);
	/** Out to the title in the middle of it: nothing of the warning or the taking left running. */
	void Halt();
	/** The exposures that were not the player's, back among the papers (after the case is opened). */
	void Refile();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47AbductDeps D;
	double St = 0;
	int32 Beat = 0;
	double WarnFrom = 0;
	double Top = 1;
	double Lost = 0;
	double From = 0;
	FVector Take = FVector::ZeroVector;
	TArray<TStrongObjectPtr<UTexture2D>> Frames;
	struct FTone { TSharedPtr<FS47GainNode> Gain; TArray<TSharedPtr<FS47OscillatorNode>> Osc; };
	TOptional<FTone> Tone;
	FS47Pressure Bed;
	TArray<TUniquePtr<FS47Group>> Rings;
	struct FRecord { TUniquePtr<FS47JacketWalker> Fig; FString Scene; bool bHeld = false, bHide = false, bNone = false; FVector Home; double HomeYaw = 0; };
	TArray<FRecord> Records;
	TArray<double> PathV;
	double PathT = 0;
	bool bCabIn = false;
	TUniquePtr<FS47Group> Lying;
	bool bHadLight = false;
	TArray<TUniquePtr<FS47CardSilhouette>> OverFigs;
	struct FOver { int32 V = 0; double T = 0; FString Phase; double Pitch = 0, Yaw = 0; TArray<FVector> Dirs; TArray<double> Off; TArray<FVector> Heads; bool bCab = false; };
	TOptional<FOver> Over;
	TUniquePtr<FS47Group> Self;

	void Calm(double Dt, const FS47AbductIn& S);
	void Sample();
	void Warning(double Dt, const FS47AbductIn& S);
	void PassOver();
	void LightStep(const FS47AbductIn& S);
	void Taken(const FS47AbductIn& S);
	void Show(UTexture2D* T, double Alpha = 1);
	void Wake(const FS47AbductIn& S);
	void OverStartCab(int32 V);
	void OverRevealCab();
	void OverStart(int32 V);
	void OverReveal();
	struct FLayout { struct FAt { FVector Q; double Head = 0; }; TArray<FAt> At; FVector Right; double Pitch = 0; };
	FLayout Layout(int32 V, double Yaw) const;
	double Facing(int32 V);
	bool Reach(const FVector& Q) const;
	double Against(const FLayout& L, double Yaw) const;
	void OverStep(double Dt, const FS47AbductIn& S);
	void OverGone();
	void OverDrop();
	void BackStep(double Dt);
	void BackCab(double Dt);
	FVector Further(const FVector& FromP, const FVector& Along) const;
	void Ring(const FVector& At, double Scale = 1);
	void Record(const FVector& At, const FString& Scene, bool bNone = false);
	void RecordsStep();
	TOptional<FS47Pix> FromAbove(const FVector& At, bool bMe, bool bTruck = false);
	void MakeFrames();
	void ToneOn();
	void ToneOff();
	void Clear();
	FVector PlayerPos() const;
	FS47Cam Camera() const;
};

namespace S47Abduct
{
	/** abduction.ts grain: grey, contrast, darkened, grain, the vignette and the lines (640 x 400). */
	SIGNAL47_API FS47Pix Grain(const FS47Pix& Src, double Seed, double Dark = 1, double Grey = 0.7, double Contrast = 1.3);
	/** One of the painted frames (4:3), cut to 16:10 and grained; unset if it is not there. */
	SIGNAL47_API TOptional<FS47Pix> Painted(const FString& Id, double Y, double Seed, double Dark = 1);
	SIGNAL47_API FS47Pix FrameLight(double Seed);
	SIGNAL47_API FS47Pix FrameHand(double Seed, const FS47Pix* Under);
	/** The frost on the flattened grass (256 x 256, white where its alpha is). */
	SIGNAL47_API UTexture2D* Frost();
	SIGNAL47_API TArray<FColor> FrostPixels();
}
