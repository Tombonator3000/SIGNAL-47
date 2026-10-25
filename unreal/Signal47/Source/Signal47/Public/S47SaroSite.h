#pragma once
#include "CoreMinimal.h"
class AS47Web;
class UStaticMeshComponent;
class UMaterialInterface;

// Original wing facade and one necessary service transformer. No story or floor changes.
class FS47SaroSite
{
public:
 static TSharedPtr<FS47SaroSite> Build(AS47Web* W);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
private:
 TWeakObjectPtr<AS47Web> Web;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;
 TWeakObjectPtr<UStaticMeshComponent> Stains;
 TWeakObjectPtr<UMaterialInterface> StainAtlas;
 TArray<int32> PartNodes, RetiredNodes;
 int32 RemovedWalkFeet=0,RemovedDriveFeet=0;
 bool bShown=false;
};
