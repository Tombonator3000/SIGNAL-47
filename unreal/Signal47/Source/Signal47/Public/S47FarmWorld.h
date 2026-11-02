#pragma once
#include "CoreMinimal.h"
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
 bool Ready(US47Main* Main);
 bool Audit(US47Main* Main,TArray<FString>& Lines);
 bool HouseVisible(const US47Main* Main);
 FVector HousePhotoPoint(const US47Main* Main);
 bool IsGateRemainder(const AS47Web* Web,const UStaticMeshComponent* Component,const UStaticMesh* Source);
 bool GateReady(const US47Main* Main);
 void ReconcileRanch(US47Main* Main);
 void Reconcile(US47Main* Main);
 bool SelfTest(TArray<FString>& Lines);
}
