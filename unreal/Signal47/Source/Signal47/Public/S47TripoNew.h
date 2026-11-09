#pragma once
#include "CoreMinimal.h"

// Tom's nine Tripo models of 9 November (TRIPO-NEW-20261108), made on his paid Tripo plan: private, under /Game/Licensed/TripoNew,
// never in Git (Content/Licensed is ignored; /Game/Licensed is cooked whole). Prepared and imported with Tools/triponew (README).
// Static meshes, each baked in a pose, foot pivot at the floor: a figure faces +Y (Three +Z), a vehicle's front is -Y (Three -Z),
// as the TripoNPC figures and the TripoFleet trucks. S47Happenings holds its own five paths (S47Hap); they are the same names.
// A build without them falls back to what stood in before (Resolve), never to nothing.
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
 // The model if it is in this build, else its stand-in if that is, else a basic cube; bOwn: the model itself.
 SIGNAL47_API const TCHAR* Resolve(const FModel& Model, bool* bOwn = nullptr);
 SIGNAL47_API bool Exists(const TCHAR* ObjectPath);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
