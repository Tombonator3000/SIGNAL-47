#pragma once
#include "CoreMinimal.h"

// GLIMPSE-ROAD-20261109 (Tom, 9 November; the glimpse tier of S47NightTiers, midnight to two, from eleven in the fourth week): at the
// wheel at night, something crosses the highway far ahead in the lights, 60 to 120 m off, and is gone before the truck comes up: a
// tall thin figure, or something low that goes on four. Never near, never contact. Some nights it leaves prints on the shoulder, there
// to be found the next day. Rare: director.json's "road_glimpse" (tier glimpse, min_nights_between 7: at most once a week), and every
// choice from the game's seed. FS47Happenings draws it and moves it; this is the pure part: the plan, the crossing, the words.
//
// The bodies are the ones there are (Tom: reuse first): his Cosmic and Nordic (S47TripoNew) when this build has them, else the tall
// TripoNPC figure that stands in for them; on four, the TripoNPC crawler. Zet is humour and stays in the humorous events. Neither the
// Nordic (a man's eyes) nor the Cosmic (no face) has the eye's dot (S47EyeGlint), until Tom says otherwise.
namespace S47RoadGlimpse
{
 enum class EBody : uint8 { Tall, Low };
 struct FPlan
 {
  FString Id;            // the body: cosmic, nordic, tall (the stand-in), shadow, crawler (PathOf); empty: none in this build
  EBody Body=EBody::Tall;
  double AheadM=90;      // how far ahead of the truck it crosses, FromM to ToM
  double Side=1;         // the shoulder it comes from as the driver sees it: +1 the right, -1 the left
  bool bLooks=false;     // the tall one stops a moment on the crown of the road, turned to the lights
  bool bTracks=false;    // it leaves prints on the far shoulder
 };
 inline constexpr double FromM=60,ToM=120;
 // Gone once the truck is this near, wherever it is in its crossing: never near.
 inline constexpr double GoneWithinM=45;
 // From this far out on one side of the centre line to as far on the other (road-local metres).
 inline constexpr double EdgeM=12;
 // The truck's lights at their far reach, narrow (the near throw is FS47Happenings::LitBy's, 80 m and 24 degrees).
 inline constexpr double BeamFarM=125,BeamFarDeg=14;
 // The asset of a body id; null for one that is not a body here.
 SIGNAL47_API const TCHAR* PathOf(const FString& Id);
 // From the seed and the night (and the truck's speed, so that it is off the road before the truck can come up): which body, from
 // which side, how far ahead. Want: "tall" or "low" or a body id (F10); empty: the seed's. Have: whether an asset is in this build.
 SIGNAL47_API FPlan PlanFor(int32 Seed,int32 Night,double TruckSpeed,TFunctionRef<bool(const TCHAR*)> Have,const FString& Want=FString());
 // Seconds from one side to the other, with the stop on the crown when it looks.
 SIGNAL47_API double CrossSeconds(const FPlan& P);
 // Where across the road it is at Age: road-local metres from the centre line toward the driver's right (+), and whether it is still
 // on its way (false once it is off the far side). bStanding: in its stop on the crown.
 SIGNAL47_API double CrossX(const FPlan& P,double Age,bool* bOn=nullptr,bool* bStanding=nullptr);
 // A little rise and fall to its gait, metres: it does not glide.
 SIGNAL47_API double Bob(const FPlan& P,double Age);
 // In the truck's lights: their near throw, or the narrow far reach.
 SIGNAL47_API bool InBeam(double Metres,double Degrees);
 // Horizon figures and the figure on foot (FS47Happenings): the shade as before, or Tom's Nordic or Cosmic when this build has them,
 // from the seed ("", "nordic" or "cosmic"). Want: one of those, or "shade" (F10); empty: the seed's.
 SIGNAL47_API FString FigureFor(int32 Seed,int32 Night,uint32 Salt,TFunctionRef<bool(const TCHAR*)> Have,const FString& Want=FString());
 // F10-NORDIC-COSMIC-20261109 (Tom, 9 November: «Nordic og Cosmic alien må inn i F10-dev-menyen også»): the F10 lines that ask for
 // each of his two by name, in each event that may have them (horizon_figure, figure_near, road_glimpse: Want above), and the two
 // close by in "Tripo new" (close: the Nordic by day and by night, the Cosmic one's glow by night). S47HappeningsDev and
 // S47TripoNewDev make their lines from this table; the self-test asks that every event has both and that each line's ask is the
 // body it gets. Hour: the clock the line is set to (.5 is 00:30, 23 is 23:00).
 struct FDevBody{const TCHAR* Event;const TCHAR* Body;const TCHAR* Label;const TCHAR* Note;double Hour;};
 SIGNAL47_API const TArray<FDevBody>& DevBodies();
 // Its label's name for a body: "the Nordic", "the Cosmic one".
 SIGNAL47_API const TCHAR* DevBodyName(const FString& Body);
 // What the player keeps: the crossing, when it was seen (Clock as "00:40"); the prints, when found.
 SIGNAL47_API FString Record(const FPlan& P,const FString& Clock);
 SIGNAL47_API FString TracksRecord(bool bLow,const FString& Clock,bool bDay);
 SIGNAL47_API FString Deed(const FPlan& P);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
