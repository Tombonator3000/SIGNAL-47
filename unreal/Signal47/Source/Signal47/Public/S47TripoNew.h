#pragma once
#include "CoreMinimal.h"

// Tom's nine Tripo models of 9 November (TRIPO-NEW-20261108), made on his paid Tripo plan: private, under /Game/Licensed/TripoNew,
// never in Git (Content/Licensed is ignored; /Game/Licensed is cooked whole). Prepared and imported with Tools/triponew (README).
// Static meshes, each baked in a pose, foot pivot at the floor: a figure faces +Y (Three +Z), a vehicle's front is -Y (Three -Z),
// as the TripoNPC figures and the TripoFleet trucks. S47Happenings holds its own five paths (S47Hap); they are the same names.
// A build without them falls back to what stood in before (Resolve), never to nothing.
// "parcel" (Tom's "pakke tripo3d-1") is no parcel: it is an eighties walkie-talkie, and it is the one on the SARO desk (DeskWalkie,
// S47FieldRadio). "walkie", with its display and keypad, looks too new for 1986 and is not used in the game.
// The Nordic and the Cosmic one may be the figure on the rise, the figure on foot and the crossing far ahead (S47RoadGlimpse); Zet
// stays in the humorous events.
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
  double SizeM;          // the height of a figure or the walkie, the length of a vehicle or the parcel, as prepared
  uint32 StandInColour;  // a stand-in's paint (0 keeps its own)
 };
 SIGNAL47_API const TArray<FModel>& Models();
 SIGNAL47_API const FModel* Find(const FString& Id);
 // The walkie on the SARO desk: Tom's eighties one ("parcel", 9 November), its face toward -X in Blender.
 SIGNAL47_API const FModel* DeskWalkie();
 // Its height on the desk, metres, antenna included (as prepared it stands 0.52 m: Tripo's scale, sized by its width).
 inline constexpr double DeskWalkieHeightM=.30;
 // The model if it is in this build, else its stand-in if that is, else a basic cube; bOwn: the model itself.
 SIGNAL47_API const TCHAR* Resolve(const FModel& Model, bool* bOwn = nullptr);
 SIGNAL47_API bool Exists(const TCHAR* ObjectPath);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
