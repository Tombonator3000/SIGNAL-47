#pragma once
#include "CoreMinimal.h"
class UMaterialBillboardComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

// Tom's nine Tripo models of 9 November (TRIPO-NEW-20261108), made on his paid Tripo plan: private, under /Game/Licensed/TripoNew,
// never in Git (Content/Licensed is ignored; /Game/Licensed is cooked whole). Prepared and imported with Tools/triponew (README).
// Static meshes, each baked in a pose, foot pivot at the floor: a figure faces +Y (Three +Z), a vehicle's front is -Y (Three -Z),
// as the TripoNPC figures and the TripoFleet trucks. S47Happenings holds its own five paths (S47Hap); they are the same names.
// A build without them falls back to what stood in before (Resolve), never to nothing.
// PT-WALKIE-20261109 (Tom): "walkie" (his "walkie talkie 3d model") is the better walkie, the one on the SARO desk (DeskWalkie,
// S47FieldRadio). "geiger" (his "pakke tripo3d-1", SM_Tripo_Parcel) is no parcel and no walkie: a Geiger counter's prototype, the
// counter held up in the hand once it is built (S47Happenings' "geiger" prop); without it the counter is the drawn one as before.
// The Nordic and the Cosmic one may be the figure on the rise, the figure on foot and the crossing far ahead (S47RoadGlimpse); Zet
// stays in the humorous events. PT-NORDIC-COSMIC-20261109: the Nordic has a man's eyes and nothing on them; the Cosmic one shines
// all over, a body of light with stars in it (AddCosmicGlow), whatever draws it.
namespace S47TripoNew
{
 enum class EKind : uint8 { Figure, Alien, Vehicle, Prop };
 struct FModel
 {
  const TCHAR* Id;       // singer, bigfoot ...
  const TCHAR* Label;    // what it is, for a log or a menu
  const TCHAR* Path;     // the private model
  const TCHAR* StandIn;  // what stands in without it (another asset, or a basic shape)
  EKind Kind;
  double SizeM;          // the height of a figure or the walkie, the length of a vehicle or the counter's case, as prepared
  uint32 StandInColour;  // a stand-in's paint (0 keeps its own)
 };
 SIGNAL47_API const TArray<FModel>& Models();
 SIGNAL47_API const FModel* Find(const FString& Id);
 // The walkie on the SARO desk: Tom's better one ("walkie"), its face toward -Y in Blender, as prepared .34 m with its antenna.
 SIGNAL47_API const FModel* DeskWalkie();
 // Its height on the desk, metres, antenna included: as prepared.
 inline constexpr double DeskWalkieHeightM=.34;
 // The Geiger counter in the hand: Tom's prototype ("geiger", SM_Tripo_Parcel), standing up with its face toward -X in Blender (as
 // prepared .52 m tall, sized by its width), brought down to a hand-held height.
 SIGNAL47_API const FModel* Geiger();
 inline constexpr double GeigerHeightM=.16;
 // The Cosmic one's light (PT-NORDIC-COSMIC-20261109): the whole body glows blue-white, faintly turquoise, with small stars in it
 // that drift and twinkle, and it breathes slowly. Unlit and additive: it shows in the dark and lights nothing round it (no lamp,
 // and a translucent overlay gives Lumen no light to bounce). An overlay on its own material, and a few star points on the body.
 struct FCosmicGlow
 {
  TWeakObjectPtr<UStaticMeshComponent> Body;
  TWeakObjectPtr<UMaterialInstanceDynamic> Skin;
  TArray<TWeakObjectPtr<UMaterialBillboardComponent>> Stars;
  TArray<FVector> StarAt;  // each star's home on the body, the mesh's own centimetres
  TArray<float> Phase;
  double Age=0;
 };
 // Whether this asset is the Cosmic one's body (the glow is for it alone: the Nordic gets nothing).
 SIGNAL47_API bool IsCosmic(const FString& AssetName);
 // On a drawn body (its parts registered): the overlay and the stars, children of Body, so they show and hide with it. False: not
 // the Cosmic one, or this build has nothing to draw it with.
 SIGNAL47_API bool AddCosmicGlow(UObject* Outer,UStaticMeshComponent* Body,FCosmicGlow& Out);
 // Each frame it is shown: the breath, the drift and the twinkle.
 SIGNAL47_API void StepCosmicGlow(FCosmicGlow& Glow,double Dt);
 // The pure part, for the self test: its brightness at Age (a slow breath, never off) and one star's at Age.
 SIGNAL47_API double CosmicBreath(double Age);
 SIGNAL47_API double CosmicTwinkle(double Age,float Phase);
 // The model if it is in this build, else its stand-in if that is, else a basic cube; bOwn: the model itself.
 SIGNAL47_API const TCHAR* Resolve(const FModel& Model, bool* bOwn = nullptr);
 SIGNAL47_API bool Exists(const TCHAR* ObjectPath);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
