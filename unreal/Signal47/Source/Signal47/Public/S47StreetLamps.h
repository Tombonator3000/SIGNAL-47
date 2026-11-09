#pragma once

#include "CoreMinimal.h"
#include "S47ExteriorFixtures.h"

class AS47Web;
class UMaterialBillboardComponent;
class UMaterialInstanceDynamic;
class USpotLightComponent;
class UStaticMeshComponent;

// STREETLAMPS-20261108: the street lamps as New Mexico had them in 1986: a tapered steel pole, an upswept davit arm and a cobra head
// (a cast aluminium housing over a prismatic glass bowl, a photocell on top), lit by high-pressure sodium. The models are original
// (unreal/ArtSource/StreetLamps). Each lamp replaces the old one's drawn pole and arm where it stood (their triangles cut from the
// export's merged batch at runtime, as S47ExteriorFixtures does) and stands on what is under it: its foot is measured down onto the
// concrete footing or the ground at build. The light stays the old one's (S47ExteriorLighting: same feed, slot and photocell); it
// moves to the new head's bowl, and turns sodium yellow. With no models imported, nothing changes. -S47StreetLampsOff: the old lamps.
//
// STREETLAMP-GLOW (9 November, Tom's picture at night: the road lit, the heads dark): the 05:27 import put every LOD1/2 section of
// the four meshes on slots of their own holding WorldGridMaterial (set_lod_from_static_mesh reuses a slot only for the same
// material object, and the LOD files came in without materials). From a few metres on a head was drawn grey, lit orange from
// under, and its lens, a dynamic material on LOD0's slot alone, never glowed. Now every slot that repeats an earlier slot's name
// draws that slot's material (the lens's dynamic one included), and a faint sodium halo hangs just under each bowl, lit with it.
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

 // ---- STREETLAMP-GLOW ----
 // A head's glow: its lens (one dynamic material on every LOD's lens slot) and its halo (a camera-facing soft spot under the bowl).
 struct FGlow
 {
  TWeakObjectPtr<UStaticMeshComponent> Head;
  TWeakObjectPtr<UMaterialInstanceDynamic> Lens;
  TWeakObjectPtr<UMaterialBillboardComponent> Halo;
  TWeakObjectPtr<UMaterialInstanceDynamic> HaloMid;
  FLinearColor Emissive = FLinearColor(-1, -1, -1, -1); // what the lens was last given
  float HaloSize = -1;
  bool bLit = false;
  explicit operator bool() const { return Head.IsValid() && Lens.IsValid(); }
 };
 // The emitter in the head's own terms (three metres): 8 mm under the bowl's lowest point (model_contract.json emitter_three_m).
 SIGNAL47_API FVector EmitterLocal();
 // Where a head's spot stands, in its own terms: 3 cm further down than the emitter (S47ExteriorLighting: aperture + normal x 0.03).
 SIGNAL47_API FVector SpotLocal();
 // Every material slot of a part whose name repeats an earlier slot's (the LOD1/2 copies) drawn with that earlier slot's material
 // as the component has it now. Returns how many slots were set.
 SIGNAL47_API int32 MatchLodSlots(UStaticMeshComponent* Part);
 // A head dressed: a dark dynamic lens material on every slot named M_StreetLamp_Lens, the other LOD slots matched, and a hidden
 // halo attached under the bowl (registered when the head is). Outer owns what is made.
 SIGNAL47_API FGlow Dress(UObject* Outer, UStaticMeshComponent* Head);
 // Lit or dark: Emissive is the lens's EmissiveFactor (linear; black: dark). The halo shows a share of it, none while the head is
 // hidden (s47.lamps.Halo scales it, 0 hides it; s47.lamps.HaloSize is its width in metres near by). Far from the eye the halo
 // keeps some three pixels' width and comes up towards the bowl's brightness, so a lit lamp reads for kilometres. Call it each
 // frame for that (unchanged values are not set again).
 SIGNAL47_API void Shine(FGlow& G, const FLinearColor& Emissive);
 // For S47ExteriorLighting: the glow of this world's lamp whose lens is Lens, lit so, and the spot that lights from it (for the
 // measure). False when no lamp has that lens.
 SIGNAL47_API bool Shine(AS47Web* W, const UMaterialInstanceDynamic* Lens, const FLinearColor& Emissive, USpotLightComponent* Light);
 // Every drawn lamp: its spot inside the head's bounds and under the bowl, and the lens and halo lit exactly when the spot is (or
 // dark when it is). Lit: how many glow now. Lines: a summary first, a line a lamp. Returns how many are wrong.
 SIGNAL47_API int32 MeasureGlow(AS47Web* W, TArray<FString>& Lines, int32& Lit);
 // -S47SelfTest: the imported head (when it is there): every LOD draws a lens slot, no slot is left on WorldGridMaterial, the
 // emitter and the spot's place lie in the head's bounds under the lens, and the lens and halo follow on and off.
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
