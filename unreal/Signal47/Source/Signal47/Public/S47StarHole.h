#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Seen.h"

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

	/** The look that notices it: degrees off the middle of the view, seconds (and with the zoom). */
	struct FNotice { double Deg, Secs, ZoomDeg, ZoomSecs; };
	static constexpr FNotice NOTICE = { 4, 3, 2.5, 1.2 };
	/** Under a free frame with it in it (core/album.ts). */
	static const TCHAR* HOLE_PRINT;
	static const TCHAR* HOLE_TOAST;
	/** world/Sky.ts STAR_HOLE: where it is (az, el), its rims (degrees), how long after the impact it
	 *  begins, how long it takes to come in (seconds). */
	struct FHoleSpec { double Az, El, Inner, Outer, After, Fade; };
	static constexpr FHoleSpec STAR_HOLE = { 26, 32, 3.4, 4.6, 10, 100 };
	/** Sky.ts starHoleAt: how much of the hole there is at a clock, when it begins at From. */
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
