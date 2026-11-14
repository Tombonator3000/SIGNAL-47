#pragma once
#include "CoreMinimal.h"

class US47Main;
struct FS47Obstacle;

// LAND-PLACES-FIXES-20261114 (→ M1, M6): what was wrong at the line shack and the cairn on the rise while they are still drawn as
// FS47Places' basic shapes, put right so that it keeps working when a model takes the shapes' place (everything here is measured off
// the place's first part, the solid one the game points at, and off where the place stands today, never off fixed world numbers):
//   the shack's lamp   shack_lamp (S47NightPlaces) burned inside the solid box and its sight test ended inside it too, so it was never
//                      seen. It now hangs in a window on the OUTSIDE of the wall that looks toward the highway and SARO (the face is
//                      worked out from where they lie), flush with it, 1.3 m up to its middle, its light just out from the wall.
//   lights and marks   FS47FreeformDirector::LightPlacePoint gave the middle of the solid; for the shack and the cairn it is now on the
//                      ground just outside the face toward the same two (the way the player comes from), 1.5 m out.
//   the cairn drifts   its tag (Reyes's, FS47Predecessor) and a ring laid round it (cairn_stones) follow where it stands today
//                      (PlaceToday/Follow, the same for every place that drifts), and each ring stone sits on the ground under itself.
//   walking, driving   the shack's box and the cairn are walk boxes of the road's list ("landplace:<id>:0") and the truck's own
//                      obstacles, exactly what is drawn or inside it, there only while the basic shapes are drawn (a model brings its
//                      own, as S47LandWindmill). The mine has no solid shape today: nothing is added there.
namespace S47LandPlaceFixes
{
 enum class EShape : uint8 { Box, Round };
 // The land places whose first part is a solid nobody walks into: the shack's box, the cairn's base stone (an ellipsoid).
 struct FSolid { const TCHAR* Place; EShape Shape; };
 SIGNAL47_API const FSolid* SolidOf(const FString& Place);

 inline constexpr double Clear = 1.5;     // lights and marks: this far out from the face (FS47FarmSite steps 1.2 m on from there)
 inline constexpr double LampUp = 1.3;    // the shack's window: its middle over the floor
 inline constexpr double LampOut = .055;  // the lamp's middle off the wall (its frame's back is .05 behind it: S47Hap::WindowParts)
 inline constexpr double SightShort = 1.2;// FS47Happenings::SeenLit ends its line this much short of the point (or 5 % of the way)

 // ---- pure (the self-test holds them) ----
 // Drawn: the solid's footprint, x and z (FBox2D's Y is z). What holds a walker: the box itself, or the square inside the ellipse.
 SIGNAL47_API FBox2D WalkRect(EShape Shape, const FBox2D& Drawn);
 // What holds the truck: the box itself, or the circle inside the ellipse.
 SIGNAL47_API FS47Obstacle TruckOf(EShape Shape, const FBox2D& Drawn);
 // The mean of the unit directions from From to each target, and the box face (an axis, outward) that looks most that way.
 SIGNAL47_API FVector2D Toward(const FVector2D& From, TConstArrayView<FVector2D> Targets);
 SIGNAL47_API FVector2D FaceOf(const FVector2D& Dir);
 // On the ground Off outside the solid: the middle of the box's face toward Dir, or the ellipse's edge along Dir.
 SIGNAL47_API FVector2D OutsidePoint(EShape Shape, const FBox2D& Drawn, const FVector2D& Dir, double Off);
 // The shack's lamp on the face with this outward Normal of the Solid (three metres, y up): its middle, and the yaw that turns its
 // front (+z) out of the wall, as FS47Happenings::PoseProp turns it.
 struct FLampPose { FVector At = FVector::ZeroVector; double Yaw = 0; };
 SIGNAL47_API FLampPose LampOn(const FBox& Solid, const FVector2D& Normal);
 // Does the segment A-B pass through the box?
 SIGNAL47_API bool SegmentHits(const FVector& A, const FVector& B, const FBox& Box);
 // A thing that stays with a drifting place at an offset: moved to Today + Off. True if it moved.
 SIGNAL47_API bool FollowTo(double& X, double& Z, double TodayX, double TodayZ, double OffX, double OffZ);
 // Where a ring stone lies (x, z) for a ring at Centre turned by Yaw, as PoseProp places a part at Local.
 SIGNAL47_API FVector2D StoneAt(const FVector2D& Centre, double Yaw, const FVector& Local);
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
