#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "S47PhotoLabTrayFix.generated.h"

class AS47Web;
class UStaticMesh;

// Native proof survives cooking, unlike editor package metadata/MeshDescription.
UCLASS()
class SIGNAL47_API US47PhotoLabTrayFixProof : public UAssetUserData
{
 GENERATED_BODY()
public:
 UPROPERTY() TObjectPtr<UStaticMesh> SourceMesh;
 UPROPERTY() FString SourceDescriptionHash;
 UPROPERTY() FString SourceRenderHash;
 UPROPERTY() FString FixedRenderHash;
 UPROPERTY() FVector SourceMin;
 UPROPERTY() FVector SourceMax;
 UPROPERTY() int32 ChangedVertices = 0;
 UPROPERTY() int32 ChangedRenderVertices = 0;
 UPROPERTY() int32 SourceTriangles = 0;
};

UCLASS()
class SIGNAL47_API US47PhotoLabTrayFixTools : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 // Editor implementation only; returns a JSON receipt. Python owns duplicate/save.
 UFUNCTION(BlueprintCallable, Category="Signal47|Authoring")
 static FString Prepare(UStaticMesh* Source, UStaticMesh* Candidate, bool bVerifyOnly);
};

namespace S47PhotoLabTrayFix
{
 bool Build(AS47Web* World);
 bool Audit(AS47Web* World, TArray<FString>& Lines);
}
