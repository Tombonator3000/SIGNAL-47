#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47EventPlaces;

// UI-20261112 (Tom: «stedskort»): the location card of a case-file show. On arriving somewhere the place is typed out in the lower
// left corner in white typewriter letters (Special Elite), a letter at a time with the teleprinter's type bar ticking quietly
// (US47Audio::TypeBar, not placed): line 1 the building or place, line 2 the town or county and the state, line 3 the time on the
// 24-hour clock as the HUD has it. It stands about three seconds and fades.
//
// Freeform (and a Story night, which is a Freeform session). No new places: every card is a place the game already has, where the
// place catalogue puts it (FS47EventPlaces, with S47TownSites' and FS47Places' own), the farm's ground (S47FarmWorld) and the
// place name of the substation (US47Flow::PlaceName). The first arrival of each day (the day turns at 08:00), never two within
// MinGap seconds, never over a panel or a fade; from a truck at speed only a whole area's card (SARO, Caliche), the rest once it
// slows. Settings, Game: "Location cards" (US47Flow::bLocationCards). SS47Hud draws it and ticks it (SS47Hud::LocationCard).
namespace S47LocationCards
{
 struct FCard
 {
  const TCHAR* Id;      // the card's own id: what is remembered per day
  const TCHAR* Place;   // where: an FS47EventPlaces id, or a S47TownSites landmark's; empty: by rule (At)
  const TCHAR* Line1;   // the building or place
  const TCHAR* Line2;   // the town or county, and the state
  double Radius;        // metres from the place (from its way in, bDoor) that count as there
  bool bArea;           // a whole area: also shown from a truck at speed, and only where no place of its own is nearer
  bool bDoor;           // measured from the place's door, not its ground outside (a building on the street)
 };
 inline constexpr double CharsPerSec=30.,LinePause=.25,Hold=3.,FadeSecs=1.2;   // the typing, between two lines, standing, going
 inline constexpr double MinGap=20.,FastMps=8.,Look=.5;                        // real seconds between two cards; "at speed"; how often it looks
 inline constexpr float TickGain=.16f,ReturnGain=.1f;                          // the type bar's level for a letter and for the end of a line
 inline constexpr const TCHAR* County=TEXT("SOCORRO COUNTY, NEW MEXICO");
 inline constexpr const TCHAR* Town=TEXT("CALICHE, NEW MEXICO");
 inline constexpr const TCHAR* SubstationName=TEXT("Caliche Junction Substation"); // US47Flow::PlaceName's

 const TArray<FCard>& Cards();
 const FCard* Find(const FString& Id);
 // Where a card's place stands, road-local (x east, z south). False for a card that has no place of its own (by rule).
 bool Spot(const FCard& C,const FS47EventPlaces& Catalogue,FVector2D& Out);
 // Pure: the card for someone standing here: the area's name (AS47Web::Area), the place name (US47Flow::PlaceName) and, when the
 // ground is on the land's one frame (bRoad), where. A place of its own first (the nearest that holds the spot), then the farm,
 // the town, the station's ground. Empty: nowhere with a card.
 FString At(const FS47EventPlaces& Catalogue,const FString& Area,const FString& PlaceName,bool bRoad,double X,double Z);
 // Pure: whether the card may come now: not yet today, MinGap since the last one, and from a truck over FastMps only an area's.
 bool May(bool bShownToday,bool bArea,bool bDriving,double Speed,double SinceLast);
 // Pure: "23:42" from the game's clock in seconds; never one of Story's numbers (FS47Places::SlipClock).
 FString Clock(double GameClock);
 // Pure: the card's lines as typed: the place, the town and state, the time.
 TArray<FString> Lines(const FCard& C,double GameClock);
 // Pure, the typing (T: seconds since the card began): when a line starts, how many of its letters are typed, when the last letter
 // is down, how strongly it shows (1 while typed and standing, to 0 over FadeSecs), and when it is gone.
 double LineStart(const TArray<FString>& L,int32 Line);
 int32 Typed(const TArray<FString>& L,int32 Line,double T);
 double TypeSecs(const TArray<FString>& L);
 double Alpha(const TArray<FString>& L,double T);
 double Life(const TArray<FString>& L);
 // Every frame (US47Main::Step): looks where the player is twice a second and asks the HUD for the card of a place just come to.
 void Step(US47Main* M,double Dt);
 // Critic 12 Nov: today's cards kept in the session's save ("locationCards": card id -> day; optional), so that a Continue does not
 // type the same places out again. Session: the FS47FreeformSession the memory belongs to.
 void Save(const void* Session,int32 Today,const S47J::FObj& O);
 void Load(const void* Session,const S47J::FObj& Saved);
 bool SelfTest(TArray<FString>& Out);
}
