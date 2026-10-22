#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"

class AS47Web;
class AS47Walker;
struct FS47Obj;

// web/src/story/seen.ts: what the night did, and whether the player saw it (FORSLAG.md, 20
// October, "Hull": nobody measured what happened and was seen). Every scare, event, glimpse, time
// away and call is written here when it happens: the game's clock, where the player was, which
// line it was on (story/lines.ts, S47Lines.h) and whether the player was looking. Kept with the
// case (main.ts, GameState.seen). It changes nothing in the night by itself; the night's one curve
// (story/curve.ts, S47Curve.h) reads it: what the player actually got is what counts as a peak.
//
// Looking: the camera faced the thing, within an angle of the middle of the view, on the screen
// and within a range, at any moment while it was there (a watch). For a glimpse that is the angle
// at which it goes (25 degrees); for a sound out in the dark, whether the player turned to it; for
// what is only heard or only on film, null, or what the thing itself counts as seen (the face: the
// print opened large; the one over you on waking: the eyes went up to it).

// The camera as main.ts hands it to the night's modules (three's PerspectiveCamera, the player's
// eyes): where it is, which way it looks and its lens, in three.js terms, read off the walker's
// camera when asked (S47Walker: the cab's view while the truck is driven).
struct SIGNAL47_API FS47Cam
{
	FVector Pos = FVector::ZeroVector;     // camera.getWorldPosition (the camera has no parent: camera.position too)
	FVector Forward = FVector(0, 0, -1);   // camera.getWorldDirection
	FVector Right = FVector(1, 0, 0);      // (1, 0, 0).applyQuaternion(camera.quaternion)
	FVector Up = FVector(0, 1, 0);         // (0, 1, 0).applyQuaternion(camera.quaternion)
	double Fov = 70;                       // vertical, degrees
	double Aspect = 16.0 / 9.0;
	double Near = 0.03, Far = 5000;        // main.ts: new PerspectiveCamera(70, aspect, 0.03, 5000)

	static FS47Cam Of(const AS47Walker* W, double Far = 5000);
	/** Vector3.project(camera): normalised device coordinates; z < 1 only for a point in front of the
	 *  camera and nearer than its far plane. */
	FVector Project(const FVector& P) const;
	/** camera.quaternion, three's terms (its -Z the view, its +Y up). */
	FQuat Quaternion() const;
};

/** three's scene.getObjectByName for the scene's own children: what the web version builds at run
 *  time and hangs straight under the scene (the glimpse, the album's figure and its smears) is in the
 *  export by its name but has no handle. Invalid when there is none. */
SIGNAL47_API FS47Obj S47SceneObject(AS47Web* Web, const FString& Name);

/** The camera faces a point: within deg of the middle of the view, on the screen, within range. */
SIGNAL47_API bool S47Facing(const FS47Cam& Cam, const FVector& At, double Range = 300, double Deg = 30);

/** SeenKind: 'glimpse' | 'call' | 'scare' | 'event' | 'abduction' | 'over' | 'dishes' | 'face' | 'roof' */
struct SIGNAL47_API FS47SeenEntry
{
	FString Kind;
	/** Which: a glimpse's place, a sound's name, an event's id ('phone', 'cows', 'steps', ...). */
	FString What;
	/** The game's clock (seconds since midnight, whole). */
	double Clock = 0;
	/** Where the player was (the save card's name for it), and the area. */
	FString Place, Area;
	/** Where the thing was, on the map (road-local metres, whole), when it had a place there. */
	TOptional<FVector2D> At;
	/** 'A' | 'B' | 'C'; empty: null. */
	FString Line;
	/** The player looked at it while it was there; unset (null) when there was nothing to look at. */
	TOptional<bool> Looking;
	/** How it ended, where that is part of it (the time away: 'away' or 'taken'); empty: none. */
	FString End;
};
using FS47SeenRef = TSharedPtr<FS47SeenEntry>;

/** Watching a thing for a while after it is written down: where it is now, for how long at most,
 *  from how far and within how many degrees of the middle of the view. Without Until, an unset
 *  place means it has gone and the watch ends; with it, nothing to look at just now (the watch ends
 *  when Until says so). */
struct SIGNAL47_API FS47SeenWatch
{
	TFunction<TOptional<FVector>()> Pos;
	double Secs = 3;
	TOptional<double> Range, Deg;
	TFunction<bool()> Until;
};

class SIGNAL47_API FS47SeenLog
{
public:
	/** At most this many are kept (the oldest go first): a night has a few dozen. */
	static constexpr int32 MAX = 300;
	/** The night so far, oldest first. */
	TArray<FS47SeenRef> Entries;
	/** How many have been written since the log was last put back (story/curve.ts reads the new ones). */
	int32 Added = 0;

	FS47SeenRef Add(const FS47SeenRef& E, const TOptional<FS47SeenWatch>& W = {});
	/** The last one written of a kind (and which), or null. */
	FS47SeenRef Last(const FString& Kind, const TOptional<FString>& What = {}) const;
	/** Is a thing still being watched (until its watch ends)? */
	bool Watching(const FS47SeenRef& E) const;
	void Stop(const FS47SeenRef& E);

	/** Each frame, before the night's things move or go: the camera on what is being watched. */
	void Update(double Dt, const FS47Cam& Cam);
	/** A save opened (its night as it was), or a new night (null). */
	void Restore(const TArray<TSharedPtr<FJsonValue>>* List);
	/** What is kept with the case. */
	TArray<TSharedPtr<FJsonValue>> Data() const;
	/** The last few, one short line each (the ?debug overlay; the log here). */
	FString Text(int32 N = 8) const;

private:
	struct FWatching { FS47SeenRef E; FS47SeenWatch W; double T = 0; };
	TArray<FWatching> Watches;
};

// main.ts townSeen: what the night has had, by key (the town's events, the night's events 'ne:steps1',
// the marks on the map 'mark:<kind>@<clock>', 'nickel', 'cut:post' ...), kept with the case. main.ts
// hijacks its add: a key added fresh, while a night runs and is not being put back, is written in
// the seen log (sawKey). Here that is OnFresh.
struct SIGNAL47_API FS47TownSeen
{
	TSet<FString> Keys;
	TFunction<void(const FString&)> OnFresh;

	void Add(const FString& K) { const bool bFresh = !Keys.Contains(K); Keys.Add(K); if (bFresh && OnFresh) OnFresh(K); }
	bool Contains(const FString& K) const { return Keys.Contains(K); }
	void Remove(const FString& K) { Keys.Remove(K); }
	void Clear() { Keys.Reset(); }
	TArray<FString> Array() const { return Keys.Array(); }
};

namespace S47Seen
{
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
