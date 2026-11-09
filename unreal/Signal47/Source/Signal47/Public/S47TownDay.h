#pragma once

#include "CoreMinimal.h"

class US47Main;
class AS47Web;

// NPC-DAY-20261109 (Tom, 9 November, about 13:50: "Sheriffen (og andre NPC-er) må ha en rutine: gå rundt, sitte inne og så videre.
// Være på jobb og være hjemme, alt etter tidspunktet." He had stood still in the substation's doorway all day). Freeform only.
//
// Each of Caliche's people has a day, worked out from the game's seed, the calendar day and the clock alone: nothing is saved, a
// Continue finds them where they would be. The sheriff (FS47TripoNpcs' "deputy" body) is at the substation 08:00 to 17:00 on a
// weekday: at his desk most of the time, now and then at the map on the wall, the coffee, the files, the window, out on the step or
// at his car, and on some afternoons out on patrol in the car (car gone). Lunch at the Mesa Diner about 12, in a booth or at the
// counter. He drives home about five (the car goes with him) and is at home in the evening, his lamp lit until he goes to bed.
// On a night the county's round comes by (FS47DeputyRound: the letter said it would), he is out in the car for it. Saturday a
// half day, Sunday at home. The diner's cook (the spare waitress figure, FS47TripoNpcs' "waitress": the MetaHuman at the counter
// is not this) works the kitchen 06:00 to 21:00 with a break; the gas station's attendant is the town's night man and is always at
// the station (the night window), now and then out at the pumps or for a smoke; the souvenir man keeps the shop's hours
// (FS47TownUse: 10 to 17), at the counter or on his stoop, and walks home up the east side in the evening.
//
// They walk between places at a walking pace (real time, not the clock's), along a small set of known paths in the town (doors,
// steps, the street's edge), turn to where they are going, breathe and sway a little standing; where a table or a counter hides
// their legs they sit (the figures are single poses: a seated figure is the standing one lowered behind the table). Inside a house
// or a back room they are not drawn. Out of the player's sight (another area, far off, or the clock jumped) they are simply where
// the plan says. The sheriff's place drives the town's own switches (FS47Town: the lit window, the knock and talk at his door, the
// car outside), and at lunch he can be talked to in his booth. Townsfolk who are gone (FS47Townsfolk) stay gone.
// Each house's lamp is lit while its owner is home and awake (S47CalicheAccess::SetRoomLamp). -S47TownDayOff: as before.
namespace S47TownDay
{
 // The people with a day: "deputy" (the sheriff), "cook" (the diner's cook: FS47TripoNpcs' spare "waitress" figure, so that the
 // waitress at the counter, FS47Townsfolk's "waitress", being gone does not take her too), "attendant", "souvenir_man".
 SIGNAL47_API const TArray<FString>& People();
 struct FSpot
 {
  const TCHAR* Id; const TCHAR* Place; double X, Z, Yaw; uint8 Kind; const TCHAR* Via;
  enum : uint8 { Stand = 0, Sit = 1, Hidden = 2, Away = 3, Anchor = 4 };
 };
 SIGNAL47_API const TArray<FSpot>& Spots();
 SIGNAL47_API const FSpot* Spot(const FString& Id);
 // "office", "diner", "home", "patrol", "gas", "souvenir": where a spot is.
 SIGNAL47_API FString PlaceOf(const FString& SpotId);
 struct FRow { double From = 0, To = 0; FString Spot; };
 // A night the county's round comes (FS47DeputyRound::Comes) and the hour it is due (past midnight it runs on from 24).
 struct FNight { bool bRound = false; double Due = 0; };
 // The day: rows from 0 to 24 hours for calendar day Day (0: the first), WeekDay 0 Monday to 6 Sunday. Prev: the night that ends
 // this morning; Tonight: the one that starts this evening.
 SIGNAL47_API TArray<FRow> Plan(int32 Seed, int32 Day, int32 WeekDay, const FString& Who, const FNight& Prev = FNight(), const FNight& Tonight = FNight());
 SIGNAL47_API FString SpotIn(const TArray<FRow>& Rows, double Hour);
 // Whether a spot is somewhere the person is awake at home (the lamp lit).
 SIGNAL47_API bool HomeAwake(const FString& SpotId);

 // ---- the running game ----
 // Each frame from FS47Townsfolk::Step (Freeform only).
 SIGNAL47_API void Step(US47Main* M, double Dt);
 SIGNAL47_API void Stop();
 SIGNAL47_API bool Active();
 // From FS47TripoNpcs::Update: where Role's body is drawn this frame (three-space metres, feet; yaw radians as its Euler(0,yaw,0)),
 // and whether at all. Default: where it would have stood (an Anchor spot is that). False: no day for it now; keep your own.
 // Role is FS47TripoNpcs' ("waitress" is the cook's body); SpotNow takes either.
 SIGNAL47_API bool Pose(const FString& Role, const FVector& DefaultFeet, double DefaultYaw, FVector& OutFeet, double& OutYaw, bool& bOutShow);
 // The spot the running plan has Role at now (empty: none).
 SIGNAL47_API FString SpotNow(const FString& Role);
 SIGNAL47_API FString DevState(US47Main* M);
 // In the running game: every planned standing spot and every path between them against the walk's colliders (diner/road), and
 // inside the rooms against their furniture. Lines: a summary first. Returns how many are in something.
 SIGNAL47_API int32 MeasureSpots(AS47Web* W, TArray<FString>& Lines);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
