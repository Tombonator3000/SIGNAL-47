#pragma once

#include "CoreMinimal.h"

class US47Inter;
class AS47Web;

// web/src/story/looks.ts. What a closer look says (the zoom: right mouse; core/Interaction.ts) about the
// night's things that had nothing to say (todo.md: the antennas, the signs, the papers on the desks,
// the motel). Two kinds:
//   - things nobody uses: the antennas, the signs at SARO and down the highway, the motel's sign, its
//     ice machine, the car and the pool. Each gets an invisible hit area of its own, only for a closer
//     look (look: true), so it never stands in the way of using what is behind it. (The web version
//     makes them here; they are in the export by their ids, 'look:S-01', 'look:yardSign' ..., and are
//     bound to their exported nodes.)
//   - things the chapters already bind for use (the papers, the phones, the diner, Caliche's street,
//     room 6): a text by their id in Interaction.looks (M->Inter->Looks), read when they have no
//     Examine of their own. The chapters bind them when they are built; the texts wait here.
// Short and plain, the way the shift log is written. No new facts the story leans on.
struct FS47LooksDeps
{
	TWeakObjectPtr<US47Inter> Inter;
	/** SARO's antennas (Exterior.dishes, AS47Web's) and the exported hit areas. */
	TWeakObjectPtr<AS47Web> Web;
	/** Dish.rogue and Dish.back for the dish at an index: driven by nothing on the console
	 *  (story/archiveDishes.ts, story/dishesFollow.ts), turning back to 026 without a sound
	 *  (story/arrayBack.ts). Unset: never. */
	TFunction<bool(int32)> Rogue, Back;
};

namespace S47Looks
{
	SIGNAL47_API void BindLooks(const FS47LooksDeps& D);
	/** What a closer look at an antenna says (its examine()). */
	SIGNAL47_API FString DishSays(const FString& Id, int32 Index, double CurAz, double CurEl, bool bMoving, bool bRogue, bool bBack);
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
