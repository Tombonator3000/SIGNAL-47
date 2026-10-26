#pragma once
#include "CoreMinimal.h"
#include "S47ExteriorFixtures.h"
class AS47Web;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class URectLightComponent;

// Native visitor arrival dressing; story/vehicle ownership stays with World/Evening.
class FS47SaroArrival
{
public:
 static TSharedPtr<FS47SaroArrival> Build(AS47Web* W);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
 void RetireCanopyLight(); // Detailed entrance uses the existing FireExit feed and one emitter.
private:
 TWeakObjectPtr<AS47Web> Web;
 TArray<TWeakObjectPtr<UPrimitiveComponent>> Parts, FloorQueries;
 TWeakObjectPtr<UStaticMeshComponent> Parking;
 TWeakObjectPtr<UStaticMeshComponent> Entrance, EntranceDoor;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> VisitorSigns;
 TWeakObjectPtr<UStaticMeshComponent> OldConcrete;
 TWeakObjectPtr<UStaticMeshComponent> OldSignFace;
 TWeakObjectPtr<UMaterialInstanceDynamic> WindowGlass;
 TArray<TWeakObjectPtr<URectLightComponent>> ArrivalLights;
 TArray<float> ArrivalLightLumens;
 TArray<S47ExteriorFixtures::FFixture> LightFixtures;
 int32 WalkMoved=0,DriveMoved=0,RemovedTriangles=0;
 bool bLastShown=false;
};
