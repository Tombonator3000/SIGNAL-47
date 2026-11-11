#pragma once
#include "CoreMinimal.h"
#include "S47NightTiers.h"

class FS47FreeformSession;
struct FS47Doc;
struct FS47NbScribble;

// MARGINALIA-20261109 (Tom, 9 November: "konspiratoriske skriblerier om de forskjellige aliens og folka man moter"): the player's own
// pencil in the notebook's WHO & WHAT, PEOPLE on the left page and THEM on the right. Each scribble is unlocked by something that has
// really happened in this game (a field record, what someone in Caliche has told, a talk on the phone), never more than that thing
// showed (S47NightTiers: lights and sounds, a far shape, near), and some are wrong guesses that are struck through once a later find
// says otherwise. THEM are named only as the player would name them, never by what they are. Someone gone is only gone.
// Nothing is saved: everything comes from the findings and the town's Told as they are saved already (FactsOf).
namespace S47Marginalia
{
 enum class ESide : uint8 { People, Them };
 // What a scribble points on to: the survey sheet (MAP), a question for the pinboard (BOARD), or nothing.
 enum class EFollow : uint8 { None, Map, Board };
 struct FRow
 {
  const TCHAR* Id;
  ESide Side;
  const TCHAR* Box;       // the box it is pencilled in: "DALE", "THE LOW ONE"
  const TCHAR* Role;      // People: the line under the name; Them: empty
  const TCHAR* Who;       // People: whose it is, for "gone:" (a Townsfolk id, "baca", "dale", "earl", "ward"); Them: empty
  const TCHAR* Trigger;   // the fact that unlocks it (a prefix of one of FactsOf's facts)
  ES47Allowed Tier;       // the most the line holds; never more than its trigger showed (TierOf)
  const TCHAR* Text;      // English, 1986, in pencil
  const TCHAR* StruckBy;  // a fact that proves the guess wrong: then it is struck through; empty: never
  EFollow Follow;
  const TCHAR* FollowText;
 };
 SIGNAL47_API const TArray<FRow>& Table();

 // What has happened, as plain facts: every finding's key, and from them and the town:
 //   "glimpse:tall" / "glimpse:low"   something crossed the road far ahead (road_glimpse_dN, by its record's words)
 //   "tracks:tall" / "tracks:low"     its prints found on the shoulder (road_tracks_dN)
 //   "rise:shade|nordic|cosmic"       the figure on the rise (horizon_figure_dN, the body the seed drew that night)
 //   "near:steps|shade|nordic|cosmic" the figure on foot (figure_near_dN; steps: nothing was seen)
 //   "told:<who>:<key>"               what one of Caliche's regulars has told (FS47Townsfolk's Told)
 //   "gone:<who>"                     their post empty now (FS47Townsfolk::Away, FS47Visitors::BacaGone)
 //   "baca:told" "dale:talk" "ward:talk" "earl:call" "earl:tip"
 struct FFacts
 {
  TSet<FString> Keys;
  bool Has(const FString& Prefix) const;
 };
 // A finding into facts (pure, for the tests). Seed and Have: the game's seed, and whether this build has an asset (S47RoadGlimpse::FigureFor).
 SIGNAL47_API void AddFinding(FFacts& Out, const FString& Key, const FString& Text, int32 Seed, TFunctionRef<bool(const TCHAR*)> Have);
 // The running game's facts: its field records (the notebook's "freeform-" papers, Docs), the town, Baca, the radio.
 SIGNAL47_API FFacts FactsOf(const FS47FreeformSession& F, const TArray<FS47Doc>& Docs);
 // The most a fact showed: a far light or a sound (Hint), a shape far off (Glimpse), something near (Full).
 SIGNAL47_API ES47Allowed TierOf(const FString& Fact);
 // The scribbles the facts unlock, in the table's order: PEOPLE first, then THEM. Each line keeps to the spoiler limits
 // (S47Script::LintText) or is left out.
 SIGNAL47_API void Fill(const FFacts& Facts, TArray<FS47NbScribble>& Out);
 // Words that name what they are: never in THEM.
 SIGNAL47_API bool NamesKind(const FString& Text);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
