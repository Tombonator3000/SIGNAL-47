#pragma once
#include "CoreMinimal.h"
#include "S47NightTiers.h"

class US47Main;
struct FS47RadioSeg;

// NIGHT-RADIO-20261109 (Tom, 9 November: «Trenger mere nattradio»): Freeform's 1420 AM, Earl's Night Line, made for each night from
// the game's seed instead of one short loop. On the air from eight in the evening to the sign-off at four. An hour is records with
// Earl between them: his own short bits, the Socorro County weather (S47Weather, the same day the paper and the sky have), the roads,
// what the town wants said (church, school, the county), callers who cannot sleep, and now and then a stretch of nothing but static.
// Some callers have seen or heard something, held to the hour's tier (S47NightTiers): before midnight only what can be explained
// away, midnight to two a figure a long way off, and only from two what came close. Text and captions only: nothing new is recorded
// (the callers and Earl's new words are mumbled and captioned as the other unrecorded radio lines are; three of Earl's recorded bits
// and the recorded station call are kept word for word, so his own voice still comes).
//
// What Earl and the callers say never repeats in the first two weeks of a game (each list is gone through in the seed's order before
// any of it comes again), no two nights of the first thirty are the same, and it answers what the player has done through what is
// already there: a tip of theirs Earl read on the air, the county's trust (SheriffTrust), the big night, and what the night's own
// records hold from the night before (a time and a direction from the dishes, as the bearing on the map has it). Nothing here puts a
// place on the map: that stays with S47Places and the tips.
struct FS47NightLineIn
{
 int32 Night=1;                 // FS47FreeformSession::Day(): eight in the evening to four in the morning are one night
 int32 Seed=0;                  // the game's (FS47FreeformSession::Seed): the weather is read from it too
 int32 SheriffTrust=0,TownTrust=0;
 int32 BigNight=0;              // the night the big night came (FS47Happenings::BigHappened), 0 not yet or never
 FString BigPlace;              // where it stood, as a voice says it
 FString TipPlace;              // the last tip of the player's that Earl read before tonight, as a voice says it
 int32 TipNight=0;              // the night he read it
 // Last night's records (FS47FreeformSession::FindingText): what, when (seconds after midnight, -1 unknown) and which way from
 // the dishes (degrees, -1 unknown). Kind: "landing", "burst" or "dark".
 struct FLogged { FString Kind; double Clock=-1, Az=-1; };
 TArray<FLogged> Logged;
};

namespace S47NightLine
{
 // Freeform's hours on 1420: from eight in the evening to four (the sign-off, S47Radio::SIGN_OFF).
 inline constexpr double OnFrom=20*3600.;
 inline bool OnAir(double SecondsAfterMidnight){const double D=FMath::Fmod(FMath::Fmod(SecondsAfterMidnight,86400.)+86400.,86400.);return D>=OnFrom||D<4*3600.;}
 // One hour of the night's programme (Hour: 20 to 27, past midnight counted on past 24): the talk and the records in turn, a little
 // more than an hour of them. OutIds/OutTiers, when asked: each talk item's id and the most it holds, in order.
 SIGNAL47_API TArray<TSharedPtr<FS47RadioSeg>> Hour(const FS47NightLineIn& In,int32 Hour,TArray<FString>* OutIds=nullptr,TArray<ES47Allowed>* OutTiers=nullptr);
 // The game's facts for tonight. False outside Freeform.
 SIGNAL47_API bool Fill(const US47Main* M,FS47NightLineIn& In);
 // What a record left in the night's log says of when and which way: "At 02:14 ... about 63 degrees from SARO ...".
 SIGNAL47_API void ReadLogged(const FString& Text,double& OutClock,double& OutAz);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
