#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47Seen.h"

class AS47Web;
class US47Audio;
class FS47SaroPower;

// web/src/story/saroDark.ts. 02:29: SARO goes dark, and Hondo writes (FORSLAG.md, 20 October, item 4;
// KAPITLER.md, The Second Exposure, new 1). Chapter one, once, while the player is out in the lab or
// the yard and the control room stands empty:
//
//   the relay    K3 clacks on the rack in the empty control room, heard through the wall. The player
//                has heard it twice before: by itself at the skip, forty seconds before the lights
//                dipped (Prologue.relayFirst), and seven seconds before the impact at 02:13. The
//                corner clock goes over to seconds (UI.clockSeconds), as it does for the 47-second
//                warning. If K3 is already open since the impact (most nights: it is a lever on the
//                rack and nothing asks for it in chapter one), it has nothing left to trip and trips
//                anyway: the same clack, and the log writes it down a second time (R5: a recording
//                of the trip, not the relay).
//   :31          everything at SARO sags for a moment, as the lights did at 23:52.
//   :40          every light that is SARO's goes out (world/saroPower.ts): the photo lab's safelights,
//                the DARKROOM lamp, the yard's and the array's sodium lamps, the wall packs, the
//                control room, the annex, and the red lamps on all 27 antennas. The hum of the
//                place dies with them (the room tone, the S-03 cabinet, the safelight's ballast), and
//                a breath later the crickets and the wind too. Your own footsteps are left. The eyes
//                need a while: the starlight comes up slowly. The highway's street lamps west of the
//                site and the motel's sign down the road stay lit: it is only SARO.
//   the dark     47 seconds. A few seconds into it, where the player can see it from and out of the
//                middle of the view, a small figure stands quite still: the outline from the glimpses
//                (core/roadside.ts), black against what is lighter behind it. Up on a roof against
//                the stars where it can be (the control room's, over the empty room; the east
//                wing's), else out on the windows side towards the highway, whose lamps still light
//                the ground beyond (DARK_SPOTS). In the lab, a few seconds in, the doorway is the one
//                thing left to see. Tom, 20 October: "Spenning og skummelt slår faste regler". It
//                bends two rules of HENDELSER.md, section 4, here only: it stands nearer than 40 m
//                (some 7 to 20), and it does not go when it is looked at. It goes with a closer look
//                (the zoom), the flashlight on it, walking up to it, and with the light.
//   the light    the power comes back: the tubes, the safelights and the screens at once, the sodium
//                lamps dim and warming up over half a minute; the hum, the crickets all at once.
//                The figure is not there. K3 stays open.
//   the telex    a few seconds on, the teleprinter in the control room starts (made here: a motor and
//                strikes), and Hondo Valley's message is on the roll, with the line in the console
//                log (KAPITLER.md: "TELEX RECEIVED  HONDO VALLEY"). Nothing on the screen says what it
//                says: whoever goes in finds it (Prologue.telexRoll; the first reading is noted).
//
// A night that did not go dark (chapter one done before 02:29, or the player away from SARO until
// 02:40) still has Hondo Valley's message on the roll, printed at 02:29 with nobody there, as
// KAPITLER.md first had it (main.ts): Ward reads it all the same ("I read the telex. All of it.").
//
// One thing at a time (main.ts, oneAtATime): it does not begin while anything else of the night's is
// going, and nothing else begins while it runs; it waits for whatever is going, until 02:40. It is
// the story's 47, not the curve's to time, but it is chapter one's one 47-second event all the same
// (story/curve.ts keeps the chapter's 47 for it until it has come or its time is past, and it lands
// the peak when it comes). Written in the night's log (story/seen.ts: the event 'blackout', and the
// figure as a scare when it stands) and kept with the case (NightShiftState.hondo, the console log).
// A paper, the notebook or the board open holds it where it is, as it holds the clock. The wet bench,
// the B-12 control and the camera's case frames wait for the light (Chapter1.dark): nothing is lost
// in the dark, and the print in the fixer comes up as it would have.
//
// The figure is the export's Silhouette('saroDark:figure', 0x1d2433) (its body and its shell, the
// rim); the painted outlines of round 14 (sil.wake, useArt) are not in the export.

/** 'idle' | 'relay' | 'dark' | 'back' | 'done' */
using FS47DarkPhase = FString;

/** Where it can stand, in SARO's terms: on the ground (feet on it there), or up on a roof (`Roof`:
 *  the height of its top, and which of BUILDINGS it is). */
struct FS47DarkSpot { const TCHAR* Id; double X, Z; const TCHAR* Where; bool bRoof = false; double RoofY = 0; int32 On = -1; };
SIGNAL47_API const TArray<FS47DarkSpot>& S47DarkSpots();

/** A rectangle in the way of the eyes (the truck on its pad). */
struct FS47DarkRect { double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0; };

/** Whether the line from a to b (xz) gets out of and past SARO's buildings (and `More` in the way).
 *  `On`: b stands on the roof of that one of BUILDINGS, which does not stand in the way (unless the
 *  eye is inside it: from in there no roof is seen). */
SIGNAL47_API bool S47ClearAt(const FVector& A, const FVector& B, const TFunction<bool(const FString&)>& Open, const TArray<FS47DarkRect>& More = {}, int32 On = -1);

struct FS47DarkIn
{
	/** Chapter one at SARO, on foot, its report not yet filed, nothing else of the night's going (one
	 *  at a time), the telex not yet had: it may begin. */
	bool bOk = false;
	/** A paper, the notebook or the board open, a cinematic, a call: what is going waits where it is. */
	bool bHold = false;
	/** Where the player is: 'lab' | 'yard' | 'room' | 'away'. */
	FString Where = TEXT("away");
	double Clock = 0;
	FS47Cam Cam;
	/** 0..1, the closer look (the zoom). */
	double Zoom = 0;
	/** The flashlight is on. */
	bool bTorch = false;
	/** Whether a door is open (Doors.isOpen). */
	TFunction<bool(const FString&)> Open;
	/** What else stands in the way of the eyes (the truck, where it is parked at SARO). */
	TArray<FS47DarkRect> Blocks;
};

struct FS47DarkDeps
{
	TWeakObjectPtr<US47Audio> Audio;
	FS47SaroPower* Power = nullptr;
	TWeakObjectPtr<AS47Web> Web;
	/** The ground's height in SARO's scene. */
	TFunction<double(double, double)> Ground;
	/** The rack and the teleprinter, in the world. */
	TFunction<FVector()> Rack, Teleprinter;
	/** K3 goes (Prologue.relayClack): true if it tripped, false if it had nothing left to trip. */
	TFunction<bool()> Relay;
	/** A line in the console log (Prologue.consoleLog), at a clock. */
	TFunction<void(const FString&, double)> Log;
	/** Hondo Valley's message is on the roll now (Prologue.hondoTelex). */
	TFunction<void(double)> Telex;
	/** The control room's radio and screens without power, and back (Prologue.powerOut). */
	TFunction<void(bool)> PowerOut;
	/** The night outside: quiet (secs, level, tau, back), as Ambience.hush. */
	TFunction<void(double, double, double, double)> Hush;
	TFunction<void(const FString&, double)> Toast;
	TFunction<void(const FString&)> Note;
	/** Settings, Flashing images: slow fades (the tubes strike without their stutter). */
	TFunction<bool()> Calm;
};

class SIGNAL47_API FS47SaroDark
{
public:
	explicit FS47SaroDark(FS47DarkDeps InD);

	FS47DarkPhase Phase = TEXT("idle");
	/** Seconds into the phase. */
	double T = 0;
	/** When K3 went (the night's clock): the telex is filed at its minute. */
	double At = 0;
	/** K3 was already open and tripped anyway. */
	bool bWasOpen = false;
	/** Where the player was when it began. */
	FString From = TEXT("away");
	/** The figure: where it stood (DARK_SPOTS id), whether it was looked at, how it went (empty: not yet). */
	struct FFigure { FString Id; bool bLooked = false; FString Gone; };
	TOptional<FFigure> Figure;
	/** The starlight's share as the eyes open (main.ts multiplies the sky light by it). */
	double Adapt = 1;
	/** The corner clock with seconds while the relay counts (main.ts, UI.clockSeconds). */
	struct FClockShow { FString Hm, Ss; };
	TOptional<FClockShow> ClockShow;

	/** It is going (one at a time: nothing else starts, main.ts). */
	bool Busy() const;
	/** The 47 seconds of the dark (saves wait, and the night's other things). */
	bool Dark() const { return Phase == TEXT("dark"); }
	/** The export has the figure (Silhouette('saroDark:figure')). */
	bool HasFigure() const { return Fig.IsValid(); }
	/** Where the figure's head is while it stands (story/seen.ts), or unset. */
	TOptional<FVector> FigureAt() const { return bFigVisible ? TOptional<FVector>(Head) : TOptional<FVector>(); }
	/** Its time: 02:29 to 02:40 (main.ts keeps the chapter's 47 for it until then). */
	static bool Window(double Clock, bool bEvening = false);
	static bool Pending(double Clock, bool bEvening = false);
	bool bEveningTimeline = false;
	/** The telex's own minute (KAPITLER.md): where the night never went dark, it printed then. */
	static const double FILED;

	void Update(double Dt, const FS47DarkIn& S);
	/** Now (the developer menu, the tests): from where the player is, whatever the clock. */
	bool Now(const FS47DarkIn& S);
	/** Stop at once, the power back as it was (the title, another save, a new night). */
	void Halt();
	/** A night put back: had already (the telex is on the roll), or not yet. */
	void Restore(bool bHad) { Halt(); if (bHad) Phase = TEXT("done"); }

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47DarkDeps D;
	FS47Obj Fig;
	bool bFigVisible = false;
	FVector FigPos = FVector::ZeroVector;
	FVector Head = FVector::ZeroVector;
	FVector Eye = FVector::ZeroVector;
	bool bSagged = false;
	struct FChar { double T = 0; bool bCr = false; };
	TArray<FChar> Chars;
	int32 Printed = 0;
	bool bMotor = false;
	bool bNoted = false;
	double SodiumT = -1;
	double SagT = -1;
	double NowClock = 0;
	/** Out where SARO's lamps could be seen going (the yard, or the control room's windows) at some
	 *  point in the dark; in the lab all the time, only the safelight and the hum were (the note). */
	bool bSawOut = false;
	bool bDoorway = false;
	double HushAt = -1;
	double AdaptT = -1;

	bool Due(const FS47DarkIn& S) const;
	void Begin(const FS47DarkIn& S);
	void RelayStep(const FS47DarkIn& S);
	void GoDark(const FS47DarkIn& S);
	void DarkStep(const FS47DarkIn& S);
	void Place(const FS47DarkIn& S);
	void Watch(const FS47DarkIn& S);
	void GoBack();
	void BackStep();
	void Finish();
	void SodiumStep(double Dt);
	void AdaptStep(double Dt);
	void ClockNow();
	void SetFigVisible(bool bOn);
	void Toast(const FString& Text, double Secs) const { if (D.Toast) D.Toast(Text, Secs); }
};
