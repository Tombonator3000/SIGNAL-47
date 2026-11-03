#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "S47SaroSeam.generated.h"

class AS47Web;
class UStaticMesh;

// The editor builds two derived assets. Original Web/Motel meshes are never saved.
UCLASS()
class SIGNAL47_API US47SaroSeamProof : public UAssetUserData
{
 GENERATED_BODY()
public:
 UPROPERTY() TObjectPtr<UStaticMesh> Source;
 UPROPERTY() int32 Revision = 0;
 UPROPERTY() int32 SourceTriangles = 0;
 UPROPERTY() int32 Triangles = 0;
 UPROPERTY() FVector SourceMin;
 UPROPERTY() FVector SourceMax;
 UPROPERTY() FString SurfaceSignature;
 // Road proof only: unchanged, actually retained g4 triangles intersecting this small band.
 UPROPERTY() TObjectPtr<UStaticMesh> Underlay;
 UPROPERTY() int32 UnderlayTriangles = 0;
 UPROPERTY() TArray<FVector> UnderlayCorners; // corridor-local Three metres, three per face
};

UCLASS()
class SIGNAL47_API US47SaroSeamTools : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Signal47|Authoring")
 static FString Prepare(UStaticMesh* SourceRoad, UStaticMesh* SourceGround, UStaticMesh* Underlay,
                        UStaticMesh* Road, UStaticMesh* Ground, bool bVerifyOnly);
};

namespace S47SaroSeam
{
 bool Build(AS47Web* Web);
 // Site metres. False leaves the caller's original floor/surface policy untouched.
 bool Height(const AS47Web* Web, double X, double Z, double& Y, bool* bRoad = nullptr);
 bool Audit(AS47Web* Web, TArray<FString>& Lines);
}
