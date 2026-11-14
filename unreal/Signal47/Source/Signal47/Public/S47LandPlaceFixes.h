#pragma once
#include "CoreMinimal.h"
#include "S47Drive.h"

class US47Main;
struct FS47Rect;

// LAND-PLACES-FIXES-20261114 (→ M1, M6): what was wrong at the line shack and the cairn on the rise while they are still drawn as
// FS47Places' basic shapes, put right so that it keeps working when a model takes the shapes' place. Everything is measured off a
// small contract per place (FContract: its solid, its turn, its window), read from the basic shapes FS47Places draws (PartOf) and
// overridden by a model that stands in for them (SetModel), and off where the place stands today, never off fixed world numbers:
//   the shack's lamp   shack_lamp (S47NightPlaces) burned inside the solid box and its sight test ended inside it too, so it was never
//                      seen. It now hangs in the shack's one drawn window (on the west wall, toward the highway and SARO), on the
//                      OUTSIDE of the wall, its light just out from it; the dark window part is covered by the lit one while it burns.
//   lights and marks   FS47FreeformDirector::LightPlacePoint gave the middle of the solid; for the shack and the cairn it is now on the
//                      ground (measured at that point) just outside the solid toward the highway and SARO, 1.5 m out.
//   the cairn drifts   its tag (Reyes's, FS47Predecessor) and a ring laid round it (cairn_stones) follow where it stands today
//                      (Follow, the same for every place that drifts), and each ring stone sits on the ground under itself (StoneSeat).
//   walking, driving   the shack's box and the cairn are walk boxes of the road's list ("landplace:<id>:0") and the truck's own
//                      obstacles, exactly what is drawn or inside it, there only while the basic shapes are drawn (a model brings its
//                      own, as S47LandWindmill). The truck's obstacles live in US47World's legs, which outlive a scene: they are let go
//                      (sent far off, their place in the list kept) when the scene changes, and taken again in the new one. After the
//                      cairn has drifted its circle is not put down on a truck parked there: it waits until the truck has left.
//                      The mine has no solid shape today: nothing is added there.
namespace S47LandPlaceFixes
{
 enum class EShape : uint8 { Box, Round };

 // What a place's solid is, in its own metres (x east, y up, z south, about where FS47Places::Stand puts it) before it is turned by
 // Yaw (radians, as FS47Happenings::PoseProp turns a prop: +z goes to (sin Yaw, 0, cos Yaw)). The window, if it has one, is the
 // middle of the drawn window on a face of the solid, WindowNormal that face's outward axis (own terms). A model stands in with its own.
 struct FContract
 {
  EShape Shape = EShape::Box;
  double Yaw = 0;
  FVector Centre = FVector::ZeroVector, Size = FVector::ZeroVector;
  bool bWindow = false;
  FVector Window = FVector::ZeroVector;
  FVector2D WindowNormal = FVector2D::ZeroVector;
 };
 // The land places with a solid nobody walks into (the shack's box, the cairn's base stone) and, for the shack, which of its basic
 // shapes (FS47Places::PartOf) is its window.
 struct FSolid { const TCHAR* Place; EShape Shape; int32 WindowPart; };
 SIGNAL47_API const FSolid* SolidOf(const FString& Place);
 // Today's: from the basic shapes as FS47Places draws them. False for a place with no solid.
 SIGNAL47_API bool Basic(const FString& Place, FContract& Out);
 // A model that stands in for the basic shapes says what it is (S47LandWindmill's way: its own measures); Clear goes back to the shapes.
 SIGNAL47_API void SetModel(const FString& Place, const FContract& C);
 SIGNAL47_API void ClearModel(const FString& Place);
 // What is drawn now: the model's while the basic shapes are hidden and a model has said what it is, else the basic one.
 SIGNAL47_API bool ContractOf(const FString& Place, bool bBasicShown, FContract& Out);

 inline constexpr double Clear = 1.5;     // lights and marks: this far out from the solid (FS47FarmSite steps 1.2 m on from there)
 inline constexpr double LampOut = .055;  // the lamp's middle off the wall (its frame's back is .05 behind it: S47Hap::WindowParts)
 inline constexpr double SightShort = 1.2;// FS47Happenings::SeenLit ends its line this much short of the point (or 5 % of the way)

 // ---- pure (the self-test holds them) ----
 // Turned by Yaw as PoseProp turns (x, z) on the ground.
 SIGNAL47_API FVector2D Turn(const FVector2D& V, double Yaw);
 // The solid's footprint in its own terms (x, z; FBox2D's Y is z), not turned.
 SIGNAL47_API FBox2D FootOf(const FContract& C);
 // Footprint: x and z (FBox2D's Y is z). What holds a walker: the box itself, or the square inside the ellipse.
 SIGNAL47_API FBox2D WalkRect(EShape Shape, const FBox2D& Footprint);
 // What holds the truck: the box itself, or the circle inside the ellipse.
 SIGNAL47_API FS47Obstacle TruckOf(EShape Shape, const FBox2D& Footprint);
 // The mean of the unit directions from From to each target, and the box face (an axis, outward) that looks most that way.
 SIGNAL47_API FVector2D Toward(const FVector2D& From, TConstArrayView<FVector2D> Targets);
 SIGNAL47_API FVector2D FaceOf(const FVector2D& Dir);
 // The face of a box (Centre, Size) a thin part (its middle PartAt) lies on: its outward axis. False if it lies on none.
 SIGNAL47_API bool FaceOfPart(const FVector& Centre, const FVector& Size, const FVector& PartAt, FVector2D& OutNormal);
 // On the ground Off outside the solid: the middle of the box's face toward Dir, or the ellipse's edge along Dir (own terms).
 SIGNAL47_API FVector2D OutsidePoint(EShape Shape, const FBox2D& Footprint, const FVector2D& Dir, double Off);
 // The shack's lamp in its window, the place standing at Stand (three metres, y up): its middle, and the yaw that turns its front (+z)
 // out of the wall, as FS47Happenings::PoseProp turns it. False with no window.
 struct FLampPose { FVector At = FVector::ZeroVector; double Yaw = 0; };
 SIGNAL47_API bool LampOn(const FContract& C, const FVector& Stand, FLampPose& Out);
 // On the ground outside the solid toward Dir (world x, z), the place standing at Stand, at the height GroundY(x, z) gives there.
 SIGNAL47_API FVector LightAt(const FContract& C, const FVector& Stand, const FVector2D& Dir, double Off, TFunctionRef<double(double, double)> GroundY);
 // Does the segment A-B pass through the box?
 SIGNAL47_API bool SegmentHits(const FVector& A, const FVector& B, const FBox& Box);
 // A thing that stays with a drifting place at an offset: for a place that drifts (FS47Places::Drifts), X, Z moved to Today + Off.
 // True if it moved. (Follow is this with the place's today from FS47Places.)
 SIGNAL47_API bool FollowAt(const FString& Id, double& X, double& Z, double TodayX, double TodayZ, double OffX = 0, double OffZ = 0);
 // Where a ring stone lies (x, z) for a ring at Centre turned by Yaw, as PoseProp places a part at Local.
 SIGNAL47_API FVector2D StoneAt(const FVector2D& Centre, double Yaw, const FVector& Local);
 // A ring stone (part at Local of a ring at Centre turned by Yaw) seated on the ground under itself: GroundAt(x, z) gives that ground
 // (FS47Happenings::WorldOf). Out: the stone's middle. False where there is no ground.
 SIGNAL47_API bool StoneSeat(const FVector2D& Centre, double Yaw, const FVector& Local, TFunctionRef<bool(double, double, FVector&)> GroundAt, FVector& Out);
 // Whether the truck (its middle, and the reach of its body from it) is clear of an obstacle.
 SIGNAL47_API bool TruckClear(const FS47Obstacle& O, const FVector2D& Truck, double Reach);

 // One place's obstacle in the truck's list: never removed (others hold places in it by index), only sent far off and brought back.
 struct FTruckHold { bool bHave = false; int32 At = -1; FS47Obstacle Written; };
 // Want: put it there (in the slot it had, if that is still ours). Null: let it go (far off, the slot kept for the next time).
 SIGNAL47_API void HoldTruck(TArray<FS47Obstacle>& Obs, FTruckHold& H, const FS47Obstacle* Want);
 // Whether an obstacle is one sent far off.
 SIGNAL47_API bool IsAway(const FS47Obstacle& O);
 struct FHeld { FString Place; bool bOn = false, bLogged = false; FTruckHold Truck; };
 // A new scene (a save loaded, the title, a new session): every place's truck obstacle let go in the legs (they outlive the scene),
 // the slots kept; nothing is on in the new scene until it is drawn there.
 SIGNAL47_API void LetGo(TArray<FHeld>& Held, TArray<FS47Obstacle>* Obs);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);

 // ---- live ----
 // Where a built place stands today (road-local x, z): the cairn after its drift. False until FS47Places has built it.
 SIGNAL47_API bool PlaceToday(const US47Main* M, const FString& Id, double& OutX, double& OutZ);
 // For a place that drifts (FS47Places::Drifts): X, Z moved to its place today + (OffX, OffZ). True if they moved.
 SIGNAL47_API bool Follow(const US47Main* M, const FString& Id, double& X, double& Z, double OffX = 0, double OffZ = 0);
 // shack_lamp (S47NightPlaces StepPlace): InOutAt is the place's ground in the area's terms (FS47Happenings::WorldOf); it becomes the
 // lamp's middle in the window, OutYaw its turn. False for every other happening, or with no shack built (the old pose then).
 SIGNAL47_API bool LampPose(US47Main* M, const FString& HappeningId, FVector& InOutAt, double& OutYaw);
 // FS47FreeformDirector::LightPlacePoint for the shack and the cairn: on the ground outside the solid. False for every other place.
 SIGNAL47_API bool LightPoint(US47Main* M, const FString& Place, FVector& OutAt);
 // Each frame (US47Main::Step): the walk boxes and the truck's obstacles where the shapes stand now, on while they are drawn.
 SIGNAL47_API void Update(US47Main* M, double Dt);
}
