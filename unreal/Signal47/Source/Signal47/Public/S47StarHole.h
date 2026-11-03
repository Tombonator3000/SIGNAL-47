#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Seen.h"
#include "S47SkyEventSpec.h"

// web/src/story/starHole.ts. No stars where the antennas point (FORSLAG.md, 20 October, "Valg for Tom"
// 6; Tom: "Spenning og skummelt slår faste regler"). From 02:15, when every dish has gone to az 026, el
// 32 with nobody driving it, a round patch of the sky there has no stars in it (world/Sky.ts draws it,
// STAR_HOLE). It is held to the dishes, not to the stars, it never grows, and it is still there at
// dawn until the stars go. This bends SPILLDESIGN.md's "the world is honest" (R1) in one place, in the
// sky, and nowhere on the ground.
//
// A found thing, not an event: the night's curve does not hold it, and nothing happens when it is
// found. But it keeps the one-at-a-time rule: it is not noticed while another of the night's things is
// going.
//   - Noticed, once in the night: on foot, out under the sky, at night, after it has come in, with the
//     middle of the view within NOTICE.Deg of it for NOTICE.Secs (a closer look, the zoom, for
//     NOTICE.ZoomSecs) and nothing solid in the way. Reyes says it in a toast and writes it in the
//     notebook; the night's log has it ('scare', 'no stars'); a card on the board.
//   - A free frame with it in it (core/album.ts) is captioned with HOLE_PRINT and keeps where it is on the
//     print (its target 'starhole'); the card on the board shows the first such print. Taken before it
//     was noticed, the frame notices it.
//   - A closer look at it (the zoom) says what there is to see (Examine).
// Kept with the town's part of the case: 'sky:hole', 'sky:hole@<clock>', 'sky:hole/<chapter>' (the patch
// of cork it goes on), 'sky:hole:print@<shot id>', the board's threads as 'sh:<a|b>'.
// FX-01 (Unreal only, not in the web version): the field is larger now (S47SkyEventSpec.h STAR_HOLE,
// rims 6/8 degrees, about fourteen across) and comes in over 8 s. A notice made with the larger field
// also keeps 'sky:hole:extent14'; an observation without it was made under the old, eight-degree field
// and keeps its eight-degree wording. Stored notes, photographs and Seen entries are never rewritten.

struct FS47StarHoleDeps
{
	FS47TownSeen* Seen = nullptr;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, float)> Toast;
	TFunction<double()> Clock;
	TFunction<FString()> Chapter;
	/** Write it in the night's log: seen, where the player is. */
	TFunction<void()> Saw;
	/** The prints of the night (core/album.ts): the url of one by its id (empty when there is none). */
	TFunction<FString(const FString&)> Print;
	TFunction<void()> Save;
	TFunction<void()> Refresh;
};

struct FS47StarHoleIn
{
	/** In play, no panel open, not in a cinematic, on foot. */
	bool bOk = false;
	/** Nothing else of the night's is going, no call on the line. */
	bool bFree = false;
	/** Out under the sky. */
	bool bOutdoors = false;
	/** The dawn has hardly begun. */
	bool bNight = false;
	/** How much of it there is now (Sky.hole, 0 to 1). */
	double Hole = 0;
	/** Where it is from here, in world terms (the sky's direction turned with the area's sky). */
	FVector Dir = FVector(0, 0, -1);
	FS47Cam Cam;
	/** The closer look (the zoom) is most of the way in. */
	bool bZoom = false;
	/** Nothing solid between the eye and the sky along Dir. */
	TFunction<bool()> Clear;
};

class SIGNAL47_API FS47StarHole
{
public:
	explicit FS47StarHole(FS47StarHoleDeps InD);

	/** The shared STAR_HOLE (S47SkyEventSpec.h): where it is (az, el), its rims (degrees), how long after
	 *  the impact it begins, how long it takes to come in (seconds). A compile-time copy of the one spec,
	 *  so FS47StarHole::STAR_HOLE.X and the sky's ::STAR_HOLE.X can never differ. */
	using FHoleSpec = FS47StarHoleSpec;
	static constexpr FS47StarHoleSpec STAR_HOLE = ::STAR_HOLE;
	/** The look that notices it: degrees off the middle of the view, seconds (and with the zoom). The
	 *  web version's 4 / 2.5 degrees sat just outside / inside its 3.4-degree inner rim; FX-01 keeps the
	 *  same proportions for the larger field: the middle of the view inside the starless part (Inner),
	 *  and the zoom's closer look three quarters of the way in. */
	struct FNotice { double Deg, Secs, ZoomDeg, ZoomSecs; };
	static constexpr FNotice NOTICE = { ::STAR_HOLE.Inner, 3, ::STAR_HOLE.Inner * 0.75, 1.2 };
	/** About how many degrees across it is, as a new notice and its card describe it (between the rims). */
	static constexpr double EXTENT_DEG = ::STAR_HOLE.Inner + ::STAR_HOLE.Outer;
	/** Under a free frame with it in it (core/album.ts). */
	static const TCHAR* HOLE_PRINT;
	static const TCHAR* HOLE_TOAST;
	/** Sky.ts starHoleAt with the shared fade (S47StarHoleAt): how much of the hole there is at a clock,
	 *  when it begins at From. Only a fallback: in the game, Life.HoleNow() is the one strength. */
	static double At(double Clock, TOptional<double> From);
	/** Sky.ts skyDir(az, el): its direction in the sky's own terms (before the area's turn). */
	static FVector SkyDir(double Az, double El);

	TUniquePtr<FS47RuleBook> Book;
	/** Seconds the eyes have been on it (the tests). */
	double Look = 0;
	/** Tests that are about other things: never noticed. */
	bool bOff = false;

	bool Noticed() const;
	/** When it was noticed (seconds since midnight), or unset. */
	TOptional<double> When() const;
	/** Noticed under the larger FX-01 field ('sky:hole:extent14'); false for older observations. */
	bool Wide() const;
	/** The first free frame that has it, by its id (empty when none). */
	FString PrintId() const;
	/** Its card on the board, once noticed. */
	TArray<FS47RuleBook*> Books();

	/** Each frame: the eyes on it, long enough, notice it. */
	void Update(double Dt, const FS47StarHoleIn& S);
	/** What a closer look (the zoom) at it says, or empty when it is not looked at. */
	FString Examine(const FS47StarHoleIn& S) const;
	/** A free frame was taken with it in it: kept for its card, and noticed now if nothing else of the
	 *  night's is going (bFree). */
	void Photographed(const FString& ShotId, bool bFree);
	/** A new night or a case opened: as the case has it; its threads. */
	void Reset();
	/** The developer menu, the tests: not noticed tonight (again). */
	void Forget();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47StarHoleDeps D;
	TArray<FString> Links;
	FString BookKey;
	TArray<TUniquePtr<FS47RuleBook>> Retired;
	FString Region() const;
	void MakeBook();
	void Notice(bool bViaPrint);
	TArray<FS47BoardCard> Cards() const;
	void KeepLinks();
};
