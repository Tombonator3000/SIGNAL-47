#pragma once
#include "CoreMinimal.h"
#include "S47Drive.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class UStaticMesh;
namespace S47FarmWorld
{
 bool Enabled(const US47Main* Main);
 FVector HouseOffset(const US47Main* Main);
 FVector HerdOffset(const US47Main* Main);
 FVector RoadToArea(const FString& Area,const FVector& RoadLocal);
 bool InRoadAccess(double RoadX,double RoadZ);
 double FloorRoad(double RoadX,double RoadZ);
 bool RoadClearance(US47Main* Main,TArray<FString>& Lines);
 int32 MovedRockCount(const US47Main* Main);
 bool Ready(US47Main* Main);
 bool Audit(US47Main* Main,TArray<FString>& Lines);
 bool HouseVisible(const US47Main* Main);
 FVector HousePhotoPoint(const US47Main* Main);
 // The four original buildings stand and show in the current area (Freeform only).
 bool BuildingsShown(const US47Main* Main);
 // How many of the three exported window glows now stand at the house's east window.
 int32 WindowGlowsAtHouse(const US47Main* Main);
 bool IsGateRemainder(const AS47Web* Web,const UStaticMeshComponent* Component,const UStaticMesh* Source);
 bool GateReady(const US47Main* Main);
 void ReconcileRanch(US47Main* Main);
 void Reconcile(US47Main* Main);
 // The parked truck is an oriented rectangle (three heading: forward is (sin,cos)), tested like FS47DriveController::Hit
 // against obstacles in one frame. Shrink is the drive's own give (.05 wide, .10 long) so a legal park stays legal.
 bool TruckHits(const TArray<FS47Obstacle>& Obstacles,double X,double Z,double Heading,double HalfWidth,double HalfLength,double Shrink=0);
 // Nearest clear spot, same heading, by rings of .5 m up to 30 m. False: none. Out equals In when it is already clear.
 bool NearestClearTruckSpot(const TArray<FS47Obstacle>& Obstacles,double X,double Z,double Heading,double HalfWidth,double HalfLength,double& OutX,double& OutZ);
 bool SelfTest(TArray<FString>& Lines);
}
