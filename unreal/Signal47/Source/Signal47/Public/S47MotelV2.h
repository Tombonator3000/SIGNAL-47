#pragma once
#include "CoreMinimal.h"
class AS47Web;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;

/** Authored motel replacement; live story nodes and existing vehicle assets remain authoritative. */
class FS47MotelV2
{
public:
 static TSharedPtr<FS47MotelV2> Build(AS47Web* Web);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
 static double FloorAt(double X,double Z);
 static TOptional<FVector> SignPhotoTarget();
 // World Three XZ rectangles: [minX,maxX,minZ,maxZ]. Empty when the replacement is inactive.
 static TArray<FVector4> LandscapeFootprints();
private:
 TWeakObjectPtr<AS47Web> Web;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts,Retired;
 TArray<int32> RetiredNamedRoots;
 struct FMovedInteraction { int32 Node; FVector Position,Scale; };
 TArray<FMovedInteraction> MovedInteractions;
 int32 PayphoneNode=INDEX_NONE;
 // Door leaves can remove earlier rows from the shared walk array. Identity must survive reindexing.
 TArray<FString> DisabledColliderIds;
 TArray<FIntPoint> RetiredSignGlows,RetiredSignPools;
 TMap<int32,FVector> ProtectedPositions;
 TMap<int32,TWeakObjectPtr<USceneComponent>> ProtectedComponents;
 bool bInitialAnchorsPreserved=false;
 int32 ExpectedPartCount=0;
 struct FGlow { TWeakObjectPtr<UMaterialInstanceDynamic> Mid; FLinearColor Base; int32 Slot; float Reference; };
 TArray<FGlow> Glows;
 struct FFloor { double MinX,MaxX,MinZ,MaxZ,Y; };
 static TArray<FFloor> Floors;
 static TWeakObjectPtr<UStaticMeshComponent> PhotoSignComponent;
 int32 PatchCount=0,CarCount=0,AddedColliders=0;
};
