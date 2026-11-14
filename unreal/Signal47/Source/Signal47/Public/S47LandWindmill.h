#pragma once
#include "CoreMinimal.h"

class US47Main;
class AS47Web;

// LAND-PLACES-WINDMILL-20261114 (→ M6, M1; Tom: the four land places modelled from reference packs, the windmill first). The stock
// windmill as a model of its own (/Game/LandPlaces/Windmill, made by Tools/landplaces/windmill): a galvanized lattice tower on four
// concrete piers, the wheel with eighteen blades, the gearbox head with its tail vane, the pump rod down to a pump in a knee-high
// housing with a hinged lid, and a round stock tank with a float valve and water in it. It stands where the game already has its
// two windmills and takes nothing from them:
//   the Baca lease   FS47Places keeps the place, its finding, its first-day traces and every hook; its eight basic shapes are hidden
//                    while the model is drawn (and stay as they were when the assets are not in the build). The model stands where
//                    FS47Places::Stand says, its head turned as the place's was (toward the bearing on the first days).
//   the farm         S47FarmWindmill keeps its place, its footprints and its hours; its cubes are not built while the model is drawn.
// What moves is moved here, by code (the animation rule: mechanical motion):
//   the wheel   the farm's turns with the wind (S47Weather::Live); the Baca wheel is wired off and its brake is on (the finding, the
//               CB and Baca himself say so): it only strains a little when its creak is heard, and turns, with no wind, while the
//               night's windmill_knock goes, and stands again when that stops;
//   the head    the farm's follows the wind slowly; the Baca head stays where it was tied;
//   the rod     one stroke to a turn of the wheel: down at the knock, up at the lighter one;
//   the lid     the pump housing's, lifted by the player. Reyes's page of April 8 lies in the housing when its turn has come
//               (FS47Predecessor book_windmill), the lid wired shut with the same bright wire until it is read: "Undo the wire" in
//               the place's panel or on the lid itself, the one reading;
//   the float   rides the water.
// Walking and driving: the piers, the housing and the tank are walk boxes of the road's list ("landplace:windmill:...") and the
// truck's own obstacles (the tower's foot as one box, the tank as a circle), there only while the model is drawn. A walker passes
// between the legs and round the housing; the truck does not go under the tower.
// -S47LandMillOff: both windmills as they were before (for the before pictures, and the fallback's own test).
namespace S47LandWindmill
{
 // The model's own measures (Tools/landplaces/windmill/build_windmill.py writes them to contract.json): metres on the place's
 // ground, x east, y up, z south, the tower's middle at the origin.
 inline constexpr double HubY=9.5,HubOff=.46,PierTop=.26,Foot=1.25,PierHalf=.19,PierDeep=.7,MastTop=9.3,Stroke=.19;
 inline constexpr double HouseX0=-.4,HouseX1=.4,HouseZ0=-.18,HouseZ1=.47,HouseTop=.6,HouseSill=.22,HingeZ=.11,HingeY=.615;
 inline constexpr double TankX=4.35,TankZ=.35,TankR=1.5,TankH=.6,TankDeep=.25,WaterY=.47;
 inline constexpr double ValveX=TankX-TankR+.2,ValveY=.74;
 inline constexpr double KnockEvery=1.7;   // S47NightPlaces' windmill_knock: seconds between a knock and the next

 // Every mesh of the model is in the build, and -S47LandMillOff is not given.
 SIGNAL47_API bool Available();
 // Each frame (US47Main): builds, seats and moves the Baca mill and the farm's.
 SIGNAL47_API void Update(US47Main* M,double Dt);
 // The farm's (S47FarmWindmill::Reconcile): shown or not, for the area.
 SIGNAL47_API void ReconcileFarm(AS47Web* W,bool bOn);
 // One knock of windmill_knock (FS47Happenings::StepPlace): the rod at the bottom of its stroke (bUp false) or at the top.
 SIGNAL47_API void Knock(bool bUp);
 SIGNAL47_API void KnockEnd();
 // A creak was heard from the windmill at this road-local spot (FS47Soundscape).
 SIGNAL47_API void Creak(const FVector2D& RoadSpot);

 // ---- pure parts (the self-test holds them) ----
 // The walk boxes round a mill, in its own metres (x east, y = z south): four piers, the housing, the tank as five boxes.
 SIGNAL47_API TArray<FBox2D> WalkBoxes(double TankAtX,double TankAtZ,double TankRadius);
 // Whether a road-local point lies under either mill's tower or tank while the model is drawn (S47FarmWorld clears the scattered
 // plants there: none grows up through the housing or out of the water).
 SIGNAL47_API bool Covers(double RoadX,double RoadZ);
 // The same ground as road-local squares, for the ground cover's blocks (S47GroundCover): each mill's foot and its tank. Empty
 // while the model is not drawn.
 SIGNAL47_API TArray<FBox2D> Plots();
 // How fast a free wheel turns in this wind (degrees a second), and the rod's height over the bottom of its stroke at a wheel angle.
 SIGNAL47_API double FreeRate(double Mph);
 SIGNAL47_API double RodLift(double WheelDeg);
 // The Unreal yaw of the head for the compass way the wheel faces (the model's wheel faces south).
 inline double HeadYaw(double CompassDeg){return CompassDeg-180.;}
 // Where the tower stands on ground of these four heights under its piers (NE, NW, SW, SE): level, every pier's top PierTop or
 // more over its own ground.
 inline double TowerSeat(const double G[4]){return FMath::Max(FMath::Max(G[0],G[1]),FMath::Max(G[2],G[3]));}
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);

 // ---- for the pictures and the tests ----
 // What is drawn now and how it sits: lines for F10 and the shot test. Traces the drawn ground under each piece when bTrace.
 SIGNAL47_API bool Audit(US47Main* M,TArray<FString>& Lines,bool bTrace);
 // The Baca lid held open or shut and the page shown whatever the trail says (bOn false: back to the game's own).
 SIGNAL47_API void DevPose(bool bOn,bool bLidOpen,bool bPage,bool bWire);
 // How far the Baca lid stands open (0 shut to 1), and how fast its wheel turns now (degrees a second).
 SIGNAL47_API double LidOpen();
 SIGNAL47_API double WheelRateNow();
}
