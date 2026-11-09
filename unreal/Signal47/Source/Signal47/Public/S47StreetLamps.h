#pragma once

#include "CoreMinimal.h"
#include "S47ExteriorFixtures.h"

class AS47Web;

// STREETLAMPS-20261108: the street lamps as New Mexico had them in 1986: a tapered steel pole, an upswept davit arm and a cobra head
// (a cast aluminium housing over a prismatic glass bowl, a photocell on top), lit by high-pressure sodium. The models are original
// (unreal/ArtSource/StreetLamps). Each lamp replaces the old one's drawn pole and arm where it stood (their triangles cut from the
// export's merged batch at runtime, as S47ExteriorFixtures does) and stands on what is under it: its foot is measured down onto the
// concrete footing or the ground at build. The light stays the old one's (S47ExteriorLighting: same feed, slot and photocell); it
// moves to the new head's bowl, and turns sodium yellow. With no models imported, nothing changes. -S47StreetLampsOff: the old lamps.
namespace S47StreetLamps
{
 // After S47RouteLampFeet::Build, which made the footings the poles stand on.
 SIGNAL47_API bool Build(AS47Web* W);
 // The heads by the name of the exterior light that belongs to each ("Highway01".."Highway08"), for S47ExteriorLighting.
 SIGNAL47_API TMap<FName, S47ExteriorFixtures::FFixture> Fixtures(AS47Web* W);
 // Every lamp's foot against what is under it now: the gap (positive: floating) to the first visible thing straight down under
 // the pole, and to the ground itself. Lines: a summary first, a line a lamp. Returns how many stand wrong (over 2 cm off).
 SIGNAL47_API int32 MeasureFeet(AS47Web* W, TArray<FString>& Lines);
 // High-pressure sodium, as Caliche's lamps are in the web version (Town.ts SODIUM 0xffa552).
 SIGNAL47_API FLinearColor Sodium();
}
