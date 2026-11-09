#pragma once

#include "CoreMinimal.h"

class AS47Web;
class UStaticMeshComponent;

// SHERIFF-CAR-20261109 (Tom's pictures 9 November, about 13:50: the sheriff's car at the substation and the coupe-pickup at the
// souvenir shop drawn as grey clay, with a lump on the patrol car's roof where its light bar should be). Every slot of those cars
// was drawn with one flat grey and the engine's grid lines, which is what the engine draws when a slot's material cannot be drawn
// in the packed game: the El Alamo's paint is a near black clear coat (glTF factor 0.027) and the Tripo patrol car's atlas is black
// and white, and both came out the same light grey, tyres and glass included. Neither the imported glTF instances
// (/InterchangeAssets/gltf parents) nor the Tripo car's own materials are what the rest of the town is drawn with.
//
// So the cars now wear the town's own lit master (/Game/Web/Base/M_Web_Lit_Opaque, what every building in Caliche is drawn with)
// and its translucent glass (M_Web_Lit_Trans_2S): one dynamic material per slot, by what the slot is (paint, tyre, chrome, glass,
// lamps, trim, plate, interior), each keeping the original's own colour map where it has one, so the sheriff's livery, the badges
// and the plates stay as painted. Every LOD's slots are matched to LOD0's of the same name first (S47StreetLamps::MatchLodSlots).
// The sheriff's car is the Fairheaven LT '80 in the county's tan and white (Tools/freemodels/repaint.py: star and SHERIFF on the
// doors, no makes or marks); the Tripo car is a black and white STATE POLICE car and comes back only with -S47TripoPatrol. Its
// light bar's two halves are red and blue glass (S47Props' beacon lenses, brighter, glossier).
//
// Whatever a slot was drawn with is logged once per mesh with whether the engine could draw it here ("S47 CARPAINT: ..."), so the
// next log says which of the two it was. -S47CarPaintOff: the cars as imported.
namespace S47CarPaint
{
 // -S47TripoPatrol: the Tripo STATE POLICE car back in the sheriff's role (the town, the night round, the highway).
 SIGNAL47_API bool TripoPatrol();
 // Whether a mesh is one of the cars this dresses (/Game/Free/Vehicles, SARO's parked sedan and pickup, the Tripo patrol car).
 SIGNAL47_API bool IsCar(const UStaticMeshComponent* C);
 // What a slot is, from its name and its material's: "paint", "atlas", "glass", "lamps", "tyre", "chrome", "trim", "plate", "lens",
 // "interior" or "other".
 SIGNAL47_API FString RoleOf(const FString& SlotName, const FString& MaterialName);
 // Every car the web actor holds now, not yet dressed. Returns how many components were dressed.
 SIGNAL47_API int32 Dress(AS47Web* W);
 // Each frame (cheap: a scan every two seconds), so the cars made later (the night round, the highway, visitors) are dressed too.
 SIGNAL47_API void Step(AS47Web* W, double Dt);
 // In the running game: every car's slots, and how many are still on what they came with. Lines: a summary first.
 SIGNAL47_API int32 Measure(AS47Web* W, TArray<FString>& Lines);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
