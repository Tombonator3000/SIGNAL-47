#pragma once
#include "CoreMinimal.h"
class AS47Web;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
/** Original architecture installed as one complete set, retaining all gameplay anchors. */
class FS47TownBuildings
{
public:
 static TSharedPtr<FS47TownBuildings> Build(AS47Web* W);
 void Update();
 bool Audit(TArray<FString>& Lines) const;
private:
 TWeakObjectPtr<AS47Web> Web;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;
 TArray<int32> Nodes,Retired;
 struct FGlow { TWeakObjectPtr<UMaterialInstanceDynamic> Mid; FString Family; FLinearColor Base; };
 struct FLamp { TWeakObjectPtr<UPointLightComponent> Light; int32 Parent; FString Family; };
 TArray<FGlow> Glows;
 TArray<FLamp> Lamps;
 int32 PatchCount=0;
};
