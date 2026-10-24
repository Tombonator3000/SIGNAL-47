#pragma once
#include "CoreMinimal.h"
class AS47Web;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Native visitor arrival dressing; story/vehicle ownership stays with World/Evening.
class FS47SaroArrival
{
public:
 static TSharedPtr<FS47SaroArrival> Build(AS47Web* W);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
private:
 TWeakObjectPtr<AS47Web> Web;
 TArray<TWeakObjectPtr<UPrimitiveComponent>> Parts, FloorQueries;
 TWeakObjectPtr<UStaticMeshComponent> Parking;
 TWeakObjectPtr<UStaticMeshComponent> OldConcrete;
 TWeakObjectPtr<UStaticMeshComponent> OldSignFace;
 TWeakObjectPtr<UMaterialInstanceDynamic> WindowGlass;
 int32 WalkMoved=0,DriveMoved=0,RemovedTriangles=0;
 bool bLastShown=false;
};
