#pragma once

#include "CoreMinimal.h"

class US47Main;

// M2 (Tom, 8 November: GEAR-FLICKER, SOUNDSCAPE, ANIMALS): the one owner of the night's signs. What is out there near the player is
// read from the public state of the systems that really put it there (the Director's event, being taken, a visit at marks or out in
// the open, a door tried, a face at a window, the bodies out, the light over the highway, the night's own events), and nothing else.
//
// The same signs also come from ordinary things, as they do out here (TODO FLASHLIGHT-BATTERY-04: a flicker alone is no sure sign):
// a gust stills the crickets, a loose contact makes the lamp blink on rough ground, the walkie hisses with distance and low ground,
// a far car bleeds over the channel, a bad tank makes the engine miss, a coyote stops for nothing anyone can see. Those are drawn
// from the game's seed by the half hour (OrdinaryAt), so that a Continue draws the same night again. A real source is stronger and
// touches more of the gear at once, but each reader may miss it (Misses, about one time in five, drawn and kept per night), so that
// nothing here is ever a sure answer. The numbers are set so that about one sign in four comes from nothing; a long run with
// -S47OmenLog prints each night's tally (signs felt, how many real) to check it.
//
// Each source has a place (three-space, in the area the player is in), how dangerous it is (0 to 1) and how far it carries. The
// readers, each in its own way:
//   FS47GearFlicker  the lamp, the walkie, the truck and the watch, by distance and danger (LevelFor), and the ordinary causes.
//   FS47Soundscape   the night's sounds stop seconds before it comes, or for a gust; they come back one at a time after.
//   FS47Animals      birds go up, a coyote that is out stops and stares near it, the cattle walk off together, the dog stops.
//   FS47Stress       how near something dangerous is (Level), not that something is going on somewhere.
// What the player felt is kept a minute (Felt); opened the notebook or the map in that minute, the time is written down as a field
// record with where it was and, when the night or an animal showed it, which way (the follow-up a careful player has).
// Freeform only. Gathered four times a second while the game runs; a menu or a paper holds it as it is.
struct FS47OmenSource
{
	FString Id;                      // "director:service_presence", "taken", "visit", "door", "peek", "body:3", "highway", "night:landing", "dev"
	FVector At = FVector::ZeroVector; // three-space, in the area the player is in now
	double Severity = 0;             // how dangerous, 0 to 1
	double Reach = 0;                // metres: nothing of it at or beyond this
	bool bComing = false;            // its first sign only: the body or the light is not here yet
	bool bResidual = false;          // what one left (marks, a ring): a faint touch on the gear, no silence, no animal
};

/** The things that read the signs, each of which may miss a real source on its own. */
enum class ES47OmenChannel : uint8 { Lamp, Radio, Truck, Clock, Hush, Animals, Count };

/** The ordinary causes of the same signs. */
enum class ES47Ordinary : uint8 { None, Gust, Contact, Skip, Traffic, Shy, Count };

struct FS47OmenOrdinary
{
	ES47Ordinary Kind = ES47Ordinary::None;
	double From = 0, Len = 0;  // world clock seconds
	double Strength = 0;       // 0.3 to 1
	double Bearing = 0;        // radians, road-local (0 north, clockwise): where a gust comes from, where a coyote looks
};

class SIGNAL47_API FS47Omen
{
public:
	explicit FS47Omen(US47Main* Main) : M(Main) {}

	/** Every frame after the Freeform session (game time). */
	void Step(double Dt);
	/** Out of Freeform, the title, a save opened: nothing is near. */
	void Reset();

	const TArray<FS47OmenSource>& Sources() const { return Now; }
	/** At the player's eyes, 0 to 1: the strongest of severity by distance. A source only coming counts for less. */
	double Level() const { return LevelNow; }
	/** The same for one reader: the sources that reader misses tonight are left out. */
	double LevelFor(ES47OmenChannel Channel) const;
	/** What a creature or the night at this place feels, 0 to 1, out to Widen times each source's reach (and never less than
	 *  MinSense metres): animals and the crickets know before the lamp does. OutFrom: where the strongest is; bOutComing: it is
	 *  only coming. Residual sources are left out; with a channel, so are the ones that reader misses tonight. */
	double SenseAt(const FVector& Where, double Widen = SenseWiden, FVector* OutFrom = nullptr, bool* bOutComing = nullptr,
		ES47OmenChannel Channel = ES47OmenChannel::Count) const;
	static constexpr double SenseWiden = 1.6, MinSense = 80;
	/** Seconds since something new came within its sense of the player; a large number when nothing has. */
	double SinceOnset() const { return OnsetAge; }
	int32 Onsets() const { return OnsetCount; }
	/** Where the last onset was, three-space. */
	FVector OnsetFrom() const { return OnsetAt; }
	/** This reader does not feel this source tonight (drawn from the seed, the night, the source and the reader). */
	bool Misses(const FString& SourceId, ES47OmenChannel Channel) const;

	/** The ordinary cause of this kind going on now, 0 when none (its strength otherwise); OutBearing where it is from. */
	double Ordinary(ES47Ordinary Kind, double* OutBearing = nullptr) const;
	int32 OrdinaryEpisodes() const { return OrdinaryCount; }

	/** A reader tells what the player could notice just now: which reader, in a few words ("the lamp cut out"), whether it came
	 *  from something real, and from where (three-space; ignored without bHasWay). Kept for the notebook a little while. */
	void Felt(ES47OmenChannel Channel, const TCHAR* What, bool bReal, const FVector& From, bool bHasWay);
	static constexpr double NoteWindow = 45;
	int32 Notes() const { return NoteCount; }
	int32 SignsFelt() const { return FeltCount; }
	int32 SignsFeltReal() const { return FeltRealCount; }

	/** Another system's own sign, as one of the sources for this long (seconds of play): what the night's events want felt before
	 *  they come (S47Happenings' quiet before a danger, say) goes through here, so that the lamp, the silence, the animals and the
	 *  walkie all answer to one owner and say the same thing. The same Id again replaces it; Seconds 0 takes it away. */
	void Post(const FS47OmenSource& Src, double Seconds);
	/** F10: a made-up presence at this place for this long, as dangerous as Severity. bComing: its first six seconds are the
	 *  sign before it (the night goes quiet, the birds go up), then it is there. */
	void DevPresence(const FVector& Where, double Severity, double Seconds, bool bFirst);
	bool DevActive() const { return DevLeft > 0; }
	void DevClear() { DevLeft = 0; }
	/** F10: an ordinary cause now, for this long (a gust, a loose contact...), as if the night had drawn it. */
	void DevOrdinary(ES47Ordinary Kind, double Seconds);

	// ---------- the pure rules (tested) ----------
	/** Full within a few metres, nothing at Reach, smooth between: a broad feeling, not a rangefinder. */
	static double Falloff(double Distance, double Span);
	/** How far a source this dangerous carries, for one that does not say (F10's): 25 m for a small thing, 240 m for the worst. */
	static double ReachFor(double Severity);
	/** The level at an eye from a list of sources. */
	static double LevelAt(const FVector& Eye, const TArray<FS47OmenSource>& In);
	static double SenseFrom(const FVector& Where, const TArray<FS47OmenSource>& In, double Widen, FVector* OutFrom, bool* bOutComing);
	/** A coming source counts this much of its severity at the eyes (the walkie hisses before the body is there). */
	static constexpr double ComingShare = 0.6;
	/** About this share of what a real source should show, each reader misses. */
	static constexpr double MissShare = 0.2;
	static bool MissesFor(int32 Seed, int32 Night, const FString& SourceId, ES47OmenChannel Channel);
	/** The ordinary cause running at this world clock, if any: one at most per half hour, at night more than by day. */
	static bool OrdinaryAt(int32 Seed, double Clock, FS47OmenOrdinary& Out);
	static constexpr double SlotSeconds = 1800;
	/** Which way the wind is from on this day (radians, road-local, 0 north, clockwise): mostly out of the south-west. */
	static double WindFrom(int32 Seed, int32 OnDay);
	/** Eight points of the compass for a road-local bearing. */
	static const TCHAR* Way(double Bearing);
	/** Road-local bearing (0 north, clockwise) from one road-local point to another (x east, z south). */
	static double BearingOf(const FVector2D& From, const FVector2D& To);
	/** What the notebook says for what was felt: the readers' words in turn, the time the watch showed, where (empty: out on the
	 *  open land), and which way it seemed to be, if any (empty: nothing to say which way). */
	static FString NoteText(const TArray<FString>& What, const FString& Watch, const FString& Where, const FString& WayWord);
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TArray<FS47OmenSource> Now;
	TSet<FString> Sensed;
	double ReadIn = 0, LevelNow = 0, OnsetAge = 1e9;
	double LevelBy[int32(ES47OmenChannel::Count)] = {};
	int32 OnsetCount = 0;
	FVector OnsetAt = FVector::ZeroVector;
	FVector DevAt = FVector::ZeroVector;
	struct FPosted { FS47OmenSource Src; double Left = 0; };
	TArray<FPosted> Posted;
	double DevLeft = 0, DevSeverity = 0, DevAge = 0;
	bool bDevComing = false, bLog = false, bLogChecked = false;
	// the ordinary cause drawn for the slot the clock is in (and F10's)
	FS47OmenOrdinary Drawn;
	int64 DrawnSlot = -1;
	ES47Ordinary DevKind = ES47Ordinary::None;
	double DevKindLeft = 0;
	int32 OrdinaryCount = 0;
	// what the player felt in the last minute, for the notebook
	TArray<FString> FeltWhat;
	double SinceFelt = 1e9;
	bool bFeltReal = false, bFeltWay = false, bModalWas = false;
	FVector FeltFrom = FVector::ZeroVector;
	FString FeltWatch;
	int32 NoteCount = 0, NotesEver = 0, FeltCount = 0, FeltRealCount = 0, LoggedNight = 0;
	int32 NightNow() const;
	void Gather(TArray<FS47OmenSource>& Out) const;
	void StepOrdinary();
	void StepNote();
	void WriteNote();
};
