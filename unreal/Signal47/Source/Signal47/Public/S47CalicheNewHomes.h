#pragma once
#include "CoreMinimal.h"
class US47Main;

// CALICHE-HOMES (Tom, 9 November about 16:40: "Må kanskje et par hus til i Caliche, så folk har steder å bo?"). Four homes for the
// town's people who had none (S47TownDay: the waitress at the night counter, Earl, the woman with the radio, the Mesa's regular).
// H05 to H07 are rooms in Content/CalicheAccess/assembly.json, set down on the H02, H01 and H04 models (their shells, roofs and
// fixtures from /Game/CalicheHomes, their rooms and doors from /Game/CalicheAccess, in other colours by slot), seated on the land
// where they stand. H08 is a trailer: Tom's Tripo motorhome where it is imported, else a single-wide of boxes. What is here: each
// home's yard (a coyote fence of latillas behind the houses), its mailbox at the street's edge, its garbage can, the windows that
// glow while somebody is home and awake (S47CalicheAccess::LampLevel, set by S47TownDay), the trailer's porch lamp, the plants
// under them shrunk away (never removed, as S47TownSites does), and their colliders for the walk and the truck. The cars are props
// (Config/Props/town.json: Earl's pickup, the junker by the trailer). -S47CalicheNewHomesOff: none of this (the rooms remain).
//
// Road-local metres (x east of the highway's centre, z south), the town's own frame; three-space x is 8000 more.
namespace S47CalicheNewHomes
{
 struct FHome
 {
  const TCHAR* Id;    // "H05": S47CalicheAccess room id ("H08": the trailer's lamp only)
  const TCHAR* Who;   // S47TownDay's name for who lives there
  bool bTrailer;
  double X0, X1, Z0, Z1;     // the footprint
  int32 Front;               // which way its door and windows face: -1 west, 1 east
  double DoorZ, WindowZ;     // along the front
  double YardX0, YardX1;     // the fenced yard behind it, across x (equal: none); its z is the footprint's
  double MailX, MailZ, CanX, CanZ;
 };
 SIGNAL47_API const TArray<FHome>& Homes();
 // From S47CalicheAccess::Init (after the rooms) and S47CalicheAccess::Update.
 void Build(US47Main* M);
 void Update(US47Main* M);
 // F10, "New homes at dusk": in the running world, each home built, its footprint and yard clear of the walk's colliders that are not
 // its own, and its windows lit exactly while the day has its owner home and awake. Lines: a summary first. Returns how many fail.
 SIGNAL47_API int32 Measure(US47Main* M, TArray<FString>& Lines);
 // No house in the road, on the power line, by a street lamp, outside the town's fence, in another house or a temporary box, on an
 // event place's spot or in an exported collider; every one of the town's people has a home to sleep in.
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
