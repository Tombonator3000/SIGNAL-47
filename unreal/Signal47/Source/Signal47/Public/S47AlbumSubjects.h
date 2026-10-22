#pragma once

#include "CoreMinimal.h"
#include "S47Album.h"

class US47Main;

// web/src/story/albumSubjects.ts: what the album can name under a free frame (core/album.ts), where
// the player is: the known things of each area by where they stand, the people of the night, the
// sky, and the things the player may not have noticed: one of them at the edge of the view (core/
// glimpses.ts), the someone in a SARO jacket standing where the time was lost and the ring of frost
// there (story/abduction.ts), the ninth cow lying at STATION 01 (story/ninthCow.ts).
//
// (Not here until their modules are ported: the cardboard alien wherever it has been put (core/
// cutouts.ts), the patch of sky with no stars where the dishes point (story/starHole.ts), Caliche's
// people and the deputy's car as the town has them now (world/Town.ts: its windows and the
// substation are named as they stand with nobody in them).)
struct FS47SubjectDeps
{
	US47Main* M = nullptr;
	/** Out under the sky (not in a room, the lab, the diner or the hut). */
	TFunction<bool()> Outdoors;
	/** Night: the dawn has hardly begun. */
	TFunction<bool()> Night;
	/** Seconds since midnight. */
	TFunction<double()> Clock;
};

namespace S47AlbumSubjects
{
	SIGNAL47_API const TCHAR* CARDBOARD();
	SIGNAL47_API const TCHAR* JACKET();
	SIGNAL47_API const TCHAR* NINTH_PRINT();
	SIGNAL47_API TArray<FS47Subject> AlbumSubjects(const FS47SubjectDeps& D);
}
