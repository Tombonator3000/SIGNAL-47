#pragma once
#include "CoreMinimal.h"
class US47Main;
class USceneComponent;
class UMaterialInstanceDynamic;

// Original EastWing furnishing. The wing owns visibility through its existing Parts list.
class FS47EastWingProps
{
public:
 bool Build(US47Main* Main, TArray<TWeakObjectPtr<USceneComponent>>& Parts);
 void SetPowered(bool bPowered){SetLevel(bPowered?1.f:0.f);}
 void SetLevel(float Level); // 0..1 tube output, follows the fixtures' strike/die-off flicker
private:
 struct FEmission { TWeakObjectPtr<UMaterialInstanceDynamic> Material; FLinearColor Powered; };
 TArray<FEmission> Emission;
 bool bBuilt=false;
};
