#pragma once
#include "CoreMinimal.h"

// The four original farm buildings in Freeform (Tools/farmbuildings): where they stand, what stops the player and
// the truck, and the flat floors under them. Road-local metres, as S47FarmWorld. Story never shows them.
namespace S47FarmBuildings
{
 // Sink: how far under the land at its middle the building's z=0 lies, so its walls meet the ground on the low side too.
 struct FSite { const TCHAR* Name; const TCHAR* Mesh; double X,Z,Yaw,Sink; };
 // The cooked meshes are there. Without them the farm keeps its old boxes and their collision.
 bool Available();
 // Barn, house, machine shed, feed shed. Yaw is the three.js turn in radians; the UE rotation is -Yaw.
 const TArray<FSite>& Sites();
 FVector Anchor(const FSite& Site);
 // A point in a building's own metres (x east, y north) as road-local X and Z.
 FVector2D LocalToRoad(const FSite& Site,double LocalX,double LocalY);
 // The walls and slab edges that meet the ground, in the building's metres: where the dark soil lies against them.
 struct FWallFoot { int32 Site; double X0,Y0,X1,Y1; };
 const TArray<FWallFoot>& WallFeet();
 // obstacles_road_local from placement.json, as (MinX,MinZ)-(MaxX,MaxZ).
 const TArray<FBox2D>& Obstacles();
 // The obstacles, and the feed shed's doorway while its door is shut.
 bool Blocked(double RoadX,double RoadZ,double Radius);
 // WEEKLY-CASE-20261109: the feed shed's doorway, the gap in its south wall between the two runs above (sheds.py B_DOOR),
 // road-local. FS47FarmShedDoor shuts it while the door is locked: Blocked counts it then, as a wall.
 FBox2D ShedDoorway();
 void SetShedDoorShut(bool bShut);
 bool ShedDoorShut();
 // The four plots with their roof overhangs: nothing grows here.
 const TArray<FBox2D>& Plots();
 bool OnPlot(double RoadX,double RoadZ,double Margin=0);
 // The flat floor under a roof the player can stand under: barn, both sheds and the house's porch.
 bool FloorAt(double RoadX,double RoadZ,double& OutY);
 // A point in the barn's own metres (x east, y north, Blender's axes) as road-local, on the barn's floor.
 FVector BarnPoint(double LocalX,double LocalY);
 // Just outside the lit east window of the house, where its glow belongs.
 FVector EastWindow();
 bool SelfTest(TArray<FString>& Lines);
}
